#include <portmidi.h>
#include <porttime.h>
#include "adsr.h"
#include "api.h"
#include "dsp_utils.h"
#include "endpoint.h"
#include "endpoint_callbacks.h"
#include "consts.h"
#include "iir.h"
/* #include "test.h" */
/* #include "modal.h" */
#include "label.h"
#include "synth.h"
#include "session.h"

extern double MTOF[];

static void synth_osc_vol_dsp_cb(Endpoint *ep)
{
    OscCfg *cfg = ep->xarg1;
    float vol = endpoint_safe_read(ep, NULL).float_v;
    if (vol > 1e-9) cfg->active = true;
    else cfg->active = false;
}

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

Synth *synth_create(Track *track)
{
    Synth *s = calloc(1, sizeof(Synth));

    snprintf(s->preset_name, MAX_NAMELENGTH, "preset.jsynth");
    s->track = track;

    s->vol = 1.0;
    s->pan = 0.5;
    
    s->base_oscs[0].active = true;
    s->base_oscs[0].amp = 0.2;
    s->base_oscs[0].type = WS_SAW;

    s->filter_active = true;
    s->base_cutoff = 0.1f;
    s->pitch_amt = 10.0f;
    s->vel_amt = 0.75f;
    s->env_amt = 1.0f;
    s->resonance = 3.0;
    
    iir_init(&s->dc_blocker, 1, 2);
    double A[] = {1.0, -1.0};
    double B[] = {0.999};
    iir_set_coeffs(&s->dc_blocker, A, B);

    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	SynthVoice *v = s->voices + i;
	adsr_params_add_follower(&s->amp_env, &v->amp_env[0]);
	adsr_params_add_follower(&s->amp_env, &v->amp_env[1]);
	
	adsr_params_add_follower(&s->filter_env, &v->filter_env[0]);
	adsr_params_add_follower(&s->filter_env, &v->filter_env[1]);

	iir_init(&v->filter, 2, 2);
	v->synth = s;
	v->available = true;
	/* v->amp_env[0].params = &s->amp_env; */
	/* v->amp_env[1].params = &s->amp_env; */
    }
    s->amp_env.a_msec = 4;
    s->amp_env.d_msec = 200;
    s->amp_env.s_ep_targ = 0.4;
    s->amp_env.r_msec = 400;
    
    s->filter_env.a_msec = 4;
    s->filter_env.d_msec = 200;
    s->filter_env.s_ep_targ = 0.4;
    s->filter_env.r_msec = 400;
    
    Session *session = session_get();
    adsr_set_params(
	&s->amp_env,
	session->proj.sample_rate * s->amp_env.a_msec / 1000,
	session->proj.sample_rate * s->amp_env.d_msec / 1000,
	s->amp_env.s_ep_targ,
	session->proj.sample_rate * s->amp_env.r_msec / 1000,
	2.0);
    adsr_set_params(
	&s->filter_env,
	session->proj.sample_rate * s->filter_env.a_msec / 1000,
	session->proj.sample_rate * s->filter_env.d_msec / 1000,
	s->filter_env.s_ep_targ,
	session->proj.sample_rate * s->filter_env.r_msec / 1000,
	2.0);

    s->monitor = true;
    s->allow_voice_stealing = true;

    api_node_register(&s->api_node, &track->api_node, "Synth");

    /* fprintf(stderr, "REGISTERING synth node to parent %p, name %s\n", &track->api_node, track->api_node.obj_name); */


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
    api_endpoint_register(&s->pan_ep, &s->api_node);


    /* endpoint_init( */
    /* 	s->amp_env */

    
    adsr_endpoints_init(&s->amp_env, &s->amp_env_page, &s->api_node, "Amp envelope");
    /* api_node_register(&s->amp_env.api_node, &s->api_node, "Amp envelope"); */
    api_node_register(&s->filter_node, &s->api_node, "Filter");
    adsr_endpoints_init(&s->filter_env, &s->filter_page, &s->filter_node, "Filter envelope");
    /* api_node_register(&s->filter_env.api_node, &s->filter_node, "Filter envelope"); */

    /* adsr_params_init(&s->amp_env); */

    s->filter_active = true;
    endpoint_init(
	&s->filter_active_ep,
	&s->filter_active,
	JDAW_BOOL,
	"filter_active",
	"Filter active",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->filter_page, "filter_active_toggle");
    endpoint_set_default_value(&s->filter_active_ep, (Value){.bool_v = true});
    api_endpoint_register(&s->filter_active_ep, &s->filter_node);


    endpoint_init(
	&s->base_cutoff_ep,
	&s->base_cutoff,
	JDAW_FLOAT,
	"base_cutoff",
	"Base cutoff",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->filter_page, "base_cutoff_slider");
    endpoint_set_default_value(&s->base_cutoff_ep, (Value){.float_v = 0.01f});
    endpoint_set_allowed_range(&s->base_cutoff_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 0.2});
    api_endpoint_register(&s->base_cutoff_ep, &s->filter_node);
    endpoint_set_label_fn(&s->base_cutoff_ep, label_freq_raw_to_hz);

    
    endpoint_init(
	&s->pitch_amt_ep,
	&s->pitch_amt,
	JDAW_FLOAT,
	"pitch_amt",
	"Pitch amt",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &s->filter_page, "pitch_amt_slider");
    endpoint_set_default_value(&s->pitch_amt_ep, (Value){.float_v = 10.0f});
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
    endpoint_set_default_value(&s->env_amt_ep, (Value){.float_v = 0.75f});
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
    endpoint_set_allowed_range(&s->resonance_ep, (Value){.float_v = 1.0f}, (Value){.float_v = 50.0f});
    api_endpoint_register(&s->resonance_ep, &s->filter_node);

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

	fmt = "%dfmod_dropdown";
	len = strlen(fmt);
	cfg->fmod_target_dropdown_id = malloc(len);
	snprintf(cfg->fmod_target_dropdown_id, len, fmt, i+1);

	fmt = "%damod_dropdown";
	len = strlen(fmt);
	cfg->amod_target_dropdown_id = malloc(len);
	snprintf(cfg->amod_target_dropdown_id, len, fmt, i+1);

	
	snprintf(synth_osc_names[i], 6, "Osc %d", i+1);
	api_node_register(&cfg->api_node, &s->api_node, synth_osc_names[i]);
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

/* TODO: remove test global */
/* bool do_blep = true; */
static float osc_sample(Osc *osc, int channel, int num_channels, float step)
{
    float sample;
    double phase_incr = osc->sample_phase_incr + osc->sample_phase_incr_addtl;
    switch(osc->type) {
    case WS_SINE:
	sample = sin(TAU * osc->phase[channel]);
	break;
    case WS_SQUARE:
	sample = osc->phase[channel] < 0.5 ? 1.0 : -1.0;
	/* if (do_blep) { */
	/* BLEP */
	sample += polyblep(phase_incr, osc->phase[channel]);
	sample -= polyblep(phase_incr, fmod(osc->phase[channel] + 0.5, 1.0)); 
	/* } */
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
	/* if (do_blep) { */\
	/* BLEP */
	sample -= polyblep(phase_incr, osc->phase[channel]);
	/* } */
	break;
    default:
	sample = 0.0;
    }
    if (osc->freq_modulator) {
	float fmod_sample = osc_sample(osc->freq_modulator, channel, num_channels, step);
	osc->phase[channel] += phase_incr * (step * (1.0f + fmod_sample));
    } else {
	osc->phase[channel] += phase_incr * step;
    }
    if (osc->amp_modulator) {
	sample *= 1.0 + osc_sample(osc->amp_modulator, channel, num_channels, step);
    }
    if (osc->phase[channel] > 1.0) {
	osc->phase[channel] -= 1.0;
    }
    /* float pan_scale; */
    /* if (channel == 0) { */
    /* 	pan_scale = osc->pan <= 0.5 ? 1.0 : (1.0f - osc->pan) * 2.0f; */
    /* } else { */
    /* 	pan_scale = osc->pan >= 0.5 ? 1.0 : osc->pan * 2.0f; */
    /* } */
    return sample * osc->amp * pan_scale(osc->pan, channel);
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
    }

    float amp_env[len];
    enum adsr_stage amp_stage = adsr_get_chunk(&v->amp_env[channel], amp_env, len);
    /* fprintf(stderr, "GET CHUNK channel %d (env %p), end stage %d\n", channel, &v->amp_env[channel], amp_stage); */
    if (amp_stage == ADSR_OVERRUN && channel == 1) {
	v->available = true;
	/* fprintf(stderr, "\tFREEING VOICE %ld (env overrun)\n", v - v->synth->voices); */
	/* return; */
    }
    Session *session = session_get();
    IIRFilter *f = &v->filter;
    float filter_env[len];
    float *filter_env_p = amp_env;
    if (v->synth->filter_active && !v->synth->use_amp_env) {
	adsr_get_chunk(&v->filter_env[channel], filter_env, len);
	filter_env_p = filter_env;
    }
    for (int i=0; i<len; i++) {
	if (v->synth->filter_active) {
	    if (i%37 == 0) { /* Update filter every 37 sample frames */
		    
		    /* + v->synth->vel_amt * (v->velocity / 127.0); */
		double freq =
		    (v->synth->base_cutoff
		     + v->synth->pitch_amt * mtof_calc(v->note_val) / (float)session->proj.sample_rate
			)
		    * (1.0f + (v->synth->env_amt * filter_env_p[i]))
		    * (1.0f - (v->synth->vel_amt * (1.0 - (float)v->velocity / 127.0f)));
		/* double freq = mtof_calc(v->note_val) * v->synth->freq_scalar * filter_env_p[i] / session->proj.sample_rate; */
		/* double velocity_rel = 1.0 - v->synth->velocity_freq_scalar * (127.0 - (double)v->velocity) / 126.0; */
		/* freq *= velocity_rel; */
		/* fprintf(stderr, "Freq: %f", freq); */
		if (freq > 0.99f) freq = 0.99f;
		if (freq > 1e-3) {
		    /* fprintf(stderr, "\t(set)\n"); */
		    iir_set_coeffs_lowpass_chebyshev(f, freq, v->synth->resonance);
		/* } else { */
		/*     fprintf(stderr, "\t(skipped)\n"); */
		}

		/* iir_set_coeffs_lowpass_chebyshev(f, (mtof_calc(v->note_val) * 20 * amp_env[i] * amp_env[i] * amp_env[i]) / session->proj.sample_rate / 2.0f, 4.0); */

	    }
	    osc_buf[i] = iir_sample(f, osc_buf[i], channel);
	}
	/* buf[i] += osc_buf[i] * (float)v->velocity / 127.0f; */
    }
    float_buf_mult(osc_buf, amp_env, len);
    float_buf_mult_const(osc_buf, (float)v->velocity / 127.0f, len);
    float_buf_add(buf, osc_buf, len);


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

static void osc_set_pitch_bend(Osc *osc, double freq_rat)
{
    Session *session = session_get();
    double bend_freq = osc->freq * freq_rat;
    osc->sample_phase_incr_addtl = (bend_freq / session->proj.sample_rate) - osc->sample_phase_incr;
    fprintf(stderr, "SPHASE: %f, ADDTL: %f\n", osc->sample_phase_incr, osc->sample_phase_incr_addtl); 
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
    fprintf(stderr, "FREQ RAT: %f\n", freq_rat);
    for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) {
	OscCfg *cfg = v->synth->base_oscs + i;
	if (!cfg->active) continue;
	for (int j=0; j<SYNTHVOICE_NUM_OSCS; j+= SYNTH_NUM_BASE_OSCS) {
	    Osc *voice = v->oscs + j;
	    osc_set_pitch_bend(voice, freq_rat);
	}
    }
}

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

	/* Reset phase every time? */
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
    /* if (!v->synth->use_amp_env) { */
    adsr_init(v->filter_env, start_rel);
    adsr_init(v->filter_env + 1, start_rel);
    /* } */

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
	fmod_carrier_unlink(s, modulator_cfg->mod_amp_of);
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

    adsr_start_release(v->amp_env, after);
    adsr_start_release(v->amp_env + 1, after);
    adsr_start_release(v->filter_env, after);
    adsr_start_release(v->filter_env + 1, after);
}


void synth_feed_midi(Synth *s, PmEvent *events, int num_events, int32_t tl_start, bool send_immediate)
{
    /* fprintf(stderr, "\n\nSYNTH FEED MIDI %d events tl start start: %d send immediate %d\n", num_events, tl_start, send_immediate); */
    /* FIRST: Update synth state from available MIDI data */
    for (int i=0; i<num_events; i++) {
	/* PmEvent e = s->device.buffer[i]; */
	PmEvent e = events[i];
	uint8_t status = Pm_MessageStatus(e.message);
	uint8_t note_val = Pm_MessageData1(e.message);
	uint8_t velocity = Pm_MessageData2(e.message);
	uint8_t msg_type = status >> 4;
	/* uint8_t channel = status & 0x0F; */

	/* fprintf(stderr, "\t\tIN SYNTH msg%d/%d %x, %d, %d (%s)\n", i, s->num_events, status, note_val, velocity, msg_type == 0x80 ? "OFF" : msg_type == 0x90 ? "ON" : "ERROR"); */
	/* if (i == 0) start = e.timestamp; */
	/* int32_t pos_rel = ((double)e.timestamp - start) * (double)session->proj.sample_rate / 1000.0; */
	/* fprintf(stderr, "EVENT %d/%d, timestamp: %d\n", i, num_events, e.timestamp); */
	if (velocity == 0) msg_type = 8;
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
			int32_t end_rel = send_immediate ? 0 : tl_start - e.timestamp;
			synth_voice_start_release(v, end_rel);
		    }
		}
	    }
	} else if (msg_type == 9) { /* Handle note on */
	    /* fprintf(stderr, "NOTE ON val: %d chan %d pos %d\n", note_val, channel, e.timestamp); */
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
		if (s->allow_voice_stealing) { /* check amp env for oldest voice */
		    int32_t oldest_dur = 0;
		    SynthVoice *oldest = NULL;
		    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
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
			int32_t start_rel = send_immediate ? 0 : tl_start - e.timestamp;
			synth_voice_assign_note(oldest, note_val, velocity, start_rel);
			note_assigned = true;
		    }
		}
	    }
	} else if (msg_type == 0xE) { /* Pitch Bend */
	    float pb = midi_pitch_bend_float_from_event(&e);
	    fprintf(stderr, "REC'd PITCH BEND! amt: %f, normalized: %f, normalized incorrect: %f\n", pb, pb - 0.5, pb - 1.0);
	    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
		SynthVoice *v = s->voices + i;
		if (!v->available) {
		    enum adsr_stage stage = v->amp_env[0].current_stage;
		    if (stage > ADSR_UNINIT && stage < ADSR_R) {
			synth_voice_pitch_bend(v, (pb - 0.5) * 200.0);
		    }
		}
	    }
	} else if (msg_type == 0xB) { /* Control change */
	    MIDICC cc = midi_cc_from_event(&e, 0);
	    /* fprintf(stderr, "REC'd contrl change type %d val %d\n", cc.type, cc.value); */
	    if (cc.type == 64) {
		s->pedal_depressed = cc.value >= 64;
		if (!s->pedal_depressed) {
		    int32_t end_rel = send_immediate ? 0 : tl_start - e.timestamp;
		    for (int i=0; i<s->num_deferred_offs; i++) {
			synth_voice_start_release(s->deferred_offs[i], end_rel);
			fprintf(stderr, "----RElease Voice %ld\n", s->deferred_offs[i] - s->voices);
			s->deferred_offs[i]->note_off_deferred = false;
			s->deferred_offs[i] = NULL;
			/* adsr_start_release(s->deferred_offs[i], end_rel); */
		    }
		    s->num_deferred_offs = 0;
		}
		/* fprintf(stderr, "DEPR? %d\n", s->pedal_depressed); */
	    }

	} else {
	    fprintf(stderr, "UNKNOWN type %d\n", msg_type);
	}
    }
}

void synth_add_buf(Synth *s, float *buf, int channel, int32_t len, float step)
{
    /* fprintf(stderr, "PED? %d\n", s->pedal_depressed); */
    /* if (channel != 0) return; */
    /* fprintf(stderr, "ADD BUF\n"); */
    if (step < 0.0) step *= -1;
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	SynthVoice *v = s->voices + i;
	synth_voice_add_buf(v, buf, len, channel, step);
    }
    for (int i=0; i<len; i++) {
	buf[i] = tanh(
	    iir_sample(&s->dc_blocker, buf[i], channel)
	    * s->vol
	    * pan_scale(s->pan, channel)
	    );
    }
}


void synth_close_all_notes(Synth *s)
{
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	SynthVoice *v = s->voices + i;
	if (!v->available) {
	    adsr_start_release(v->amp_env, 0);
	    adsr_start_release(v->amp_env + 1, 0);
	    adsr_start_release(v->filter_env, 0);
	    adsr_start_release(v->filter_env + 1, 0);

	}
    }
}

void synth_write_preset_file(const char *filepath, Synth *s)
{
    fprintf(stderr, "SAVING FILE AT %s\n", filepath);
    FILE *f = fopen(filepath, "w");
    api_node_serialize(f, &s->api_node);
    fclose(f);
}

void synth_read_preset_file(const char *filepath, Synth *s)
{
    fprintf(stderr, "OPENING FILE AT %s\n", filepath);
    FILE *f = fopen(filepath, "r");
    api_node_deserialize(f, &s->api_node);
    fclose(f);
}

void synth_destroy(Synth *s)
{
    for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) {
	OscCfg *cfg = s->base_oscs + i;
	free(cfg->active_id);
	free(cfg->type_id);
	free(cfg->amp_id);
	free(cfg->pan_id);
	free(cfg->octave_id);
	free(cfg->tune_coarse_id);
	free(cfg->tune_fine_id);
	free(cfg->fmod_target_dropdown_id);
	free(cfg->amod_target_dropdown_id);
    }
    free(s);
}
