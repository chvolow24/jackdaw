/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    transport.c

    * Audio callback fns
    * Playback and recording
 *****************************************************************************************************************/

#include <unistd.h>
#include "audio_clip.h"
#include "audio_connection.h"
#include "clipref.h"
#include "color.h"
/* #include "dsp.h" */
#include "dsp_utils.h"
#include "midi_clip.h"
#include "user_event.h"
#include "mixdown.h"
#include "project.h"
#include "session_endpoint_ops.h"
#include "pure_data.h"
#include "session.h"
#include "synth.h"
#include "timeline.h"
#include "transport.h"

extern pthread_t DSP_THREAD_ID;
extern pthread_t CURRENT_THREAD_ID;
extern struct colors colors;

void copy_conn_buf_to_clip(Clip *clip, enum audio_conn_type type);
void transport_record_callback(void* user_data, uint8_t *stream, int len)
{
    AudioConn *conn = (AudioConn *)user_data;
    AudioDevice *dev = &conn->c.device;

    /* double time_diff = 1000.0f * ((double)conn->callback_clock.clock - session->audio_io.playback_conn->callback_clock.clock) / CLOCKS_PER_SEC; */
    /* fprintf(stdout, "TIME DIFF ms: %f\n", time_diff); */


    
    /* fprintf(stdout, "Playback: %ld, %d; Record: %ld, %d\n", */
    /* 	    session->audio_io.playback_conn->callback_clock.clock, */
    /* 	    session->audio_io.playback_conn->callback_clock.timeline_pos, */
    /* 	    conn->callback_clock.clock, */
    /* 	    conn->callback_clock.timeline_pos); */

    Session *session = session_get();
    Project *proj = &session->proj;

    if (!conn->current_clip_repositioned) {
        struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	/* TODO

	   These latency estimates, used for latency compensation, are
	   spurious. I can more precisely estimate latency if/when I switch
	   from SDL to PortAudio (or a similar, more audio-focused library
	   that can report on ADC and DAC time in a callback).

	*/
	double est_latency_mult = 60.0f;
	double record_latency_ms = (double)1000.0f * 64.0 / proj->sample_rate;
	double playback_latency_ms = est_latency_mult * record_latency_ms;

	struct realtime_tick pb_cb_tick = session->audio_io.playback_conn->callback_time;
	double elapsed_pb_chunk_ms = TIMESPEC_DIFF_MS(now, pb_cb_tick.ts) - playback_latency_ms;

	int32_t tl_pos_now = pb_cb_tick.timeline_pos + (int32_t)(elapsed_pb_chunk_ms * proj->sample_rate / 1000.0f);
	int32_t tl_pos_rec_chunk = tl_pos_now - (record_latency_ms * proj->sample_rate / 1000);
	
	for (uint16_t i=0; i<conn->current_clip->num_refs; i++) {
	    ClipRef *cr = conn->current_clip->refs[i];
	    cr->tl_pos = tl_pos_rec_chunk;
	}
	conn->current_clip_repositioned = true;
    }
    
    uint32_t stream_len_samples = len / sizeof(int16_t);

    if (dev->write_bufpos_samples + stream_len_samples < dev->rec_buf_len_samples) {
	/* fprintf(stdout, "Fits! bufpos: %d\n", dev->write_bufpos_samples); */
        memcpy(dev->rec_buffer + dev->write_bufpos_samples, stream, len);
	dev->write_bufpos_samples += stream_len_samples;
    } else {
	/* fprintf(stdout, "Leftover: %d\n", dev->rec_buf_len_samples - (dev->write_bufpos_samples + stream_len_samples)); */
	for (int i=proj->active_clip_index; i<proj->num_clips; i++) {
	    Clip *clip = proj->clips[i];
	    if (!clip->recorded_from) continue;
	    if (clip->recorded_from->type == DEVICE && &clip->recorded_from->c.device == dev) {	
		copy_conn_buf_to_clip(clip, DEVICE);
	    }
	     /* clip->write_bufpos_sframes += dev->write_bufpos_samples / clip->channels; */
	 }
	 memcpy(dev->rec_buffer, stream, len);
	 dev->write_bufpos_samples = stream_len_samples;
	 /* dev->write_bufpos_samples = 0; */
	 /* device_stop_recording(dev); */
	 /* fprintf(stderr, "ERROR: overwriting audio buffer of device: %s\n", dev->name); */
     }


 }

static float *get_source_mode_chunk(uint8_t channel, float *chunk, uint32_t len_sframes, int32_t start_pos_sframes, float step)
 {
     /* float *chunk = malloc(sizeof(float) * len_sframes); */
     /* if (!chunk) { */
     /* 	 fprintf(stderr, "Error: unable to allocate chunk from source clip\n"); */
     /* } */
     Session *session = session_get();
     
     float *src_buffer = session->source_mode.src_clip->channels == 1 ? session->source_mode.src_clip->L :
	 channel == 0 ? session->source_mode.src_clip->L : session->source_mode.src_clip->R;


     for (uint32_t i=0; i<len_sframes; i++) {
	 int sample_i = (int) (i * step + start_pos_sframes);
	 if (sample_i < session->source_mode.src_clip->len_sframes && sample_i > 0) {
	     chunk[i] = src_buffer[sample_i];
	 } else {
	     chunk[i] = 0;
	 }
     }
     return chunk;
 }

void transport_recording_update_cliprects();

void transport_playback_callback(void* user_data, uint8_t* stream, int len)
{
    Session *session = session_get();
    /* fprintf(stdout, "\nSTART cb\n"); */
    /* clock_t a,b; */
    /* a = clock(); */
    Project *proj = &session->proj;
    Timeline *tl = ACTIVE_TL;
    AudioConn *conn = (AudioConn *)user_data;
    clock_gettime(CLOCK_MONOTONIC, &(conn->callback_time.ts));
    conn->callback_time.timeline_pos = tl->play_pos_sframes + (tl->buf_read_pos % (proj->fourier_len_sframes / proj->chunk_size_sframes));

    memset(stream, '\0', len);
    uint32_t stream_len_samples = len / sizeof(int16_t);
    uint32_t len_sframes = stream_len_samples / proj->channels;
    float chunk_L[len_sframes];
    float chunk_R[len_sframes];
    if (session->source_mode.source_mode) {
	get_source_mode_chunk(0, chunk_L, len_sframes, session->source_mode.src_play_pos_sframes, session->source_mode.src_play_speed);
	get_source_mode_chunk(1, chunk_R, len_sframes, session->source_mode.src_play_pos_sframes, session->source_mode.src_play_speed);
    } else {
	int wait_count = 0;
	while (sem_trywait(tl->readable_chunks) != 0) {
	    wait_count++;
	    if (wait_count > 100) return;
	}
	memcpy(chunk_L, tl->buf_L + tl->buf_read_pos, sizeof(float) * len_sframes);
	memcpy(chunk_R, tl->buf_R + tl->buf_read_pos, sizeof(float) * len_sframes);
	sem_post(tl->writable_chunks);
	for (int i=0; i<session->midi_io.num_synths; i++) {
	    Synth *s = session->midi_io.synths[i];
	    if (s->monitor) {
		synth_add_buf(s, chunk_L, 0, len_sframes);
		synth_add_buf(s, chunk_R, 1, len_sframes);
	    }
	}

	tl->buf_read_pos += len_sframes;
	if (tl->buf_read_pos >= proj->fourier_len_sframes * RING_BUF_LEN_FFT_CHUNKS) {
	    tl->buf_read_pos = 0;
	}
    }

    int16_t *stream_fmt = (int16_t *)stream;
    for (uint32_t i=0; i<stream_len_samples; i+=2)
    {
	float val_L = chunk_L[i/2];
	float val_R = chunk_R[i/2];
	stream_fmt[i] = (int16_t) (val_L * INT16_MAX);
	stream_fmt[i+1] = (int16_t) (val_R * INT16_MAX);
    }


    if (session->source_mode.source_mode) {
	session->source_mode.src_play_pos_sframes += session->source_mode.src_play_speed * stream_len_samples / session->source_mode.src_clip->channels;
	if (session->source_mode.src_play_pos_sframes < 0 || session->source_mode.src_play_pos_sframes > session->source_mode.src_clip->len_sframes) {
	    session->source_mode.src_play_pos_sframes = 0;
	}
	tl->needs_redraw = true;

    } else {
	timeline_move_play_position(tl, session->playback.play_speed * stream_len_samples / proj->channels);
    }
}

static void *transport_dsp_thread_fn(void *arg)
{
    Session *session = session_get();
    DSP_THREAD_ID = pthread_self();
    CURRENT_THREAD_ID = DSP_THREAD_ID;
    
    Timeline *tl = (Timeline *)arg;
    
    if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) != 0) {
	perror("pthread set cancel state");
	exit(1);
    }

    int len = tl->proj->fourier_len_sframes;
    /* pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL); */
    float buf_L[len];
    float buf_R[len];

    int N = len / tl->proj->chunk_size_sframes;
    bool init = true;

    clock_t performance_timer;
    double last_t;
    while (1) {
	/* clock_t cd = clock(); */
	performance_timer = clock();
	pthread_testcancel();
	float play_speed = session->playback.play_speed;

	/* GET MIXDOWN */
	get_mixdown_chunk(tl, buf_L, 0, len, tl->read_pos_sframes, session->playback.play_speed);
	get_mixdown_chunk(tl, buf_R, 1, len, tl->read_pos_sframes, session->playback.play_speed);
	
	/* double timed = (((double)clock() - cd)/CLOCKS_PER_SEC); */
	/* fprintf(stderr, "Mixdown: %f\n", timed * 1000); */
	/* cd = clock(); */

	/* DSP */
	double dL[len];
	double dR[len];
	for (int i=0; i<len; i++) {
	    dL[i] = (double)buf_L[i];
	    dR[i] = (double)buf_R[i];
	}
	double complex lfreq[len];
	double complex rfreq[len];
	FFT(dL, lfreq, len);
	FFT(dR, rfreq, len);

	get_magnitude(lfreq, tl->proj->output_L_freq, len);
	get_magnitude(rfreq, tl->proj->output_R_freq, len);

	/* timed = (((double)clock() - cd)/CLOCKS_PER_SEC); */
	/* fprintf(stderr, "DSP: %f\n", timed * 1000); */
	/* cd = clock(); */
	

	/* Move the read (DSP) pos */
	tl->read_pos_sframes += len * play_speed;

	if (session->playback.loop_play) {
	    int32_t loop_len = tl->out_mark_sframes - tl->in_mark_sframes;
	    if (loop_len > 0) {
		int64_t remainder = tl->read_pos_sframes - tl->out_mark_sframes;
		/* int32_t remainder = tl->read_pos_sframes - tl->out_mark_sframes; */
		if (remainder > 0) {
		    if (remainder > loop_len) remainder = 0;
		    tl->read_pos_sframes = tl->in_mark_sframes + remainder;
		}
	    }
	}
	/* timed = (((double)clock() - cd)/CLOCKS_PER_SEC); */
	/* fprintf(stderr, "loop: %f\n", timed * 1000); */
	/* cd = clock(); */


	/* if (session->playback.loop_play && tl->out_mark_sframes > tl->in_mark_sframes) { */
	/*     int32_t remainder = tl->read_pos_sframes - tl->out_mark_sframes; */
	/*     if (remainder > 0) { */
	/* 	int32_t new_pos = tl->in_mark_sframes + remainder; */
	/* 	if (new_pos > tl->out_mark_sframes) new_pos = tl->in_mark_sframes; */
	/* 	tl->read_pos_sframes = new_pos; */
	/*     } */
	/* } */

	/* Copy buffer */	
	for (int i=0; i<N; i++) {
	    sem_wait(tl->writable_chunks);
	}
	/* timed = (((double)clock() - cd)/CLOCKS_PER_SEC); */
	/* fprintf(stderr, "sem: %f\n", timed * 1000); */
	/* cd = clock(); */

	
	memcpy(tl->buf_L + tl->buf_write_pos, buf_L, sizeof(float) * len);
	memcpy(tl->buf_R + tl->buf_write_pos, buf_R, sizeof(float) * len);
	memcpy(tl->proj->output_L, buf_L, sizeof(float) * len);
	memcpy(tl->proj->output_R, buf_R, sizeof(float) * len);

	for (uint16_t i=tl->proj->active_clip_index; i<tl->proj->num_clips; i++) {
	    Clip *clip = tl->proj->clips[i];
	    AudioConn *conn = clip->recorded_from;
	    if (!conn) continue;
	    if (conn->type == JACKDAW) {
		/* if (conn->c.jdaw.write_bufpos_sframes + len < conn->c.jdaw.rec_buf_len_sframes) { */
		memcpy(conn->c.jdaw.rec_buffer_L + conn->c.jdaw.write_bufpos_sframes, buf_L, sizeof(float) * len);
		memcpy(conn->c.jdaw.rec_buffer_R + conn->c.jdaw.write_bufpos_sframes, buf_R, sizeof(float) * len);
		conn->c.jdaw.write_bufpos_sframes += len;
		if (conn->c.jdaw.write_bufpos_sframes + 2 * len >= conn->c.jdaw.rec_buf_len_sframes) {
		    copy_conn_buf_to_clip(clip, JACKDAW);
		}
		break;
	    }
	}
	/* timed = (((double)clock() - cd)/CLOCKS_PER_SEC); */
	/* fprintf(stderr, "clips: %f\n", timed * 1000); */
	/* cd = clock(); */


        /* TODO: scrutinize! */
	for (int i=0; i<session->midi_io.num_inputs; i++) {
	    MIDIDevice *d = session->midi_io.inputs + i;
	    if (d->info->opened) {
		midi_device_read(d);
	    }
	}
	for (int i=session->proj.active_midi_clip_index; i<session->proj.num_midi_clips; i++) {
	    MIDIClip *mclip = session->proj.midi_clips[i];
	    if (!mclip->recorded_from) continue;
	    mclip->len_sframes += len;
	    MIDIDevice *d = mclip->recorded_from;
	    midi_device_record_chunk(d);
	}


	
	tl->buf_write_pos += len;
	if (tl->buf_write_pos >= len * RING_BUF_LEN_FFT_CHUNKS) {
	    tl->buf_write_pos = 0;
	}
	for (int i=0; i<N; i++) {
	    sem_post(tl->readable_chunks);
	}
	if (init) {
	    sem_post(tl->unpause_sem);
	    init = false;
	}
	/* timed = (((double)clock() - cd)/CLOCKS_PER_SEC); */
	/* fprintf(stderr, "cleanup: %f\n", timed * 1000); */
	/* cd = clock(); */

	
	session_do_ongoing_changes(session, JDAW_THREAD_DSP);
	session_flush_val_changes(session, JDAW_THREAD_DSP);
	session_flush_callbacks(session, JDAW_THREAD_DSP);

	/* timed = (((double)clock() - cd)/CLOCKS_PER_SEC); */
	/* fprintf(stderr, "ongoing: %f\n", timed * 1000); */
	/* cd = clock(); */

	
	double time = ((double)clock() - performance_timer) / CLOCKS_PER_SEC;
	double alloc_time = (double)session->proj.fourier_len_sframes / session->proj.sample_rate;
	double out = 0.9 * last_t + 0.1 * (time / alloc_time);
	/* fprintf(stderr, "STRESS: %f\n", out); */
	last_t = out;
	
    }

    return NULL;
}


/* extern double *del_line_l, *del_line_r; */
/* extern int16_t del_line_len; */
void transport_start_playback()
{
    Session *session = session_get();
    if (session->playback.playing) return;
    session->playback.playing = true;
    Timeline *tl = ACTIVE_TL;
    tl->read_pos_sframes = tl->play_pos_sframes;

    for (uint8_t i=0; i<tl->num_tracks; i++) {
	Track *track = tl->tracks[i];
	for (uint8_t a=0; a<track->num_automations; a++) {
	    automation_clear_cache(track->automations[a]);
	}
    }
    
    pthread_attr_t attr;
    int sched_policy = SCHED_RR;
    int ret;
    if ((ret = pthread_attr_init(&attr)) != 0) {
	fprintf(stderr, "pthread_attr_init: %s\n", strerror(ret));
    }
    if ((ret = pthread_attr_setschedpolicy(&attr, sched_policy)) != 0) {
	fprintf(stderr, "pthread_attr_setschedpolicy: %s\n", strerror(ret));
    }
    int priority_max = sched_get_priority_max(sched_policy);
    if (priority_max < 0) {
	perror("sched_get_priority_max");
	exit(1);
    }
    
    /* Set priority */
    struct sched_param dsp_sched;
    dsp_sched.sched_priority = priority_max;
    if ((ret = pthread_attr_setschedparam(&attr, &dsp_sched)) != 0) {
	fprintf(stderr, "pthread_attr_setschedparam: %s\n", strerror(ret));
    }

    /* Set stack size */
    size_t orig_stack_size;
    size_t desired_stack_size = 2 * sizeof(double) * session->proj.sample_rate;
    int page_size = getpagesize();
    int num_pages = desired_stack_size / page_size;
    desired_stack_size = num_pages * page_size;
    if ((ret = pthread_attr_getstacksize(&attr, &orig_stack_size) != 0)) {
	fprintf(stderr, "pthread_attr_getstacksize: %s\n", strerror(ret));
    }
    if (orig_stack_size < desired_stack_size) {
	if ((ret = pthread_attr_setstacksize(&attr, desired_stack_size)) != 0) {
	    fprintf(stderr, "pthread_attr_setstacksize: %s\n", strerror(ret));
	}
    }
    if ((ret = pthread_create(&session->dsp_thread, &attr, transport_dsp_thread_fn, (void *)tl)) != 0) {
	fprintf(stderr, "pthread_create: %s\n", strerror(ret));
    }

    sem_wait(tl->unpause_sem);
    audioconn_start_playback(session->audio_io.playback_conn);

    PageEl *el = panel_area_get_el_by_id(session->gui.panels, "panel_quickref_play");
    Textbox *play_button = ((Button *)el->component)->tb;
    textbox_set_background_color(play_button, &colors.play_green);
}

void transport_stop_playback()
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    audioconn_stop_playback(session->audio_io.playback_conn);

    if (session->dsp_thread) pthread_cancel(session->dsp_thread);
    /* Unblock DSP thread */
    for (int i=0; i<512; i++) {
	sem_post(tl->writable_chunks);
	sem_post(tl->readable_chunks);
    }

    /* Exhaust all sems */
    while (sem_trywait(tl->unpause_sem) == 0) {};
    while (sem_trywait(tl->writable_chunks) == 0) {};
    while (sem_trywait(tl->readable_chunks) == 0) {};

    /* Reset writeable chunks sem */
    for (int i=0; i<session->proj.fourier_len_sframes * RING_BUF_LEN_FFT_CHUNKS / session->proj.chunk_size_sframes; i++) {
	/* fprintf(stdout, "\t->reinitiailizing writable chunks\n"); */
	sem_post(tl->writable_chunks);
    }
    tl->buf_read_pos = 0;
    tl->buf_write_pos = 0;
    session->playback.playing = false;
    /* pthread_kill(proj->dsp_thread, SIGINT); */
    /* fprintf(stdout, "Cancelled!\n"); */
    session->source_mode.src_play_speed = 0.0f;
    session->playback.play_speed = 0.0f;

    session_do_ongoing_changes(session, JDAW_THREAD_DSP);
    session_flush_val_changes(session, JDAW_THREAD_DSP);
    session_flush_callbacks(session, JDAW_THREAD_DSP);

    
    PageEl *el = panel_area_get_el_by_id(session->gui.panels, "panel_quickref_play");
    Textbox *play_button = ((Button *)el->component)->tb;
    textbox_set_background_color(play_button, &colors.quickref_button_blue);

}

void transport_start_recording()
{
    transport_stop_playback();
    /* session->playback.play_speed = 1.0f; */
    timeline_play_speed_set(1.0);
    transport_start_playback();

    AudioConn *conns_to_activate[MAX_PROJ_AUDIO_CONNS];
    uint8_t num_conns_to_activate = 0;
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    tl->record_from_sframes = tl->play_pos_sframes;
    AudioConn *conn;
    /* for (int i=0; i<proj->num_record_conns; i++) { */
    /* 	if ((dev = proj->record_conns[i]) && dev->active) { */
    /* 	    conn_open(proj, dev); */
    /* 	    SDL_PauseAudioConn(dev->id, 0); */
    /* 	    Clip *clip = project_add_clip(dev); */
    /* 	    clip->recording = true; */
    /* 	} */
    /* } */
    bool no_tracks_active = true;
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	Track *track = tl->tracks[i];
	/* Clip *clip = NULL; */
	/* bool home = false; */
	if (track->active) {
	    no_tracks_active = false;
	    if (track->input_type == AUDIO_CONN) {
		Clip *clip = NULL;
		AudioConn *conn = track->input;
		if (!(conn->active)) {
		    conn->active = true;
		    conns_to_activate[num_conns_to_activate] = conn;
		    num_conns_to_activate++;
		    if (audioconn_open(session, conn) != 0) {
			fprintf(stderr, "Error opening audio device to record\n");
			return;
		    }
		    clip = clip_create(conn, track);
		    clip->recording = true;
		    /* home = true; */
		    conn->current_clip = clip;
		    conn->current_clip_repositioned = false;
		} else {
		    clip = conn->current_clip;
		    conn->current_clip_repositioned = false;
		}
		clipref_create(track, tl->record_from_sframes, CLIP_AUDIO, clip);
	    } else if (track->input_type == MIDI_DEVICE) {
		MIDIDevice *mdevice = track->input;
		if (!mdevice->info->opened) {
		    midi_device_open(mdevice);
		}
		MIDIClip *mclip = midi_clip_create(mdevice, track);
		mclip->recording = true;
		mdevice->current_clip = mclip;
		/* conn->current_clip_repositioned = false; */
		clipref_create(track, tl->record_from_sframes, CLIP_MIDI, mclip);
	    }
	}
    }
    if (no_tracks_active) {
	Track *track = tl->tracks[tl->track_selector];
	if (!track) {
	    return;
	}
	/* Clip *clip = NULL; */
	/* bool home = false; */
	if (track->input_type == AUDIO_CONN) {
	    conn = track->input;
	    Clip *clip = NULL;
	    if (!(conn->active)) {
		conn->active = true;
		conns_to_activate[num_conns_to_activate] = conn;
		num_conns_to_activate++;
		if (audioconn_open(session, conn) != 0) {
		    fprintf(stderr, "Error opening audio device to record\n");
		    exit(1);
		}
		clip = clip_create(conn, track);
		clip->recording = true;
		/* home = true; */
		conn->current_clip = clip;
		conn->current_clip_repositioned = false;
	    } else {
		clip = conn->current_clip;
		conn->current_clip_repositioned = false;
	    }
	    /* Clip ref is created as "home", meaning clip data itself is associated with this ref */
	    clipref_create(track, tl->record_from_sframes, CLIP_AUDIO, clip);
	} else if (track->input_type == MIDI_DEVICE) {
	    MIDIDevice *mdevice = track->input;
	    if (!mdevice->info->opened) {
		midi_device_open(mdevice);
	    }
	    MIDIClip *mclip = midi_clip_create(mdevice, track);
	    mclip->recording = true;
	    mdevice->current_clip = mclip;
	    /* conn->current_clip_repositioned = false; */
	    clipref_create(track, tl->record_from_sframes, CLIP_MIDI, mclip);
	}
    }

    for (uint8_t i=0; i<num_conns_to_activate; i++) {
	conn = conns_to_activate[i];
	if (!conn->open) {
	    audioconn_open(session, conn);
	}
	conn->c.device.write_bufpos_samples = 0;
	/* device_open(proj, dev); */
	/* device_start_recording(dev); */
    }
    for (uint8_t i=0; i<num_conns_to_activate; i++) {
	conn = conns_to_activate[i];
	audioconn_start_recording(conn);
    }
    session->playback.recording = true;


    PageEl *el = panel_area_get_el_by_id(session->gui.panels, "panel_quickref_record");
    Textbox *record_button = ((Button *)el->component)->tb;
    textbox_set_background_color(record_button, &colors.red);
    /* pd_jackdaw_record_get_block(); */

}

void create_clip_buffers(Clip *clip, uint32_t len_sframes)
{
    /* if (clip->L != NULL) { */

    /* 	fprintf(stderr, "Error: clip %s already has a buffer allocated\n", clip->name); */
    /* 	exit(1); */
    /* } */
    /* fprintf(stdout, "CREATING CLIP BUFFERS %d\n", len_sframes); */
    uint32_t buf_len_bytes = sizeof(float) * len_sframes;
    if (!clip->L) {
	clip->L = malloc(buf_len_bytes);
    } else {
	clip->L = realloc(clip->L, buf_len_bytes);
    }
    if (!clip->L) {
	fprintf(stderr, "Fatal error: clip buffer allocation failed\n");
	exit(1);
    }
    if (clip->channels == 2) {
	if (!clip->R) {
	    clip->R = malloc(buf_len_bytes);
	} else {
	    clip->R = realloc(clip->R, buf_len_bytes);
	}
	if (!clip->R) {
	    fprintf(stderr, "Fatal error: clip buffer allocation failed\n");
	    exit(1);
	}
    }
}

/* void copy_pd_buf_to_clip(Clip *clip) */
/* { */
/*     clip->len_sframes = clip->write_bufpos_sframes + clip->recorded_from->c.pd.write_bufpos_sframes; */
/*     create_clip_buffers(clip, clip->len_sframes); */
/*     memcpy(clip->L + clip->write_bufpos_sframes, clip->recorded_from->c.pd.rec_buffer_L, clip->recorded_from->c.pd.write_bufpos_sframes * sizeof(float)); */
/*     memcpy(clip->R + clip->write_bufpos_sframes, clip->recorded_from->c.pd.rec_buffer_R, clip->recorded_from->c.pd.write_bufpos_sframes * sizeof(float)); */
/*     clip->write_bufpos_sframes = clip->len_sframes; */
/* } */
void copy_conn_buf_to_clip(Clip *clip, enum audio_conn_type type)
{
    switch (type) {
    case DEVICE:
	clip->len_sframes = clip->write_bufpos_sframes + clip->recorded_from->c.device.write_bufpos_samples / clip->channels;
	create_clip_buffers(clip, clip->len_sframes);
	for (int i=0; i<clip->recorded_from->c.device.write_bufpos_samples; i+=clip->channels) {
	    float sample_L = (float) clip->recorded_from->c.device.rec_buffer[i] / INT16_MAX;
	    float sample_R = (float) clip->recorded_from->c.device.rec_buffer[i+1] / INT16_MAX;
	    clip->L[clip->write_bufpos_sframes + i/clip->channels] = sample_L;
	    clip->R[clip->write_bufpos_sframes + i/clip->channels] = sample_R;;
	}
	clip->write_bufpos_sframes = clip->len_sframes;

	break;
    case PURE_DATA:
	clip->len_sframes = clip->write_bufpos_sframes + clip->recorded_from->c.pd.write_bufpos_sframes;
	create_clip_buffers(clip, clip->len_sframes);
	/* if (SDL_TryLockMutex(clip->recorded_from->c.pd.buf_lock) == SDL_MUTEX_TIMEDOUT) { */
	/*     /\* SDL_UnlockMutex(clip->recorded_from->c.pd.buf_lock); *\/ */
	/*     return; */
	/* } */
	memcpy(clip->L + clip->write_bufpos_sframes, clip->recorded_from->c.pd.rec_buffer_L, clip->recorded_from->c.pd.write_bufpos_sframes * sizeof(float));
	/* if (SDL_TryLockMutex(clip->recorded_from->c.pd.buf_lock) == SDL_MUTEX_TIMEDOUT) { */
	/*     /\* SDL_UnlockMutex(clip->recorded_from->c.pd.buf_lock); *\/ */
	/*     return; */
	/* } */
	memcpy(clip->R + clip->write_bufpos_sframes, clip->recorded_from->c.pd.rec_buffer_R, clip->recorded_from->c.pd.write_bufpos_sframes * sizeof(float));
	clip->write_bufpos_sframes = clip->len_sframes;
	break;
    case JACKDAW:
	/* OUROBOROS */
	/* fprintf(stdout, "JDAW COPY BUF TO CLIP\n"); */
	clip->len_sframes = clip->write_bufpos_sframes + clip->recorded_from->c.jdaw.write_bufpos_sframes;
	create_clip_buffers(clip, clip->len_sframes);
	memcpy(clip->L + clip->write_bufpos_sframes, clip->recorded_from->c.jdaw.rec_buffer_L, clip->recorded_from->c.jdaw.write_bufpos_sframes * sizeof(float));
	memcpy(clip->R + clip->write_bufpos_sframes, clip->recorded_from->c.jdaw.rec_buffer_R, clip->recorded_from->c.jdaw.write_bufpos_sframes * sizeof(float));
	clip->write_bufpos_sframes = clip->len_sframes;
	clip->recorded_from->c.jdaw.write_bufpos_sframes = 0;
	/* clip->len_sframes = clip->write_bufpos_sframes; */
	break;
    default:
	break;
    }
}

static NEW_EVENT_FN(undo_record_new_clips, "undo create new clip(s)")
    ClipRef **clips = (ClipRef **)obj1;
    uint8_t num = val1.uint8_v;
    for (uint8_t i=0; i<num; i++) {
	clipref_delete(clips[i]);
    }
}

static NEW_EVENT_FN(redo_record_new_clips, "undo create new clip(s)")
    ClipRef **clips = (ClipRef **)obj1;
    uint8_t num = val1.uint8_v;
    for (uint8_t i=0; i<num; i++) {
	clipref_undelete(clips[i]);
    }
}

static NEW_EVENT_FN(dispose_forward_record_new_clips, "")
    ClipRef **clips = (ClipRef **)obj1;
    uint8_t num = val1.uint8_v;
    for (uint8_t i=0; i<num; i++) {
	ClipRef *cr = clips[i];
	clipref_destroy_no_displace(cr);
    }
}

void transport_stop_recording()
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;
    ClipRef **created_clips = calloc(tl->num_tracks * 2, sizeof(ClipRef *));
    uint16_t num_created = 0;
    for (uint16_t i=session->proj.active_clip_index; i<session->proj.num_clips; i++) {
	Clip *clip = session->proj.clips[i];
	if (clip->len_sframes == 0) {
	    for (int i=0; i<clip->num_refs; i++) {
		if (clip->refs[i]->grabbed) clipref_ungrab(clip->refs[i]);
	    }
	    clip_destroy(clip);
	} else {
	    for (uint16_t j=0; j<clip->num_refs; j++) {
		ClipRef *ref = clip->refs[j];
		if (num_created >= tl->num_tracks * 2 - 1) {
		    created_clips = realloc(created_clips, num_created * 2 * sizeof(ClipRef *));
		}
		created_clips[num_created] = ref;
		num_created++;
	    }
	}
    }
    created_clips = realloc(created_clips, num_created * sizeof(ClipRef *));
    Value num_created_v = {.uint8_v = num_created};
    user_event_push(
	
	undo_record_new_clips,
	redo_record_new_clips,
	NULL,
	dispose_forward_record_new_clips,
	(void *)created_clips,
	NULL,
	num_created_v,num_created_v,num_created_v,num_created_v,
	0, 0, true, false);
	    
	    

    transport_stop_playback();
    AudioConn *conn;
    AudioConn *conns_to_close[64];
    uint8_t num_conns_to_close = 0;
    for (int i=0; i<session->audio_io.num_record_conns; i++) {
	if ((conn = session->audio_io.record_conns[i]) && conn->active) {
	    audioconn_stop_recording(conn);
	    conns_to_close[num_conns_to_close] = conn;
	    num_conns_to_close++;
	    /* audioconn_close(conn); */
	    conn->active = false;
	}
    }
    session->playback.recording = false;

    while (session->proj.active_clip_index < session->proj.num_clips) {
	Clip *clip = session->proj.clips[session->proj.active_clip_index];
	/* fprintf(stdout, "CLIP BUFFERS: %p, %p\n", clip->L, clip->R); */
	/* exit(0); */
	if (!clip->recorded_from) {
	    session->proj.active_clip_index++;
	    continue;
	}
	switch (clip->recorded_from->type) {
	case DEVICE:
	    copy_conn_buf_to_clip(clip, DEVICE);
	    break;
	case PURE_DATA:
	    copy_conn_buf_to_clip(clip, PURE_DATA);
	    /* complete_pd_clip(clip); */
	    break;
	case JACKDAW:
	    copy_conn_buf_to_clip(clip, JACKDAW);
	    break;
	    
	}
	/* fprintf(stderr, "SETTING %s rec to false\n", clip->name); */
	clip->recording = false;
	session->proj.active_clip_index++;
    }
    session->proj.active_midi_clip_index = session->proj.num_midi_clips;

    while (num_conns_to_close > 0) {
	audioconn_close(conns_to_close[--num_conns_to_close]);
    }
    for (int i=0; i<session->midi_io.num_inputs; i++) {
	MIDIDevice *d = session->midi_io.inputs + i;
	if (!d->info->is_virtual) {
	    midi_device_close(d);
	}
    }
    PageEl *el = panel_area_get_el_by_id(session->gui.panels, "panel_quickref_record");
    Textbox *record_button = ((Button *)el->component)->tb;
    textbox_set_background_color(record_button, &colors.quickref_button_blue );
}

void transport_set_mark(Timeline *tl, bool in)
{
    Session *session = session_get();
    if (!session->source_mode.source_mode) {
	if (in) {
	    tl->in_mark_sframes = tl->play_pos_sframes;
	} else {
	    tl->out_mark_sframes = tl->play_pos_sframes;
	}
	timeline_reset_loop_play_lemniscate(tl);
    } else {
	if (in) {
	    session->source_mode.src_in_sframes = session->source_mode.src_play_pos_sframes;
	} else {
	    session->source_mode.src_out_sframes = session->source_mode.src_play_pos_sframes;
	}
    }
    tl->needs_redraw = true;
}

void transport_set_mark_to(Timeline *tl, int32_t pos, bool in)
{
    Session *session = session_get();
    if (tl) {
	if (in) {
	    tl->in_mark_sframes = pos;
	} else {
	    tl->out_mark_sframes = pos;
	}
	timeline_reset_loop_play_lemniscate(tl);
    } else if (session->source_mode.source_mode) {
	if (in) {
	    session->source_mode.src_in_sframes = pos;
	} else {
	    session->source_mode.src_out_sframes = pos;
	}
    }
}

void transport_goto_mark(Timeline *tl, bool in)
{
    if (in) {
	timeline_set_play_position(tl, tl->in_mark_sframes);
    } else {
	timeline_set_play_position(tl, tl->out_mark_sframes);
    }
}

/* Called in main thread to update clipref GUIs */
void transport_recording_update_cliprects()
{
    Session *session = session_get();
    /* fprintf(stdout, "update clip rects...\n"); */
    for (int i=session->proj.active_clip_index; i<session->proj.num_clips; i++) {
	/* fprintf(stdout, "updating %d/%d\n", i, proj->num_clips); */
	Clip *clip = session->proj.clips[i];

	if (!clip->recorded_from) continue; /* E.g. wav loaded during recording */
	switch(clip->recorded_from->type) {
	case DEVICE:
	    clip->len_sframes = clip->recorded_from->c.device.write_bufpos_samples / clip->channels + clip->write_bufpos_sframes;
	    break;
	case PURE_DATA:
	    clip->len_sframes = clip->recorded_from->c.pd.write_bufpos_sframes + clip->write_bufpos_sframes;
	    break;
	case JACKDAW:
	    clip->len_sframes = clip->recorded_from->c.jdaw.write_bufpos_sframes + clip->write_bufpos_sframes;
	    break;
	}

	for (uint16_t j=0; j<clip->num_refs; j++) {
	    ClipRef *cr = clip->refs[j];
	    cr->end_in_clip = clip->len_sframes;
	    clipref_reset(cr, false);
	}
    }
    for (int i=session->proj.active_midi_clip_index; i<session->proj.num_midi_clips; i++) {
	/* fprintf(stdout, "updating %d/%d\n", i, proj->num_clips); */
	MIDIClip *mclip = session->proj.midi_clips[i];

	for (uint16_t j=0; j<mclip->num_refs; j++) {
	    ClipRef *cr = mclip->refs[j];
	    cr->end_in_clip = mclip->len_sframes;
	    clipref_reset(cr, false);
	}
    }
}
