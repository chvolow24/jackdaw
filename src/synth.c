#include <portmidi.h>
#include <porttime.h>
#include "adsr.h"
#include "api.h"
#include "dot_jdaw.h"
#include "dsp_utils.h"
#include "effect.h"
#include "endpoint.h"
#include "endpoint_callbacks.h"
#include "errno.h"
#include "consts.h"
#include "iir.h"
/* #include "test.h" */
/* #include "modal.h" */
#include "label.h"
#include "log.h"
#include "synth.h"
#include "session.h"
#include "tmp.h"
#include "user_event.h"

extern double MTOF[];

static void synth_osc_vol_dsp_cb(Endpoint *ep)
{
    OscCfg *cfg = ep->xarg1;
    if (cfg->amp > 1e-9) cfg->active = true;
    else cfg->active = false;
}

static void base_cutoff_dsp_cb(Endpoint *ep)
{
    Synth *s = ep->xarg1;
    Value cutoff = ep->current_write_val;
    s->base_cutoff = dsp_scale_freq(cutoff.float_v);
}

static void fixed_freq_dsp_cb(Endpoint *ep)
{
    OscCfg *cfg = ep->xarg1;
    float freq_unscaled = ep->current_write_val.float_v;
    cfg->fixed_freq = dsp_scale_freq(freq_unscaled);
}

/* static void num_voices_cb(Endpoint *ep) */
/* { */
/*     int num_voices = ep->current_write_val.int_v; */

/*     Synth *synth = ep->xarg1; */
/*     OscCfg *cfg = ep->xarg2; */
/*     int osc_i = cfg - synth->base_oscs; */

/*     int unison_i = 0; */
/*     while (osc_i < SYNTHVOICE_NUM_OSCS) { */
/* 	for (int i=0; i<SYNTH_NUM_VOICES; i++) { */
/* 	    SynthVoice *v = synth->voices + i; */
/* 	    if (unison_i < num_voices) { */
/* 		v->oscs[osc_i].active = true; */
/* 	    } else { */
/* 		v->oscs[osc_i].active = false; */
/* 	    } */
/* 	} */
/* 	osc_i += SYNTH_NUM_BASE_OSCS; */
/* 	unison_i++; */
/*     } */    
/* } */

static void type_cb(Endpoint *ep)
{
    Synth *synth = ep->xarg1;
    OscCfg *cfg = ep->xarg2;
    int osc_i = cfg - synth->base_oscs;
    int type_i = ep->current_write_val.int_v;   
    while (osc_i < SYNTHVOICE_NUM_OSCS) {
	for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	    SynthVoice *v = synth->voices + i;
	    v->oscs[osc_i].type = type_i;
	}
	osc_i += SYNTH_NUM_BASE_OSCS;
    } 
}

static void unison_stereo_spread_dsp_cb(Endpoint *ep)
{
    Synth *synth = ep->xarg1;
    OscCfg *cfg = ep->xarg2;
    int osc_i = cfg - synth->base_oscs + SYNTH_NUM_BASE_OSCS;
    float max_offset = ep->current_write_val.float_v / 2.0;
    int unison_i = 0;
    int divisor = 1;
    while (osc_i < SYNTHVOICE_NUM_OSCS) {
	float offset = max_offset / divisor;
	if (unison_i % 2 != 0) {
	    offset *= -1;
	    divisor++;
	}
	/* fprintf(stderr, "OSC I %d, unison %d, offset %f\n", osc_i, unison_i, offset); */
	for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	    SynthVoice *v = synth->voices + i;
	    v->oscs[osc_i].pan_offset = offset;
	}
	osc_i += SYNTH_NUM_BASE_OSCS;
	unison_i++;
	/* divisor+=2; */
    } 
    /* while (osc_i < SYNTHVOICE_NUM_OSCS) { */
    /* 	for (int i=0; i<SYNTH_NUM_VOICES; i++) { */
    /* 	    SynthVoice *v = synth->voices + i; */
    /* 	    float offset = max_offset / divisor; */
    /* 	    if (unison_i % 2 != 0) { */
    /* 		offset *= -1; */
    /* 		if (i==0) */
    /* 		    divisor++; */
    /* 	    } */
    /* 	    v->oscs[osc_i].pan_offset = offset; */
    /* 	} */
    /* 	osc_i += SYNTH_NUM_BASE_OSCS; */
    /* 	unison_i++; */
    /* 	/\* divisor+=2; *\/ */
    /* }  */
}

static void detune_cents_dsp_cb(Endpoint *ep)
{
    Synth *synth = ep->xarg1;
    OscCfg *cfg = ep->xarg2;
    int osc_i = cfg - synth->base_oscs + SYNTH_NUM_BASE_OSCS;
    float max_offset_cents = ep->current_write_val.float_v;
    /* float voice_offset = max_offset_cents / (1 + cfg->unison.num_voices) * 2; */
    
    int unison_i = 0;
    int divisor = 1;
    /* int sign = -1; */
    int pair_i = 0;
    int pair_item = 0;
    while (osc_i < SYNTHVOICE_NUM_OSCS) {

	if (unison_i != 0) {
	    if (unison_i % 2 == 0) {
		pair_i++;
		pair_item = 0;
		divisor++;
	    }
	}
	float offset = max_offset_cents / divisor;

	if (pair_i % 2 == 0 && pair_item == 0) {
	    offset *= -1;
	} else if (pair_i % 2 != 0 && pair_item == 1) {
	    offset *= -1;
	}

	/* fprintf(stderr, "OSC I: %d, unison I: %d; pair i: %d item %d: offset %f\n", osc_i, unison_i, pair_i, pair_item, offset); */
	for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	    SynthVoice *v = synth->voices + i;
	    v->oscs[osc_i].detune_cents = offset;
	}
	pair_item++;

	osc_i += SYNTH_NUM_BASE_OSCS;
	unison_i++;


    }
    
    /* while (osc_i < SYNTHVOICE_NUM_OSCS) { */
    /* 	for (int i=0; i<SYNTH_NUM_VOICES; i++) { */
    /* 	    SynthVoice *v = synth->voices + i; */

    /* 	    if (i==0 && unison_i != 0) { */
    /* 		if (unison_i % 2 == 0) { */
    /* 		    pair_i++; */
    /* 		    pair_item = 0; */
    /* 		    divisor++; */
    /* 		} */
    /* 	    } */
    /* 	    float offset = max_offset_cents / divisor; */

    /* 	    if (pair_i % 2 == 0 && pair_item == 0) { */
    /* 		offset *= -1; */
    /* 	    } else if (pair_i % 2 != 0 && pair_item == 1) { */
    /* 		offset *= -1; */
    /* 	    } */
    /* 	    if (i ==0) { */
    /* 		pair_item++; */
    /* 	    } */
	    
    /* 	    v->oscs[osc_i].detune_cents = offset; */
    /* 	    fprintf(stderr, "VOICE %d, pair i %d, offset? %f\n", i, pair_i, offset); */
    /* 	} */
    /* 	osc_i += SYNTH_NUM_BASE_OSCS; */
    /* 	unison_i++; */
    /* } */
}

/* static void unison_vol_cb(Endpoint *ep) */
/* { */
/*     Synth *s = ep->xarg1; */
/*     OscCfg *cfg = ep->xarg2; */
/*     int osc_i = cfg - s->base_oscs; */
/*     while (osc_i < SYNTHVOICE_NUM_OSCS) { */
/* 	for (int i=0; i<SYNTH_NUM_VOICES; i++) { */
/* 	    SynthVoice *v = s->voices + i; */
/* 	    /\* v->unison_vol =  *\/ */
/* 	} */
/* 	osc_i += SYNTH_NUM_BASE_OSCS; */
/* 	unison_i++; */
/*     } */
/* } */

static void tuning_cb(Endpoint *ep)
{
    Synth *s = ep->xarg1;
    OscCfg *cfg = ep->xarg2;
    int octave = endpoint_safe_read(&cfg->octave_ep, NULL).int_v;
    int coarse = endpoint_safe_read(&cfg->tune_coarse_ep, NULL).int_v;
    float fine = endpoint_safe_read(&cfg->tune_fine_ep, NULL).float_v;
    float tune_cents = octave * 1200 + coarse * 100 + fine;
    /* fprintf(stderr, "TUNE CENTS: %f\n", tune_cents); */
    int osc_i = cfg - s->base_oscs;
    while (osc_i < SYNTHVOICE_NUM_OSCS) {
	for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	    SynthVoice *v = s->voices + i;
	    v->oscs[osc_i].tune_cents = tune_cents;
	}
	osc_i += SYNTH_NUM_BASE_OSCS;
    }
}

extern Window *main_win;

static void fmod_target_dsp_cb(Endpoint *ep)
{
    OscCfg *cfg = ep->xarg1;
    Synth *synth = ep->xarg2;
    int self = cfg - synth->base_oscs;
    int target = endpoint_safe_read(ep, NULL).int_v - 1;
    if (self < 0 || self > 5) {
	fprintf(stderr, "Error: osc cfg does not belong to listed synth (index %d)\n", self);
	return;
    }
    if (self == target) {
	return;
    }
    int ret;
    if (target < 0) {
	ret = synth_set_freq_mod_pair(synth, NULL, cfg);
    } else {
	ret = synth_set_freq_mod_pair(synth, synth->base_oscs + target, cfg);
    }
    if (ret == 0) {
	long modulator_i = cfg - synth->base_oscs;
	long carrier_i = target;
	if (modulator_i < carrier_i) {
	    cfg->fmod_dropdown_reset = carrier_i;
	} else {
	    cfg->fmod_dropdown_reset = carrier_i + 1;
	}
	if (cfg->fmod_dropdown_reset > 3) {
	    TESTBREAK;
	}
    } else if (ret == 1) { /* carrier cfg is null */
	cfg->fmod_dropdown_reset = 0;
    }
}

static void amod_target_dsp_cb(Endpoint *ep)
{
    OscCfg *cfg = ep->xarg1;
    Synth *synth = ep->xarg2;
    int self = cfg - synth->base_oscs;
    int target = endpoint_safe_read(ep, NULL).int_v - 1;
    if (self < 0 || self > 5) {
	fprintf(stderr, "Error: osc cfg does not belong to listed synth (index %d)\n", self);
	return;
    }
    if (self == target) {
	return;
    }
    int ret;
    if (target < 0) {
	ret = synth_set_amp_mod_pair(synth, NULL, cfg);
    } else {
	ret = synth_set_amp_mod_pair(synth, synth->base_oscs + target, cfg);
    }
    if (ret == 0) {
	long modulator_i = cfg - synth->base_oscs;
	long carrier_i = target;
	if (modulator_i < carrier_i) {
	    cfg->amod_dropdown_reset = carrier_i;
	} else {
	    cfg->amod_dropdown_reset = carrier_i + 1;
	}
    } else if (ret == 1) { /* carrier cfg is null */
	cfg->amod_dropdown_reset = 0;
    }
}

#define PORTAMENTO_EXP 1.5
static int portamento_scale(int unscaled)
{
    return round(pow((double)unscaled, PORTAMENTO_EXP));
}

static int portamento_unscale(int scaled)
{
    return round(pow((double)scaled, 1.0 / PORTAMENTO_EXP));
}

/* typedef void (*LabelStrFn)(char *dst, size_t dstsize, Value val, ValType type); */
static void portamento_labelfn(char *dst, size_t dstsize, Value val, ValType type)
{
    int unscaled = val.int_v;
    int scaled = portamento_scale(unscaled);
    snprintf(dst, dstsize, "%d ms", scaled);
}

static void portamento_len_dsp_cb(Endpoint *ep)
{
    Synth *synth = ep->xarg1;
    int unscaled = ep->current_write_val.int_v;
    synth->portamento_len_msec = portamento_scale(unscaled);
}

Synth *synth_create(Track *track)
{
    Synth *s = calloc(1, sizeof(Synth));

    snprintf(s->preset_name, MAX_NAMELENGTH, "preset.jsynth");
    s->track = track;

    s->vol = 1.0;
    s->pan = 0.5;
    s->sync_phase = true;
    s->noise_amt = 0.0f;
    s->noise_apply_env = false;
    
    s->base_oscs[0].active = true;
    /* s->base_oscs[0].amp_unscaled = 0.5; */
    s->base_oscs[0].amp = 0.25;
    s->base_oscs[0].type = WS_SAW;

    /* s->base_oscs[0].fix_freq = true; */
    /* s->base_oscs[0].fixed_freq = 0.1; */

    s->filter_active = false;
    s->base_cutoff_unscaled = 0.1f;
    s->base_cutoff = dsp_scale_freq(s->base_cutoff_unscaled);
    s->pitch_amt = 2.0f;
    s->vel_amt = 0.75f;
    s->env_amt = 5.0f;
    s->resonance = 2.0;

    /* double dc_block_coeff = exp(-1.0/(0.0025 * session_get_sample_rate())); */
    double dc_block_coeff = exp(-1.0/(0.002 * session_get_sample_rate()));
    double DC_BLOCK_A[] = {1.0, -1.0};
    double DC_BLOCK_B[] = {dc_block_coeff};
    iir_init(&s->dc_blocker, 1, 2);
    iir_set_coeffs(&s->dc_blocker, DC_BLOCK_A, DC_BLOCK_B);
    for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) {
	OscCfg *cfg = s->base_oscs + i;
	cfg->unison.detune_cents = 10;
	cfg->unison.stereo_spread = 0.1;
    }
	
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	SynthVoice *v = s->voices + i;
	adsr_params_add_follower(&s->amp_env, &v->amp_env[0]);
	adsr_params_add_follower(&s->amp_env, &v->amp_env[1]);
	
	adsr_params_add_follower(&s->filter_env, &v->filter_env[0]);
	adsr_params_add_follower(&s->filter_env, &v->filter_env[1]);
	
	adsr_params_add_follower(&s->noise_amt_env, &v->noise_amt_env[0]);
	adsr_params_add_follower(&s->noise_amt_env, &v->noise_amt_env[1]);

	iir_init(&v->filter, 2, 2);
	v->synth = s;
	v->available = true;

	for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) {
	    OscCfg *cfg = s->base_oscs + i;
	    for (int j=0; j<SYNTHVOICE_NUM_OSCS; j+= SYNTH_NUM_BASE_OSCS) {
		Osc *osc = v->oscs + i + j;
		osc->cfg = cfg;
		osc->voice = v;
		iir_init(&osc->tri_dc_blocker, 1, 1);
		iir_set_coeffs(&osc->tri_dc_blocker, DC_BLOCK_A, DC_BLOCK_B);

	    }
	}
	/* v->amp_env[0].params = &s->amp_env; */
	/* v->amp_env[1].params = &s->amp_env; */
    }
    s->amp_env.a_msec = 4;
    s->amp_env.d_msec = 200;
    s->amp_env.s_ep_targ = 0.7;
    s->amp_env.r_msec = 20;
    
    s->filter_env.a_msec = 4;
    s->filter_env.d_msec = 200;
    s->filter_env.s_ep_targ = 0.4;
    s->filter_env.r_msec = 400;

    s->noise_amt_env.a_msec = 4;
    s->noise_amt_env.d_msec = 200;
    s->noise_amt_env.s_ep_targ = 0.4;
    s->noise_amt_env.r_msec = 400;
    
    adsr_set_params(
	&s->amp_env,
	session_get_sample_rate() * s->amp_env.a_msec / 1000,
	session_get_sample_rate() * s->amp_env.d_msec / 1000,
	s->amp_env.s_ep_targ,
	session_get_sample_rate() * s->amp_env.r_msec / 1000,
	2.0);
    adsr_set_params(
	&s->filter_env,
	session_get_sample_rate() * s->filter_env.a_msec / 1000,
	session_get_sample_rate() * s->filter_env.d_msec / 1000,
	s->filter_env.s_ep_targ,
	session_get_sample_rate() * s->filter_env.r_msec / 1000,
	2.0);
    adsr_set_params(
	&s->noise_amt_env,
	session_get_sample_rate() * s->noise_amt_env.a_msec / 1000,
	session_get_sample_rate() * s->noise_amt_env.d_msec / 1000,
	s->noise_amt_env.s_ep_targ,
	session_get_sample_rate() * s->noise_amt_env.r_msec / 1000,
	2.0);

    s->monitor = true;

    api_node_register(&s->api_node, &track->api_node, s->preset_name, "Synth");

    endpoint_init(
	&s->vol_ep,
	&s->vol,
	JDAW_FLOAT,
	"vol",
	"Volume",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->osc_page, "master_amp_slider");
    endpoint_set_default_value(&s->vol_ep, (Value){.float_v = 1.0});
    endpoint_set_allowed_range(&s->vol_ep, (Value){.float_v = 0.0}, (Value){.float_v = 3.0});
    api_endpoint_register(&s->vol_ep, &s->api_node);

    endpoint_init(
	&s->pan_ep,
	&s->pan,
	JDAW_FLOAT,
	"pan",
	"Pan",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->osc_page, "master_pan_slider");
    endpoint_set_default_value(&s->pan_ep, (Value){.float_v = 0.5});
    endpoint_set_allowed_range(&s->pan_ep, (Value){.float_v = 0.0}, (Value){.float_v = 1.0});
    endpoint_set_label_fn(&s->pan_ep, label_pan);
    api_endpoint_register(&s->pan_ep, &s->api_node);


    /* endpoint_init( */
    /* 	s->amp_env */

    
    adsr_endpoints_init(&s->amp_env, &s->amp_env_page, &s->api_node, "Amp envelope");
    api_node_register(&s->filter_node, &s->api_node, NULL, "Filter");
    api_node_register(&s->noise_node, &s->api_node, NULL, "Noise");
    adsr_endpoints_init(&s->filter_env, &s->filter_page, &s->filter_node, "Filter envelope");
    adsr_endpoints_init(&s->noise_amt_env, &s->noise_page, &s->noise_node, "Noise envelope");
    /* api_node_register(&s->filter_env.api_node, &s->filter_node, "Filter envelope"); */

    /* adsr_params_init(&s->amp_env); */

    /* s->filter_active = true; */
    endpoint_init(
	&s->filter_active_ep,
	&s->filter_active,
	JDAW_BOOL,
	"filter_active",
	"Filter active",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->filter_page, "filter_active_toggle");
    endpoint_set_default_value(&s->filter_active_ep, (Value){.bool_v = false});
    api_endpoint_register(&s->filter_active_ep, &s->filter_node);


    endpoint_init(
	&s->base_cutoff_ep,
	&s->base_cutoff_unscaled,
	JDAW_FLOAT,
	"base_cutoff",
	"Base cutoff",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, base_cutoff_dsp_cb,
	s, NULL, &s->filter_page, "base_cutoff_slider");
    endpoint_set_default_value(&s->base_cutoff_ep, (Value){.float_v = 0.1f});
    endpoint_set_allowed_range(&s->base_cutoff_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 1.0f});
    endpoint_set_label_fn(&s->base_cutoff_ep, label_freq_raw_to_hz);
    api_endpoint_register(&s->base_cutoff_ep, &s->filter_node);

    
    endpoint_init(
	&s->pitch_amt_ep,
	&s->pitch_amt,
	JDAW_FLOAT,
	"pitch_amt",
	"Pitch amt",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->filter_page, "pitch_amt_slider");
    endpoint_set_default_value(&s->pitch_amt_ep, (Value){.float_v = 5.0f});
    endpoint_set_allowed_range(&s->pitch_amt_ep, (Value){.float_v = 0.00}, (Value){.float_v = 50.0});
    api_endpoint_register(&s->pitch_amt_ep, &s->filter_node);

    endpoint_init(
	&s->vel_amt_ep,
	&s->vel_amt,
	JDAW_FLOAT,
	"vel_amt",
	"Vel amt",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->filter_page, "vel_amt_slider");
    endpoint_set_default_value(&s->vel_amt_ep, (Value){.float_v = 0.75f});
    endpoint_set_allowed_range(&s->vel_amt_ep, (Value){.float_v = 0.00}, (Value){.float_v = 1.0});
    api_endpoint_register(&s->vel_amt_ep, &s->filter_node);

    endpoint_init(
	&s->env_amt_ep,
	&s->env_amt,
	JDAW_FLOAT,
	"env_amt",
	"Pitch amt",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->filter_page, "env_amt_slider");
    endpoint_set_default_value(&s->env_amt_ep, (Value){.float_v = 5.0f});
    endpoint_set_allowed_range(&s->env_amt_ep, (Value){.float_v = 0.00}, (Value){.float_v = 50.0});
    api_endpoint_register(&s->env_amt_ep, &s->filter_node);


    /* endpoint_init( */
    /* 	&s->freq_scalar_ep, */
    /* 	&s->freq_scalar, */
    /* 	JDAW_FLOAT, */
    /* 	"freq_scalar", */
    /* 	"Freq scalar", */
    /* 	JDAW_THREAD_DSP, */
    /* 	page_el_gui_cb, NULL, NULL, */
    /* 	NULL, NULL, &s->filter_page, "freq_scalar_slider"); */
    /* endpoint_set_default_value(&s->freq_scalar_ep, (Value){.float_v = 20.0f}); */
    /* endpoint_set_allowed_range(&s->freq_scalar_ep, (Value){.float_v = 0.1}, (Value){.float_v = 100.0}); */
    /* api_endpoint_register(&s->freq_scalar_ep, &s->filter_node); */

    endpoint_init(
	&s->resonance_ep,
	&s->resonance,
	JDAW_FLOAT,
	"resonance",
	"Resonance",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->filter_page, "resonance_slider");
    endpoint_set_default_value(&s->resonance_ep, (Value){.float_v = 5.0f});
    endpoint_set_allowed_range(&s->resonance_ep, (Value){.float_v = 1.0f}, (Value){.float_v = 15.0f});
    api_endpoint_register(&s->resonance_ep, &s->filter_node);

    endpoint_init(
	&s->sync_phase_ep,
	&s->sync_phase,
	JDAW_BOOL,
	"sync_phase",
	"Sync phase",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->osc_page, "sync_phase_toggle");
    endpoint_set_default_value(&s->sync_phase_ep, (Value){.bool_v = true});
    api_endpoint_register(&s->sync_phase_ep, &s->api_node);

    endpoint_init(
	&s->noise_amt_ep,
	&s->noise_amt,
	JDAW_FLOAT,
	"noise_amt",
	"Noise amount",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->noise_page, "noise_amt_slider");
    endpoint_set_default_value(&s->noise_amt_ep, (Value){.float_v = 0.0f});
    endpoint_set_allowed_range(&s->noise_amt_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 2.0});
    api_endpoint_register(&s->noise_amt_ep, &s->noise_node);

    endpoint_init(
	&s->noise_apply_env_ep,
	&s->noise_apply_env,
	JDAW_BOOL,
	"noise_apply_env",
	"Apply noise envelope",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->noise_page, "noise_apply_env_toggle");
    endpoint_set_default_value(&s->noise_apply_env_ep, (Value){.bool_v = false});
    api_endpoint_register(&s->noise_apply_env_ep, &s->noise_node);


    /* s->velocity_freq_scalar = 0.5; */
    /* endpoint_init( */
    /* 	&s->velocity_freq_scalar_ep, */
    /* 	&s->velocity_freq_scalar, */
    /* 	JDAW_FLOAT, */
    /* 	"velocity_freq_scalar", */
    /* 	"Velocity freq scalar", */
    /* 	JDAW_THREAD_DSP, */
    /* 	page_el_gui_cb, NULL, NULL, */
    /* 	NULL, NULL, &s->filter_page, "velocity_freq_scalar_slider"); */
    /* endpoint_set_default_value(&s->velocity_freq_scalar_ep, (Value){.float_v = 1.0}); */
    /* endpoint_set_allowed_range(&s->velocity_freq_scalar_ep, (Value){.float_v = 0.0}, (Value){.float_v = 1.0}); */
    /* api_endpoint_register(&s->velocity_freq_scalar_ep, &s->filter_node); */

    s->use_amp_env = true;
    endpoint_init(
	&s->use_amp_env_ep,
	&s->use_amp_env,
	JDAW_BOOL,
	"use_amp_env",
	"Use amp env",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->filter_page, "use_amp_env_toggle");
    endpoint_set_default_value(&s->use_amp_env_ep, (Value){.bool_v = true});
    api_endpoint_register(&s->use_amp_env_ep, &s->filter_node);


    s->num_voices = SYNTH_NUM_VOICES;
    endpoint_init(
	&s->num_voices_ep,
	&s->num_voices,
	JDAW_INT,
	"num_voices",
	"Num voices",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->polyphony_page, "num_voices_slider");
    endpoint_set_default_value(&s->num_voices_ep, (Value){.int_v = SYNTH_NUM_VOICES});
    endpoint_set_allowed_range(&s->num_voices_ep, (Value){.int_v = 2}, (Value){.int_v = SYNTH_NUM_VOICES});
    api_endpoint_register(&s->num_voices_ep, &s->api_node);
    
    s->allow_voice_stealing = true;
    endpoint_init(
	&s->allow_voice_stealing_ep,
	&s->allow_voice_stealing,
	JDAW_BOOL,
	"allow_voice_stealing",
	"Allow voice stealing",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->polyphony_page, "allow_voice_stealing_toggle");
    endpoint_set_default_value(&s->allow_voice_stealing_ep, (Value){.bool_v = true});
    api_endpoint_register(&s->allow_voice_stealing_ep, &s->api_node);    
    
    s->mono_mode = false;
    endpoint_init(
	&s->mono_mode_ep,
	&s->mono_mode,
	JDAW_BOOL,
	"mono_mode",
	"Mono mode",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->polyphony_page, "mono_mode_toggle");
    endpoint_set_default_value(&s->mono_mode_ep, (Value){.bool_v=false});
    api_endpoint_register(&s->mono_mode_ep, &s->api_node);

    static const int portamento_default_len_msec = 100;
    
    s->portamento_len_msec = portamento_default_len_msec;
    s->portamento_len_unscaled = portamento_unscale(portamento_default_len_msec);
    endpoint_init(
	&s->portamento_len_msec_ep,
	&s->portamento_len_unscaled,
	JDAW_INT,
	"portamento_len_msec", "Portamento len msec",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, portamento_len_dsp_cb,
	s, NULL, &s->polyphony_page, "portamento_len_slider");
    endpoint_set_default_value(&s->portamento_len_msec_ep, (Value){.int_v = s->portamento_len_unscaled});
    endpoint_set_allowed_range(&s->portamento_len_msec_ep, (Value){.int_v = 0}, (Value){.int_v = portamento_unscale(8000)});
    endpoint_set_label_fn(&s->portamento_len_msec_ep, portamento_labelfn);
    api_endpoint_register(&s->portamento_len_msec_ep, &s->api_node);

       
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

	fmt = "%dfix_freq";
	len = strlen(fmt);
	cfg->fix_freq_id = malloc(len);
	snprintf(cfg->fix_freq_id, len, fmt, i+1);

	fmt = "%dfixed_freq";
	len = strlen(fmt);
	cfg->fixed_freq_id = malloc(len);
	snprintf(cfg->fixed_freq_id, len, fmt, i+1);

	fmt = "%dunison_num_voices";
	len = strlen(fmt);
	cfg->unison.num_voices_id = malloc(len);
	snprintf(cfg->unison.num_voices_id, len, fmt, i+1);

	fmt = "%dunison_detune_cents";
	len = strlen(fmt);
	cfg->unison.detune_cents_id = malloc(len);
	snprintf(cfg->unison.detune_cents_id, len, fmt, i+1);

	fmt = "%dunison_relative_amp";
	len = strlen(fmt);
	cfg->unison.relative_amp_id = malloc(len);
	snprintf(cfg->unison.relative_amp_id, len, fmt, i+1);

	fmt = "%dunison_stereo_spread";
	len = strlen(fmt);
	cfg->unison.stereo_spread_id = malloc(len);
	snprintf(cfg->unison.stereo_spread_id, len, fmt, i+1);

	fmt = "%dfmod_dropdown";
	len = strlen(fmt);
	cfg->fmod_target_dropdown_id = malloc(len);
	snprintf(cfg->fmod_target_dropdown_id, len, fmt, i+1);

	fmt = "%damod_dropdown";
	len = strlen(fmt);
	cfg->amod_target_dropdown_id = malloc(len);
	snprintf(cfg->amod_target_dropdown_id, len, fmt, i+1);

	
	snprintf(synth_osc_names[i], 6, "Osc %d", i+1);
	api_node_register(&cfg->api_node, &s->api_node, NULL, synth_osc_names[i]);
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
	    NULL, NULL, type_cb,
	    s, cfg, NULL, NULL);
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
	endpoint_set_default_value(&cfg->amp_ep, (Value){.float_v = 0.25f});
	endpoint_set_allowed_range(&cfg->amp_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 2.0f});
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
	    &cfg->fix_freq_ep,
	    &cfg->fix_freq,
	    JDAW_BOOL,
	    "fix_freq",
	    "Fix freq",
	    JDAW_THREAD_DSP,
	    page_el_gui_cb, NULL, NULL,
	    NULL, NULL, &s->osc_page, cfg->fix_freq_id);
	endpoint_set_default_value(&cfg->fix_freq_ep, (Value){.bool_v = false});
	api_endpoint_register(&cfg->fix_freq_ep, &cfg->api_node);

	endpoint_init(
	    &cfg->fixed_freq_ep,
	    &cfg->fixed_freq_unscaled,
	    JDAW_FLOAT,
	    "fixed_freq",
	    "Fixed freq",
	    JDAW_THREAD_DSP,
	    page_el_gui_cb, NULL, fixed_freq_dsp_cb,
	    cfg, NULL, &s->osc_page, cfg->fixed_freq_id);
	endpoint_set_default_value(&cfg->fixed_freq_ep, (Value){.float_v = 0.1f});
	endpoint_set_allowed_range(&cfg->fixed_freq_ep, (Value){.float_v = 0.0001f}, (Value){.float_v = 0.8f});
	endpoint_set_label_fn(&cfg->fixed_freq_ep, label_freq_raw_to_hz);
	api_endpoint_register(&cfg->fixed_freq_ep, &cfg->api_node);
	cfg->fixed_freq_unscaled = 0.1f;
	cfg->fixed_freq = dsp_scale_freq(cfg->fixed_freq_unscaled);

	endpoint_init(
	    &cfg->octave_ep,
	    &cfg->octave,
	    JDAW_INT,
	    "octave",
	    "Octave",
	    JDAW_THREAD_DSP,
	    page_el_gui_cb, NULL, tuning_cb,
	    s, cfg, &s->osc_page, cfg->octave_id);
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
	    page_el_gui_cb, NULL, tuning_cb,
	    s, cfg, &s->osc_page, cfg->tune_coarse_id);
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
	    page_el_gui_cb, NULL, tuning_cb,
	    s, cfg, &s->osc_page, cfg->tune_fine_id);
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
	    s, cfg, &s->osc_page, cfg->unison.num_voices_id);
	endpoint_set_allowed_range(&cfg->unison.num_voices_ep, (Value){.int_v = 0}, (Value){.int_v = SYNTH_MAX_UNISON_OSCS - 1});
	api_endpoint_register(&cfg->unison.num_voices_ep, &cfg->api_node);
	endpoint_set_label_fn(&cfg->unison.num_voices_ep, label_int_plus_one);

	endpoint_init(
	    &cfg->unison.detune_cents_ep,
	    &cfg->unison.detune_cents,
	    JDAW_FLOAT,
	    "unison_detune_cents",
	    "Unison detune cents",
	    JDAW_THREAD_DSP,
	    page_el_gui_cb, NULL, detune_cents_dsp_cb,
	    s, cfg, &s->osc_page, cfg->unison.detune_cents_id);
	endpoint_set_allowed_range(&cfg->unison.detune_cents_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 100.0f});
	endpoint_set_default_value(&cfg->unison.detune_cents_ep, (Value){.float_v = 10.0f});
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
	    page_el_gui_cb, NULL, unison_stereo_spread_dsp_cb,
	    s, cfg, &s->osc_page, cfg->unison.stereo_spread_id);
	endpoint_set_allowed_range(&cfg->unison.stereo_spread_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 1.0f});
	endpoint_set_default_value(&cfg->unison.stereo_spread_ep, (Value){.float_v = 0.0f});
	api_endpoint_register(&cfg->unison.stereo_spread_ep, &cfg->api_node);

	endpoint_init(
	    &cfg->fmod_target_ep,
	    &cfg->fmod_target,
	    JDAW_INT,
	    "fmod_target",
	    "Freq mod target",
	    JDAW_THREAD_DSP,
	    page_el_gui_cb, NULL, fmod_target_dsp_cb,
	    cfg, s, &s->osc_page, cfg->fmod_target_dropdown_id);
	endpoint_set_allowed_range(&cfg->fmod_target_ep, (Value){.int_v=0}, (Value){.int_v=5});
	cfg->fmod_target_ep.automatable = false;
	api_endpoint_register(&cfg->fmod_target_ep, &cfg->api_node);
	
	endpoint_init(
	    &cfg->amod_target_ep,
	    &cfg->amod_target,
	    JDAW_INT,
	    "amod_target",
	    "Amp mod target",
	    JDAW_THREAD_DSP,
	    page_el_gui_cb, NULL, amod_target_dsp_cb,
	    cfg, s, &s->osc_page, cfg->amod_target_dropdown_id);
	endpoint_set_allowed_range(&cfg->amod_target_ep, (Value){.int_v=0}, (Value){.int_v=5});
	cfg->amod_target_ep.automatable = false;
	api_endpoint_register(&cfg->amod_target_ep, &cfg->api_node);	    
    }

    int err = pthread_mutex_init(&s->audio_proc_lock, NULL);
    if (err != 0) {
	fprintf(stderr, "Error: unable to init synth audio proc lock: %s\n", strerror(err));
	exit(1);
    }

    effect_chain_init(&s->effect_chain, track->tl->proj, &s->api_node, "synth", track->tl->proj->chunk_size_sframes);
    effect_chain_block_type(&s->effect_chain, EFFECT_FIR_FILTER);
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

static void osc_set_freq(Osc *osc, double freq_hz);
bool do_blep = true;

static void osc_reset_params(Osc *osc, int32_t chunk_len)
{
    OscCfg *cfg = osc->cfg;
    float note;
    if (cfg->fix_freq) {
	/* Lowpass filter to control signal to smooth stairsteps */
	float f_raw = dsp_scale_freq_to_hz(cfg->fixed_freq_unscaled);
	float f = f_raw * 0.01 + osc->freq_last_sample * 0.99;
	note = ftom_calc(f);
	osc->freq_last_sample = f;
    } else {
	if (osc->voice->do_portamento && osc->voice->portamento_len_sframes > 0) {
	    /* float portamento_incr = (float)(osc->voice->note_val - osc->voice->portamento_from) / osc->voice->portamento_len_bufs; */
	    note = (float)osc->voice->portamento_from + ((double)osc->voice->portamento_elapsed_sframes * (osc->voice->note_val - osc->voice->portamento_from) / osc->voice->portamento_len_sframes);
	} else {
	    note = (float)osc->voice->note_val;
	}
    }
    note += (osc->detune_cents + osc->tune_cents) / 100.0f;
    osc_set_freq(osc, mtof_calc(note));
    int unison_index = (osc - osc->voice->oscs) / SYNTH_NUM_BASE_OSCS;
    if (unison_index > 0) { /* UNISON VOL */
	osc->amp = cfg->amp * cfg->unison.relative_amp;
	osc->pan = cfg->pan + osc->pan_offset;
	if (osc->pan > 1.0f) osc->pan = 1.0f;
	if (osc->pan < 0.0f) osc->pan = 0.0f;
    } else {
	osc->amp = cfg->amp;
	osc->pan = cfg->pan;
    }
}

static void osc_get_buf_preamp(Osc *osc, float step, int len, int after)
{
    if (len > MAX_OSC_BUF_LEN) {
	fprintf(stderr, "Error: Synth Oscs cannot support buf size > %d (req size %d)\n", MAX_OSC_BUF_LEN, len);
	#ifdef TESTBUILD
	exit(1);
	#endif
	len = MAX_OSC_BUF_LEN;
    }
    float *fmod_samples;
    float *amod_samples;
    float fmod_amp;
    if (osc->freq_modulator) {
	fmod_samples = osc->freq_modulator->buf;
	osc_get_buf_preamp(osc->freq_modulator, step, len, after);
	fmod_amp = powf(osc->freq_modulator->cfg->amp, 3.0f);
	float_buf_mult_const(fmod_samples, fmod_amp, len);
    }
    if (osc->amp_modulator) {
	amod_samples = osc->amp_modulator->buf;
	osc_get_buf_preamp(osc->amp_modulator, step, len, after);
	float amp = osc->amp_modulator->cfg->amp;
	float_buf_mult_const(amod_samples, amp, len);
	
    }
    double phase_incr = osc->sample_phase_incr + osc->sample_phase_incr_addtl;
    double phase = osc->phase;
    /* int unison_i = (long)((osc - osc->voice->oscs) - (osc->cfg - osc->voice->synth->base_oscs)) / SYNTH_NUM_BASE_OSCS; */
    size_t zeroset_len = after > len ? len * sizeof(int) : after * sizeof(int);
    memset(osc->buf, '\0', zeroset_len);
    bool do_unison_compensation = false;
    float unison_compensation = 1.0f;
    if (osc->cfg->unison.num_voices > 0 && !osc->cfg->mod_freq_of && !osc->cfg->mod_amp_of) {
	unison_compensation =  1.0 / (0.2 * osc->cfg->unison.num_voices * osc->cfg->unison.relative_amp + 1.0);
	do_unison_compensation = true;
    }

    if (phase_incr > 0.4) { /* Nearing nyquist */
	memset(osc->buf, '\0', sizeof(float) * len);
    } else {
	for (int i=after; i<len; i++) {
	    float sample;
	    if (osc->cfg->fix_freq && i % 93 == 0) {
		/* fprintf(stderr, "resetting %d\n", i); */
		osc_reset_params(osc, 0);
	    }
	    /* double phase_incr = osc->sample_phase_incr + osc->sample_phase_incr_addtl; */
	    switch(osc->type) {
	    case WS_SINE:
		sample = sinf(TAU * phase);
		break;
	    case WS_SQUARE:
		sample = phase < 0.5 ? 1.0 : -1.0;
		sample += polyblep(phase_incr, phase);
		sample -= polyblep(phase_incr, fmod(phase + 0.5, 1.0)); 
		break;
	    case WS_TRI:
		if (do_blep) {
		    sample = phase < 0.5 ? 1.0 : -1.0;
		    sample += polyblep(phase_incr, phase);
		    sample -= polyblep(phase_incr, fmod(phase + 0.5, 1.0)); 

		    sample *= 4.0 * phase_incr;
		    sample += osc->last_out_tri;
		    osc->last_out_tri = sample;
		    sample = iir_sample(&osc->tri_dc_blocker, sample, 0);
		} else {
		    sample =
			phase <= 0.25 ?
			phase * 4.0f :
			phase <= 0.5f ?
			(0.5f - phase) * 4.0 :
			phase <= 0.75f ?
			(phase - 0.5f) * -4.0 :
			(1.0f - phase) * -4.0;
		}

		break;
	    case WS_SAW:
		sample = 2.0f * phase - 1.0;
		if (do_blep) {
		    /* BLEP */
		    sample -= polyblep(phase_incr, phase);
		}
		break;
	    default:
		sample = 0.0;
	    }
	    if (osc->freq_modulator) {
		/* Raise fmod sample to 3 to get fmod values in more useful range */
		/* float fmod_sample = pow(osc_sample(osc->freq_modulator, channel, num_channels, step, chunk_index), 3.0); */
		/* fprintf(stderr, "fmod sample in osc %p: %f\n", osc, fmod_sample); */
		phase += phase_incr * step * (1.0 - fmod_samples[i]);
	    } else {
		phase += phase_incr * step;
	    }
	    if (osc->amp_modulator) {
		sample *= 1.0 + amod_samples[i];
		/* sample *= 1.0 + osc_sample(osc->amp_modulator, channel, num_channels, step, chunk_index); */
	    }
	    if (phase > 1.0) {
		phase = phase - floor(phase);
	    } else if (phase < 0.0) {
		phase = 1.0 + phase + ceil(phase);
	    }
	    /* Reverse polarity on some voices:
	       ignore for now */
	    /* if (!osc->cfg->mod_freq_of && !osc->cfg->mod_amp_of && unison_i != 0 && unison_i % 3 == 0) { */
		/* sample *= -1; */
	    /* } */
	    osc->buf[i] = sample;
	}
	if (do_unison_compensation) {
	    float_buf_mult_const(osc->buf + after, unison_compensation, len - after);
	}
	osc->phase = phase;
    }
    /* osc->phase_incr =  */
    /* apply_amp_and_pan: */
    /* 	(void)0; */
    /* 	float bottom_sample; */
    /* 	if (channel == 0) { */
    /* 	    osc->saved_L_out[chunk_index] = sample; */
    /* 	    bottom_sample = sample; */
    /* 	} else { */
    /* 	    bottom_sample = osc->saved_L_out[chunk_index]; */
    /* 	} */
    /* 	return bottom_sample * osc->amp * pan_scale(osc->pan, channel); */
    /* } */
}


/* static float osc_sample(Osc *osc, int channel, int num_channels, float step, int chunk_index) */
/* { */
/*     if (channel != 0) goto apply_amp_and_pan; */
/*     OscCfg *cfg = osc->cfg; */
/*     /\* float amp = cfg->amp; *\/ */
/*     /\* float pan = cfg->pan; *\/ */
/*     if (cfg->fix_freq) { */
/* 	/\* Lowpass filter to control signal to smooth stairsteps *\/ */
/* 	float f_raw = dsp_scale_freq_to_hz(cfg->fixed_freq_unscaled); */
/* 	float f = f_raw * 0.001 + osc->freq_last_sample[channel] * 0.999; */
/* 	osc_set_freq(osc, f); */
/* 	osc->freq_last_sample[channel] = f; */
/*     } else { */
/* 	if (osc->reset_params_ctr[channel] <= 0) { */
/* 	    osc->reset_params_ctr[channel] = 192; /\* 2msec precision for 96kHz *\/ */
/* 	    float note = (float)osc->voice->note_val + (osc->detune_cents + osc->tune_cents) / 100.0f; */
/* 	    osc_set_freq(osc, mtof_calc(note)); */
/* 	    int unison_index = (osc - osc->voice->oscs) / SYNTH_NUM_BASE_OSCS; */
/* 	    if (unison_index > 0) { /\* UNISON VOL *\/ */
/* 		osc->amp = cfg->amp * cfg->unison.relative_amp; */
/* 		osc->pan = cfg->pan + osc->pan_offset; */
/* 		if (osc->pan > 1.0f) osc->pan = 1.0f; */
/* 		if (osc->pan < 0.0f) osc->pan = 0.0f; */
/* 	    } else { */
/* 		osc->amp = cfg->amp; */
/* 		osc->pan = cfg->pan; */
/* 	    } */
/* 	} */
/* 	osc->reset_params_ctr[channel]--; */
/*     } */
/*     float sample; */
/*     double phase_incr = osc->sample_phase_incr + osc->sample_phase_incr_addtl; */
/*     switch(osc->type) { */
/*     case WS_SINE: */
/* 	sample = sin(TAU * osc->phase[channel]); */
/* 	break; */
/*     case WS_SQUARE: */
/* 	sample = osc->phase[channel] < 0.5 ? 1.0 : -1.0; */
/* 	/\* if (do_blep) { *\/ */
/* 	/\* BLEP *\/ */
/* 	sample += polyblep(phase_incr, osc->phase[channel]); */
/* 	sample -= polyblep(phase_incr, fmod(osc->phase[channel] + 0.5, 1.0));  */
/* 	/\* } *\/ */
/* 	break; */
/*     case WS_TRI: */
/* 	sample = */
/* 	    osc->phase[channel] <= 0.25 ? */
/* 	        osc->phase[channel] * 4.0f : */
/* 	    osc->phase[channel] <= 0.5f ? */
/* 	        (0.5f - osc->phase[channel]) * 4.0 : */
/* 	    osc->phase[channel] <= 0.75f ? */
/* 	        (osc->phase[channel] - 0.5f) * -4.0 : */
/* 	        (1.0f - osc->phase[channel]) * -4.0; */
/*         break; */
/*     case WS_SAW: */
/* 	sample = 2.0f * osc->phase[channel] - 1.0; */
/* 	if (do_blep) { */
/* 	/\* BLEP *\/ */
/* 	    sample -= polyblep(phase_incr, osc->phase[channel]); */
/* 	} */
/* 	break; */
/*     default: */
/* 	sample = 0.0; */
/*     } */
/*     if (osc->freq_modulator) { */
/* 	/\* Raise fmod sample to 3 to get fmod values in more useful range *\/ */
/* 	float fmod_sample = pow(osc_sample(osc->freq_modulator, channel, num_channels, step, chunk_index), 3.0); */
/* 	/\* fprintf(stderr, "fmod sample in osc %p: %f\n", osc, fmod_sample); *\/ */
/* 	osc->phase[channel] += phase_incr * (step * (1.0f + fmod_sample)); */
/*     } else { */
/* 	osc->phase[channel] += phase_incr * step; */
/*     } */
/*     if (osc->amp_modulator) { */
/* 	sample *= 1.0 + osc_sample(osc->amp_modulator, channel, num_channels, step, chunk_index); */
/*     } */
/*     if (osc->phase[channel] > 1.0) { */
/* 	osc->phase[channel] = osc->phase[channel] - floor(osc->phase[channel]); */
/*     } else if (osc->phase[channel] < 0.0) { */
/* 	osc->phase[channel] = 1.0 + osc->phase[channel] + ceil(osc->phase[channel]); */
/*     } */
/*     /\* fprintf(stderr, "Osc: %p phase: %f\n", osc, osc->phase[channel]); *\/ */
/*     /\* float pan_scale; *\/ */
/*     /\* if (channel == 0) { *\/ */
/*     /\* 	pan_scale = osc->pan <= 0.5 ? 1.0 : (1.0f - osc->pan) * 2.0f; *\/ */
/*     /\* } else { *\/ */
/*     /\* 	pan_scale = osc->pan >= 0.5 ? 1.0 : osc->pan * 2.0f; *\/ */
/*     /\* } *\/ */
/*     #ifdef TESTBUILD */
/*     if (fpclassify(sample * osc->amp * pan_scale(osc->pan, channel)) < 3) { */
/* 	breakfn(); */
/*     } */
/*     /\* if (fpclassify(osc->sample_phase_incr,  *\/ */
/*     #endif */
/* apply_amp_and_pan: */
/*     (void)0; */
/*     float bottom_sample; */
/*     if (channel == 0) { */
/* 	osc->saved_L_out[chunk_index] = sample; */
/* 	bottom_sample = sample; */
/*     } else { */
/* 	bottom_sample = osc->saved_L_out[chunk_index]; */
/*     } */
/*     return bottom_sample * osc->amp * pan_scale(osc->pan, channel); */
/* } */

static void synth_voice_add_buf(SynthVoice *v, float *buf, int32_t len, int channel, float step)
{
    if (v->available) return;
    /* if (strcmp(v->synth->track->name, "Track 6") == 0) { */
    /* 	fprintf(stderr, "\t\tVoice %ld\n", v - v->synth->voices); */
    /* 	fprintf(stderr, "\t\t\tADSR stage: %d (%d remaining)\n", v->amp_env->current_stage, v->amp_env->env_remaining); */
    /* } */
    /* fprintf(stderr, "\tadding voice %ld\n", v - v->synth->voices); */
    float osc_buf[len];
    memset(osc_buf, '\0', len * sizeof(float));
    for (int i=0; i<SYNTHVOICE_NUM_OSCS; i++) {
	osc_reset_params(v->oscs + i, len);
    }
    int after = v->amp_env->current_stage == ADSR_UNINIT ? v->amp_env->env_remaining : 0;
    for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) {
	OscCfg *cfg = v->synth->base_oscs + i;
	/* fprintf(stderr, "\t\tvoice %ld osc %d\n", v - v->synth->voices, i); */
	Osc *osc = v->oscs + i;
	/* osc_reset_params(osc); */
	if (!cfg->active) continue;
	if (cfg->mod_freq_of || cfg->mod_amp_of) continue;

	/* if (osc->freq_modulator) */
	/*     osc_reset_params(osc->freq_modulator); */
	/* if (osc->amp_modulator) */
	/*     osc_reset_params(osc->amp_modulator); */
	float ind_osc_buf[len];
	if (channel == 0) {
	    osc_get_buf_preamp(osc, step, len, after);
	}
	memcpy(ind_osc_buf, osc->buf, len * sizeof(float));
	float amp = osc->amp * pan_scale(osc->pan, channel);
        float_buf_mult_const(ind_osc_buf, amp, len);
	float_buf_add(osc_buf, ind_osc_buf, len);
	
	/* for (int i=0; i<len; i++) { */
	/*     osc_buf[i] += osc_sample(osc, channel, 2, step, i); */
	/* } */
	for (int j=0; j<cfg->unison.num_voices; j++) {
	    osc = v->oscs + i + SYNTH_NUM_BASE_OSCS * (j + 1);
	    /* osc_reset_params(osc); */
	    /* if (osc->freq_modulator) */
	    /* 	osc_reset_params(osc->freq_modulator); */
	    /* if (osc->amp_modulator) */
	    /* 	osc_reset_params(osc->amp_modulator); */

	    if (channel == 0) {
		osc_get_buf_preamp(osc, step, len, after);
	    }
	    memcpy(ind_osc_buf, osc->buf, len * sizeof(float));
	    amp = osc->amp * pan_scale(osc->pan, channel);
	    float_buf_mult_const(ind_osc_buf, amp, len);
	    float_buf_add(osc_buf, ind_osc_buf, len);
	}
    }

    float amp_env[len];
    enum adsr_stage amp_stage = adsr_get_chunk(&v->amp_env[channel], amp_env, len);
    /* fprintf(stderr, "\t\tGot amp chunk channel %d (env %p), end stage %d\n", channel, &v->amp_env[channel], amp_stage); */
    if (amp_stage == ADSR_OVERRUN && channel == 1) {
	v->available = true;
	/* fprintf(stderr, "\t\t\tFREEING VOICE %ld (env overrun)\n", v - v->synth->voices); */
	/* return; */
    }
    
    IIRFilter *f = &v->filter;
    float filter_env[len];
    float *filter_env_p = amp_env;
    if (v->synth->filter_active && !v->synth->use_amp_env) {
	adsr_get_chunk(&v->filter_env[channel], filter_env, len);
	filter_env_p = filter_env;
    }
    bool do_noise = false;
    float noise_env[len];
    if (v->synth->noise_amt > 1e-9) {
	do_noise = true;
	if (v->synth->noise_apply_env) {
	    adsr_get_chunk(&v->noise_amt_env[channel], noise_env, len);
	}
    }
    for (int i=0; i<len; i++) {
	if (do_noise) {
	    float env = 1.0;
	    if (v->synth->noise_apply_env) {
		env = noise_env[i];
	    }
	    osc_buf[i] += env * (((float)(rand() % INT16_MAX) / INT16_MAX) - 0.5) * 2.0 * v->synth->noise_amt;
	}
	if (v->synth->filter_active) {
	    if (i%37 == 0) { /* Update filter every 37 sample frames */
		    
		    /* + v->synth->vel_amt * (v->velocity / 127.0); */
		float note;
		if (v->do_portamento && v->portamento_len_sframes > 0) {
		    /* float portamento_incr = (float)(osc->voice->note_val - osc->voice->portamento_from) / osc->voice->portamento_len_bufs; */
		    note = (float)v->portamento_from + ((double)v->portamento_elapsed_sframes * (v->note_val - v->portamento_from) / v->portamento_len_sframes);
		} else {
		    note = v->note_val;
		}
	    
		double freq =
		    (v->synth->base_cutoff
		     + v->synth->pitch_amt * mtof_calc(note) / (float)session_get_sample_rate()
			)
		    * (1.0f + (v->synth->env_amt * filter_env_p[i]))
		    * (1.0f - (v->synth->vel_amt * (1.0 - (float)v->velocity / 127.0f)));
		if (freq > 0.99f) freq = 0.99f;
		if (freq < 1e-3) freq = 1e-3;
		/* if (freq > 1e-3) { */
		/* fprintf(stderr, "SET filter freq %f (env %f, stage %d), res %f\n", freq, filter_env_p[i], v->filter_env[channel].current_stage, v->synth->resonance); */
		iir_set_coeffs_lowpass_chebyshev(f, freq, v->synth->resonance);
		/* } */
	    }
	    /* float pre_filter = osc_buf[i]; */
	    osc_buf[i] = iir_sample(f, osc_buf[i], channel);
	    /* if (fabs(osc_buf[i]) > 5.0) { */
	    /* 	fprintf(stderr, "Err post filter %f (pre %f)\n", osc_buf[i], pre_filter); */
	    /* 	fprintf(stderr, "\tLast set freq: %f (%fHz), coeffs A: %f %f %f, B: %f %f\n", last_set_freq, dsp_scale_freq_to_hz(last_set_freq),   f->A[0], f->A[1], f->A[2], f->B[0], f->B[1]); */
	    /* 	fprintf(stderr, "\tPol loc %f+%fi, rad %f\n", creal(f->poles[0]), cimag(f->poles[0]), cabs(f->poles[0])); */
	    /* 	exit(0); */
	    /* } */
	}
	/* buf[i] += osc_buf[i] * (float)v->velocity / 127.0f; */
    }
    if (v->do_portamento && v->portamento_elapsed_sframes < v->portamento_len_sframes) {
	v->portamento_elapsed_sframes += len * step;
	if (v->portamento_elapsed_sframes > v->portamento_len_sframes)
	    v->portamento_elapsed_sframes = v->portamento_len_sframes;
    }

    float_buf_mult(osc_buf, amp_env, len);
    float_buf_mult_const(osc_buf, (float)v->velocity / 127.0f, len);
    float_buf_add(buf, osc_buf, len);
}

static void osc_set_freq(Osc *osc, double freq_hz)
{
    osc->freq = freq_hz;
    osc->sample_phase_incr = freq_hz / session_get_sample_rate();
    #ifdef TESTBUILD
    /* fprintf(stderr, "Osc %p, Freq hz %f, phase incr %f\n", osc, freq_hz, osc->sample_phase_incr); */
    if (fpclassify(osc->sample_phase_incr) == FP_NAN) {
	fprintf(stderr, "ERROR: phase incr not valid in osc_set_freq\n");
	breakfn();
    }
    #endif
    /* fprintf(stderr, "OSC %p SFI: %f, ADDTL: %f\n", osc, osc->sample_phase_incr, osc->sample_phase_incr_addtl); */
}

static void osc_set_pitch_bend(Osc *osc, double freq_rat)
{
    /* fprintf(stderr, "SET PB\n"); */
    double bend_freq = osc->freq * freq_rat;
    osc->sample_phase_incr_addtl = (bend_freq / session_get_sample_rate()) - osc->sample_phase_incr;
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

static void synth_voice_pitch_bend(SynthVoice *v, float cents)
{
    double freq_rat = pow(2.0, cents / 1200.0);
    /* fprintf(stderr, "FREQ RAT: %f\n", freq_rat); */
    for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) {
	/* OscCfg *cfg = v->synth->base_oscs + i; */
	/* if (!cfg->active) continue; */
	for (int j=0; j<SYNTHVOICE_NUM_OSCS; j+= SYNTH_NUM_BASE_OSCS) {
	    Osc *voice = v->oscs + i + j;
	    osc_set_pitch_bend(voice, freq_rat);
	}
    }
}

void synth_pitch_bend(Synth *s, float cents)
{
    s->pitch_bend_cents += cents;
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	synth_voice_pitch_bend(s->voices + i, s->pitch_bend_cents);
    }
}

static void synth_voice_assign_note(SynthVoice *v, double note, int velocity, int32_t start_rel)
{
    /* srand(time(NULL)); */
    Synth *synth = v->synth;

    bool portamento = false;
    bool stolen = false;
    if (!v->available) {
	stolen = true;
	if (v->synth->mono_mode) {
	    v->portamento_from = v->note_val;
	    v->portamento_len_sframes = synth->portamento_len_msec / 1000.0 * session_get_sample_rate();
	    v->portamento_elapsed_sframes = 0;
	    v->do_portamento = true;
	    portamento = true;
	}
    } else {
	iir_clear(&v->filter);
	v->do_portamento = false;
    }

    v->note_val = note;
    /* memset(v->filter.memIn[0], '\0', sizeof(float) * v->filter.degree); */
    /* memset(v->filter.memIn[1], '\0', sizeof(float) * v->filter.degree); */
    /* memset(v->filter.memOut[0], '\0', sizeof(float) * v->filter.degree); */
    /* memset(v->filter.memOut[1], '\0', sizeof(float) * v->filter.degree); */
    for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) {
	OscCfg *cfg = synth->base_oscs + i;
	if (!cfg->active) continue;	
	/* Osc *osc = v->oscs + i; */
	for (int j=0; j<SYNTHVOICE_NUM_OSCS; j+=SYNTH_NUM_BASE_OSCS) {
	    Osc *osc = v->oscs + i + j;
	    osc->type = cfg->type;
	    /* osc->sample_phase_incr_addtl = 0; */
	    /* osc->reset_params_ctr[0] = 0; */
	    /* osc->reset_params_ctr[1] = 0; */
	    
	    if (synth->sync_phase && !portamento) {
		osc->phase = 0.0;
	    }
	}
    }
    v->velocity = velocity;
    v->available = false;

    if (portamento || stolen) {
	/* fprintf(stderr, "SYNTH VOICE %ld reinit!\n", v - v->synth->voices); */
	adsr_reinit(v->amp_env, start_rel);
	adsr_reinit(v->amp_env + 1, start_rel);

	adsr_reinit(v->filter_env, start_rel);
	adsr_reinit(v->filter_env + 1, start_rel);

	adsr_reinit(v->noise_amt_env, start_rel);
	adsr_reinit(v->noise_amt_env + 1, start_rel);
    } else {
	adsr_init(v->amp_env, start_rel);
	adsr_init(v->amp_env + 1, start_rel);

	adsr_init(v->filter_env, start_rel);
	adsr_init(v->filter_env + 1, start_rel);

	adsr_init(v->noise_amt_env, start_rel);
	adsr_init(v->noise_amt_env + 1, start_rel);
    }
}

static void fmod_carrier_unlink(Synth *s, OscCfg *carrier)
{
    carrier->freq_mod_by = NULL;
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	SynthVoice *v = s->voices + i;
	for (int j=carrier-s->base_oscs; j<SYNTHVOICE_NUM_OSCS; j+= SYNTH_NUM_BASE_OSCS) {
	    Osc *osc = v->oscs + j;
	    if (osc->freq_modulator) {
		osc->freq_modulator = NULL;
	    }
	}
    }
}

static void amod_carrier_unlink(Synth *s, OscCfg *carrier)
{
    carrier->amp_mod_by = NULL;
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	SynthVoice *v = s->voices + i;
	for (int j=carrier-s->base_oscs; j<SYNTHVOICE_NUM_OSCS; j+= SYNTH_NUM_BASE_OSCS) {
	    Osc *osc = v->oscs + j;
	    if (osc->amp_modulator) {
		osc->amp_modulator = NULL;
	    }
	}
    }
}
/* Return 0 if success */
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
		fmod_carrier_unlink(s, cfg);
		/* cfg is old carrier */
		/* fprintf(stderr, "Osc%d turning off light\n", (int)(cfg - s->base_oscs) + 1); */

	    }
	}
	return 1;
    } else if (carrier_cfg->freq_mod_by) {
        status_set_errstr("Modulator already assigned to that osc");
	return -1;
    } else { /* Loop detection */
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

    if (modulator_cfg->mod_freq_of) {
	fmod_carrier_unlink(s, modulator_cfg->mod_freq_of);
    }
    int carrier_i = carrier_cfg - s->base_oscs;

    /* fprintf(stderr, "Mod i carr i: %d %d\n", modulator_i, carrier_i); */
    modulator_cfg->mod_freq_of = carrier_cfg;
    carrier_cfg->freq_mod_by = modulator_cfg;

	
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	for (int j=0; j<SYNTH_MAX_UNISON_OSCS; j++) {
	    SynthVoice *v = s->voices + i;
	    /* fprintf(stderr, "Setting pair MOD %d CARR %d\n", modulator_i + SYNTH_NUM_BASE_OSCS * j, carrier_i + SYNTH_NUM_BASE_OSCS * j); */
	    Osc *modulator = v->oscs + modulator_i + SYNTH_NUM_BASE_OSCS * j;
	    Osc *carrier = v->oscs + carrier_i + SYNTH_NUM_BASE_OSCS * j;
	    carrier->freq_modulator = modulator;
	}
    }
    return 0;
}

int synth_set_amp_mod_pair(Synth *s, OscCfg *carrier_cfg, OscCfg *modulator_cfg)
{
    if (!modulator_cfg) {
	fprintf(stderr, "Error: synth_set_amp_mod_pair: modulator_cfg arg is mandatory\n");
	return -1;
	
    }
    int modulator_i = modulator_cfg - s->base_oscs;
    
    if (!carrier_cfg) {
	modulator_cfg->mod_amp_of = NULL;
	for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) {
	    OscCfg *cfg = s->base_oscs + i;
	    if (cfg->amp_mod_by == modulator_cfg) {
		amod_carrier_unlink(s, cfg);
		/* cfg is old carrier */
		/* fprintf(stderr, "Osc%d turning off light\n", (int)(cfg - s->base_oscs) + 1); */

	    }
	}
	return 1;
    } else if (carrier_cfg->amp_mod_by) {
        status_set_errstr("Modulator already assigned to that osc");
	return -1;
    } else { /* Loop detection */
	OscCfg *loop_detect = carrier_cfg;
	while (loop_detect->mod_amp_of) {
	    if (loop_detect->mod_amp_of == modulator_cfg) {
		fprintf(stderr, "ERROR! loop detected!\n");
		status_set_errstr("Invalid: modulation loop detected");
		synth_set_amp_mod_pair(s, NULL, modulator_cfg);
		return -2;
	    }
	    loop_detect = loop_detect->mod_amp_of;
	}
    }

    if (modulator_cfg->mod_amp_of) {
	amod_carrier_unlink(s, modulator_cfg->mod_amp_of);
    }
    int carrier_i = carrier_cfg - s->base_oscs;

    modulator_cfg->mod_amp_of = carrier_cfg;
    carrier_cfg->amp_mod_by = modulator_cfg;

	
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	for (int j=0; j<SYNTH_MAX_UNISON_OSCS; j++) {
	    SynthVoice *v = s->voices + i;
	    Osc *modulator = v->oscs + modulator_i + SYNTH_NUM_BASE_OSCS * j;
	    Osc *carrier = v->oscs + carrier_i + SYNTH_NUM_BASE_OSCS * j;
	    carrier->amp_modulator = modulator;
	}
    }
    return 0;
}

void synth_voice_start_release(SynthVoice *v, int32_t after)
{
    if (after < 0) after = 0;
    adsr_start_release(v->amp_env, after);
    adsr_start_release(v->amp_env + 1, after);
    adsr_start_release(v->filter_env, after);
    adsr_start_release(v->filter_env + 1, after);
    adsr_start_release(v->noise_amt_env, after);
    adsr_start_release(v->noise_amt_env + 1, after);
}    

void synth_feed_midi(
    Synth *s,
    PmEvent *events,
    int num_events,
    int32_t tl_start,
    bool send_immediate)
{
    if (num_events == 0) return;
    /* fprintf(stderr, "\n\nSYNTH FEED MIDI %d events tl start start: %d send immediate %d\n", num_events, tl_start, send_immediate); */
    /* FIRST: Update synth state from available MIDI data */
    /* fprintf(stderr, "SYNTH FEED MIDI tl start: %d\n", tl_start); */
    for (int i=0; i<num_events; i++) {
	/* PmEvent e = s->device.buffer[i]; */
	PmEvent e = events[i];
	uint8_t status = Pm_MessageStatus(e.message);
	uint8_t note_val = Pm_MessageData1(e.message);
	uint8_t velocity = Pm_MessageData2(e.message);
	uint8_t msg_type = status >> 4;

	/* fprintf(stderr, "IN SYNTH:(%d) %s, pitch %d vel %d\n", */
	/* 	e.timestamp, */
	/* 	msg_type == 8 ? "note OFF" : */
	/* 	msg_type == 9 ? "note ON" : */
	/* 	"unknown", */
	/* 	Pm_MessageData1(e.message), */
	/* 	Pm_MessageData2(e.message)); */
	/* fprintf(stderr, "\ttype %d vals %d %d, channel %d\n", msg_type, note_val, velocity, status & 0x0F); */

	/* uint8_t channel = status & 0x0F; */

	/* fprintf(stderr, "\t\tIN SYNTH msg%d/%d %x, %d, %d (%s)\n", i, s->num_events, status, note_val, velocity, msg_type == 0x80 ? "OFF" : msg_type == 0x90 ? "ON" : "ERROR"); */
	/* if (i == 0) start = e.timestamp; */
	/* int32_t pos_rel = ((double)e.timestamp - start) * (double)session_get_sample_rate() / 1000.0; */
	/* fprintf(stderr, "EVENT %d/%d, timestamp: %d\n", i, num_events, e.timestamp); */
	if (msg_type == 9 && velocity == 0) msg_type = 8;
	if (msg_type == 8) {
	    /* HANDLE NOTE OFF */
	    /* fprintf(stderr, "NOTE OFF val: %d pos %d\n", note_val, e.timestamp); */
	    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
		SynthVoice *v = s->voices + i;
		if (!v->available && v->note_val == note_val) {
		    /* fprintf(stderr, "Voice %ld\n", v - s->voices); */
		    /* if (v->note_off_deferred) { */
		    /* 	fprintf(stderr, "\t(already deferred)\n"); */
		    /* } */
		    if (s->pedal_depressed) {
			if (!v->note_off_deferred) {
			    /* fprintf(stderr, "NUM DEFERRED OFFS: %d\n", s->num_deferred_offs); */
			    v->note_off_deferred = true;
			    if (s->num_deferred_offs < SYNTH_NUM_VOICES) {
				s->deferred_offs[s->num_deferred_offs] = v;
				s->num_deferred_offs++;
			    } else {
				fprintf(stderr, "ERROR: no room to defer note off\n");
			    }
			}
		    } else {
			int32_t end_rel = send_immediate ? 0 : e.timestamp - tl_start;
			/* fprintf(stderr, "\t\tStart release voice %d, end rel %d\n", i, end_rel); */
			synth_voice_start_release(v, end_rel);
		    }
		}
	    }
	} else if (msg_type == 9) { /* Handle note on */
	    /* fprintf(stderr, "NOTE ON val: %d pos %d\n", note_val, e.timestamp); */
	    bool note_assigned = false;
	    for (int i=0; i<s->num_voices; i++) {
		SynthVoice *v = s->voices + i;
		if (v->available || s->mono_mode) {
		    /* fprintf(stderr, "\t=>assigned to voice %d\n", i); */
		    int32_t start_rel = send_immediate ? 0 : e.timestamp - tl_start;
		    synth_voice_assign_note(v, note_val, velocity, start_rel);
		    note_assigned = true;
		    break;
		}
	    }
	    if (!note_assigned) {
		if (s->allow_voice_stealing) { /* check amp env for oldest voice */
		    int32_t oldest_dur = 0;
		    SynthVoice *oldest = NULL;
		    for (int i=0; i<s->num_voices; i++) {
			SynthVoice *v = s->voices + i;
			if (!v->available) {
			    int32_t dur = adsr_query_position(v->amp_env);
			    if (dur > oldest_dur) {
				oldest_dur =  dur;
				oldest = v;
			    }
			}
		    }
		    if (oldest) { /* steal the oldest voice */
			int32_t start_rel = send_immediate ? 0 : e.timestamp - tl_start;
			synth_voice_assign_note(oldest, note_val, velocity, start_rel);
			note_assigned = true;
		    }
		}
	    }
	} else if (msg_type == 0xE) { /* Pitch Bend */
	    float pb = midi_pitch_bend_float_from_event(&e);
	    /* fprintf(stderr, "REC'd PITCH BEND! amt: %f, normalized: %f, normalized incorrect: %f\n", pb, pb - 0.5, pb - 1.0); */
	    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
		SynthVoice *v = s->voices + i;
		synth_voice_pitch_bend(v, 2.0f * (pb - 0.5) * 200.0);
		/* if (!v->available) { */
		/*     enum adsr_stage stage = v->amp_env[0].current_stage; */
		/*     if (stage > ADSR_UNINIT) { */
		/*     } */
		/* } */
	    }
	} else if (msg_type == 0xB) { /* Control change */
	    uint8_t type = Pm_MessageData1(e.message);
	    uint8_t value = Pm_MessageData2(e.message);
	    /* MIDICC cc = midi_cc_from_event(&e, 0); */
	    /* fprintf(stderr, "REC'd contrl change type %d val %d\n", cc.type, cc.value); */
	    if (type == 64) {
		s->pedal_depressed = value >= 64;
		if (!s->pedal_depressed) {
		    int32_t end_rel = send_immediate ? 0 : e.timestamp - tl_start;
		    for (int i=0; i<s->num_deferred_offs; i++) {
			synth_voice_start_release(s->deferred_offs[i], end_rel);
			/* fprintf(stderr, "----RElease Voice %ld\n", s->deferred_offs[i] - s->voices); */
			s->deferred_offs[i]->note_off_deferred = false;
			s->deferred_offs[i] = NULL;
			/* adsr_start_release(s->deferred_offs[i], end_rel); */
		    }
		    s->num_deferred_offs = 0;
		}
		/* fprintf(stderr, "DEPR? %d\n", s->pedal_depressed); */
	    }

	}
    }
}

/* void synth_feed_note( */
/*     Synth *s, */
/*     int pitch, */
/*     int velocity, */
/*     int32_t dur) */
/* { */
/*     Session *session = session_get(); */
/*     PmEvent events[2]; */
/*     events[0].timestamp = Pt_Time(); */
/*     events[0].message = Pm_Message(0x90, (uint8_t)pitch, (uint8_t)velocity); */
/*     events[1].timestamp = Pt_Time() + dur / session_get_sample_rate(); */
/*     events[1].message = Pm_Message(0x80, (uint8_t)pitch, (uint8_t)velocity); */
/*     synth_feed_midi(s, events, 2, ACTIVE_TL->play_pos_sframes, false); */
/* } */



void synth_silence(Synth *s);

void synth_debug_summary(Synth *s, int channel, int32_t len, float step)
{
    fprintf(stderr, "Synth debug summary channel %d, len %d step %f:\n", channel, len, step);
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	SynthVoice *v = s->voices + i;
	if (v->available) continue;
	fprintf(stderr, "\tVoice %d: note %d, vel %d\n", i, v->note_val, v->velocity);
	fprintf(stderr, "\t\tAMP ENV stage %d, rem %d, start release after %d\n", s->amp_env.followers[0]->current_stage, s->amp_env.followers[0]->env_remaining, s->amp_env.followers[0]->start_release_after);
    }
}

void synth_add_buf(Synth *s, float *buf, int channel, int32_t len, float step)
{
    /* synth_debug_summary(s, channel, len, step); */
    /* fprintf(stderr, "PED? %d\n", s->pedal_depressed); */
    /* if (channel != 0) return; */
    if (step < 0.0) step *= -1;
    if (step > 5.0) {
	synth_silence(s);
	memset(buf, '\0', len * sizeof(float));
	return;
    }
    pthread_mutex_lock(&s->audio_proc_lock);
    float internal_buf[len];
    memset(internal_buf, 0, sizeof(internal_buf));
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	SynthVoice *v = s->voices + i;
	synth_voice_add_buf(v, internal_buf, len, channel, step);
    }
    effect_chain_buf_apply(&s->effect_chain, internal_buf, len, channel, 1.0);
    /* float sum = 0.0f; */
    /* for (int i=0; i<len; i++) { */
    /* 	sum += fabs(internal_buf[i]); */
    /* } */
    /* fprintf(stderr, "\tInternal buf tot %f\n", sum); */
    /* fprintf(stderr, "\tvol: %f pan scale: %f\n", s->vol, pan_scale(s->pan, channel)); */
    /* float sum = 0.0f; */
    for (int i=0; i<len; i++) {
	/* float dc_blocked = iir_sample(&s->dc_blocker, internal_buf[i], channel); */
	/* sum += fabs(dc_blocked); */
	buf[i] += tanh(
	    /* dc_blocked */
	    iir_sample(&s->dc_blocker, internal_buf[i], channel)
	    * s->vol
	    * pan_scale(s->pan, channel)
	    );
	/* sum += fabs(buf[i]); */
	
    }
    /* fprintf(stderr, "\t=>end synth buf, avg amp %f\n", sum / len); */
    /* if (sum / len > 0.25) { */
    /* 	fprintf(stderr, "AMP VIOLATION\n"); */
    /* 	exit(1); */
    /* } */
    /* fprintf(stderr, "\tdc blocked sum %f\n", sum); */
    pthread_mutex_unlock(&s->audio_proc_lock);
}

int32_t synth_make_notes(Synth *s, int *pitches, int *velocities, int num_pitches, float **buf_L_dst, float **buf_R_dst)
{
    /* Session *session = session_get(); */
    /* if (session->midi_io.monitor_synth == s) { */
    /* 	session->midi_io.monitor_synth = NULL; */
    /* 	reset_monitor_synth = true; */
    /* } */
    int32_t sr = session_get_sample_rate();
    int32_t len = 0;
    int32_t alloc_len = sr;
    float *buf_L = malloc(alloc_len * sizeof(float));
    float *buf_R = malloc(alloc_len * sizeof(float));
    memset(buf_L, '\0', alloc_len * sizeof(float));
    memset(buf_R, '\0', alloc_len * sizeof(float));
    synth_silence(s);
    for (int i=0; i<num_pitches; i++) {
	synth_voice_assign_note(s->voices + i, pitches[i], velocities[i], 0);
    }
    int32_t incr_len = 4096;
    /* fprintf(stderr, "SYNTH ASSIGNED %d voices\n", num_pitches); */
    bool release_started = false;
    while (!s->voices[0].available) {
	if (len + incr_len >= alloc_len) {
	    alloc_len *= 2;
	    buf_L = realloc(buf_L, alloc_len * sizeof(float));
	    buf_R = realloc(buf_R, alloc_len * sizeof(float));
	    memset(buf_L + alloc_len / 2, '\0', alloc_len * sizeof(float) / 2);
	    memset(buf_R + alloc_len / 2, '\0', alloc_len * sizeof(float) / 2);
	}
	synth_add_buf(s, buf_L + len, 0, incr_len, 1.0);
	synth_add_buf(s, buf_R + len, 1, incr_len, 1.0);
	len += incr_len;
	
	/* Quarter-second sustain time */
	if (!release_started && len >= (sr / 4)) {
	    for (int i=0; i<num_pitches; i++) {
		/* fprintf(stderr, "\t\tStarting release for voice %d\n", i); */
		synth_voice_start_release(s->voices + i, 0);
	    }
	    release_started = true;
	}
    }
    buf_L = realloc(buf_L, len * sizeof(float));
    buf_R = realloc(buf_R, len * sizeof(float));
    *buf_L_dst = buf_L;
    *buf_R_dst = buf_R;
    /* if (reset_monitor_synth) { */
    /* 	session->midi_io.monitor_synth = s; */
    /* } */

    return len;
}


void synth_close_all_notes(Synth *s)
{
    /* fprintf(stderr, "CLOSE ALL NOTES\n"); */
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	SynthVoice *v = s->voices + i;
	synth_voice_pitch_bend(v, 0.0);
	if (!v->available) {
	    /* fprintf(stderr, "\tkill voice %d\n", i); */
	    adsr_start_release(v->amp_env, 0);
	    adsr_start_release(v->amp_env + 1, 0);
	    adsr_start_release(v->filter_env, 0);
	    adsr_start_release(v->filter_env + 1, 0);
	    adsr_start_release(v->noise_amt_env, 0);
	    adsr_start_release(v->noise_amt_env + 1, 0);
	}
    }
}
void synth_silence(Synth *s)
{
    /* exit(1); */
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	SynthVoice *v = s->voices + i;
	v->available = true;
	v->amp_env->current_stage = ADSR_OVERRUN;
    }
}
void synth_clear_all(Synth *s)
{
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	SynthVoice *v = s->voices + i;
	if (!v->available) {
	    v->available = true;
	}
    }
}

void synth_write_preset_file(const char *filepath, Synth *s)
{
    FILE *f = fopen(filepath, "w");
    fprintf(f, "JSYNTHv02\n");
    jdaw_write_effect_chain(f, &s->effect_chain);
    api_node_serialize(f, &s->api_node);
    fclose(f);
}

struct synth_and_preset_path {
    Synth *synth;
    char *preset_path_copy;
};

static void synth_read_preset_file_internal(const char *filepath, Synth *s, bool from_undo);

NEW_EVENT_FN(undo_read_synth_preset, "undo read synth preset file")
    struct synth_and_preset_path *sp = obj1;
    synth_read_preset_file_internal(obj2, sp->synth, true);
}

NEW_EVENT_FN(redo_read_synth_preset, "redo read synth preset file")
    struct synth_and_preset_path *sp = obj1;
    synth_read_preset_file_internal(sp->preset_path_copy, sp->synth, true);
}

NEW_EVENT_FN(dispose_read_synth_preset, "")
    struct synth_and_preset_path *sp = obj1;
    free(sp->preset_path_copy);
    char *backup_filepath = obj2;
    fprintf(stderr, "DISPOSING read synth preset %s\n", backup_filepath); 
    FILE *f = fopen(backup_filepath, "r");
    if (!f) {
        log_tmp(LOG_ERROR, "Unable to delete tmp file at %s; file could not be opened\n", backup_filepath);
	/* free(backup_filepath); */
	return;
    }
    char hdr[6];
    fread(hdr, 1, 6, f);
    if (strncmp(hdr, "JSYNTH", 6) != 0) {
        log_tmp(LOG_ERROR, "Unable to safely delete tmp file at %s; expected JSYNTH hdr not found\n", backup_filepath);
	fclose(f);
	/* free(backup_filepath); */
	return;	
    }
    fclose(f);
    if (remove(backup_filepath) != 0) {
	log_tmp(LOG_ERROR, "Unable to remove tmp file at %s: %s\n", backup_filepath, strerror(errno));
    }
}


static void synth_read_preset_file_internal(const char *filepath, Synth *s, bool from_undo)
{   
    FILE *f = fopen(filepath, "r");
    if (!f) {
	status_set_errstr("File does not exist or could not be opened.");
	return;
    }


    if (!from_undo) {
	/* Backup the current synth settings for undo */
	static int backup_file_index = 0;
	char backup_filename[32] = {0};
	snprintf(backup_filename, 32, "jackdaw_synth%d.jsynth", backup_file_index);
	backup_file_index++;
	char *backup_filepath = create_tmp_file(backup_filename);
	synth_write_preset_file(backup_filepath, s);

	struct synth_and_preset_path *sp = malloc(sizeof(struct synth_and_preset_path));
	sp->synth = s;
	sp->preset_path_copy = strdup(filepath);
	user_event_push(
	    undo_read_synth_preset,
	    redo_read_synth_preset,
	    dispose_read_synth_preset, dispose_read_synth_preset,
	    sp,
	    backup_filepath,
	    (Value){0}, (Value){0}, (Value){0}, (Value){0},
	    0, 0, true, true);
    }
	
    for (int i=0; i<s->effect_chain.num_effects; i++) {
	api_node_deregister(&s->effect_chain.effects[i]->api_node);
    }
    
    effect_chain_deinit(&s->effect_chain);
    
    char hdr[10];
    fread(hdr, 1, 10, f);
    if (strncmp(hdr, "JSYNTHv", 7) == 0) {
	int version = (hdr[7] - '0') * 10 + (hdr[8] - '0') * 10;
	fprintf(stderr, "JSYNTH VERSION: %d\n", version);
	if (version >= 2) {
	    fprintf(stderr, "\treading effect chain\n");
	    /* BUG: READ FILESPEC NOT SET */
	    jdaw_read_effect_chain(f, &session_get()->proj, &s->effect_chain, &s->api_node, "synth", s->track->tl->proj->chunk_size_sframes);
	}
    } else {
	fseek(f, SEEK_SET, 0);
    }
    
    if (api_node_deserialize(f, &s->api_node) == 0) {
	char buf[256];
	snprintf(buf, 256, "%s", filepath);
	const char *last_slash_pos = buf;
	char *ptr = buf;
	while (*ptr) {
	    if (*ptr == '/') {
		last_slash_pos = ptr;
	    }
	    ptr++;
	    if (*ptr == '.') *ptr = '\0';
	}
	if (last_slash_pos != buf) last_slash_pos++;
	snprintf(s->preset_name, 64, "%s", last_slash_pos);
	timeline_check_set_midi_monitoring();
	
    }
    fclose(f);
}

void synth_read_preset_file(const char *filepath, Synth *s)
{
    synth_read_preset_file_internal(filepath, s, false);
}

void synth_destroy(Synth *s)
{
    Session *session = session_get();
    if (session->midi_io.monitor_synth == s) {
	session->midi_io.monitor_device = NULL;
	session->midi_io.monitor_synth = NULL;
    }
    effect_chain_deinit(&s->effect_chain);
    for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) {
	OscCfg *cfg = s->base_oscs + i;
	free(cfg->active_id);
	free(cfg->type_id);
	free(cfg->amp_id);
	free(cfg->pan_id);
	free(cfg->octave_id);
	free(cfg->tune_coarse_id);
	free(cfg->tune_fine_id);
	free(cfg->fix_freq_id);
	free(cfg->fixed_freq_id);
	free(cfg->unison.num_voices_id);
	free(cfg->unison.detune_cents_id);
	free(cfg->unison.relative_amp_id);
	free(cfg->unison.stereo_spread_id);
	free(cfg->fmod_target_dropdown_id);
	free(cfg->amod_target_dropdown_id);
    }
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	iir_deinit(&s->voices[i].filter);
	for (int j=0; j<SYNTHVOICE_NUM_OSCS; j++) {
	    Osc *osc = s->voices[i].oscs + j;
	    iir_deinit(&osc->tri_dc_blocker);
	}
    }
    iir_deinit(&s->dc_blocker);
    adsr_params_deinit(&s->amp_env);
    adsr_params_deinit(&s->noise_amt_env);
    adsr_params_deinit(&s->filter_env);
    free(s);
}
