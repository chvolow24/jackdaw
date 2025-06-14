#include <portmidi.h>
#include <porttime.h>
#include "adsr.h"
#include "dsp_utils.h"
#include "consts.h"
#include "synth.h"
#include "session.h"

extern double MTOF[];

Synth *synth_create(Track *parent_track)
{
    /* Session *session = session_get(); */
    Synth *s = calloc(1, sizeof(Synth));
    s->track = parent_track;
    /* synth_base_osc_set_freq_modulator(s, s->base_oscs + 0, s->base_oscs + 1); */
    /* synth_base_osc_set_freq_modulator(s, s->base_oscs + 2, s->base_oscs + 3); */

    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	s->voices[i].synth = s;
	s->voices[i].available = true;
	s->voices[i].amp_env[0].params = &s->amp_env;
	s->voices[i].amp_env[1].params = &s->amp_env;
    }
    adsr_set_params(
	&s->amp_env,
	96 * 8,
	96 * 400,
	0.2,
	96 * 500,
	3.0);
    s->monitor = true;
    s->allow_voice_stealing = true;

    for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) {
	OscCfg *cfg = s->base_oscs + i;
	endpoint_init(
	    &cfg->active_ep,
	    &cfg->active,
	    JDAW_BOOL,
	    "active",
	    "Active",
	    JDAW_THREAD_DSP,
	    
	    /*
	      endpoint_init(Endpoint *ep,
	      void *val,
	      ValType t,
	      const char *local_id,
	      const char *display_name,
	      enum jdaw_thread owner_thread,
	      EndptCb gui_cb,
	      EndptCb proj_cb,
	      EndptCb dsp_cb,
	      void *xarg1, void *xarg2, void *xarg3, void *xarg4) -> int */
	    
    }

    return s;
}

/* int synth_create_virtual_device(Synth *s) */
/* { */
/*     if (call_count > 0) { */
/* 	fprintf(stderr, "CALL COUNT!\n"); */
/* 	exit(1); */
/*     } */
/*     call_count++; */
/*     static const char *device_name = "Jackdaw Synth"; */

/*     PmError err = Pm_CreateVirtualInput( */
/* 	device_name, */
/* 	MIDI_INTERFACE_NAME, */
/* 	NULL); */
/*     if (err  < 0) { */
/* 	fprintf(stderr, "Error creating virtual device: %s\n", Pm_GetErrorText(err)); */
/* 	exit(1); */
/*     } */
/*     /\* if (err == pmInterfaceNotSupported) { *\/ */
/*     /\* 	fprintf(stderr, "Error: Interface \"%s\" not supported\n", MIDI_INTERFACE_NAME); *\/ */
/*     /\* 	return -1; *\/ */
/*     /\* } else if (err == pmInvalidDeviceId) { *\/ */
/*     /\* 	fprintf(stderr, "Error: device name \"%s\" invalid or already in use\n", device_name); *\/ */
/*     /\* 	return -1; *\/ */
/*     /\* } else  *\/ */
/*     /\* PmDeviceID id = err; *\/ */
/*     /\* s->device.id = id; *\/ */
/*     /\* s->device.info = Pm_GetDeviceInfo(id); *\/ */
    
/*     /\* err = Pm_OpenInput(&s->device.stream, s->device.id, NULL, PM_EVENT_BUF_NUM_EVENTS, NULL, NULL); *\/ */
/*     if (err != pmNoError) { */
/* 	fprintf(stderr, "ERROR OPENING DEVICE: %s\n", Pm_GetErrorText(err)); */
/* 	exit(1); */
/*     } */
    
/*     return id; */
/* } */

/* Osc LFO = { */
/*     WS_SINE, */
/*     {0.0, 0.0}, */
/*     0.005, */
/*     0, */
/*     0.2, */
/*     0.5, */
/*     NULL, */
/*     NULL */
/* }; */
static float osc_sample(Osc *osc, int channel, int num_channels, float step)
{
    float sample;
    switch(osc->type) {
    case WS_SINE:
	sample = sin(TAU * osc->phase[channel]);
	break;
    case WS_SQUARE:
	sample = osc->phase[channel] < 0.5 ? -1.0 : 1.0;
	break;
    case WS_TRI:
	sample =
	    osc->phase[channel] <= 0.25 ?
	        osc->phase[channel] * 4.0f :
	    osc->phase[channel] <= 0.5f ?
	        (0.5f - osc->phase[channel]) * 4.0 :
	    osc->phase[channel] <= 0.75f ?
	        (osc->phase[channel] - 0.5f) * -4.0 :
	        (1.0f - osc->phase[channel]) * -4.0;
        break;
    case WS_SAW:
	sample = 2.0f * osc->phase[channel] - 1.0;
	break;
    default:
	sample = 0.0;
    }
    if (osc->freq_modulator) {
	float fmod_sample = osc_sample(osc->freq_modulator, channel, num_channels, step);
	/* fprintf(stderr, "Phase incr: %f fmod: %f\n", osc->sample_phase_incr, fmod_sample); */
	osc->phase[channel] += osc->sample_phase_incr * (step + fmod_sample);
    } else {
	osc->phase[channel] += osc->sample_phase_incr * step;
    }
    if (osc->phase[channel] > 1.0) {
	osc->phase[channel] -= 1.0;
    }
    float pan_scale;
    if (channel == 0) {
	pan_scale = osc->pan <= 0.5 ? 1.0 : (1.0f - osc->pan) * 2.0f;
    } else {
	pan_scale = osc->pan >= 0.5 ? 1.0 : osc->pan * 2.0f;
    }
    return sample * osc->amp * pan_scale;
}


static void synth_voice_add_buf(SynthVoice *v, float *buf, int32_t len, int channel, float step)
{
    if (v->available) return;
    /* fprintf(stderr, "\tvoice %ld has data\n", v - v->synth->voices); */
    for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) {
	OscCfg *cfg = v->synth->base_oscs + i;
	if (!cfg->active) continue;
	if (cfg->mod_freq_of || cfg->mod_amp_of) continue;
	/* fprintf(stderr, "\t\tvoice %ld osc %d\n", v - v->synth->voices, i); */
	Osc *osc = v->oscs + i;
	float osc_buf[len];
	for (int i=0; i<len; i++) {
	    osc_buf[i] = osc_sample(osc, channel, 2, step);
	}
	for (int j=0; j<cfg->unison.num_voices; j++) {
	    Osc *detune_voice = v->oscs + i + SYNTH_NUM_BASE_OSCS * (j + 1);
	    for (int i=0; i<len; i++) {
		/* fprintf(stderr, "adding detune voice %d/%d, freq %f\n", j, cfg->detune.num_voices, detune_voice->freq); */
		osc_buf[i] += osc_sample(detune_voice, channel, 2, step);
	    }
	}
	/* for (int j=0; j< */
	float amp_env[len];
	enum adsr_stage amp_stage = adsr_get_chunk(&v->amp_env[channel], amp_env, len);
	float_buf_mult(osc_buf, amp_env, len);
	if (amp_stage == ADSR_OVERRUN) {
	    v->available = true;
	    /* fprintf(stderr, "\tFREEING VOICE %ld (env overrun)\n", v - v->synth->voices); */
	    /* return; */
	}
	for (int i=0; i<len; i++) {
	    buf[i] += osc_buf[i] * (float)v->velocity / 127.0f;
	}
	
	/* for (int i=0; i<len; i++) { */
	/*     float sample = osc_sample(osc, channel, 2, step); */
	/*     bool is_finished = false; */
	/*     float env = adsr_sample(&v->amp_env[channel], &is_finished); */
	/*     if (is_finished) { */
	/* 	v->available = true; */
	/* 	/\* fprintf(stderr, "Passed adsr on voice %ld\n", v - v->synth->voices); *\/ */
	/* 	return; */
	/*     }	 */
	/*     sample *= env * (float)v->velocity / 127.0f; */
 	/*     buf[i] += sample; */
	/*     buf[i] = tanh(buf[i]); */
	/*     /\* v->last_env[channel] = env; *\/ */
	/*     /\* v->env_remaining[channel]--; *\/ */
	/*     v->note_end_rel[channel]--; */
	/*     v->note_start_rel[channel]--; */
	/*     /\* if (channel == 0) { *\/ */
	/*     /\* 	fprintf(stderr, "Env stage %d, remaining %d\n", v->amp_env_stage[channel], v->env_remaining[channel]); *\/ */
	/*     /\* } *\/ */
	/* } */
	    /* if (start_rel < v->synth->amp_env.a) { */
	    /* 	/\* fprintf(stderr, "\t\t\t\tAttack!\n"); *\/ */
	    /* 	sample *= (float)start_rel / v->synth->amp_env.a; */
	    /* } else if (start_rel < v->synth->amp_env.a + v->synth->amp_env.d) { */
	    /* 	/\* fprintf(stderr, "\t\t\t\tDecay!\n"); *\/ */
	    /* 	sample *= v->synth->amp_env.s + (float)(start_rel - v->synth->amp_env.a) / v->synth->amp_env.d; */
	    /* } else if (end_rel < 0) { */
	    /* 	sample *= v->synth->amp_env.s; */
	    /* 	/\* fprintf(stderr, "\t\t\t\tSustain! * %f\n", v->synth->amp_env.s); *\/ */
	    /* } else if (end_rel < v->synth->amp_env.r) { */
	    /* 	/\* fprintf(stderr, "Num %d - %d\n", v->synth->amp_env.r, end_rel); *\/ */
	    /* 	sample *= v->synth->amp_env.s * (float)(v->synth->amp_env.r - end_rel) / v->synth->amp_env.r; */
	    /* 	/\* fprintf(stderr, "\t\t\t\tRelease!\n"); *\/ */
	    /* } else { */
	    /* 	v->available = true; */
	    /* 	/\* fprintf(stderr, "Passed ADSR!\n"); *\/ */
	    /* 	return; /\* We've passed ADSR *\/ */

	    /* } */
	    /* buf[i] += sample; */
	    /* start_rel++; */
	    /* end_rel++; */
	/* } */

	/* v->note_start_rel += len; */
	/* if (channel == 1) { */
	/*     v->note_start_rel += len; */
	/*     v->note_end_rel += len; */
	/* } */
    }
    /* for (int i=0; i<len; i++) { */
    /* 	buf[i] = tanh(buf[i]); */
    /* } */
}

static void osc_set_freq(Osc *osc, double freq_hz)
{
    Session *session = session_get();
    osc->freq = freq_hz;
    osc->sample_phase_incr = freq_hz / session->proj.sample_rate;    
}
/* static void synth_voice_set_note(SynthVoice *v, uint8_t note_val, uint8_t velocity) */
/* { */
/*     v->note_val = note_val; */
/*     for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) { */
/* 	/\* osc_set_freq(v->oscs + i, MTOF[note_val]); *\/ */
/* 	osc_set_freq(v->oscs + i, mtof_calc(note_val)); */
/*     } */
/*     osc_set_freq(v->oscs + 1, mtof_calc((double)note_val + 0.02)); */
/*     /\* fprintf(stderr, "OSC 1 %f OSC 2: %f\n", v->oscs[0].freq, v->oscs[1].freq); *\/ */
/*     v->velocity = velocity; */
/* } */

static void synth_voice_assign_note(SynthVoice *v, double note, int velocity, int32_t start_rel)
{
    /* srand(time(NULL)); */
    Synth *synth = v->synth;
    v->note_val = note;
    for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) {
	OscCfg *cfg = synth->base_oscs + i;
	if (!cfg->active) continue;	
	Osc *osc = v->oscs + i;
	double base_midi_note = note + cfg->octave * 12 + cfg->tune_coarse + cfg->tune_fine / 100.0f;
	osc->amp = cfg->amp;
	osc->pan = cfg->pan;
	osc->type = cfg->type;
	osc->phase[0] = 0.0;
	osc->phase[1] = 0.0;
	/* osc->freq_modulator = &LFO; */
	/* osc_set_freq(v->oscs + i, MTOF[note_val]); */
	osc_set_freq(osc, mtof_calc(base_midi_note));
	fprintf(stderr, "BASE NOTE: %f\n", base_midi_note);
	for (int j=0; j<cfg->unison.num_voices; j++) {
	    Osc *detune_voice = v->oscs + i + SYNTH_NUM_BASE_OSCS * (j + 1);
	    detune_voice->type = cfg->type;
	    double detune_midi_note =
		j % 2 == 0 ?
		base_midi_note - cfg->unison.detune_cents / 100.0f * (j + 1) :
		base_midi_note + cfg->unison.detune_cents / 100.0f * (j + 1);
	    fprintf(stderr, "DETUNE MIDI NOTE: %f (detune cents? %f /100? %f\n", detune_midi_note, cfg->unison.detune_cents, cfg->unison.detune_cents / 100.0f);
	    osc_set_freq(detune_voice, mtof_calc(detune_midi_note));
	    detune_voice->amp = osc->amp * cfg->unison.relative_amp;
	    detune_voice->pan =
		j % 2 == 0 ?
		osc->pan + (j + 1) * cfg->unison.stereo_spread / 2.0f / (float)cfg->unison.num_voices :
		osc->pan - (j + 1) * cfg->unison.stereo_spread / 2.0f / (float)cfg->unison.num_voices;
	    if (detune_voice->pan > 1.0) detune_voice->pan = 1.0;
	    if (detune_voice->pan < 0.0) detune_voice->pan = 0.0;
	    detune_voice->phase[0] = 0.0;
	    detune_voice->phase[1] = 0.0;
	    /* int randomized_pan_index = rand() % cfg->detune.num_voices; */	    
	    
	}
    }
    /* osc_set_freq(v->oscs + 1, mtof_calc((double)note + 0.02)); */
    /* fprintf(stderr, "OSC 1 %f OSC 2: %f\n", v->oscs[0].freq, v->oscs[1].freq); */
    v->velocity = velocity;

    /* v->note_start_rel[0] = start_rel; */
    /* v->note_start_rel[1] = start_rel; */
    /* v->note_end_rel[0] = INT32_MIN; */
    /* v->note_end_rel[1] = INT32_MIN; */
    v->available = false;
    adsr_init(v->amp_env, start_rel);
    adsr_init(v->amp_env + 1, start_rel);
    /* v->amp_env_stage[0] = 0; */
    /* v->amp_env_stage[1] = 0; */
    /* v->env_remaining[0] = v->synth->amp_env.a; */
    /* v->env_remaining[1] = v->synth->amp_env.a; */
    /* v->oscs[0].phase[0] = 0.0; */
    /* v->oscs[0].phase[1] = 0.0; */
    /* v->oscs[1].phase[0] = 0.0; */
    /* v->oscs[1].phase[0] = 0.0; */
    
    /* v->oscs[0].pan = 0.4; */
    /* v->oscs[1].pan = 0.6; */
    

}

void synth_base_osc_set_freq_modulator(Synth *s, OscCfg *carrier_cfg, OscCfg *modulator_cfg)
{
    
    if (!carrier_cfg) return;
    int carrier_i = carrier_cfg - s->base_oscs;
    if (!modulator_cfg) {
	for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	    SynthVoice *v = s->voices + i;
	    for (int j=carrier_i; j<SYNTHVOICE_NUM_OSCS; j+= SYNTH_NUM_BASE_OSCS) {
		Osc *osc = v->oscs + j;
		osc->freq_modulator = NULL;
	    }
	}
    } else {
	modulator_cfg->mod_freq_of = carrier_cfg;
	carrier_cfg->freq_mod_by = modulator_cfg;
	int modulator_i = modulator_cfg - s->base_oscs;
	
	for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	    SynthVoice *v = s->voices + i;
	    Osc *modulator = v->oscs + modulator_i;
	    Osc *carrier = v->oscs + carrier_i;
	    carrier->freq_modulator = modulator;
	    /* Don't do detune voices */
	/*     for (int j=carrier_i; j<SYNTHVOICE_NUM_OSCS; j+= SYNTH_NUM_BASE_OSCS) { */
	/* 	Osc *osc = v->oscs + j; */
	/* 	osc->freq_modulator = modulator; */
	/*     } */
	}
    }
}

void synth_feed_midi(Synth *s, PmEvent *events, int num_events, int32_t tl_start, bool send_immediate)
{
    /* fprintf(stderr, "\n\nSYNTH FEED MIDI %d events tl start start: %d send immediate %d\n", num_events, tl_start, send_immediate); */
    /* FIRST: Update synth state from available MIDI data */
    for (int i=0; i<num_events; i++) {
	/* PmEvent e = s->device.buffer[i]; */
	PmEvent e = events[i];
	/* PmTimestamp ts = e.timestamp; */
	uint8_t status = Pm_MessageStatus(e.message);
	uint8_t note_val = Pm_MessageData1(e.message);
	uint8_t velocity = Pm_MessageData2(e.message);
	uint8_t msg_type = status;

	/* fprintf(stderr, "\t\tIN SYNTH msg%d/%d %x, %d, %d (%s)\n", i, s->num_events, status, note_val, velocity, msg_type == 0x80 ? "OFF" : msg_type == 0x90 ? "ON" : "ERROR"); */
	/* if (i == 0) start = e.timestamp; */
	/* int32_t pos_rel = ((double)e.timestamp - start) * (double)session->proj.sample_rate / 1000.0; */
	/* fprintf(stderr, "EVENT %d/%d, timestamp: %d\n", i, num_events, e.timestamp); */
	if (velocity == 0) msg_type = 8;
	if (msg_type == 0x80) {
	    /* HANDLE NOTE OFF */
	    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
		SynthVoice *v = s->voices + i;
		if (!v->available && v->note_val == note_val) {
		    /* v->note_end_rel[0] = 0; */
		    /* v->note_end_rel[1] = 0; */
		    /* fprintf(stderr, "START RELEASE ON VOICE %d\n", i); */
		    int32_t end_rel = send_immediate ? 0 : tl_start - e.timestamp;
		    adsr_start_release(v->amp_env, end_rel);
		    adsr_start_release(v->amp_env + 1, end_rel);
		}
	    }
	} else if (msg_type == 0x90) {
	    bool note_assigned = false;
	    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
		SynthVoice *v = s->voices + i;
		if (v->available) {
		    /* fprintf(stderr, "ASSIGN VOICE %d\n", i); */
		    int32_t start_rel = send_immediate ? 0 : tl_start - e.timestamp;
		    synth_voice_assign_note(v, note_val, velocity, start_rel);
		    note_assigned = true;
		    break;
		}
	    }
	    if (!note_assigned) {
		/* fprintf(stderr, "No voices remaining! Voice stealing %d\n", s->allow_voice_stealing); */
		if (s->allow_voice_stealing) {
		    int32_t oldest_dur = 0;
		    SynthVoice *oldest = NULL;
		    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
			SynthVoice *v = s->voices + i;
			if (!v->available) {
			    int32_t dur = adsr_query_position(v->amp_env);
			    /* fprintf(stderr, "Voice %d dur %d\n", i, dur); */
			    if (dur > oldest_dur) {
				oldest_dur =  dur;
				oldest = v;
			    }
			}
			/* if (v->note_start_rel[0] < note_start_rel) { */
			/*     oldest = v; */
			/*     note_start_rel = v->note_start_rel[0]; */
			/* } */
		    }
		    if (oldest) {
			/* fprintf(stderr, "STOLE VOICE!\n"); */
			int32_t start_rel = send_immediate ? 0 : tl_start - e.timestamp;
			synth_voice_assign_note(oldest, note_val, velocity, start_rel);
			note_assigned = true;
		    }
		}
	    }
	}
    }
}

void synth_add_buf(Synth *s, float *buf, int channel, int32_t len, float step)
{
    /* if (channel != 0) return; */
    /* fprintf(stderr, "ADD BUF\n"); */
    if (step < 0.0) step *= -1;
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	SynthVoice *v = s->voices + i;
	synth_voice_add_buf(v, buf, len, channel, step);
    }    
}


void synth_close_all_notes(Synth *s)
{
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	SynthVoice *v = s->voices + i;
	if (!v->available) {
	    adsr_start_release(v->amp_env, 0);
	    adsr_start_release(v->amp_env + 1, 0);
	}
    }
}
