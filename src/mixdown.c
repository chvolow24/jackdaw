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
#include "eq.h"
#include "project.h"
#include "iir.h"

#define AMP_EPSILON 1e-7f

/* IIRFilter lowp_test; */
/* bool lowp_inited = false; */

extern Project *proj;


static void float_buf_add(float *main, float *add, int32_t len_sframes)
{
    for (int32_t i=0; i<len_sframes; i++) {
	main[i] += add[i];
    }
}

static void float_buf_mult(float *main, float *by, int32_t len_sframes)
{
    for (int32_t i=0; i<len_sframes; i++) {
	main[i] *= by[i];
    }
}
static void make_pan_chunk(float *pan_vals, int32_t len_sframes, uint8_t channel) 
{
    /* double pan_scale = channel == 0 ? */
    /* 	raw_pan <= 0.5 ? 1 : (1.0f - raw_pan) * 2 */
    /* 	: raw_pan >= 0.5 ? 1 : raw_pan * 2; */

    if (channel == 0) {
	for (int i=0; i<len_sframes; i++) {
	    pan_vals[i] = pan_vals[i] <= 0.5f ? 1.0f : (1.0f - pan_vals[i]) * 2;
	}
    } else {
	for (int i=0; i<len_sframes; i++) {
	    pan_vals[i] = pan_vals[i] >= 0.5f ? 1.0f :  pan_vals[i] * 2;
	}
    }
}


float get_track_channel_chunk(Track *track, float *chunk, uint8_t channel, int32_t start_pos_sframes, uint32_t len_sframes, float step)
{

    uint32_t chunk_bytelen = sizeof(float) * len_sframes;
    memset(chunk, '\0', chunk_bytelen);
    if (track->muted || (track->solo_muted && !track->bus_out)) {
	return 0.0f;
    }

    /************************* VOL/PAN AUTOMATION *************************/
    Automation *vol_auto = NULL;
    Automation *pan_auto = NULL;
    
    for (uint8_t i=0; i<track->num_automations; i++) {
	Automation *a = track->automations[i];
	if (a->type == AUTO_VOL) vol_auto = a;
	else if (a->type == AUTO_PAN) pan_auto = a;
	if (a->endpoint == &track->vol_ep) vol_auto = a;
	else if (a->endpoint == &track->pan_ep) pan_auto = a;
    }

    for (int i=0; i<track->num_automations; i++) {
	Automation *a = track->automations[i];
	if (a->read && !a->write && a->endpoint) {
	    Value val = automation_get_value(a, start_pos_sframes, step);	    
	    endpoint_write(a->endpoint, val, true, true, true, false);
	}
    }

    float vol_vals[len_sframes];
    float pan_vals[len_sframes];

    float playspeed_rolloff = 1.0;
    float fabs_playspeed = fabs(proj->play_speed);
    
    if (proj->playing && fabs(proj->play_speed) < 0.01) {
	playspeed_rolloff = fabs_playspeed * 100.0f;
    }

    /* Construct volume buffer for linear scaling */
    if (vol_auto && !vol_auto->write && vol_auto->read) {
	automation_get_range(vol_auto, vol_vals, len_sframes, start_pos_sframes, step);
	for (int i=0; i<len_sframes; i++) {
	    vol_vals[i] *= playspeed_rolloff;
	}
    } else {
	Value vol_val = endpoint_safe_read(&track->vol_ep, NULL);
	for (int i=0; i<len_sframes; i++) {
	    vol_vals[i] = vol_val.float_v * playspeed_rolloff;;
	}
    }

    /* Construct pan buffer for linear scaling */
    if (pan_auto && !pan_auto->write && pan_auto->read) {
	automation_get_range(pan_auto, pan_vals, len_sframes, start_pos_sframes, step);
	make_pan_chunk(pan_vals, len_sframes, channel);
    } else {
	Value pan_val = endpoint_safe_read(&track->pan_ep, NULL);
	float pan_scale = pan_val.float_v;
	pan_scale = channel == 0 ?
	    pan_scale <= 0.5 ? 1.0 : (1.0f - pan_scale) * 2 :
	    pan_scale >= 0.5 ? 1.0 : pan_scale * 2;
	for (int i=0; i<len_sframes; i++) {
	    pan_vals[i] = pan_scale;
	}
    }

    float total_amp = 0.0f;

    /* Get data from clip sources */
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
	
	while (chunk_i < len_sframes) {    
	    /* Clip overlaps */
	    if (pos_in_clip_sframes >= 0 && pos_in_clip_sframes < cr_len) {
		float sample = clip_buf[(int32_t)pos_in_clip_sframes + cr->in_mark_sframes];
		chunk[chunk_i] += sample;
		total_amp += fabs(chunk[chunk_i]);
	    }
	    pos_in_clip_sframes += step;
	    chunk_i++;
	}	
	pthread_mutex_unlock(&cr->lock);
    }

    float bus_buf[len_sframes];
    for (uint8_t i=0; i<track->num_bus_ins; i++) {
	Track *bus_in = track->bus_ins[i];
	float amp = get_track_channel_chunk(bus_in, bus_buf, channel, start_pos_sframes, len_sframes, step);
	if (amp > 0.0) {
	    float_buf_add(chunk, bus_buf, len_sframes);
	    total_amp += amp;
	}
    }

    if (total_amp < AMP_EPSILON) {
	eq_advance(&track->eq, channel);
    } else {
	eq_buf_apply(&track->eq, chunk, len_sframes, channel);
	filter_buf_apply(&track->fir_filter, chunk, len_sframes, channel);
	saturation_buf_apply(&track->saturation, chunk, len_sframes, channel);
    }
    
    total_amp += delay_line_buf_apply(&track->delay_line, chunk, len_sframes, channel);
    if (total_amp > AMP_EPSILON) {
	float_buf_mult(chunk, vol_vals, len_sframes);
	float_buf_mult(chunk, pan_vals, len_sframes);
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

	/* Track will be processed as a bus in */
	if (track->bus_out) {
	    continue;
	}
	
	float track_chunk[len_sframes];

	/* start = clock(); */
        float track_chunk_amp = get_track_channel_chunk(track, track_chunk, channel, start_pos_sframes, len_sframes, step);
	/* track_subtotals[t] += ((double)clock() - start) / CLOCKS_PER_SEC; */
	

	if (track_chunk_amp > AMP_EPSILON) { /* Checks if any clip audio available */
	    audio_in_track = true;
	}

	/* start = clock(); */
	/* if (audio_in_track && track->fir_filter.active) { /\* Only apply FIR filter if there is audio *\/ */
	/*     apply_filter(&track->fir_filter, track, channel, len_sframes, track_chunk); */
	/* } */
	/* filter += ((double)clock() - start) / CLOCKS_PER_SEC; */

	/* start = clock(); */

	/* TODO: correct conditional clipping after applying delay line */
	/* float del_line_total_amp = 1.0f; */
	
	/* if (track->delay_line.active) { */
	/*     DelayLine *dl = &track->delay_line; */
	/*     pthread_mutex_lock(&dl->lock); */
	/*     double *del_line = channel == 0 ? dl->buf_L : dl->buf_R; */
	/*     int32_t *del_line_pos = channel == 0 ? &dl->pos_L : &dl->pos_R; */
	/*     for (int16_t i=0; i<len_sframes; i++) { */
	/* 	double track_sample = track_chunk[i]; */
	/* 	int32_t pos = *del_line_pos; */
	/* 	if (channel == 0) { */
	/* 	    pos -= dl->stereo_offset * dl->len; */
	/* 	    if (pos < 0) { */
	/* 		pos += dl->len; */
	/* 	    } */
	/* 	    /\* pos %= dl->len; *\/ */
	/* 	} */
	/* 	track_chunk[i] += del_line[pos]; */
	/* 	del_line_total_amp += fabs(del_line[pos]); */
	/* 	/\* int tap = *del_line_pos - 1025; *\/ */
	/* 	/\* if (tap < 0) tap = dl->len + tap; *\/ */
	/* 	/\* track_chunk[i] += del_line[tap]; *\/ */
	/* 	/\* tap -= 2031; *\/ */
	/* 	/\* if (tap < 0) tap = dl->len + tap; *\/ */
	/* 	/\* track_chunk[i] += del_line[tap]; *\/ */
	/* 	/\* tap -= 3000; *\/ */
	/* 	/\* if (tap < 0) tap = dl->len + tap; *\/ */
	/* 	/\* track_chunk[i] += del_line[tap]; *\/ */
	/* 	/\* tap -= 2044; *\/ */
	/* 	/\* if (tap < 0) tap = dl->len + tap; *\/ */
	/* 	/\* track_chunk[i] += del_line[tap]; *\/ */
		
	/* 	del_line[*del_line_pos] += track_sample; */
	/* 	del_line[*del_line_pos] *= dl->amp; */

	/* 	/\* clip delay line *\/ */
	/* 	if (del_line[*del_line_pos] > 1.0) del_line[*del_line_pos] = 1.0; */
	/* 	else if (del_line[*del_line_pos] < -1.0) del_line[*del_line_pos] = -1.0; */
		
	/* 	/\* fprintf(stdout, "Del pos vs len? %d %d\n", *del_line_pos, dl.len); *\/ */
	/* 	if (*del_line_pos + 1 >= dl->len) { */
	/* 	    /\* fprintf(stdout, "\tSETTING zero\n"); *\/ */
	/* 	    *del_line_pos = 0; */
	/* 	} else { */
	/* 	    /\* fprintf(stdout, "\tinc %d->", *del_line_pos); *\/ */
	/* 	    (*del_line_pos)++; */
	/* 	    /\* fprintf(stdout, "%d\n", *del_line_pos); *\/ */
	/* 	} */
	/* 	/\* fprintf(stdout, "del line pos: %d\n", *del_line_pos); *\/ */
	/*     } */
	/*     pthread_mutex_unlock(&dl->lock); */
	/* } */

	/* if (del_line_total_amp > AMP_EPSILON) { /\* There is something to play back *\/ */
	/*     audio_in_track = true; */
	/* } */
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
