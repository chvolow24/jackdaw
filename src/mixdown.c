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

#include "automation.h"
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

    /************************* VOL/PAN AUTOMATION *************************/
    Automation *vol_auto = NULL;
    Automation *pan_auto = NULL;
    Automation *fir_filter_cutoff = NULL;
    Automation *fir_filter_bandwidth = NULL;
    Automation *delay_time = NULL;
    Automation *delay_amp = NULL;
    /* float vol_init = track->vol; */
    /* float pan_init = track->pan; */
    Value m_zero = {.float_v = 0.0f};
    AutomationSlope vol_m = {.dx = 1, .dy = m_zero};
    AutomationSlope pan_m = {.dx = 1, .dy = m_zero};
    /* double vol_m = 0.0; */
    /* double pan_m = 0.0f; */
    /* int32_t vol_next_kf; */
    /* int32_t pan_next_kf; */
    for (uint8_t i=0; i<track->num_automations; i++) {
	Automation *a = track->automations[i];
	if (a->type == AUTO_VOL) vol_auto = a;
	else if (a->type == AUTO_PAN) pan_auto = a;
	else if (a->type == AUTO_FIR_FILTER_CUTOFF) fir_filter_cutoff = a;
	else if (a->type == AUTO_FIR_FILTER_BANDWIDTH) fir_filter_bandwidth = a;
	else if (a->type == AUTO_DEL_TIME) delay_time = a;
	else if (a->type == AUTO_DEL_AMP) delay_amp = a;
    }
    
    
    if (vol_auto && vol_auto->read) {
	float vol_init = automation_get_value(vol_auto, start_pos_sframes, step).float_v;
	track->vol = vol_init;
	slider_reset(track->vol_ctrl);
    }
    if (pan_auto && pan_auto->read) {
	float pan_init = automation_get_value(pan_auto, start_pos_sframes, step).float_v;
	track->pan = pan_init;
	slider_reset(track->pan_ctrl);
    }
    bool bandwidth_set = false;
    bool cutoff_set = false;
    double cutoff_hz;
    double bandwidth_hz;
    if (fir_filter_cutoff && fir_filter_cutoff->read) {
	double cutoff_raw = automation_get_value(fir_filter_cutoff, start_pos_sframes, step).double_v;
	cutoff_hz = dsp_scale_freq_to_hz(cutoff_raw);
	cutoff_set = true;
    }
    if (fir_filter_bandwidth && fir_filter_bandwidth->read) {
	double bandwidth_raw = automation_get_value(fir_filter_bandwidth, start_pos_sframes, step).double_v;
	bandwidth_hz = dsp_scale_freq_to_hz(bandwidth_raw);
	bandwidth_set = true;
    }
    FIRFilter *f;
    if ((f = track->fir_filter)) {
	if (cutoff_set && bandwidth_set) {
	    FilterType t = f->type;
	    filter_set_params_hz(f, t, cutoff_hz, bandwidth_hz);
	} else if (cutoff_set) {
	    filter_set_cutoff_hz(f, cutoff_hz);
	} else if (bandwidth_set) {
	    filter_set_bandwidth_hz(f, bandwidth_hz);
	}
    }

    if (track->delay_line_active) {
	int32_t del_time = track->delay_line.len;
	double del_amp = track->delay_line.amp;
	bool del_line_edit = false;
	if (delay_time && delay_time->read) {
	    del_time = automation_get_value(delay_time, start_pos_sframes, step).int32_v;
	    del_line_edit = true;
	}
	if (delay_amp && delay_amp->read) {
	    del_amp = automation_get_value(delay_amp, start_pos_sframes, step).double_v;
	    del_line_edit = true;
	}
	if (del_line_edit) {
	    /* fprintf(stdout, "DEL TIME: %d\n", del_time); */
	    if (del_time < 0) {
		fprintf(stderr, "ERROR: del time read value negative: %d\n", del_time);
	    } else {
		delay_line_set_params(&track->delay_line, del_amp, del_time);
	    }
	}
    }

    /**********************************************************************/
    bool clip_read = false;
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

	/* double rpan = track->pan < */
	int32_t abs_pos = start_pos_sframes;
	while (chunk_i < len_sframes) {
	    /* if (step > 0) { */
	    /* 	if (vol_auto && vol_auto->current && vol_auto->current->next && abs_pos >= vol_next_kf) { */
	    /* 	    vol_auto->current = vol_auto->current->next; */
	    /* 	    if (vol_auto->current->next) */
	    /* 		vol_next_kf = vol_auto->current->next->pos; */
	    /* 	    vol_m = vol_auto->current->m_fwd; */
	    /* 	    /\* fprintf(stderr, "RESETTING VOL CURRENT %d\n", vol_next_kf); *\/ */
	    /* 	} */
	    /* 	if (pan_auto && pan_auto->current && pan_auto->current->next && abs_pos >= pan_next_kf) { */
	    /* 	    pan_auto->current = pan_auto->current->next; */
	    /* 	    if (pan_auto->current->next) */
	    /* 		pan_next_kf = pan_auto->current->next->pos; */
	    /* 	    pan_m = pan_auto->current->m_fwd; */
	    /* 	    /\* fprintf(stderr, "RESETTING PAN CURRENT %d\n", pan_next_kf); *\/ */
	    /* 	} */
	    /* } else { */
	    /* 	if (vol_auto) { */
	    /* 	    if (vol_auto->current && vol_auto->current->prev && abs_pos <= vol_next_kf) { */
	    /* 		vol_auto->current = vol_auto->current->prev; */
	    /* 		vol_next_kf = vol_auto->current->pos; */
	    /* 		vol_m = vol_auto->current->m_fwd; */
	    /* 		/\* fprintf(stderr, "RESETTING VOL CURRENT %d\n", vol_next_kf); *\/ */
	    /* 	    } else { */
	    /* 		vol_auto->current = NULL; */
	    /* 	    } */
	    /* 	} */
	    /* 	if (pan_auto) { */
	    /* 	    if (pan_auto->current && pan_auto->current->prev && abs_pos <= pan_next_kf) { */
	    /* 		pan_auto->current = pan_auto->current->prev; */
	    /* 		pan_next_kf = pan_auto->current->pos; */
	    /* 		pan_m = pan_auto->current->m_fwd; */
	    /* 		/\* fprintf(stderr, "RESETTING PAN CURRENT %d\n", pan_next_kf); *\/ */
	    /* 	    } else { */
	    /* 		pan_auto->current = NULL; */
	    /* 	    } */
	    /* 	} */

	    /* } */
	    float vol;
	    if (vol_auto && vol_auto->read) {
		vol = automation_get_value(vol_auto, abs_pos, step).float_v;
	    } else {
		vol = track->vol;
	    }
	    float raw_pan;
	    if (pan_auto && pan_auto->read) {
		raw_pan = automation_get_value(pan_auto, abs_pos, step).float_v;
	    } else {
		raw_pan = track->pan;
	    }

						
	    /* float raw_pan = automation_get_value(pan_auto, abs_pos, step).float_v; */
	    /* fprintf(stdout, "RAW PAN: %f, RAW VOL: %f\n"); */
	    /* float raw_pan = pan_init + chunk_i * pan_m.dy.float_v / pan_m.dx; */
	    double pan_scale = channel == 0 ?
		raw_pan <= 0.5 ? 1 : (1.0f - raw_pan) * 2
	    : raw_pan >= 0.5 ? 1 : raw_pan * 2;

	    
	    if (pos_in_clip_sframes >= 0 && pos_in_clip_sframes < cr_len) {
		/* chunk[chunk_i] += clip_buf[(int32_t)pos_in_clip_sframes + cr->in_mark_sframes] * (track->vol + chunk_i * vol_m.dy.float_v / vol_m.dx) * pan_scale; */
		chunk[chunk_i] += clip_buf[(int32_t)pos_in_clip_sframes + cr->in_mark_sframes] * vol * pan_scale;
	    }
	    pos_in_clip_sframes += step;
	    /* fprintf(stdout, "Chunk %d: %f\n", chunk_i, chunk[chunk_i]); */
	    chunk_i++;
	    abs_pos += step;
	}	
	SDL_UnlockMutex(cr->lock);
	clip_read = true;
    }
    if (!clip_read) {
	if (vol_auto)
	    vol_auto->current = NULL;
	if (pan_auto)
	    pan_auto->current = NULL;
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


/* /\* DELAY TESTS *\/ */
/* double *del_line_l = NULL; */
/* double *del_line_r = NULL; */
/* int32_t del_line_len; */
/* int32_t del_line_pos_l = 0; */
/* int32_t del_line_pos_r = 0; */
/* double del_line_amp = 0.8; */
/* /\* END TESTS *\/ */

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
	if (track->delay_line_active) {
	    DelayLine *dl = &track->delay_line;
	    SDL_LockMutex(dl->lock);
	    double *del_line = channel == 0 ? dl->buf_L : dl->buf_R;
	    int32_t *del_line_pos = channel == 0 ? &dl->pos_L : &dl->pos_R;
	    /* do_fn2(); */
	    /* int32_t *del_line_pos = channel==0 ? &track->delay_line_L.pos : &track->delay_line_R.pos; */

	    for (int16_t i=0; i<len_sframes; i++) {
		/* fprintf(stdout, "Writing %f pos %d\n", del_line[del_line_pos], del_line_pos); */
		
		double track_sample = track_chunk[i];
		int32_t pos = *del_line_pos;
		if (channel == 0) {
		    pos -= dl->stereo_offset * dl->len;
		    if (pos < 0) {
			pos += dl->len;
		    }
		    /* pos %= dl->len; */
		}
		track_chunk[i] += del_line[pos];
		/* int tap = *del_line_pos - 1025; */
		/* if (tap < 0) tap = dl->len + tap; */
		/* track_chunk[i] += del_line[tap]; */
		/* tap -= 2031; */
		/* if (tap < 0) tap = dl->len + tap; */
		/* track_chunk[i] += del_line[tap]; */
		/* tap -= 3000; */
		/* if (tap < 0) tap = dl->len + tap; */
		/* track_chunk[i] += del_line[tap]; */
		/* tap -= 2044; */
		/* if (tap < 0) tap = dl->len + tap; */
		/* track_chunk[i] += del_line[tap]; */
		
		del_line[*del_line_pos] += track_sample;
		del_line[*del_line_pos] *= dl->amp;
		/* fprintf(stdout, "Del pos vs len? %d %d\n", *del_line_pos, dl.len); */
		if (*del_line_pos + 1 >= dl->len) {
		    /* fprintf(stdout, "\tSETTING zero\n"); */
		    *del_line_pos = 0;
		} else {
		    /* fprintf(stdout, "\tinc %d->", *del_line_pos); */
		    (*del_line_pos)++;
		    /* fprintf(stdout, "%d\n", *del_line_pos); */
		}
		/* fprintf(stdout, "del line pos: %d\n", *del_line_pos); */
	    }
	    SDL_UnlockMutex(dl->lock);
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
