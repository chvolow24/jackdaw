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

#include <string.h>
#include <sys/errno.h>
#include "porttime.h"
#include "audio_clip.h"
#include "audio_connection.h"
#include "clipref.h"
#include "color.h"
#include "consts.h"
#include "dsp_utils.h"
#include "error.h"
#include "log.h"
#include "midi_clip.h"
#include "midi_io.h"
#include "midi_qwerty.h"
#include "user_event.h"
#include "mixdown.h"
#include "piano_roll.h"
#include "project.h"
#include "pure_data.h"
#include "session.h"
#include "session_endpoint_ops.h"
#include "timeline.h"
#include "transport.h"

#define JDAW_TRANSPORT_LOG_ALL
#define JDAW_TRANSPORT_PRINT_ALL

#define TRANSPORT_PERFORMANCE_LOG_TICKS_PER 10
static bool transport_performance_logging = false;
static int transport_performance_log_elapsed_ticks = 0;
static double dur_proc = 0.0;
static double dur_wait = 0.0;


#ifdef TESTBUILD
void toggle_transport_logging()
{
    transport_performance_logging = !transport_performance_logging;
}

void transport_log(const char *fmt, ...)
{
    if (transport_performance_logging) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
    }
}
#else
void transport_log(const char *fmt, ...) {}
void toggle_transport_logging();
#endif


double timespec_elapsed_ms(const struct timespec *start, const struct timespec *end) {
    long sec_diff = end->tv_sec - start->tv_sec;
    long nsec_diff = end->tv_nsec - start->tv_nsec;
    return (sec_diff * 1e3) + (nsec_diff * 1e-6);
}

static struct timespec glob_start, glob_end;
static void timer_start()
{
    clock_gettime(CLOCK_REALTIME, &glob_start);
}

static void timer_stop_and_print(const char *msg)
{
    clock_gettime(CLOCK_REALTIME, &glob_end);
    transport_log("(%f): %s\n", timespec_elapsed_ms(&glob_start, &glob_end), msg);
}

extern struct colors colors;

/* static bool do_quit_internal; */
struct dsp_chunk_info {
    int32_t tl_start;
    /* int32_t ring_buf_start; */
    float playspeed;
    int elapsed_playback_chunks;
};

static void copy_device_buf_to_clips(AudioDevice *dev);
void copy_conn_buf_to_clip(Clip *clip, enum audio_conn_type type);
					 
void transport_record_callback(void* user_data, uint8_t *stream, int len)
{
    AudioDevice *dev = user_data;    
    uint32_t stream_len_samples = len / sizeof(int16_t);

    /* Simple latency compensation */
    /* if (!session->playback.new_cliprefs_repositioned) { */
    /* 	/\* TODO: real latency compensation (probably with PortAudio *\/ */
    /* 	int32_t playback_latency_sframes = session->proj.chunk_size_sframes * 5; */
    /* 	int32_t recorded_chunk_tl_pos = ACTIVE_TL->play_pos_sframes - playback_latency_sframes - stream_len_samples / dev->spec.channels; */
    /* 	for (uint16_t i = session->proj.active_clip_index; i<session->proj.num_clips; i++) { */
    /* 	    Clip *clip = session->proj.clips[i]; */
    /* 	    if (!clip->recording) break; */
    /* 	    for (int i=0; i<clip->num_refs; i++) { */
    /* 		ClipRef *cr = clip->refs[i]; */
    /* 		fprintf(stderr, "Tl pos %d->%d\n", cr->tl_pos, recorded_chunk_tl_pos); */
    /* 		cr->tl_pos = recorded_chunk_tl_pos; */
    /* 	    } */
    /* 	} */
    /* 	session->playback.new_cliprefs_repositioned = true; */
    /* } */


    /* If there's room in the device record buffer, copy directly to that */
    if (dev->write_bufpos_samples + stream_len_samples < dev->rec_buf_len_samples) {
	memcpy(dev->rec_buffer + dev->write_bufpos_samples, stream, len);
	dev->write_bufpos_samples += stream_len_samples;
    } else { /* Dump data to clip(s) before writing to dev buffer */
	copy_device_buf_to_clips(dev);
	/* Now that there's room, copy to rec buffer */
	memcpy(dev->rec_buffer, stream, len);
	dev->write_bufpos_samples = stream_len_samples;
     }
 }

static float *get_source_mode_chunk(uint8_t channel, float *chunk, uint32_t len_sframes, int32_t start_pos_sframes, float step)
 {
     /* float *chunk = malloc(sizeof(float) * len_sframes); */
     /* if (!chunk) { */
     /* 	 fprintf(stderr, "Error: unable to allocate chunk from source clip\n"); */
     /* } */
     Session *session = session_get();
     Clip *clip = NULL;
     if (session->source_mode.src_clip_type == CLIP_AUDIO) {
	 clip = session->source_mode.src_clip;
     } else {
	 return NULL; /* TODO: source mode for MIDI */
     }
     if (!clip) return NULL; /* band-aid for a rare bug */
     
     float *src_buffer = clip->channels == 1 ? clip->L :
	 channel == 0 ? clip->L : clip->R;


     for (uint32_t i=0; i<len_sframes; i++) {
	 int sample_i = (int) (i * step + start_pos_sframes);
	 if (sample_i < clip->len_sframes && sample_i > 0) {
	     chunk[i] = src_buffer[sample_i];
	 } else {
	     chunk[i] = 0;
	 }
     }
     return chunk;
 }

/* void transport_recording_update_cliprects(); */

/* static inline float clip(float f) */
/* { */
/*     if (f > 1.0) return 1.0; */
/*     if (f < -1.0) return -1.0; */
/*     return f; */
/* } */

#define MAX_QUEUED_BUFS 64
struct transport_buf_queue {
    int num_queued;
    QueuedBuf queue[MAX_QUEUED_BUFS];
};

static struct transport_buf_queue queue_loc;

static void loc_dequeue_buf(int index)
{
    if (queue_loc.queue[index].free_when_done) {
	free(queue_loc.queue[index].buf[0]);
	if (queue_loc.queue[index].channels > 1) {
	    free(queue_loc.queue[index].buf[1]);
	}
    }
    if (index < queue_loc.num_queued - 1) {
	memmove(queue_loc.queue + index, queue_loc.queue + index + 1, (queue_loc.num_queued - index - 1) * sizeof(QueuedBuf));
    }
    queue_loc.num_queued--;
}

static void loc_queue_bufs(QueuedBuf *qb, int num_bufs)
{
    if (queue_loc.num_queued + num_bufs > MAX_QUEUED_BUFS) {
	fprintf(stderr, "Error: reached max num queued bufs\n");
	return;
    }
    memcpy(queue_loc.queue + queue_loc.num_queued, qb, num_bufs * sizeof(QueuedBuf));
    queue_loc.num_queued += num_bufs;
}

static void loc_queued_bufs_add(float *chunk_L, float *chunk_R, int len_sframes)
{
    for (int i=0; i<queue_loc.num_queued; i++) {
	int chunk_start = 0;
	QueuedBuf *qb = queue_loc.queue + i;
	/* fprintf(stderr, "\t%d: play after: %d\n", i, qb->play_after_sframes); */
	if (qb->play_after_sframes > len_sframes) {
	    qb->play_after_sframes -= len_sframes;
	    continue;
	} else if (qb->play_after_sframes > 0) {
	    chunk_start = qb->play_after_sframes;
	    qb->play_after_sframes = 0;
	}
	int len_rem = qb->len_sframes - qb->play_index;
	int add_len = len_rem < len_sframes - chunk_start ? len_rem : len_sframes - chunk_start;
	float_buf_add(chunk_L + chunk_start, qb->buf[0] + qb->play_index, add_len);
	if (qb->channels > 1) {
	    float_buf_add(chunk_R + chunk_start, qb->buf[1] + qb->play_index, add_len);
	} else {
	    float_buf_add(chunk_R + chunk_start, qb->buf[0] + qb->play_index, add_len);
	}
	qb->play_index += add_len;
	/* Buf is finished; remove from queue and decrement i to avoid skipping anything */
	if (qb->play_index >= qb->len_sframes) {
	    loc_dequeue_buf(i);
	    i--;
	}
    }
}

void transport_playback_callback(void* user_data, uint8_t* stream, int len)
{
    Session *session = session_get();
    set_thread_id(JDAW_THREAD_PLAYBACK);

    /* Take care of queued audio bufs */
    int err;
    if ((err = pthread_mutex_lock(&session->queued_ops.queued_audio_buf_lock)) != 0) {
	fprintf(stderr, "Error locking queued audio buf lock (in playback cb): %s\n", strerror(err));
    }
    loc_queue_bufs(session->queued_ops.queued_audio_bufs, session->queued_ops.num_queued_audio_bufs);
    session->queued_ops.num_queued_audio_bufs = 0;
    if ((err = pthread_mutex_unlock(&session->queued_ops.queued_audio_buf_lock)) != 0) {
	fprintf(stderr, "Error unlocking queued audio buf lock (in playback cb): %s\n", strerror(err));
    }
    if (!session->playback.playing && !session->midi_io.monitoring && queue_loc.num_queued == 0) {
	memset(stream, '\0', len);
	return;
    }


    Project *proj = &session->proj;
    Timeline *tl = ACTIVE_TL;
    AudioDevice *dev = (AudioDevice *)user_data;

    memset(stream, '\0', len);
    uint32_t stream_len_samples = len / sizeof(int16_t);
    uint32_t len_sframes = stream_len_samples / proj->channels;
    float chunk_L[len_sframes];
    float chunk_R[len_sframes];
    memset(chunk_L, '\0', sizeof(chunk_L));
    memset(chunk_R, '\0', sizeof(chunk_L));
    
    /* Gather data from timeline, generated in DSP threadfn */
    if (session->playback.playing) {
	if (session->source_mode.source_mode) {
	    get_source_mode_chunk(0, chunk_L, len_sframes, session->source_mode.src_play_pos_sframes, session->source_mode.src_play_speed);
	    get_source_mode_chunk(1, chunk_R, len_sframes, session->source_mode.src_play_pos_sframes, session->source_mode.src_play_speed);
	} else {
	    int wait_count = 0;
	    while (sem_trywait(tl->readable_chunks) != 0) {
		wait_count++;
		if (wait_count > 100) {
		    transport_log("Playback callback early exit (can't wait on readable chunks)\n");
		    return;
		}
	    }
	    memcpy(chunk_L, tl->buf_L + tl->buf_read_pos, sizeof(float) * len_sframes);
	    memcpy(chunk_R, tl->buf_R + tl->buf_read_pos, sizeof(float) * len_sframes);
	    tl->buf_read_pos += len_sframes;
	    if (tl->buf_read_pos >= proj->fourier_len_sframes * RING_BUF_LEN_FFT_CHUNKS) {
		tl->buf_read_pos = 0;
	    }
	}
    }

    /* transport_log("playback callback, cleared buffer..\n"); */
    /* Check for monitor synth and add buf to chunk_L and chunk_R */
    MIDIDevice *d = session->midi_io.monitor_device;
    Synth *s = session->midi_io.monitor_synth;
    if (d && s) {
	midi_device_read(d);
	float playspeed = session->playback.play_speed;
	if (session->piano_roll) {
	    piano_roll_feed_midi(d->buffer, d->num_unconsumed_events);
	}
	synth_feed_midi(s, d->buffer, d->num_unconsumed_events, 0, true);
	if (d->current_clip && d->current_clip->recording) {
	    midi_device_output_chunk_to_clip(d, 1);
	    d->current_clip->len_sframes += len_sframes;
	}
	d->num_unconsumed_events = 0;
	if (fabs(playspeed) < 1e-6 || !session->playback.playing) playspeed = 1.0f;

	/* Allocate half of chunk time to synth */
	double alloced_msec = 0.25 * 1000.0 * session->proj.chunk_size_sframes / session->proj.sample_rate;
	synth_add_buf(s, chunk_L, 0, len_sframes, playspeed, true, alloced_msec); /* TL Pos ignored */
	synth_add_buf(s, chunk_R, 1, len_sframes, playspeed, true, alloced_msec); /* TL Pos ignored */
    }

    /* Check for queued bufs and add to chunk_L and chunk_R */
    loc_queued_bufs_add(chunk_L, chunk_R, len_sframes);

    float_buf_mult_const(chunk_L, session->playback.output_vol, len_sframes);
    float_buf_mult_const(chunk_R, session->playback.output_vol, len_sframes);
    int16_t *stream_fmt = (int16_t *)stream;
    for (uint32_t i=0; i<stream_len_samples; i+=2)
    {
	float val_L = chunk_L[i/2];
	float val_R = chunk_R[i/2];
	envelope_follower_sample(&session->proj.output_L_ef, val_L);
	envelope_follower_sample(&session->proj.output_R_ef, val_R);
	stream_fmt[i] = (int16_t)(clip_float_sample(val_L) * INT16_MAX);
	stream_fmt[i+1] = (int16_t)(clip_float_sample(val_R) * INT16_MAX);
    }

    if (session->source_mode.source_mode && session->source_mode.src_clip_type == CLIP_AUDIO) {
	Clip *clip = session->source_mode.src_clip;
	if (clip) {
	    session->source_mode.src_play_pos_sframes += round(session->source_mode.src_play_speed * stream_len_samples / clip->channels);	    
	    if (session->source_mode.src_play_pos_sframes < 0) {
		session->source_mode.src_play_pos_sframes = 0;
	    } else if (session->source_mode.src_play_pos_sframes >= clip->len_sframes) {
		session->source_mode.src_play_pos_sframes = clip->len_sframes - 1;
	    }
	}
	tl->needs_redraw = true;
    } else if (session->playback.playing) {
	/* timer_start(); */
	struct dsp_chunk_info *chunk_info = tl->dsp_chunks_info + tl->dsp_chunks_info_read_i;
	chunk_info->elapsed_playback_chunks++;
	int32_t new_play_pos = chunk_info->tl_start + proj->chunk_size_sframes * chunk_info->elapsed_playback_chunks * chunk_info->playspeed;
	timeline_move_play_position(tl, new_play_pos - tl->play_pos_sframes);
	int N = proj->fourier_len_sframes / proj->chunk_size_sframes;
	if (chunk_info->elapsed_playback_chunks >= N) {
	    tl->dsp_chunks_info_read_i++;
	    if (tl->dsp_chunks_info_read_i >= RING_BUF_LEN_FFT_CHUNKS) {
		tl->dsp_chunks_info_read_i = 0;
	    }
	}
	sem_post(tl->writable_chunks);
	/* timer_stop_and_print("Did playback_things"); */
    }
    /* timer_start(); */
    session_do_ongoing_changes(session, JDAW_THREAD_PLAYBACK);
    session_flush_val_changes(session, JDAW_THREAD_PLAYBACK);
    session_flush_callbacks(session, JDAW_THREAD_PLAYBACK);
    /* timer_stop_and_print("Did ongoing changes"); */
    /* transport_log("...done ongoing changes\n"); */

    if (dev->channel_dsts[0].conn->request_playhead_reset) {
	int32_t saved_write_pos = tl->buf_write_pos;
	
	/* Give DSP thread a new starting position */
	tl->read_pos_sframes = dev->channel_dsts[0].conn->request_playhead_pos;

	/* "Read" the rest of the mixdown buffer so DSP restarts */
	while (tl->buf_read_pos != saved_write_pos) {
	    int semret = sem_trywait(tl->readable_chunks);
	    if (semret != 0) {
		error_exit("ERROR: unable to wait on readable chunks! sem_trywait: %s", strerror(errno));
	    }
	    tl->buf_read_pos += len_sframes;
	    sem_post(tl->writable_chunks);
	    if (tl->buf_read_pos >= proj->fourier_len_sframes * RING_BUF_LEN_FFT_CHUNKS) {
		tl->buf_read_pos = 0;
	    }
	}

	/* Reset the dsp chunks info indices */
	tl->dsp_chunks_info_read_i = 0;
	tl->dsp_chunks_info_write_i = 0;

	dev->channel_dsts[0].conn->request_playhead_reset = false;
    }
    /* if (log_fn_exit) { */
    /* 	log_tmp(LOG_DEBUG, "Exiting playback callback\n"); */
    /* } */
}

static volatile bool cancel_dsp_thread = false;

static void *transport_dsp_thread_fn(void *arg)
{
    Session *session = session_get();
    set_thread_id(JDAW_THREAD_DSP);
    log_tmp(LOG_INFO, "DSP thread init\n");
    
    Timeline *tl = (Timeline *)arg;
    
    int len = tl->proj->fourier_len_sframes;
    float buf_L[len];
    float buf_R[len];

    int N = len / tl->proj->chunk_size_sframes;
    bool init = true;

    if (!tl->dsp_chunks_info) {
	tl->dsp_chunks_info = calloc(RING_BUF_LEN_FFT_CHUNKS, sizeof(struct dsp_chunk_info));
	tl->dsp_chunks_info_read_i = 0;
	tl->dsp_chunks_info_write_i = 0;
    }
    
    cancel_dsp_thread = false;
    while (!cancel_dsp_thread) {
	/* transport_log("Loop iter\n"); */
	/* Performance timer */
	struct timespec tspec_start;
	struct timespec tspec_end;
	/* double dur_proc = 0.0; */
	/* double dur_wait = 0.0; */
	if (transport_performance_logging) {
	    clock_gettime(CLOCK_REALTIME, &tspec_start);
	}

	/* pthread_testcancel(); */
	float play_speed = session->playback.play_speed;
	/* tl->last_read_playspeed = play_speed; */
	/* tl->current_dsp_chunk_start = tl->read_pos_sframes; */
	
	/* GET MIXDOWN */	
	get_mixdown_chunk(tl, buf_L, 0, len, tl->read_pos_sframes, play_speed);
	get_mixdown_chunk(tl, buf_R, 1, len, tl->read_pos_sframes, play_speed);
	

	/* DSP */
	float dL[len * 2];
	float dR[len * 2];
	for (int i=0; i<len; i++) {
	    float hamming_v = HAMMING_SCALAR * hamming(i, len);
	    dL[i] = hamming_v * buf_L[i];
	    dR[i] = hamming_v * buf_R[i];
	}
	memset(dL + len, 0, sizeof(float) * len);
	memset(dR + len, 0, sizeof(float) * len);
	double complex lfreq[len * 2];
	double complex rfreq[len * 2];

	FFTf(dL, lfreq, len * 2);
	FFTf(dR, rfreq, len * 2);

	get_magnitude(lfreq, tl->proj->output_L_freq, len);
	get_magnitude(rfreq, tl->proj->output_R_freq, len);

	/* End processing */
	if (transport_performance_logging) {
	    clock_gettime(CLOCK_REALTIME, &tspec_end);
	    dur_proc += timespec_elapsed_ms(&tspec_start, &tspec_end);
	    clock_gettime(CLOCK_REALTIME, &tspec_start);
	}

	/* Copy buffer */
	
	for (int i=0; i<N; i++) {
	    sem_wait(tl->writable_chunks);
	}

	if (transport_performance_logging) {
	    clock_gettime(CLOCK_REALTIME, &tspec_end);
	    dur_wait += timespec_elapsed_ms(&tspec_start, &tspec_end);
	    clock_gettime(CLOCK_REALTIME, &tspec_start);
	}

	memcpy(tl->buf_L + tl->buf_write_pos, buf_L, sizeof(float) * len);
	memcpy(tl->buf_R + tl->buf_write_pos, buf_R, sizeof(float) * len);
	memcpy(tl->proj->output_L, buf_L, sizeof(float) * len);
	memcpy(tl->proj->output_R, buf_R, sizeof(float) * len);

	for (uint16_t i=tl->proj->active_clip_index; i<tl->proj->num_clips; i++) {
	    Clip *clip = tl->proj->clips[i];
	    AudioConn *conn = clip->recorded_from;
	    if (!conn) continue;
	    if (conn->type == JACKDAW) {
		JDAWConn *jconn = conn->obj;
		/* if (jconn->write_bufpos_sframes + len < jconn->rec_buf_len_sframes) { */
		memcpy(jconn->rec_buffer_L + jconn->write_bufpos_sframes, buf_L, sizeof(float) * len);
		memcpy(jconn->rec_buffer_R + jconn->write_bufpos_sframes, buf_R, sizeof(float) * len);
		jconn->write_bufpos_sframes += len;
		if (jconn->write_bufpos_sframes + 2 * len >= jconn->rec_buf_len_sframes) {
		    copy_conn_buf_to_clip(clip, JACKDAW);
		}
		break;
	    }
	}

	/* Log chunk info for playback */
	struct dsp_chunk_info *chunk_info = tl->dsp_chunks_info + tl->dsp_chunks_info_write_i;
	chunk_info->elapsed_playback_chunks = 0;
	chunk_info->tl_start = tl->read_pos_sframes;
	/* chunk_info->ring_buf_start = tl->buf_write_pos; */
	chunk_info->playspeed = play_speed;
	tl->dsp_chunks_info_write_i++;
	if (tl->dsp_chunks_info_write_i >= RING_BUF_LEN_FFT_CHUNKS) tl->dsp_chunks_info_write_i = 0;


	/* Increment the playback ring buffer write pos */
	tl->buf_write_pos += len;
	if (tl->buf_write_pos >= len * RING_BUF_LEN_FFT_CHUNKS) {
	    tl->buf_write_pos = 0;
	}
	
	/* Move the read (DSP) pos */
	tl->read_pos_sframes += len * play_speed;
	
	/* TODO: needs work post play position refactor */
	if (session->playback.loop_play) {
	    int32_t loop_len = tl->out_mark_sframes - tl->in_mark_sframes;
	    if (loop_len > 0) {
		int64_t remainder = tl->read_pos_sframes - tl->out_mark_sframes;
		/* int32_t remainder = tl->read_pos_sframes - tl->out_mark_sframes; */
		if (remainder > 0) {
		    if (remainder > loop_len) remainder = 0;
		    tl->read_pos_sframes = tl->in_mark_sframes + remainder;
		    timeline_flush_unclosed_midi_notes();
		}
	    }
	}
	
	for (int i=0; i<N; i++) {
	    sem_post(tl->readable_chunks);
	}
	if (init) {
	    sem_post(tl->unpause_sem);
	    init = false;
	}
	
	session_do_ongoing_changes(session, JDAW_THREAD_DSP);
	session_flush_val_changes(session, JDAW_THREAD_DSP);
	session_flush_callbacks(session, JDAW_THREAD_DSP);

	if (transport_performance_logging) {
	    clock_gettime(CLOCK_REALTIME, &tspec_end);
	    dur_proc += timespec_elapsed_ms(&tspec_start, &tspec_end);

	    if (transport_performance_log_elapsed_ticks < TRANSPORT_PERFORMANCE_LOG_TICKS_PER) {
		transport_performance_log_elapsed_ticks++;
	    } else {
		double alloced_msec = transport_performance_log_elapsed_ticks * 1000.0 * (double)session->proj.fourier_len_sframes / session_get_sample_rate();
		transport_log("TOTAL: %.2fms\n\t%.2fms (%.2f%%) proc\n\t%.2f ms (%.2f%%) wait\n\tstress: %.2f\n",
			      dur_proc + dur_wait,
			      dur_proc, 100 * dur_proc / alloced_msec,
			      dur_wait, 100 * dur_wait / alloced_msec,
			      dur_proc / alloced_msec);
		dur_proc = 0.0;
		dur_wait = 0.0;
		transport_performance_log_elapsed_ticks = 0;
	    }
	}
    }
    log_tmp(LOG_INFO, "DSP thread exit\n");
    sem_post(tl->unpause_sem);

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
    size_t desired_stack_size = 2 * sizeof(double) * session_get_sample_rate();
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
    if ((ret = pthread_create(get_thread_addr(JDAW_THREAD_DSP), &attr, transport_dsp_thread_fn, (void *)tl)) != 0) {
	fprintf(stderr, "pthread_create: %s\n", strerror(ret));
    }

    sem_wait(tl->unpause_sem);
    if (audioconn_start_playback(session->audio_io.playback_conn) < 0) {
	log_tmp(LOG_ERROR, "In start playback, audio connection could not be opened; stopping playback.\n");
	transport_stop_playback();
	return;
    }

    PageEl *el = panel_area_get_el_by_id(session->gui.panels, "panel_quickref_play");
    Textbox *play_button = ((Button *)el->component)->tb;
    textbox_set_background_color(play_button, &colors.play_green);
}

/* Account for continuous-playback ramifications of calls to timeline_set_play_position(); */
void transport_execute_playhead_jump(Timeline *tl, int32_t new_pos)
{
    Session *session = session_get();
    if (session->playback.playing) {
	/* To be handled in playback thread */
	session->audio_io.playback_conn->request_playhead_reset = true;
	session->audio_io.playback_conn->request_playhead_pos = new_pos;
    } else {
	/* Handled now: reconcile read (DSP thread) pos and play (playback thread) pos */
	tl->read_pos_sframes = new_pos;
	tl->play_pos_sframes = new_pos;
    }
}

void transport_stop_playback()
{
    Session *session = session_get();
    if (!session->playback.playing) return;
    Timeline *tl = ACTIVE_TL;
    if (!tl) return;
    audioconn_stop_playback(session->audio_io.playback_conn);

    /* Busy waiting */
    /* uint32_t test = 0; */
    /* AudioDevice *dev = session->audio_io.playback_conn->obj; */
    /* while (dev->request_close) { */
    /* 	test++; */
    /* 	if (test > 2e8) { */
    /* 	    log_tmp(LOG_ERROR, "Audio device \"%s\" requested close, but has not closed. Closing from DSP thread.\n", session->audio_io.playback_conn->name); */
    /* 	    SDL_PauseAudioDevice(dev->id, 1); */
    /* 	    dev->playing = false; */
    /* 	    TESTBREAK; */
    /* 	    break; */
    /* 	} */
    /* } */

    cancel_dsp_thread = true;
    /* pthread_cancel(*get_thread_addr(JDAW_THREAD_DSP)); */
    
    /* Unblock DSP thread */
    for (int i=0; i<512; i++) {
	sem_post(tl->writable_chunks);
	sem_post(tl->readable_chunks);
    }

    /* Wait for DSP thread to exit */
    sem_wait(tl->unpause_sem);

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
    tl->dsp_chunks_info_read_i = 0;
    tl->dsp_chunks_info_write_i = 0;

    /* TODO: MAYBE */
    tl->read_pos_sframes = tl->play_pos_sframes;

    
    session->playback.playing = false;
    /* pthread_kill(proj->dsp_thread, SIGINT); */
    /* fprintf(stdout, "Cancelled!\n"); */
    session->source_mode.src_play_speed = 0.0f;
    /* session->playback.play_speed = 0.0f; */

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
    AudioConn *conns_to_activate[MAX_PROJ_AUDIO_CONNS];
    uint8_t num_conns_to_activate = 0;
    Session *session = session_get();
    session->playback.new_cliprefs_repositioned = false;
    Timeline *tl = ACTIVE_TL;
    tl->record_from_sframes = tl->play_pos_sframes;
    
    AudioConn *conn;
    bool activate_mqwert = false;
    bool no_tracks_active = true;
    
    /* Iterate through tracks to check for active ones (else use selected track) */
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	Track *track = tl->tracks[i];
	/* Clip *clip = NULL; */
	/* bool home = false; */
	if (track->active) {
	    no_tracks_active = false;
	    if (track->input_type == AUDIO_CONN) {
		Clip *clip = NULL;
		AudioConn *conn = track->input;
		if (!conn->active || !conn->current_clip) {
		    if (audioconn_open(session, conn) < 0) {
			log_tmp(LOG_WARN, "Error opening audio conn \"%s\" for recording; device may have already been opened\n", conn->name);
			audioconn_remove(conn);
			continue;
		    }
		    /* setting conn->active here accounts for redundancy */
		    conn->active = true; 
		    conns_to_activate[num_conns_to_activate] = conn;
		    num_conns_to_activate++;

		    clip = clip_create(conn, track); /* Sets clip num channels */
		    clip->recording = true;
		    conn->current_clip = clip;
		    conn->current_clip_repositioned = false;
		} else {
		    clip = conn->current_clip;
		    conn->current_clip_repositioned = false;
		}
		clipref_create(track, tl->record_from_sframes, CLIP_AUDIO, clip);
	    } else if (track->input_type == MIDI_DEVICE) {
		MIDIDevice *mdevice = track->input;
		mdevice->recording = true;
		midi_device_open(mdevice);
		if (mdevice->type == MIDI_DEVICE_QWERTY) {
		    activate_mqwert = true;
		}
		MIDIClip *mclip = midi_clip_create(mdevice, track);
		mdevice->record_start = Pt_Time();
		mclip->recording = true;
		/* mclilen_sframesp-> */
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
		if (audioconn_open(session, conn) < 0) {
		    log_tmp(LOG_WARN, "Error opening audio conn \"%s\" for recording; device may have already been opened\n", conn->name);
		    audioconn_remove(conn);
		    goto end_get_conns;
		}
		conn->active = true;
		conns_to_activate[num_conns_to_activate] = conn;
		num_conns_to_activate++;
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
	    midi_device_open(mdevice);
	    fprintf(stderr, "Device type??? %d\n", mdevice->type);
	    if (mdevice->type == MIDI_DEVICE_QWERTY) {
		activate_mqwert = true;
	    }
	    MIDIClip *mclip = midi_clip_create(mdevice, track);
	    mclip->recording = true;
	    mdevice->current_clip = mclip;
	    mdevice->record_start = Pt_Time();

	    /* conn->current_clip_repositioned = false; */
	    clipref_create(track, tl->record_from_sframes, CLIP_MIDI, mclip);
	}
    }
end_get_conns:
    for (uint8_t i=0; i<num_conns_to_activate; i++) {
	conn = conns_to_activate[i];
	audioconn_open(session, conn);

	/* TODO: why is this here? move to audioconn_open ? */
	/* if (conn->type == DEVICE) { */
	/*     ((AudioDevice *)conn->obj)->write_bufpos_samples = 0; */
	/* } */
    }
    for (uint8_t i=0; i<num_conns_to_activate; i++) {
	conn = conns_to_activate[i];
	audioconn_start_recording(conn);
    }
    session->playback.recording = true;

    if (activate_mqwert) {
	mqwert_activate();
    }

    PageEl *el = panel_area_get_el_by_id(session->gui.panels, "panel_quickref_record");
    Textbox *record_button = ((Button *)el->component)->tb;
    textbox_set_background_color(record_button, &colors.red);


    timeline_play_speed_set(1.0);
    transport_start_playback();

    /* pd_jackdaw_record_get_block(); */

}

void create_clip_buffers(Clip *clip, uint32_t len_sframes)
{
    /* if (clip->L != NULL) { */

    /* 	fprintf(stderr, "Error: clip %s already has a buffer allocated\n", clip->name); */
    /* 	exit(1); */
    /* } */
    /* fprintf(stdout, "CREATING CLIP BUFFERS %d\n", len_sframes); */
    pthread_mutex_lock(&clip->buf_realloc_lock);
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
    pthread_mutex_unlock(&clip->buf_realloc_lock);
    
}

/* void copy_pd_buf_to_clip(Clip *clip) */
/* { */
/*     clip->len_sframes = clip->write_bufpos_sframes + clip->recorded_from->c.pd.write_bufpos_sframes; */
/*     create_clip_buffers(clip, clip->len_sframes); */
/*     memcpy(clip->L + clip->write_bufpos_sframes, clip->recorded_from->c.pd.rec_buffer_L, clip->recorded_from->c.pd.write_bufpos_sframes * sizeof(float)); */
/*     memcpy(clip->R + clip->write_bufpos_sframes, clip->recorded_from->c.pd.rec_buffer_R, clip->recorded_from->c.pd.write_bufpos_sframes * sizeof(float)); */
/*     clip->write_bufpos_sframes = clip->len_sframes; */
/* } */
static void copy_device_buf_to_clips(AudioDevice *dev)
{
    Clip *clips[dev->spec.channels];
    float *buffers[dev->spec.channels];
    memset(clips, 0, sizeof(clips));
    memset(buffers, 0, sizeof(buffers));
    /* int32_t read_pos = ACTIVE_TL->read_pos_sframes; */
    for (int c=0; c<dev->spec.channels; c++) {
	clips[c] = dev->channel_dsts[c].conn->current_clip;
	bool clip_redundant = false;
	for (int j=0; j<c; j++) {
	    if (clips[j] == clips[c]) {
		clip_redundant = true;
		break;
	    }
	}
	if (!clip_redundant && clips[c]) {
	    clips[c]->len_sframes = clips[c]->write_bufpos_sframes + dev->write_bufpos_samples / dev->spec.channels;
	    create_clip_buffers(clips[c], clips[c]->len_sframes);
	}

    }
    for (int c=0; c<dev->spec.channels; c++) {
	if (!clips[c]) continue;
	if (dev->channel_dsts[c].channel == 0) {
	    clips[c] = dev->channel_dsts[c].conn->current_clip;
	    buffers[c] = clips[c]->L + clips[c]->write_bufpos_sframes;
	} else {
	    clips[c] = dev->channel_dsts[c].conn->current_clip;
	    buffers[c] = dev->channel_dsts[c].conn->current_clip->R + clips[c]->write_bufpos_sframes;
	}

    }
    for (int i=0; i<dev->write_bufpos_samples; i++) {
	if (buffers[i % dev->spec.channels]) {
	    buffers[i % dev->spec.channels][i / dev->spec.channels] = (double)dev->rec_buffer[i] / INT16_MAX;
	}
    }
    for (int c=0; c<dev->spec.channels; c++) {
	if (clips[c]) {
	    clips[c]->write_bufpos_sframes = clips[c]->len_sframes;
	}
    }
}

void copy_conn_buf_to_clip(Clip *clip, enum audio_conn_type type)
{
    switch (type) {
    case DEVICE:
	log_tmp(LOG_ERROR, "copy_conn_buf_to_clip deprecated for audio devices; use copy_device_buf_to_clips (one per cb!) instead\n");
	error_exit("copy_conn_buf_to_clip deprecated for audio devices; use copy_device_buf_to_clips (one per cb!) instead\n");
	break;
    case PURE_DATA: {
	PdConn *pdconn = clip->recorded_from->obj;
	clip->len_sframes = clip->write_bufpos_sframes + pdconn->write_bufpos_sframes;
	create_clip_buffers(clip, clip->len_sframes);
	memcpy(clip->L + clip->write_bufpos_sframes, pdconn->rec_buffer_L, pdconn->write_bufpos_sframes * sizeof(float));
	memcpy(clip->R + clip->write_bufpos_sframes, pdconn->rec_buffer_R, pdconn->write_bufpos_sframes * sizeof(float));
	clip->write_bufpos_sframes = clip->len_sframes;
    }
	break;
    case JACKDAW: {
	/* OUROBOROS */
	JDAWConn *jconn = clip->recorded_from->obj;
	clip->len_sframes = clip->write_bufpos_sframes + jconn->write_bufpos_sframes;
	create_clip_buffers(clip, clip->len_sframes);
	memcpy(clip->L + clip->write_bufpos_sframes, jconn->rec_buffer_L, jconn->write_bufpos_sframes * sizeof(float));
	memcpy(clip->R + clip->write_bufpos_sframes, jconn->rec_buffer_R, jconn->write_bufpos_sframes * sizeof(float));
	clip->write_bufpos_sframes = clip->len_sframes;
	jconn->write_bufpos_sframes = 0;
    }
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

    transport_stop_playback();
    timeline_play_speed_set(0.0);
    
    /* First, stop audio devices and copy remaining data */
    AudioConn *conn;
    AudioConn *conns_to_close[MAX_SESSION_AUDIO_CONNS];
    int num_conns_to_close = 0;
    AudioDevice *devices_to_dump[MAX_SESSION_AUDIO_DEVICES];
    int num_devices_to_dump = 0;
    for (int i=0; i<session->audio_io.num_record_conns; i++) {
	if ((conn = session->audio_io.record_conns[i]) && conn->active) {
	    audioconn_stop_recording(conn);
	    conns_to_close[num_conns_to_close] = conn;
	    num_conns_to_close++;
	    /* audioconn_close(conn); */
	    conn->active = false;
	    if (conn->type == DEVICE) {
		bool included = false;
		for (int j=0; j<num_devices_to_dump; j++) {
		    if (devices_to_dump[j] == conn->obj) {
			included = true;
			break;
		    }
		}
		if (!included) {
		    devices_to_dump[num_devices_to_dump] = conn->obj;
		    /* device_stop_recording(devices_to_dump[num_devices_to_dump]); */
		    /* sem_post(devices_to_dump[num_devices_to_dump]->request_close); */
		    /* devices_to_dump[num_devices_to_dump]->request_close = true; */
		    num_devices_to_dump++;
		}
	    }

	}
    }

    for (int i=0; i<num_devices_to_dump; i++) {
	copy_device_buf_to_clips(devices_to_dump[i]);
    }
    session->playback.recording = false;

    while (num_conns_to_close > 0) {
	audioconn_close(conns_to_close[--num_conns_to_close]);
    }

    ClipRef **created_clips = calloc(tl->num_tracks * 2, sizeof(ClipRef *));
    uint16_t num_created = 0;
    for (uint16_t i=session->proj.active_clip_index; i<session->proj.num_clips; i++) {
	Clip *clip = session->proj.clips[i];
	if (clip->len_sframes == 0) {
	    for (int i=0; i<clip->num_refs; i++) {
		if (clip->refs[i]->grabbed) timeline_clipref_ungrab(clip->refs[i]);
	    }
	    for (int i=0; i<session->audio_io.num_record_conns; i++) {
		AudioConn *conn = session->audio_io.record_conns[i];
		if (conn->current_clip == clip) {
		    conn->current_clip = NULL;
		}
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
		clipref_reset(ref, true);
	    }
	}
    }	    
    
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
	    clip->recorded_from->current_clip = NULL;
	    /* Do nothing; handled above */
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
	for (int i=0; i<clip->num_refs; i++) {
	    clip->refs[i]->end_in_clip = clip->len_sframes;
	}
	session->proj.active_clip_index++;
    }

    /* MIDI */
    while (session->proj.active_midi_clip_index < session->proj.num_midi_clips) {
	MIDIClip *mclip = session->proj.midi_clips[session->proj.active_midi_clip_index];
	mclip->recording = false;
	if (mclip->recorded_from) {
	    mclip->recorded_from->recording = false;
	    midi_device_close_all_notes(mclip->recorded_from);
	}
	session->proj.active_midi_clip_index++;
	for (int i=0; i<mclip->num_refs; i++) {
	    mclip->refs[i]->end_in_clip = mclip->len_sframes;
	    created_clips[num_created] = mclip->refs[i];
	    num_created++;
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

    



    /* mqwert_deactivate(); */
    /* for (int i=0; i<session->midi_io.num_inputs; i++) { */
    /* 	MIDIDevice *d = session->midi_io.inputs + i; */
    /* 	if (!d->info->is_virtual) { */
    /* 	    midi_device_close(d); */
    /* 	} */
    /* } */
    PageEl *el = panel_area_get_el_by_id(session->gui.panels, "panel_quickref_record");
    Textbox *record_button = ((Button *)el->component)->tb;
    textbox_set_background_color(record_button, &colors.quickref_button_blue );

    tl->needs_redraw = true;
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
	timeline_set_play_position(tl, tl->in_mark_sframes, true);
    } else {
	timeline_set_play_position(tl, tl->out_mark_sframes, true);
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
	int32_t orig_len = clip->len_sframes;

	if (!clip->recorded_from) continue; /* E.g. wav loaded during recording */
	switch(clip->recorded_from->type) {
	case DEVICE:
	    /* Handled in buf to clip */
	    clip->len_sframes = ((AudioDevice *)clip->recorded_from->obj)->write_bufpos_samples / ((AudioDevice *)clip->recorded_from->obj)->spec.channels + clip->write_bufpos_sframes;
	    /* fprintf(stderr, "Reset clip len in MAIN: %d\n", clip->len_sframes); */
	    break;
	case PURE_DATA:
	    clip->len_sframes = ((PdConn *)clip->recorded_from->obj)->write_bufpos_sframes + clip->write_bufpos_sframes;
	    break;
	case JACKDAW:
	    clip->len_sframes = ((JDAWConn *)clip->recorded_from->obj)->write_bufpos_sframes + clip->write_bufpos_sframes;
	    break;
	}

	for (uint16_t j=0; j<clip->num_refs; j++) {
	    ClipRef *cr = clip->refs[j];
	    cr->end_in_clip = clip->len_sframes;
	    clipref_reset(cr, false);
	}
	clip->len_sframes = orig_len;
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
