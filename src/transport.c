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
#include "audio_device.h"
#include "mixdown.h"
#include "project.h"
#include "timeline.h"
#include "wav.h"

extern Project *proj;

static void copy_device_buf_to_clip(Clip *clip);
void transport_record_callback(void* user_data, uint8_t *stream, int len)
{

    AudioDevice *dev = (AudioDevice *)user_data;
    uint32_t stream_len_samples = len / sizeof(int16_t);

    if (dev->write_bufpos_samples + stream_len_samples < dev->rec_buf_len_samples) {
        memcpy(dev->rec_buffer + dev->write_bufpos_samples, stream, len);
    } else {
	for (int i=proj->active_clip_index; i< proj->num_clips; i++) {
	    Clip *clip = proj->clips[proj->active_clip_index];
	    copy_device_buf_to_clip(clip);
	    clip->write_bufpos_sframes += dev->write_bufpos_samples / clip->channels;
	}
        dev->write_bufpos_samples = 0;
        /* device_stop_recording(dev); */
        /* fprintf(stderr, "ERROR: overwriting audio buffer of device: %s\n", dev->name); */
    }

    dev->write_bufpos_samples += stream_len_samples;

}

static float *get_source_mode_chunk(uint8_t channel, uint32_t len_sframes, int32_t start_pos_sframes, float step)
{
    float *chunk = malloc(sizeof(float) * len_sframes);
    if (!chunk) {
	fprintf(stderr, "Error: unable to allocate chunk from source clip\n");
    }
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

void transport_playback_callback(void* user_data, uint8_t* stream, int len)
{
    memset(stream, '\0', len);

    uint32_t stream_len_samples = len / sizeof(int16_t);

    float *chunk_L, *chunk_R;
    if (proj->source_mode) {
	chunk_L = get_source_mode_chunk(0, stream_len_samples / proj->channels, proj->src_play_pos_sframes, proj->src_play_speed);
	chunk_R = get_source_mode_chunk(1, stream_len_samples / proj->channels, proj->src_play_pos_sframes, proj->src_play_speed);
    } else {
	Timeline *tl = proj->timelines[proj->active_tl_index];
	chunk_L = get_mixdown_chunk(tl, 0, stream_len_samples / proj->channels, tl->play_pos_sframes, proj->play_speed);
	chunk_R = get_mixdown_chunk(tl, 1, stream_len_samples / proj->channels, tl->play_pos_sframes, proj->play_speed);
    }

    int16_t *stream_fmt = (int16_t *)stream;
    for (uint32_t i=0; i<stream_len_samples; i+=2)
    {
        float val_L = chunk_L[i/2];
        float val_R = chunk_R[i/2];
        proj->output_L[i/2] = val_L;
        proj->output_R[i/2] = val_R;
        stream_fmt[i] = (int16_t) (val_L * INT16_MAX);
        stream_fmt[i+1] = (int16_t) (val_R * INT16_MAX);
    }

    free(chunk_L);
    free(chunk_R);
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
    }
}



void transport_start_playback()
{
    device_start_playback(proj->playback_device);
}

void transport_stop_playback()
{
    proj->src_play_speed = 0.0f;
    proj->play_speed = 0.0f;
    device_stop_playback(proj->playback_device);
}
void transport_start_recording()
{
    AudioDevice *devices_to_activate[MAX_PROJ_AUDIO_DEVICES];
    uint8_t num_devices_to_activate = 0;
    Timeline *tl = proj->timelines[proj->active_tl_index];
    tl->record_from_sframes = tl->play_pos_sframes;
    AudioDevice *dev;
    /* for (int i=0; i<proj->num_record_devices; i++) { */
    /* 	if ((dev = proj->record_devices[i]) && dev->active) { */
    /* 	    device_open(proj, dev); */
    /* 	    SDL_PauseAudioDevice(dev->id, 0); */
    /* 	    Clip *clip = project_add_clip(dev); */
    /* 	    clip->recording = true; */
    /* 	} */
    /* } */
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	Track *track = tl->tracks[i];
	Clip *clip = NULL;
	bool home = false;
	if (track->active) {
	    dev = track->input;
	    if (!(dev->active)) {
		dev->active = true;
		devices_to_activate[num_devices_to_activate] = dev;
		num_devices_to_activate++;
		if (device_open(proj, dev) != 0) {
		    fprintf(stderr, "Error opening audio device to record\n");
		    exit(1);
		}
		clip = project_add_clip(dev, track);
		clip->recording = true;
		home = true;
		dev->current_clip = clip;
	    } else {
		clip = dev->current_clip;
	    }
	    /* Clip ref is created as "home", meaning clip data itself is associated with this ref */
	    track_create_clip_ref(track, clip, tl->record_from_sframes, home);
	}
	
    }

    for (uint8_t i=0; i<num_devices_to_activate; i++) {
	dev = devices_to_activate[i];
	if (!dev->open) {
	    device_open(proj, dev);
	}
	/* device_open(proj, dev); */
	/* device_start_recording(dev); */
    }
    fprintf(stdout, "OPENED ALL DEVICES TO RECORD\n");
    for (uint8_t i=0; i<num_devices_to_activate; i++) {
	dev = devices_to_activate[i];
	device_start_recording(dev);
    }
    proj->recording = true;
    proj->play_speed = 1.0f;
    transport_start_playback();
   
}

static void create_clip_buffers(Clip *clip, uint32_t len_sframes)
{
    /* if (clip->L != NULL) { */
	
    /* 	fprintf(stderr, "Error: clip %s already has a buffer allocated\n", clip->name); */
    /* 	exit(1); */
    /* } */
    uint32_t buf_len_bytes = sizeof(float) * len_sframes;
    if (!clip->L) {
	clip->L = malloc(buf_len_bytes);
    } else {
	fprintf(stdout, "REALLOCATING L %lu\n", buf_len_bytes);
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

static void copy_device_buf_to_clip(Clip *clip) {
    fprintf(stderr, "Enter copy_buff_to_clip\n");
    fprintf(stdout, "\n\nRESETTING clip len to clip write buffpos (%ld) + recorded from bufpos (%d)\n", clip->write_bufpos_sframes, clip->recorded_from->write_bufpos_samples);
    clip->len_sframes = clip->write_bufpos_sframes + clip->recorded_from->write_bufpos_samples / clip->channels;
    create_clip_buffers(clip, clip->len_sframes);
    for (int i=0; i<clip->recorded_from->write_bufpos_samples; i+=clip->channels) {
        float sample_L = (float) clip->recorded_from->rec_buffer[i] / INT16_MAX;
        float sample_R = (float) clip->recorded_from->rec_buffer[i+1] / INT16_MAX;
        // fprintf(stderr, "Copying samples to clip %d: %f, %d: %f\n", i, sample_L, i+1, sample_R);
        clip->L[clip->write_bufpos_sframes + i/clip->channels] = sample_L;
        clip->R[clip->write_bufpos_sframes + i/clip->channels] = sample_R;
        // (clip->pre_proc)[i] = sample;
        // (clip->post_proc)[i] = sample;
    }
    /* clip->recording = false; */
    return;
}


void transport_stop_recording()
{
    AudioDevice *dev;
    for (int i=0; i<proj->num_record_devices; i++) {
	if ((dev = proj->record_devices[i]) && dev->active) {
	    device_stop_recording(dev);
	    dev->active = false;
	}
    }
    proj->recording = false;
    transport_stop_playback();

    while (proj->active_clip_index < proj->num_clips) {
	Clip *clip = proj->clips[proj->active_clip_index];
	copy_device_buf_to_clip(clip);
	clip->recording = false;
	proj->active_clip_index++;
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
	tl->play_pos_sframes = tl->in_mark_sframes;
    } else {
	tl->play_pos_sframes = tl->out_mark_sframes;
    }
}

void transport_recording_update_cliprects()
{
    for (uint8_t i=proj->active_clip_index; i<proj->num_clips; i++) {
	Clip *clip = proj->clips[i];
	
	clip->len_sframes = clip->recorded_from->write_bufpos_samples / clip->channels + clip->write_bufpos_sframes;
	for (uint8_t j=0; j<clip->num_refs; j++) {
	    ClipRef *cr = clip->refs[j];
	    cr->out_mark_sframes = clip->len_sframes;
	    clipref_reset(cr);
	}
    }
}
