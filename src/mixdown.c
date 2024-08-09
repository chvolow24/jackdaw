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
    mixdown.c

    * Get sampels from tracks/clips for playback or export
 *****************************************************************************************************************/

#include "dsp.h"
#include "project.h"


extern Project *proj;

/* /\* Query track clips and return audio sample representing a given point in the timeline.  */
/* channel is 0 for left or mono, 1 for right *\/ */
/* static float get_track_sample(Track *track, uint8_t channel, int32_t tl_pos_sframes) */
/* { */
/*     if (track->muted || track->solo_muted) { */
/*         return 0; */
/*     } */

/*     float sample = 0; */


/*     for (int i=0; i<track->num_clips; i++) { */
/*         ClipRef *cr = (track->clips)[i]; */

/* 	/\* TODO: reinstate this!! *\/ */
/*         /\* if (cr->clip->recording) { *\/ */
/*             /\* continue; *\/ */
/*         /\* } *\/ */
	
/*         float *clip_buf = (channel == 0) ? cr->clip->L : cr->clip->R; */
/*         int32_t pos_in_clip_sframes = tl_pos_sframes - cr->pos_sframes; */
/*         // fprintf(stderr, "Pos in clip: %d\n", pos_in_clip_sframes); */
/*         if (pos_in_clip_sframes >= 0 && pos_in_clip_sframes < cr->clip->len_sframes) { */
/*             /\* if (pos_in_clip_sframes < cr->start_ramp_len) { *\/ */
/*             /\*     float ramp_value = (float) pos_in_clip_sframes / clip->start_ramp_len; *\/ */
/*             /\*     sample += ramp_value * clip_buf[pos_in_clip_sframes]; *\/ */
/*             /\* } else if (pos_in_clip_sframes > clip->len_sframes - clip->end_ramp_len) { *\/ */
/*             /\*     float ramp_value = (float) (clip->len_sframes - pos_in_clip_sframes) / clip->end_ramp_len; *\/ */
/*             /\*     // ramp_value *= ramp_value; *\/ */
/*             /\*     // ramp_value *= ramp_value; *\/ */
/*             /\*     // ramp_value *= ramp_value; *\/ */
/*             /\*     // fprintf(stderr, "(END RAMP) Pos in clip: %d; scale: %f\n", pos_in_clip, ramp_value); *\/ */
/*             /\*     // fprintf(stderr, "\t Sample pre & post: %d, %d\n", (clip->post_proc)[pos_in_clip], (int16_t) ((clip->post_proc)[pos_in_clip] * ramp_value)); *\/ */
/*             /\*     // sample += (int16_t) ((clip->post_proc)[pos_in_clip] * ramp_value); *\/ */
/*             /\*     sample += ramp_value * clip_buf[pos_in_clip_sframes]; *\/ */
/*             /\* } else { *\/ */

/*                 sample += clip_buf[pos_in_clip_sframes]; */
/*             /\* } *\/ */
/*         } */
/*     } */

/*     return sample; */
/* } */

/* int ijk=0; */

float *get_track_channel_chunk(Track *track, float *chunk, uint8_t channel, int32_t start_pos_sframes, uint32_t len_sframes, float step)
{
    /* fprintf(stderr, "Start get track buf %d\n", i++); */
    uint32_t chunk_bytelen = sizeof(float) * len_sframes;
    /* float *chunk = calloc(1, chunk_bytelen); */
    memset(chunk, '\0', chunk_bytelen);
    if (track->muted || track->solo_muted) {
        return chunk;
    }
    // uint32_t end_pos_sframes = start_pos_sframes + len_sframes;
    for (uint8_t i=0; i<track->num_clips; i++) {
	ClipRef *cr = track->clips[i];
	if (!cr) {
	    continue;
	}
	if (proj->dragging && cr->grabbed) {
	    continue;
	}
	SDL_LockMutex(cr->lock);
	if (cr->clip->recording) {
	    SDL_UnlockMutex(cr->lock);
	    continue;
	}
	double pos_in_clip_sframes = start_pos_sframes - cr->pos_sframes;
	int32_t end_pos = pos_in_clip_sframes + (step * len_sframes);
	int32_t min, max;
	if (end_pos >= pos_in_clip_sframes) {
	    min = pos_in_clip_sframes;
	    max = end_pos;
	} else {
	    min = end_pos;
	    max = pos_in_clip_sframes;
	}
	int32_t cr_len = clip_ref_len(cr);
	if (max < 0 || min > cr_len) {
	    SDL_UnlockMutex(cr->lock);
	    continue;
	}
	float *clip_buf = (channel == 0) ? cr->clip->L : cr->clip->R;
	int16_t chunk_i = 0;
	/* float pan_scale = (channel == 0) ? (track->pan - 0.5) * 2 : track->pan * 2; */
	double pan_scale = channel == 0 ?
	    track->pan <= 0.5 ? 1 : (1.0f - track->pan) * 2
	    : track->pan >= 0.5 ? 1 : track->pan * 2;
	/* double rpan = track->pan < */
	while (chunk_i < len_sframes) {
	    if (pos_in_clip_sframes >= 0 && pos_in_clip_sframes < cr_len) {
		chunk[chunk_i] += clip_buf[(int32_t)pos_in_clip_sframes + cr->in_mark_sframes] * track->vol * pan_scale;
	    }
	    pos_in_clip_sframes += step;
	    /* fprintf(stdout, "Chunk %d: %f\n", chunk_i, chunk[chunk_i]); */
	    chunk_i++;
	}
	
	
	SDL_UnlockMutex(cr->lock);
    }
    /* for (uint32_t i=0; i<len_sframes; i++) { */
    /*     for (uint8_t clip_i=0; clip_i<track->num_clips; clip_i++) { */
    /*         ClipRef *cr = (track->clips)[clip_i]; */

    /* 	    SDL_LockMutex(cr->lock); */
    /* 	    if (cr->clip->recording) { */
    /* 		continue; */
    /* 	    } */
    /* 	    int32_t cr_len = clip_ref_len(cr); */
	    
    /*         float *clip_buf = (channel == 0) ? cr->clip->L : cr->clip->R; */
    /*         int32_t pos_in_clip_sframes = i * step + start_pos_sframes - cr->pos_sframes; */
    /*         if (pos_in_clip_sframes >= 0 && pos_in_clip_sframes < cr_len) { */
    /* 		chunk[i] += clip_buf[pos_in_clip_sframes + cr->in_mark_sframes]; */
    /*         } */
    /* 	    SDL_UnlockMutex(cr->lock); */
    /*     } */
    /* } */

    return chunk;
}

/* 
Sum track samples over a chunk of timeline and return an array of samples. from_mark_in indicates that samples
should be collected from the in mark rather than from the play head.
*/
float *get_mixdown_chunk(Timeline* tl, float *mixdown, uint8_t channel, uint32_t len_sframes, int32_t start_pos_sframes, float step)
{
    uint32_t chunk_len_bytes = sizeof(float) * len_sframes;
    /* float *mixdown = malloc(chunk_len_bytes); */
    memset(mixdown, '\0', chunk_len_bytes);
    if (!mixdown) {
        fprintf(stderr, "\nError: could not allocate mixdown chunk.");
        exit(1);
    }

    /* long unsigned track_mixdown_time = 0; */
    /* long unsigned track_filter_time = 0; */
    for (uint8_t t=0; t<tl->num_tracks; t++) {
        Track *track = tl->tracks[t];
        /* double panctrlval = track->pan_ctrl->value; */
        /* double lpan = track->pan_ctrl->value < 0 ? 1 : 1 - panctrlval; */
        /* double rpan = track->pan_ctrl->value > 0 ? 1 : 1 + panctrlval; */
        /* double pan = channel == 0 ? lpan : rpan; */
	float track_chunk[len_sframes];
	/* clock_t a, b; */
	/* a=clock(); */
        get_track_channel_chunk(track, track_chunk, channel, start_pos_sframes, len_sframes, step);
	/* b=clock(); */
	/* track_mixdown_time+=(b-a); */
	if (track->fir_filter_active) {
	    /* a = clock(); */
	    apply_filter(track->fir_filter, track, channel, len_sframes, track_chunk);
	    /* b= clock(); */
	    /* track_filter_time = (b-a); */
	}
        for (uint32_t i=0; i<len_sframes; i++) {
            /* mixdown[i] += track_chunk[i] * pan * track->vol_ctrl->value; */
	    mixdown[i] += track_chunk[i];
	    /* fprintf(stdout, "Track chunk %d: %f\n", i, track_chunk[i]); */
	    if (t == tl->num_tracks - 1) {
		if (mixdown[i] > 1.0f) {
		    mixdown[i] = 1.0f;
		    /* fprintf(stdout, "Clip up mixdown\n"); */
		} else if (mixdown[i] < -1.0f) {
		    mixdown[i] = -1.0f;
		    /* fprintf(stdout, "Clip down mixdown\n"); */
		}
	    }
        }

	
        /* free(track_chunk); */
    }
    /* fprintf(stdout, "\tMixdown: %lu\n\tFilter: %lu\n", track_mixdown_time, track_filter_time); */


    return mixdown;
}
