/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/

/*****************************************************************************************************************
    transport.c

    * Audio callback fns
    * Playback and recording
 *****************************************************************************************************************/

#include <pthread.h>
#include <string.h>
#include "audio_connection.h"
#include "dsp.h"
#include "mixdown.h"
#include "project.h"
#include "pure_data.h"
#include "timeline.h"
#include "wav.h"

extern Project *proj;

void copy_conn_buf_to_clip(Clip *clip, enum audio_conn_type type);
void transport_record_callback(void* user_data, uint8_t *stream, int len)
{

    AudioDevice *dev = (AudioDevice *)user_data;
    uint32_t stream_len_samples = len / sizeof(int16_t);

    if (dev->write_bufpos_samples + stream_len_samples < dev->rec_buf_len_samples) {
	/* fprintf(stdout, "Fits! bufpos: %d\n", dev->write_bufpos_samples); */
        memcpy(dev->rec_buffer + dev->write_bufpos_samples, stream, len);
	dev->write_bufpos_samples += stream_len_samples;
    } else {
	/* fprintf(stdout, "Leftover: %d\n", dev->rec_buf_len_samples - (dev->write_bufpos_samples + stream_len_samples)); */
	for (int i=proj->active_clip_index; i<proj->num_clips; i++) {
	    Clip *clip = proj->clips[i];
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
     float *src_buffer = channel == 0 ? proj->src_clip->L : proj->src_clip->R;


     for (uint32_t i=0; i<len_sframes; i++) {
	 int sample_i = (int) (i * step + start_pos_sframes);
	 if (sample_i < proj->src_clip->len_sframes && sample_i > 0) {
	     chunk[i] = src_buffer[sample_i];
	 } else {
	     chunk[i] = 0;
	 }
     }
     return chunk;
 }

void transport_recording_update_cliprects();
void ____transport_playback_callback(void* user_data, uint8_t* stream, int len)
{
    memset(stream, '\0', len);

    /* fprintf(stdout, "PLAyback callbac\n"); */
    uint32_t stream_len_samples = len / sizeof(int16_t);
    uint32_t len_sframes = stream_len_samples / proj->channels;
    float chunk_L[len_sframes];
    float chunk_R[len_sframes];
    if (proj->source_mode) {
	/* chunk_L = get_source_mode_chunk(0, len_sframes, proj->src_play_pos_sframes, proj->src_play_speed); */
	/* chunk_R = get_source_mode_chunk(1, len_sframes, proj->src_play_pos_sframes, proj->src_play_speed); */
	get_source_mode_chunk(0, chunk_L, len_sframes, proj->src_play_pos_sframes, proj->src_play_speed);
	get_source_mode_chunk(1, chunk_R, len_sframes, proj->src_play_pos_sframes, proj->src_play_speed);
    } else {
	Timeline *tl = proj->timelines[proj->active_tl_index];
	/* fprintf(stdout, "Getting mixdown...\n"); */
	get_mixdown_chunk(tl, chunk_L, 0, len_sframes, tl->play_pos_sframes, proj->play_speed);
	get_mixdown_chunk(tl, chunk_R, 1, len_sframes, tl->play_pos_sframes, proj->play_speed);
	/* fprintf(stdout, "Done.\n"); */
    }

    int16_t *stream_fmt = (int16_t *)stream;
    double *output_l = malloc(sizeof(double) * proj->chunk_size_sframes);
    double *output_r = malloc(sizeof(double) * proj->chunk_size_sframes);
    for (uint32_t i=0; i<stream_len_samples; i+=2)
    {
	float val_L = chunk_L[i/2];
	float val_R = chunk_R[i/2];
	/* proj->output_L[i/2] = val_L; */
	/* proj->output_R[i/2] = val_R; */
	output_l[i/2] = val_L;
	output_r[i/2] = val_R;
	stream_fmt[i] = (int16_t) (val_L * INT16_MAX);
	stream_fmt[i+1] = (int16_t) (val_R * INT16_MAX);
    }

    double complex  *lfreq = malloc(sizeof(double complex) * proj->chunk_size_sframes);
    double complex *rfreq = malloc(sizeof(double complex) * proj->chunk_size_sframes);
    FFT(output_l, lfreq, proj->chunk_size_sframes);
    FFT(output_r, rfreq, proj->chunk_size_sframes);
    /* double *ok = malloc(sizeof(double) * proj->chunk_size_sframes); */
    get_magnitude(lfreq, proj->output_L_freq, proj->chunk_size_sframes);
    get_magnitude(rfreq, proj->output_R_freq, proj->chunk_size_sframes);
    
	    /* free(chunk_L); */
    /* free(chunk_R); */

    // for (uint8_t i = 0; i<40; i+=2) {
    //     fprintf(stderr, "%hd ", (int16_t)(stream[i]));
    // }
    // if (proj->tl->play_offset == 0) {
    //     long            ms; // Milliseconds
    //     time_t          s;  // Seconds
    //     struct timespec spec;
    //     clock_gettime(CLOCK_REALTIME, &spec);
    //     s  = spec.tv_sec;
    //     ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
    //     if (ms > 999) {
    //         s++;
    //         ms = 0;
    //     }
    //     // proj->tl->play_offset = ms;
    // }
    if (proj->source_mode) {
	proj->src_play_pos_sframes += proj->src_play_speed * stream_len_samples / proj->channels;
	if (proj->src_play_pos_sframes < 0 || proj->src_play_pos_sframes > proj->src_clip->len_sframes) {
	    proj->src_play_pos_sframes = 0;
	}
    } else {
	/* fprintf(stdout, "Move pos: %d\n", (int)proj->play_speed * stream_len_samples / proj->channels); */
	timeline_move_play_position(proj->play_speed * stream_len_samples / proj->channels);
	transport_recording_update_cliprects();
	for (uint8_t i=proj->active_clip_index; i<proj->num_clips; i++) {
	    Clip *clip = proj->clips[i];
	    AudioConn *conn = clip->recorded_from;
	    if (conn->type == JACKDAW) {
		if (conn->c.jdaw.write_bufpos_sframes + proj->chunk_size_sframes < conn->c.jdaw.rec_buf_len_sframes) {
		    memcpy(conn->c.jdaw.rec_buffer_L + conn->c.jdaw.write_bufpos_sframes, proj->output_L, sizeof(float) * proj->chunk_size_sframes);
		    memcpy(conn->c.jdaw.rec_buffer_R + conn->c.jdaw.write_bufpos_sframes, proj->output_R, sizeof(float) * proj->chunk_size_sframes);
		    conn->c.jdaw.write_bufpos_sframes += proj->chunk_size_sframes;
		} else {
		    copy_conn_buf_to_clip(clip, JACKDAW);
		}
		break;
	    }
	}
    }
}

void transport_playback_callback(void* user_data, uint8_t* stream, int len)
{
    /* fprintf(stdout, "Start cb\n"); */
    memset(stream, '\0', len);
    Timeline *tl = proj->timelines[proj->active_tl_index];
    /* fprintf(stdout, "PLAyback callbac\n"); */
    uint32_t stream_len_samples = len / sizeof(int16_t);
    uint32_t len_sframes = stream_len_samples / proj->channels;
    float chunk_L[len_sframes];
    float chunk_R[len_sframes];
    if (proj->source_mode) {
	/* chunk_L = get_source_mode_chunk(0, len_sframes, proj->src_play_pos_sframes, proj->src_play_speed); */
	/* chunk_R = get_source_mode_chunk(1, len_sframes, proj->src_play_pos_sframes, proj->src_play_speed); */
	get_source_mode_chunk(0, chunk_L, len_sframes, proj->src_play_pos_sframes, proj->src_play_speed);
	get_source_mode_chunk(1, chunk_R, len_sframes, proj->src_play_pos_sframes, proj->src_play_speed);
    } else {
	/* fprintf(stdout, "wait readable...\n"); */
	sem_wait(tl->readable_chunks);
	/* fprintf(stdout, "done!\n"); */
	/* fprintf(stdout, "READ. buf read pos: %d. src: %p\n", tl->buf_read_pos, tl->buf_L + tl->buf_read_pos); */
	memcpy(chunk_L, tl->buf_L + tl->buf_read_pos, sizeof(float) * len_sframes);
	memcpy(chunk_R, tl->buf_R + tl->buf_read_pos, sizeof(float) * len_sframes);
	sem_post(tl->writable_chunks);
	tl->buf_read_pos += len_sframes;
	if (tl->buf_read_pos >= proj->fourier_len_sframes * 2) {
	    tl->buf_read_pos = 0;
	}
    }

    int16_t *stream_fmt = (int16_t *)stream;
    /* double *output_l = malloc(sizeof(double) * proj->chunk_size_sframes); */
    /* double *output_r = malloc(sizeof(double) * proj->chunk_size_sframes); */
    for (uint32_t i=0; i<stream_len_samples; i+=2)
    {
	float val_L = chunk_L[i/2];
	float val_R = chunk_R[i/2];
	/* proj->output_L[i/2] = val_L; */
	/* proj->output_R[i/2] = val_R; */
	/* output_l[i/2] = val_L; */
	/* output_r[i/2] = val_R; */
	stream_fmt[i] = (int16_t) (val_L * INT16_MAX);
	stream_fmt[i+1] = (int16_t) (val_R * INT16_MAX);
    }

    /* double complex  *lfreq = malloc(sizeof(double complex) * proj->chunk_size_sframes); */
    /* double complex *rfreq = malloc(sizeof(double complex) * proj->chunk_size_sframes); */
    /* FFT(output_l, lfreq, proj->chunk_size_sframes); */
    /* FFT(output_r, rfreq, proj->chunk_size_sframes); */
    /* double *ok = malloc(sizeof(double) * proj->chunk_size_sframes); */
    /* get_magnitude(lfreq, proj->output_L_freq, proj->chunk_size_sframes); */
    /* get_magnitude(rfreq, proj->output_R_freq, proj->chunk_size_sframes); */
    
	    /* free(chunk_L); */
    /* free(chunk_R); */

    // for (uint8_t i = 0; i<40; i+=2) {
    //     fprintf(stderr, "%hd ", (int16_t)(stream[i]));
    // }
    // if (proj->tl->play_offset == 0) {
    //     long            ms; // Milliseconds
    //     time_t          s;  // Seconds
    //     struct timespec spec;
    //     clock_gettime(CLOCK_REALTIME, &spec);
    //     s  = spec.tv_sec;
    //     ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
    //     if (ms > 999) {
    //         s++;
    //         ms = 0;
    //     }
    //     // proj->tl->play_offset = ms;
    // }
    if (proj->source_mode) {
	proj->src_play_pos_sframes += proj->src_play_speed * stream_len_samples / proj->channels;
	if (proj->src_play_pos_sframes < 0 || proj->src_play_pos_sframes > proj->src_clip->len_sframes) {
	    proj->src_play_pos_sframes = 0;
	}
    } else {
	/* fprintf(stdout, "Move pos: %d\n", (int)proj->play_speed * stream_len_samples / proj->channels); */
	timeline_move_play_position(proj->play_speed * stream_len_samples / proj->channels);
	transport_recording_update_cliprects();
	/* for (uint8_t i=proj->active_clip_index; i<proj->num_clips; i++) { */
	/*     Clip *clip = proj->clips[i]; */
	/*     AudioConn *conn = clip->recorded_from; */
	/*     if (conn->type == JACKDAW) { */
	/* 	if (conn->c.jdaw.write_bufpos_sframes + proj->chunk_size_sframes < conn->c.jdaw.rec_buf_len_sframes) { */
	/* 	    memcpy(conn->c.jdaw.rec_buffer_L + conn->c.jdaw.write_bufpos_sframes, proj->output_L, sizeof(float) * proj->chunk_size_sframes); */
	/* 	    memcpy(conn->c.jdaw.rec_buffer_R + conn->c.jdaw.write_bufpos_sframes, proj->output_R, sizeof(float) * proj->chunk_size_sframes); */
	/* 	    conn->c.jdaw.write_bufpos_sframes += proj->chunk_size_sframes; */
	/* 	} else { */
	/* 	    copy_conn_buf_to_clip(clip, JACKDAW); */
	/* 	} */
	/* 	break; */
	/*     } */
	/* } */
    }
}

static void *transport_dsp_thread_fn(void *arg)
{
    Timeline *tl = (Timeline *)arg;
    
    if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) != 0) {
	perror("pthread set cancel state");
	exit(1);
    }

    int len = proj->fourier_len_sframes;
    /* pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL); */
    float buf_L[len];
    float buf_R[len];

    int N = len / proj->chunk_size_sframes;
    bool init = true;
    while (1) {
	pthread_testcancel();
	float play_speed = proj->play_speed;

	/* GET MIXDOWN */
	get_mixdown_chunk(tl, buf_L, 0, len, tl->read_pos_sframes, proj->play_speed);
	get_mixdown_chunk(tl, buf_R, 1, len, tl->read_pos_sframes, proj->play_speed);


	/* DSP */

	double dL[len];
	double dR[len];
	for (int i=0; i<len; i++) {
	    dL[i] = (double)buf_L[i];
	    dR[i] = (double)buf_R[i];
	}
	double complex  lfreq[len];
	double complex rfreq[len];
	FFT(dL, lfreq, len);
	FFT(dR, rfreq, len);
    /* double *ok = malloc(sizeof(double) * proj->chunk_size_sframes); */
	get_magnitude(lfreq, proj->output_L_freq, len);
	get_magnitude(rfreq, proj->output_R_freq, len);



	/* Copy buffer */
	tl->read_pos_sframes += len * play_speed;
	for (int i=0; i<N; i++) {
	    sem_wait(tl->writable_chunks);
	}
	memcpy(tl->buf_L + tl->buf_write_pos, buf_L, sizeof(float) * len);
	memcpy(tl->buf_R + tl->buf_write_pos, buf_R, sizeof(float) * len);
	memcpy(proj->output_L, buf_L, sizeof(float) * len);
	memcpy(proj->output_R, buf_R, sizeof(float) * len);

	for (uint8_t i=proj->active_clip_index; i<proj->num_clips; i++) {
	    Clip *clip = proj->clips[i];
	    AudioConn *conn = clip->recorded_from;
	    if (conn->type == JACKDAW) {
		if (conn->c.jdaw.write_bufpos_sframes + len < conn->c.jdaw.rec_buf_len_sframes) {
		    memcpy(conn->c.jdaw.rec_buffer_L + conn->c.jdaw.write_bufpos_sframes, proj->output_L, sizeof(float) * len);
		    memcpy(conn->c.jdaw.rec_buffer_R + conn->c.jdaw.write_bufpos_sframes, proj->output_R, sizeof(float) * len);
		    conn->c.jdaw.write_bufpos_sframes += len;
		} else {
		    copy_conn_buf_to_clip(clip, JACKDAW);
		}
		break;
	    }
	}

	
	tl->buf_write_pos += len;
	if (tl->buf_write_pos >= len * 2) {
	    tl->buf_write_pos = 0;
	}
	for (int i=0; i<N; i++) {
	    sem_post(tl->readable_chunks);
	}
	if (init) {
	    sem_post(tl->unpause_sem);
	    init = false;
	}
    }
    return NULL;
}



void transport_start_playback()
{
    
    if (proj->playing) return;
    fprintf(stdout, "START playback\n");
    proj->playing = true;
    Timeline *tl = proj->timelines[proj->active_tl_index];
    tl->read_pos_sframes = tl->play_pos_sframes;
    pthread_create(&proj->dsp_thread, NULL, transport_dsp_thread_fn, (void *)tl); 
    sem_wait(tl->unpause_sem);
    audioconn_start_playback(proj->playback_conn);
}

void transport_stop_playback()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    for (int i=0; i<512; i++) {
	sem_post(tl->writable_chunks);
	sem_post(tl->readable_chunks);
    }
    pthread_cancel(proj->dsp_thread);
    audioconn_stop_playback(proj->playback_conn);
    /* fprintf(stdout, "RESETTING SEMS from tl %p\n", tl); */
    while (sem_trywait(tl->unpause_sem) == 0) {};
    while (sem_trywait(tl->writable_chunks) == 0) {};
    while (sem_trywait(tl->readable_chunks) == 0) {};
    for (int i=0; i<proj->fourier_len_sframes * 2 / proj->chunk_size_sframes; i++) {
	/* fprintf(stdout, "\t->reinitiailizing writable chunks\n"); */
	sem_post(tl->writable_chunks);
    }
    tl->buf_read_pos = 0;
    tl->buf_write_pos = 0;
    proj->playing = false;
    /* pthread_kill(proj->dsp_thread, SIGINT); */
    /* fprintf(stdout, "Cancelled!\n"); */
    proj->src_play_speed = 0.0f;
    proj->play_speed = 0.0f;

}

void transport_start_recording()
{
    proj->play_speed = 1.0f;
    transport_start_playback();
    AudioConn *conns_to_activate[MAX_PROJ_AUDIO_CONNS];
    uint8_t num_conns_to_activate = 0;
    Timeline *tl = proj->timelines[proj->active_tl_index];
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
	Clip *clip = NULL;
	bool home = false;
	if (track->active) {
	    no_tracks_active = false;
	    conn = track->input;
	    if (!(conn->active)) {
		conn->active = true;
		conns_to_activate[num_conns_to_activate] = conn;
		num_conns_to_activate++;
		if (audioconn_open(proj, conn) != 0) {
		    fprintf(stderr, "Error opening audio device to record\n");
		    return;
		}
		clip = project_add_clip(conn, track);
		clip->recording = true;
		home = true;
		conn->current_clip = clip;
	    } else {
		clip = conn->current_clip;
	    }
	    /* Clip ref is created as "home", meaning clip data itself is associated with this ref */
	    track_create_clip_ref(track, clip, tl->record_from_sframes, home);
	}
    }
    if (no_tracks_active) {
	Track *track = tl->tracks[tl->track_selector];
	if (!track) {
	    return;
	}
	Clip *clip = NULL;
	bool home = false;
	conn = track->input;
	if (!(conn->active)) {
	    conn->active = true;
	    conns_to_activate[num_conns_to_activate] = conn;
	    num_conns_to_activate++;
	    if (audioconn_open(proj, conn) != 0) {
		fprintf(stderr, "Error opening audio device to record\n");
		exit(1);
	    }
	    fprintf(stdout, "\n\nProject add clip!\n");
	    clip = project_add_clip(conn, track);
	    clip->recording = true;
	    home = true;
	    conn->current_clip = clip;
	} else {
	    clip = conn->current_clip;
	}
	/* Clip ref is created as "home", meaning clip data itself is associated with this ref */
	track_create_clip_ref(track, clip, tl->record_from_sframes, home);
    }

    for (uint8_t i=0; i<num_conns_to_activate; i++) {
	conn = conns_to_activate[i];
	if (!conn->open) {
	    audioconn_open(proj, conn);
	}
	conn->c.device.write_bufpos_samples = 0;
	/* device_open(proj, dev); */
	/* device_start_recording(dev); */
    }
    fprintf(stdout, "OPENED ALL DEVICES TO RECORD\n");
    for (uint8_t i=0; i<num_conns_to_activate; i++) {
	conn = conns_to_activate[i];
	audioconn_start_recording(conn);
    }
    proj->recording = true;

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
    if (clip->channels == 2) {
	if (!clip->R) {
	    clip->R = malloc(buf_len_bytes);
	} else {
	    clip->R = realloc(clip->R, buf_len_bytes);
	}
    }
    if (!clip->R || !clip->L) {
	fprintf(stderr, "Error: failed to allocate space for clip buffer\n");
	exit(1);
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
     /* clip->recorded_from->write_bufpos_samples = 0; */
     /* clip->recording = false; */
}

 /* static void complete_pd_clip(Clip *clip) */
 /* { */

 /* } */



void transport_stop_recording()
{

    transport_stop_playback();
    AudioConn *conn;
    AudioConn *conns_to_close[64];
    uint8_t num_conns_to_close = 0;
    for (int i=0; i<proj->num_record_conns; i++) {
	if ((conn = proj->record_conns[i]) && conn->active) {
	    audioconn_stop_recording(conn);
	    conns_to_close[num_conns_to_close] = conn;
	    num_conns_to_close++;
	    /* audioconn_close(conn); */
	    conn->active = false;
	}
    }
    proj->recording = false;

    while (proj->active_clip_index < proj->num_clips) {
	Clip *clip = proj->clips[proj->active_clip_index];
	/* fprintf(stdout, "CLIP BUFFERS: %p, %p\n", clip->L, clip->R); */
	/* exit(0); */
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
	clip->recording = false;
	proj->active_clip_index++;
    }

    while (num_conns_to_close > 0) {
	audioconn_close(conns_to_close[--num_conns_to_close]);
    }

    
}

void transport_set_mark(Timeline *tl, bool in)
{
    if (tl) {
	if (in) {
	    tl->in_mark_sframes = tl->play_pos_sframes;
	} else {
	    tl->out_mark_sframes = tl->play_pos_sframes;
	}
    } else if (proj->source_mode) {
	if (in) {
	    proj->src_in_sframes = proj->src_play_pos_sframes;
	} else {
	    proj->src_out_sframes = proj->src_play_pos_sframes;
	}
		
    }
}

void transport_goto_mark(Timeline *tl, bool in)
{
    if (in) {
	timeline_set_play_position(tl->in_mark_sframes);
    } else {
	timeline_set_play_position(tl->out_mark_sframes);
    }
}

void transport_recording_update_cliprects()
{
    /* fprintf(stdout, "update clip rects...\n"); */
    for (uint8_t i=proj->active_clip_index; i<proj->num_clips; i++) {
	/* fprintf(stdout, "updating %d/%d\n", i, proj->num_clips); */
	Clip *clip = proj->clips[i];

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

	for (uint8_t j=0; j<clip->num_refs; j++) {
	    ClipRef *cr = clip->refs[j];
	    cr->out_mark_sframes = clip->len_sframes;
	    clipref_reset(cr);
	}
    }
}
