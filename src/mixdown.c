/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    mixdown.c

    * Get samples from tracks/clips for playback or export
 *****************************************************************************************************************/

#include <math.h>
#include <pthread.h>
#include "automation.h"
#include "dsp.h"
#include "project.h"

#define AMP_EPSILON 0.00001f

extern Project *proj;

float get_track_channel_chunk(Track *track, float *chunk, uint8_t channel, int32_t start_pos_sframes, uint32_t len_sframes, float step)
{
    uint32_t chunk_bytelen = sizeof(float) * len_sframes;
    memset(chunk, '\0', chunk_bytelen);
    if (track->muted || track->solo_muted) {
	return 0.0f;
    }

    /************************* VOL/PAN AUTOMATION *************************/
    Automation *vol_auto = NULL;
    Automation *pan_auto = NULL;
    Automation *fir_filter_cutoff = NULL;
    Automation *fir_filter_bandwidth = NULL;
    Automation *delay_time = NULL;
    Automation *delay_amp = NULL;
    Automation *play_speed = NULL;

    for (uint8_t i=0; i<track->num_automations; i++) {
	Automation *a = track->automations[i];
	if (a->type == AUTO_VOL) vol_auto = a;
	else if (a->type == AUTO_PAN) pan_auto = a;
	else if (a->type == AUTO_FIR_FILTER_CUTOFF) fir_filter_cutoff = a;
	else if (a->type == AUTO_FIR_FILTER_BANDWIDTH) fir_filter_bandwidth = a;
	else if (a->type == AUTO_DEL_TIME) delay_time = a;
	else if (a->type == AUTO_DEL_AMP) delay_amp = a;
	else if (a->type == AUTO_PLAY_SPEED) play_speed = a;
    }

    float vol_vals[len_sframes];
    float pan_vals[len_sframes];

    float playspeed_rolloff = 1.0;
    float fabs_playspeed = fabs(proj->play_speed);
    /* Check for proj->playing to ensure write wav is not broken */
    if (proj->playing && fabs(proj->play_speed) < 0.01) {
	playspeed_rolloff = fabs_playspeed * 100.0f;
    }

    
    if (vol_auto && !vol_auto->write && vol_auto->read) {
	float vol_init = automation_get_value(vol_auto, start_pos_sframes, step).float_v;
	endpoint_write(&track->vol_ep, (Value){.float_v = vol_init}, true, true, true, false);
	automation_get_range(vol_auto, vol_vals, len_sframes, start_pos_sframes, step); 
    } else {
	Value vol_val = endpoint_safe_read(&track->vol_ep, NULL);
	for (int i=0; i<len_sframes; i++) {
	    vol_vals[i] = vol_val.float_v;
	}
    }
    if (pan_auto && !pan_auto->write && pan_auto->read) {
	float pan_init = automation_get_value(pan_auto, start_pos_sframes, step).float_v;
	endpoint_write(&track->pan_ep, (Value){.float_v = pan_init}, true, true, true, false);
	automation_get_range(pan_auto, pan_vals, len_sframes, start_pos_sframes, step);
    } else {
	Value pan_val = endpoint_safe_read(&track->pan_ep, NULL);
	for (int i=0; i<len_sframes; i++) {
	    pan_vals[i] = pan_val.float_v;
	}
    }

    if (channel == 0) {
	if (fir_filter_cutoff && fir_filter_cutoff->read && !fir_filter_cutoff->write) {
	    Value cutoff_raw = automation_get_value(fir_filter_cutoff, start_pos_sframes, step);
	    endpoint_write(&track->fir_filter.cutoff_ep, cutoff_raw, true, true, true, false);
	}
	if (fir_filter_bandwidth && fir_filter_bandwidth->read && !fir_filter_bandwidth->write) {
	    Value bandwidth_raw = automation_get_value(fir_filter_bandwidth, start_pos_sframes, step);
	    endpoint_write(&track->fir_filter.bandwidth_ep, bandwidth_raw, true, true, true, false);
	}
	if (delay_time && delay_time->read && !delay_time->write) {
	    Value del_time = automation_get_value(delay_time, start_pos_sframes, step);

	    /* int32_t del_time_msec = (double)del_time.int32_v * 1000.0 / proj->sample_rate; */
	    endpoint_write(&track->delay_line.len_ep, del_time, true, true, true, false);
	}
	if (delay_amp && delay_amp->read && !delay_amp->write) {
	    Value del_amp = automation_get_value(delay_amp, start_pos_sframes, step);
	    endpoint_write(&track->delay_line.amp_ep, del_amp, true, true, true, false);
	}
	if (play_speed && play_speed->read && !play_speed->write) {
	    float play_speed_val = (proj->play_speed / fabs(proj->play_speed)) * automation_get_value(play_speed, start_pos_sframes, step).float_v;
	    endpoint_write(&proj->play_speed_ep, (Value){.float_v = play_speed_val}, true, true, true, false);
	}
    }
    /* FIRFilter *f = &track->fir_filter; */
    
    /* if (f->frequency_response && channel == 0) { */
    /* 	if (cutoff_set && bandwidth_set) { */
    /* 	    FilterType t = f->type; */
    /* 	    filter_set_params_hz(f, t, cutoff_hz, bandwidth_hz); */
    /* 	} else if (cutoff_set) { */
    /* 	    filter_set_cutoff_hz(f, cutoff_hz); */
    /* 	} else if (bandwidth_set) { */
    /* 	    filter_set_bandwidth_hz(f, bandwidth_hz); */
    /* 	} */
    /* } */

    /* if (track->delay_line_active && channel == 0) { */
    /* 	int32_t del_time = track->delay_line.len; */
    /* 	double del_amp = track->delay_line.amp; */
    /* 	bool del_line_edit = false; */
    /* 	if (delay_time && delay_time->read && !delay_time->write) { */
    /* 	    del_time = automation_get_value(delay_time, start_pos_sframes, step).int32_v; */
    /* 	    del_line_edit = true; */
    /* 	} */
    /* 	if (delay_amp && delay_amp->read && !delay_amp->write) { */
    /* 	    del_amp = automation_get_value(delay_amp, start_pos_sframes, step).double_v; */
    /* 	    del_line_edit = true; */
    /* 	} */
    /* 	if (del_line_edit) { */
    /* 	    /\* fprintf(stdout, "DEL TIME: %d\n", del_time); *\/ */
    /* 	    /\* fprintf(stderr, "DEL TIME: %d\n", del_time); *\/ */
    /* 	    if (del_time < 0) { */
    /* 		fprintf(stderr, "ERROR: del time read value negative: %d\n", del_time); */
    /* 	    } else { */
    /* 		delay_line_set_params(&track->delay_line, del_amp, del_time); */
    /* 	    } */
    /* 	} */
    /* } */

    /**********************************************************************/
    /* bool clip_read = false; */

    float total_amp = 0.0f;
    for (uint16_t i=0; i<track->num_clips; i++) {
	ClipRef *cr = track->clips[i];
	if (!cr) {
	    continue;
	}
	if (proj->dragging && cr->grabbed) {
	    continue;
	}
	pthread_mutex_lock(&cr->lock);
	if (cr->clip->recording) {
	    pthread_mutex_unlock(&cr->lock);
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
	int32_t cr_len = clipref_len(cr);
	if (max < 0 || min > cr_len) {
	    pthread_mutex_unlock(&cr->lock);
	    continue;
	}
	float *clip_buf =
	    cr->clip->channels == 1 ? cr->clip->L :
	    channel == 0 ? cr->clip->L : cr->clip->R;
	int16_t chunk_i = 0;
	/* float pan_scale = (channel == 0) ? (track->pan - 0.5) * 2 : track->pan * 2; */

	/* double rpan = track->pan < */
	/* int32_t abs_pos = start_pos_sframes; */
	while (chunk_i < len_sframes) {
	    
	    /* Clip overlaps */
	    if (pos_in_clip_sframes >= 0 && pos_in_clip_sframes < cr_len) {
		/* float vol; */
		/* if (vol_auto && vol_auto->read && !vol_auto->write) { */
		/*     vol = automation_get_value(vol_auto, abs_pos, step).float_v; */
		/* } else { */
		/*     /\* vol = track->vol; *\/		     */
		/*     Value volval = endpoint_safe_read(&track->vol_ep, NULL); */
		/*     vol = volval.float_v; */
		/* } */
		/* float raw_pan; */
		/* if (pan_auto && pan_auto->read && !pan_auto->write) { */
		/*     raw_pan = automation_get_value(pan_auto, abs_pos, step).float_v; */
		/* } else { */
		/*     raw_pan = track->pan; */
		/* } */
		float vol = vol_vals[chunk_i];
		float raw_pan = pan_vals[chunk_i];
		double pan_scale = channel == 0 ?
		    raw_pan <= 0.5 ? 1 : (1.0f - raw_pan) * 2
		    : raw_pan >= 0.5 ? 1 : raw_pan * 2;
		/* static const float coeffs[] = {0.1, 0.2, 0.3, 0.2, 0.1, -0.09, -0.18, -0.1, -0.05}; */
		/* static float filterbuf[9]; */
		
		float add = clip_buf[(int32_t)pos_in_clip_sframes + cr->in_mark_sframes] * vol * pan_scale * playspeed_rolloff;
		/* add = do_filter(add, filterbuf, coeffs, 9); */
		chunk[chunk_i] += add;
		
		total_amp += fabs(chunk[chunk_i]);
	    }
	    pos_in_clip_sframes += step;
	    /* fprintf(stdout, "Chunk %d: %f\n", chunk_i, chunk[chunk_i]); */
	    chunk_i++;
	    /* abs_pos += step; */
	}	
	pthread_mutex_unlock(&cr->lock);
	/* clip_read = true; */
    }

    return total_amp;
}


/* clock_t start; */
/* double pre_track; */
/* double track_subtotals[255]; */
/* double filter; */
/* double after_track; */
/* double grand_total; */

/* 
Sum track samples over a chunk of timeline and return an array of samples. from_mark_in indicates that samples
should be collected from the in mark rather than from the play head.
*/
float *get_mixdown_chunk(Timeline* tl, float *mixdown, uint8_t channel, uint32_t len_sframes, int32_t start_pos_sframes, float step)
{

    /* start = clock(); */
    /* clock_t start = clock(); */
    uint32_t chunk_len_bytes = sizeof(float) * len_sframes;
    /* float *mixdown = malloc(chunk_len_bytes); */
    memset(mixdown, '\0', chunk_len_bytes);
    if (!mixdown) {
        fprintf(stderr, "\nError: could not allocate mixdown chunk.");
        exit(1);
    }

    int32_t end_pos_sframes = start_pos_sframes + len_sframes * step;
    for (uint8_t i=0; i<tl->num_click_tracks; i++) {
	ClickTrack *tt = tl->click_tracks[i];
	click_track_mix_metronome(tt, mixdown, len_sframes, start_pos_sframes, end_pos_sframes, step);
    }

    /* long unsigned track_mixdown_time = 0; */
    /* long unsigned track_filter_time = 0; */
    /* pre_track += (double)(clock() - start) / CLOCKS_PER_SEC; */
    
    for (uint8_t t=0; t<tl->num_tracks; t++) {
	bool audio_in_track = false;
        Track *track = tl->tracks[t];

	float track_chunk[len_sframes];

	/* start = clock(); */
        float track_chunk_amp = get_track_channel_chunk(track, track_chunk, channel, start_pos_sframes, len_sframes, step);
	/* track_subtotals[t] += ((double)clock() - start) / CLOCKS_PER_SEC; */
	

	if (track_chunk_amp > AMP_EPSILON) { /* Checks if any clip audio available */
	    audio_in_track = true;
	}

	/* start = clock(); */
	if (audio_in_track && track->fir_filter_active) { /* Only apply FIR filter if there is audio */
	    apply_filter(&track->fir_filter, track, channel, len_sframes, track_chunk);
	}
	/* filter += ((double)clock() - start) / CLOCKS_PER_SEC; */

	/* start = clock(); */
	float del_line_total_amp = 0.0f;
	if (track->delay_line_active) {
	    DelayLine *dl = &track->delay_line;
	    pthread_mutex_lock(&dl->lock);
	    double *del_line = channel == 0 ? dl->buf_L : dl->buf_R;
	    int32_t *del_line_pos = channel == 0 ? &dl->pos_L : &dl->pos_R;
	    for (int16_t i=0; i<len_sframes; i++) {
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
		del_line_total_amp += fabs(del_line[pos]);
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

		/* clip delay line */
		if (del_line[*del_line_pos] > 1.0) del_line[*del_line_pos] = 1.0;
		else if (del_line[*del_line_pos] < -1.0) del_line[*del_line_pos] = -1.0;
		
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
	    pthread_mutex_unlock(&dl->lock);
	}

	if (del_line_total_amp > AMP_EPSILON) { /* There is something to play back */
	    audio_in_track = true;
	}
	if (audio_in_track || t == tl->num_tracks - 1) {
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
	}
	/* after_track += ((double)clock() - start) / CLOCKS_PER_SEC; */

    }
    return mixdown;
    /* fprintf(stderr, "MIXDOWN TOTAL DUR: %f\n", 1000 * ((double)clock() - start)/ CLOCKS_PER_SEC); */
}
