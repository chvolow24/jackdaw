/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <math.h>
#include <pthread.h>
#include "audio_clip.h"
#include "automation.h"
#include "clipref.h"
#include "consts.h"
#include "dsp_utils.h"
#include "effect.h"
#include "midi_io.h"
#include "project.h"
#include "session.h"
#include "synth.h"

#define AMP_EPSILON 1e-7f

static void make_pan_chunk(float *pan_vals, int32_t len_sframes, uint8_t channel) 
{
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


/* double timespec_elapsed_ms(const struct timespec *start, const struct timespec *end); */
float get_track_channel_chunk(Track *track, float *chunk, uint8_t channel, int32_t start_pos_sframes, uint32_t output_chunk_len_sframes, float step)
{
    Session *session = session_get();
    uint32_t chunk_bytelen = sizeof(float) * output_chunk_len_sframes;
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

    if (channel == 0) {
	for (int i=0; i<track->num_automations; i++) {
	    Automation *a = track->automations[i];
	    if (a->read && !a->write && a->endpoint) {
		Value val = automation_get_value(a, start_pos_sframes, step);	    
		endpoint_write(a->endpoint, val, true, true, true, false);
	    }
	}
    }

    float vol_vals[output_chunk_len_sframes];
    float pan_vals[output_chunk_len_sframes];


    /* Construct volume buffer for linear scaling */
    if (vol_auto && !vol_auto->write && vol_auto->read) {
	automation_get_range(vol_auto, vol_vals, output_chunk_len_sframes, start_pos_sframes, step);
	for (int i=0; i<output_chunk_len_sframes; i++) {
	    vol_vals[i] = pow(vol_vals[i], VOL_EXP);
	}
    } else {
	/* Value vol_val = endpoint_safe_read(&track->vol_ep, NULL); */
	/* float vol_val = track-> */
	for (int i=0; i<output_chunk_len_sframes; i++) {
	    vol_vals[i] = pow(track->vol, VOL_EXP);
	}
    }

    /* Construct pan buffer for linear scaling */
    if (pan_auto && !pan_auto->write && pan_auto->read) {
	automation_get_range(pan_auto, pan_vals, output_chunk_len_sframes, start_pos_sframes, step);
	make_pan_chunk(pan_vals, output_chunk_len_sframes, channel);
    } else {
	Value pan_val = endpoint_safe_read(&track->pan_ep, NULL);
	float pan_scale = pan_val.float_v;
	pan_scale = channel == 0 ?
	    pan_scale <= 0.5 ? 1.0 : (1.0f - pan_scale) * 2 :
	    pan_scale >= 0.5 ? 1.0 : pan_scale * 2;
	for (int i=0; i<output_chunk_len_sframes; i++) {
	    pan_vals[i] = pan_scale;
	}
    }

    float total_amp = 0.0f;

    /* Get data from clip sources */
    /* struct timespec tspec_start; */
    /* struct timespec tspec_end; */
    /* clock_gettime(CLOCK_REALTIME, &tspec_start); */
    for (uint16_t i=0; i<track->num_clips; i++) {
	ClipRef *cr = track->clips[i];
	if (!cr) {
	    continue;
	}
	if (session->dragging && cr->grabbed && cr->grabbed_edge == CLIPREF_EDGE_NONE) {
	    continue;
	}
	Clip *clip = NULL;
	MIDIClip *mclip = NULL;
	if (cr->type == CLIP_AUDIO) clip = cr->source_clip;
	else if (cr->type == CLIP_MIDI) mclip = cr->source_clip;
	
	/* if (!clip) continue; /\* TODO: Handle MIDI clips here *\/ */
	if (mclip && mclip->recording) continue;

	pthread_mutex_lock(&cr->lock);
	if ((clip && clip->recording) || (mclip && mclip->recording)) {
	    pthread_mutex_unlock(&cr->lock);
	    continue;
	}
	double pos_in_clip_sframes = start_pos_sframes - cr->tl_pos;
	int32_t end_pos = pos_in_clip_sframes + (step * output_chunk_len_sframes);
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
	int num_events = 0;
	if (channel == 0 && mclip && step > 0.0 && fabs(step) < 50.0) {

	    /* clock_gettime(CLOCK_REALTIME, &tspec_start); */
	    PmEvent events[256];
	    num_events = midi_clipref_output_chunk(cr, events, 256, start_pos_sframes, start_pos_sframes + (float)output_chunk_len_sframes * step);
	    if (track->midi_out && step > 0.0) {
		switch(track->midi_out_type) {
		case MIDI_OUT_SYNTH: {
		    Synth *synth = track->midi_out;
		    if (session->midi_io.monitor_synth == synth) {
			/* int32_t remaining_to_process = track->tl->read_pos_sframes - track->tl->play_pos_sframes; */
			int32_t loc_start_pos = track->tl->play_pos_sframes < start_pos_sframes ? track->tl->play_pos_sframes : start_pos_sframes;
			synth_feed_midi(
			    synth,
			    events,
			    num_events,
			    loc_start_pos,
			    /* track->tl->play_pos_sframes, */
			    false);
		    } else {
			synth_feed_midi( 
			    synth,
			    events,
			    num_events,
			    start_pos_sframes,
			    false);
		    }
		    /* memcpy(synth->events, events, sizeof(PmEvent) * num_events); */
		    /* synth->num_events = num_events; */
		}
		    break;
		case MIDI_OUT_DEVICE:
		    break;
		}
	    }
	    /* fprintf(stderr, "%d events sent! (%d-%d)\n", num_events, start_pos_sframes, (int32_t)((float)start_pos_sframes + output_chunk_len_sframes * step)); */
	}
	if (clip) {
	    float *clip_buf =
		clip->channels == 1 ? clip->L :
		channel == 0 ? clip->L : clip->R;
	    int16_t chunk_i = 0;
	    while (chunk_i < output_chunk_len_sframes) {
		if (pos_in_clip_sframes >= 0 && pos_in_clip_sframes < cr_len - 1) { /* Truncate last sample to allow for interpolation */
		    float sample;
		    double clip_index_f = pos_in_clip_sframes + (double)cr->start_in_clip;
		    if (fabs(step) != 1.0f) {
			int index_left = (int)floor(clip_index_f);
			double diff_left = clip_index_f - (double)index_left;
			double diff = clip_buf[index_left + 1] - clip_buf[index_left];
			sample = clip_buf[index_left] + diff_left * diff;
		    } else {
			sample = clip_buf[(int)clip_index_f];
		    }
		    sample *= cr->gain;
		    chunk[chunk_i] += sample;
		    total_amp += fabs(chunk[chunk_i]);
		}
		pos_in_clip_sframes += step;
		chunk_i++;
	    }
	}
	pthread_mutex_unlock(&cr->lock);
    }
    /* clock_gettime(CLOCK_REALTIME, &tspec_end); */
    /* double elapsed_ms = timespec_elapsed_ms(&tspec_start, &tspec_end); */
    /* if (elapsed_ms > 0.04) { */
    /* 	fprintf(stderr, "%f %s\n", timespec_elapsed_ms(&tspec_start, &tspec_end), track->name); */
    /* } */
    /* clock_gettime(CLOCK_REALTIME, &tspec_start); */
    if (track->midi_out && track->midi_out != session->midi_io.monitor_synth) {
    /* if (track->midi_out && !session->playback.recording) { */
	switch(track->midi_out_type) {
	case MIDI_OUT_SYNTH: {
	    Synth *synth = track->midi_out;
	    synth_add_buf(synth, chunk, channel, output_chunk_len_sframes, step);
	    /* synth_add_buf(synth, chunk, channel, output_chunk_len_sframes, start_pos_sframes, false, step); */
	    total_amp += 1.0;
	}
	    break;
	case MIDI_OUT_DEVICE:
	    break;
	}
    }
    /* if (strcmp(track->name, "Track 6") == 0) { */
    /* 	clock_gettime(CLOCK_REALTIME, &tspec_end); */
    /* 	double elapsed_ms = timespec_elapsed_ms(&tspec_start, &tspec_end); */
    /* 	fprintf(stderr, "\tSynth %f\n", elapsed_ms); */
    /* } */



    float bus_buf[output_chunk_len_sframes];
    for (uint8_t i=0; i<track->num_bus_ins; i++) {
	Track *bus_in = track->bus_ins[i];
	float amp = get_track_channel_chunk(bus_in, bus_buf, channel, start_pos_sframes, output_chunk_len_sframes, step);
	if (amp > 0.0) {
	    float_buf_add(chunk, bus_buf, output_chunk_len_sframes);
	    total_amp += amp;
	}
    }
    /* if (total_amp < AMP_EPSILON) { */
    /* 	eq_advance(&track->eq, channel); */
    /* } else { */
    /* 	eq_buf_apply(&track->eq, chunk, len_sframes, channel); */
    /* 	filter_buf_apply(&track->fir_filter, chunk, len_sframes, channel); */
    /* 	saturation_buf_apply(&track->saturation, chunk, len_sframes, channel); */
    /* } */
    
    /* total_amp += delay_line_buf_apply(&track->delay_line, chunk, len_sframes, channel); */
    total_amp = effect_chain_buf_apply(track->effects, track->num_effects, chunk, output_chunk_len_sframes, channel, total_amp);
    if (total_amp > AMP_EPSILON) {
	float_buf_mult(chunk, vol_vals, output_chunk_len_sframes);
	float_buf_mult(chunk, pan_vals, output_chunk_len_sframes);
    }

    return total_amp;
}


/* clock_t start; */
/* double pre_track; */
/* double track_subtotals[255]; */
/* double filter; */
/* double after_track; */
/* double grand_total; */


extern Window *main_win;

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

    
    /* static AllpassGroup diffuser[2]; */
    /* static bool diffuser_init = false; */

    /* static LopDelay lop_delay[2]; */
    /* if (!diffuser_init) { */
    /* 	memset(diffuser, 0, sizeof(AllpassGroup) * 2); */
    /* 	fprintf(stderr, "INITIALIZING FILTERS.\n"); */
    /* 	allpass_group_init_schroeder(diffuser); */
    /* 	allpass_group_init_schroeder(diffuser + 1); */
    /* 	diffuser_init = true; */

    /* 	lop_delay_init(lop_delay, 20000, 0.99, 0.2); */
    /* 	lop_delay_init(lop_delay + 1, 20000, 0.99, 0.2); */
    /* } */
    
    for (uint8_t t=0; t<tl->num_tracks; t++) {
	bool audio_in_track = false;
        Track *track = tl->tracks[t];

	/* Track will be processed as a bus in */
	if (track->bus_out) {
	    continue;
	}
	
	float track_chunk[len_sframes];

        float track_chunk_amp = get_track_channel_chunk(track, track_chunk, channel, start_pos_sframes, len_sframes, step);
	    	
	if (track_chunk_amp > AMP_EPSILON) { /* Checks if any clip audio available */
	    audio_in_track = true;
	}
	if (audio_in_track) {
	    float_buf_add(mixdown, track_chunk, len_sframes);
	}
    }

    /* PLAYSPEED ROLLOFF:
       
       If playspeed drops below threshold, attenuate volume linearly
       based on previous rolloff value. Attenuating volume mitigates
       large DC offsets at extremely slow playspeeds, and linear ramps
       prevent pops at mixdown chunk boundaries when playspeed adjusted
       continuously
     */
    float fabs_playspeed = fabs(step);
    const float playspeed_thresh = 0.2;
    static float playspeed_rolloff[2] = {1.0f, 1.0f};
    

    if (fabs_playspeed < playspeed_thresh) {
	/* float playspeed_rolloff = pow(fabs_playspeed / playspeed_thresh, VOL_EXP * 2);	 */
	float new_playspeed_rolloff = tanh(4.0f * fabs_playspeed / playspeed_thresh);
	double m = (new_playspeed_rolloff - playspeed_rolloff[channel]) / len_sframes;
	float rolloff_buf[len_sframes];
	/* memset(rolloff_buf, 0, sizeof(rolloff_buf)); */
	/* float start = playspeed_rolloff[channel]; */
	float val = playspeed_rolloff[channel];
	for (int i=0; i<len_sframes; i++) {
	    rolloff_buf[i] = val;
	    val += m;
	    if (val < 0.0f) val = 0.0f;
	    else if (val > 1.0f) val = 1.0f;
	}

	playspeed_rolloff[channel] = new_playspeed_rolloff;
	float_buf_mult(mixdown, rolloff_buf, len_sframes);
    }

    return mixdown;
}
