#include <portmidi.h>
#include <porttime.h>
#include "adsr.h"
#include "dsp_utils.h"
#include "endpoint.h"
#include "endpoint_callbacks.h"
#include "consts.h"
#include "iir.h"
#include "synth.h"
#include "session.h"

extern double MTOF[];

void synth_osc_vol_dsp_cb(Endpoint *ep)
{
    OscCfg *cfg = ep->xarg1;
    float vol = endpoint_safe_read(ep, NULL).float_v;
    fprintf(stderr, "DSP CB vol? %f\n", vol);
    if (vol > 1e-9) cfg->active = true;
    else cfg->active = false;
}

Synth *synth_create(Track *track)
{
    /* Session *session = session_get(); */
    Synth *s = calloc(1, sizeof(Synth));
    s->track = track;

    s->base_oscs[0].active = true;
    s->base_oscs[0].amp = 0.2;
    for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) {
	s->base_oscs[i].type = WS_SAW;
	if (i==1) {
	    s->base_oscs[i].type = WS_SQUARE;
	}
	if (i==2)
	    s->base_oscs[i].type = WS_SINE;
    }
    
    /* synth_base_osc_set_freq_modulator(s, s->base_oscs + 0, s->base_oscs + 1); */
    /* synth_base_osc_set_freq_modulator(s, s->base_oscs + 2, s->base_oscs + 3); */

    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	iir_init(&s->voices[i].filter, 2, 2);
	s->voices[i].synth = s;
	s->voices[i].available = true;
	s->voices[i].amp_env[0].params = &s->amp_env;
	s->voices[i].amp_env[1].params = &s->amp_env;
    }
    adsr_set_params(
	&s->amp_env,
	96 * 1,
	96 * 400,
	0.2,
	96 * 500,
	2.0);
    s->monitor = true;
    s->allow_voice_stealing = true;

    /* endpoint_init( */
    /* 	&s->vol_ep, */
    /* 	&s->, */


    static char synth_osc_names[SYNTH_NUM_BASE_OSCS][6];
    
    for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) {
	OscCfg *cfg = s->base_oscs + i;
	cfg->pan = 0.5;

	const char *fmt = "%dactive";
	int len = strlen(fmt);
	cfg->active_id = malloc(len);
	snprintf(cfg->active_id, len, fmt, i+1);

	fmt = "%dtype";
	len = strlen(fmt);
	cfg->type_id = malloc(len);
	snprintf(cfg->type_id, len, fmt, i+1);

	fmt = "%dvol";
	len = strlen(fmt);
	cfg->amp_id = malloc(len);
	snprintf(cfg->amp_id, len, fmt, i+1);

	fmt = "%dpan";
	len = strlen(fmt);
	cfg->pan_id = malloc(len);
	snprintf(cfg->pan_id, len, fmt, i+1);

	fmt = "%doctave";
	len = strlen(fmt);
	cfg->octave_id = malloc(len);
	snprintf(cfg->octave_id, len, fmt, i+1);

	fmt = "%dtune_coarse";
	len = strlen(fmt);
	cfg->tune_coarse_id = malloc(len);
	snprintf(cfg->tune_coarse_id, len, fmt, i+1);

	fmt = "%dtune_fine";
	len = strlen(fmt);
	cfg->tune_fine_id = malloc(len);
	snprintf(cfg->tune_fine_id, len, fmt, i+1);

	fmt = "%dnum_voices";
	len = strlen(fmt);
	cfg->unison.num_voices_id = malloc(len);
	snprintf(cfg->unison.num_voices_id, len, fmt, i+1);

	fmt = "%ddetune_cents";
	len = strlen(fmt);
	cfg->unison.detune_cents_id = malloc(len);
	snprintf(cfg->unison.detune_cents_id, len, fmt, i+1);

	fmt = "%drelative_amp";
	len = strlen(fmt);
	cfg->unison.relative_amp_id = malloc(len);
	snprintf(cfg->unison.relative_amp_id, len, fmt, i+1);

	fmt = "%dstereo_spread";
	len = strlen(fmt);
	cfg->unison.stereo_spread_id = malloc(len);
	snprintf(cfg->unison.stereo_spread_id, len, fmt, i+1);
	
	snprintf(synth_osc_names[i], 6, "Osc %d", i+1);
	api_node_register(&cfg->api_node, &track->api_node, synth_osc_names[i]);
	endpoint_init(
	    &cfg->active_ep,
	    &cfg->active,
	    JDAW_BOOL,
	    "active",
	    "Active",
	    JDAW_THREAD_DSP,
	    page_el_gui_cb, NULL, NULL,
	    NULL, NULL, &s->osc_page, cfg->active_id);
	bool default_val = i==0 ? true : false;
	endpoint_set_default_value(
	    &cfg->active_ep,
	    (Value){.bool_v = default_val});
	api_endpoint_register(&cfg->active_ep, &cfg->api_node);

	endpoint_init(
	    &cfg->type_ep,
	    &cfg->type,
	    JDAW_INT,
	    "type",
	    "Type",
	    JDAW_THREAD_DSP,
	    NULL, NULL, NULL,
	    NULL, NULL, NULL, NULL);
	endpoint_set_default_value(&cfg->type_ep, (Value){.int_v = 0});
	endpoint_set_allowed_range(&cfg->type_ep, (Value){.int_v = 0}, (Value){.int_v = 3});
	api_endpoint_register(&cfg->type_ep, &cfg->api_node);


	endpoint_init(
	    &cfg->amp_ep,
	    &cfg->amp,
	    JDAW_FLOAT,
	    "vol",
	    "Vol",
	    JDAW_THREAD_DSP,
	    page_el_gui_cb, NULL, synth_osc_vol_dsp_cb,
	    cfg, NULL, &s->osc_page, cfg->amp_id);
	endpoint_set_default_value(&cfg->amp_ep, (Value){.float_v = 0.5f});
	endpoint_set_allowed_range(&cfg->amp_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 1.0f});
	api_endpoint_register(&cfg->amp_ep, &cfg->api_node);

	endpoint_init(
	    &cfg->pan_ep,
	    &cfg->pan,
	    JDAW_FLOAT,
	    "pan",
	    "Pan",
	    JDAW_THREAD_DSP,
	    page_el_gui_cb, NULL, NULL,
	    NULL, NULL, &s->osc_page, cfg->pan_id);
	endpoint_set_default_value(&cfg->pan_ep, (Value){.float_v = 0.5f});
	endpoint_set_allowed_range(&cfg->pan_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 1.0f});
	api_endpoint_register(&cfg->pan_ep, &cfg->api_node);

	endpoint_init(
	    &cfg->octave_ep,
	    &cfg->octave,
	    JDAW_INT,
	    "octave",
	    "Octave",
	    JDAW_THREAD_DSP,
	    page_el_gui_cb, NULL, NULL,
	    NULL, NULL, &s->osc_page, cfg->octave_id);
	endpoint_set_default_value(&cfg->octave_ep, (Value){.int_v = 0});
	endpoint_set_allowed_range(&cfg->octave_ep, (Value){.int_v = -8}, (Value){.int_v = 8});
	api_endpoint_register(&cfg->octave_ep, &cfg->api_node);

	endpoint_init(
	    &cfg->tune_coarse_ep,
	    &cfg->tune_coarse,
	    JDAW_INT,
	    "tune_coarse",
	    "Coarse tune",
	    JDAW_THREAD_DSP,
	    page_el_gui_cb, NULL, NULL,
	    NULL, NULL, &s->osc_page, cfg->tune_coarse_id);
	endpoint_set_default_value(&cfg->tune_coarse_ep, (Value){.int_v = 0});
	endpoint_set_allowed_range(&cfg->tune_coarse_ep, (Value){.int_v = -11}, (Value){.int_v = 11});
	api_endpoint_register(&cfg->tune_coarse_ep, &cfg->api_node);
	
	endpoint_init(
	    &cfg->tune_fine_ep,
	    &cfg->tune_fine,
	    JDAW_FLOAT,
	    "tune_fine",
	    "Fine tune",
	    JDAW_THREAD_DSP,
	    page_el_gui_cb, NULL, NULL,
	    NULL, NULL, &s->osc_page, cfg->tune_fine_id);
	endpoint_set_default_value(&cfg->tune_fine_ep, (Value){.float_v = 0.0f});
	endpoint_set_allowed_range(&cfg->tune_fine_ep, (Value){.float_v = -100.0f}, (Value){.float_v = 100.0f});
	api_endpoint_register(&cfg->tune_fine_ep, &cfg->api_node);

	endpoint_init(
	    &cfg->unison.num_voices_ep,
	    &cfg->unison.num_voices,
	    JDAW_INT,
	    "unison_num_voices",
	    "Num unison voices",
	    JDAW_THREAD_DSP,
	    page_el_gui_cb, NULL, NULL,
	    NULL, NULL, &s->osc_page, cfg->unison.num_voices_id);
	endpoint_set_allowed_range(&cfg->unison.num_voices_ep, (Value){.int_v = 0}, (Value){.int_v = 4});
	api_endpoint_register(&cfg->unison.num_voices_ep, &cfg->api_node);

	endpoint_init(
	    &cfg->unison.detune_cents_ep,
	    &cfg->unison.detune_cents,
	    JDAW_FLOAT,
	    "unison_detune_cents",
	    "Unison detune cents",
	    JDAW_THREAD_DSP,
	    page_el_gui_cb, NULL, NULL,
	    NULL, NULL, &s->osc_page, cfg->unison.detune_cents_id);
	endpoint_set_allowed_range(&cfg->unison.detune_cents_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 50.0f});
	endpoint_set_default_value(&cfg->unison.detune_cents_ep, (Value){.float_v = 5.0f});
	api_endpoint_register(&cfg->unison.detune_cents_ep, &cfg->api_node);

	endpoint_init(
	    &cfg->unison.relative_amp_ep,
	    &cfg->unison.relative_amp,
	    JDAW_FLOAT,
	    "unison_relative_amp",
	    "Unison relative amp",
	    JDAW_THREAD_DSP,
	    page_el_gui_cb, NULL, NULL,
	    NULL, NULL, &s->osc_page, cfg->unison.relative_amp_id);
	endpoint_set_allowed_range(&cfg->unison.relative_amp_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 1.0f});
	endpoint_set_default_value(&cfg->unison.relative_amp_ep, (Value){.float_v = 0.4f});
	api_endpoint_register(&cfg->unison.relative_amp_ep, &cfg->api_node);

	endpoint_init(
	    &cfg->unison.stereo_spread_ep,
	    &cfg->unison.stereo_spread,
	    JDAW_FLOAT,
	    "unison_stereo_spread",
	    "Unison stereo spread",
	    JDAW_THREAD_DSP,
	    page_el_gui_cb, NULL, NULL,
	    NULL, NULL, &s->osc_page, cfg->unison.stereo_spread_id);
	endpoint_set_allowed_range(&cfg->unison.stereo_spread_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 2.0f});
	endpoint_set_default_value(&cfg->unison.stereo_spread_ep, (Value){.float_v = 0.4f});
	api_endpoint_register(&cfg->unison.stereo_spread_ep, &cfg->api_node);







	
	    
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


/* From Paul Batchelor, https://pbat.ch/sndkit/blep/ */
static float polyblep(float incr, float phase)
{
    if (phase < incr) {
        phase /= incr;
        return phase + phase - phase * phase - 1.0;
    } else if(phase > 1.0 - incr) {
        phase = (phase - 1.0) / incr;
        return phase * phase + phase + phase + 1.0;
    }

    return 0.0;
}

bool do_blep = true;
static float osc_sample(Osc *osc, int channel, int num_channels, float step)
{
    float sample;
    switch(osc->type) {
    case WS_SINE:
	sample = sin(TAU * osc->phase[channel]);
	break;
    case WS_SQUARE:
	sample = osc->phase[channel] < 0.5 ? 1.0 : -1.0;
	if (do_blep) {
	    sample += polyblep(osc->sample_phase_incr, osc->phase[channel]);
	    sample -= polyblep(osc->sample_phase_incr, fmod(osc->phase[channel] + 0.5, 1.0)); 
	}
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
	if (do_blep) {
	    sample -= polyblep(osc->sample_phase_incr, osc->phase[channel]);
	}
	break;
    default:
	sample = 0.0;
    }
    if (osc->freq_modulator) {
	float fmod_sample = osc_sample(osc->freq_modulator, channel, num_channels, step);
	osc->phase[channel] += osc->sample_phase_incr * (step * (1.0f + fmod_sample));
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
    float osc_buf[len];
    memset(osc_buf, '\0', len * sizeof(float));
    /* fprintf(stderr, "\tvoice %ld has data\n", v - v->synth->voices); */
    for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) {
	OscCfg *cfg = v->synth->base_oscs + i;
	if (!cfg->active) continue;
	if (cfg->mod_freq_of || cfg->mod_amp_of) continue;
	/* fprintf(stderr, "\t\tvoice %ld osc %d\n", v - v->synth->voices, i); */
	Osc *osc = v->oscs + i;
	for (int i=0; i<len; i++) {
	    osc_buf[i] += osc_sample(osc, channel, 2, step);
	}
	for (int j=0; j<cfg->unison.num_voices; j++) {
	    Osc *detune_voice = v->oscs + i + SYNTH_NUM_BASE_OSCS * (j + 1);
	    for (int i=0; i<len; i++) {
		/* fprintf(stderr, "adding detune voice %d/%d, freq %f\n", j, cfg->detune.num_voices, detune_voice->freq); */
		osc_buf[i] += osc_sample(detune_voice, channel, 2, step);
	    }
	}
	/* for (int j=0; j< */
	
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

    float amp_env[len];
    enum adsr_stage amp_stage = adsr_get_chunk(&v->amp_env[channel], amp_env, len);
    float_buf_mult(osc_buf, amp_env, len);
    if (amp_stage == ADSR_OVERRUN) {
	v->available = true;
	/* fprintf(stderr, "\tFREEING VOICE %ld (env overrun)\n", v - v->synth->voices); */
	/* return; */
    }
    Session *session = session_get();
    IIRFilter *f = &v->filter;
    for (int i=0; i<len; i++) {
	if (i%10 == 0) {
	    double freq = mtof_calc(v->note_val) * 20 * amp_env[i] * amp_env[i] * amp_env[i] / session->proj.sample_rate;
	    if (freq > 1e-3) {
		iir_set_coeffs_lowpass_chebyshev(f, freq, 4.0);
	    }
	    /* iir_set_coeffs_lowpass_chebyshev(f, (mtof_calc(v->note_val) * 20 * amp_env[i] * amp_env[i] * amp_env[i]) / session->proj.sample_rate / 2.0f, 4.0); */

	}
	osc_buf[i] = iir_sample(f, osc_buf[i], channel);
	buf[i] += osc_buf[i] * (float)v->velocity / 127.0f;
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
    iir_clear(&v->filter);
    /* memset(v->filter.memIn[0], '\0', sizeof(float) * v->filter.degree); */
    /* memset(v->filter.memIn[1], '\0', sizeof(float) * v->filter.degree); */
    /* memset(v->filter.memOut[0], '\0', sizeof(float) * v->filter.degree); */
    /* memset(v->filter.memOut[1], '\0', sizeof(float) * v->filter.degree); */
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
	/* fprintf(stderr, "BASE NOTE: %f\n", base_midi_note); */
	for (int j=0; j<cfg->unison.num_voices; j++) {
	    Osc *detune_voice = v->oscs + i + SYNTH_NUM_BASE_OSCS * (j + 1);
	    detune_voice->type = cfg->type;
	    double detune_midi_note =
		j % 2 == 0 ?
		base_midi_note - cfg->unison.detune_cents / 100.0f * (j + 1) :
		base_midi_note + cfg->unison.detune_cents / 100.0f * (j + 1);
	    /* fprintf(stderr, "DETUNE MIDI NOTE: %f (detune cents? %f /100? %f\n", detune_midi_note, cfg->unison.detune_cents, cfg->unison.detune_cents / 100.0f); */
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

int synth_set_freq_mod_pair(Synth *s, OscCfg *carrier_cfg, OscCfg *modulator_cfg)
{
    if (!modulator_cfg) {
	fprintf(stderr, "Error: synth_set_freq_mod_pair: modulator_cfg arg is mandatory\n");
	return -1;
	
    }
    int modulator_i = modulator_cfg - s->base_oscs;
    
    if (!carrier_cfg) {
	modulator_cfg->mod_freq_of = NULL;
	for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) {
	    OscCfg *cfg = s->base_oscs + i;
	    if (cfg->freq_mod_by == modulator_cfg) {
		/* cfg is old carrier */
		cfg->freq_mod_by = NULL;
		for (int i=0; i<SYNTH_NUM_VOICES; i++) {
		    SynthVoice *v = s->voices + i;
		    for (int j=cfg-s->base_oscs; j<SYNTHVOICE_NUM_OSCS; j+= SYNTH_NUM_BASE_OSCS) {
			Osc *osc = v->oscs + j;
			if (osc->freq_modulator) {
			    osc->freq_modulator = NULL;
			}
		    }
		}

	    }
	}
	return 1;
    } else if (carrier_cfg->freq_mod_by) {
        status_set_errstr("Modulator already assigned to that osc");
	return -1;
    } else {
	OscCfg *loop_detect = carrier_cfg;
	while (loop_detect->mod_freq_of) {
	    if (loop_detect->mod_freq_of == modulator_cfg) {
		fprintf(stderr, "ERROR! loop detected!\n");
		status_set_errstr("Invalid: modulation loop detected");
		synth_set_freq_mod_pair(s, NULL, modulator_cfg);
		return -2;
	    }
	    loop_detect = loop_detect->mod_freq_of;
	}
    }
    
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
    return 0;
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
    for (int i=0; i<len; i++) {
	buf[i] = tanh(buf[i]);
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
