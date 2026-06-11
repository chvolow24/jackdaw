#include <math.h>
#include "endpoint_callbacks.h"
#include "mod_delay.h"
#include "pitch_shifter.h"
#include "session.h"
#define MAX_PS_DELAY_LEN (session_get_sample_rate() / 2)

static double get_md_freq_from_params(double shift_cents, double amp)
{
    double freq_ratio = pow(2.0, shift_cents / 1200.0);
    double freq = 2.0 * (freq_ratio - 1.0) / amp;
    return freq;
}

void pitch_shifter_set_shift_amt(PitchShifter *ps, double shift_cents)
{
    ps->shift_cents = shift_cents;
    double freq = get_md_freq_from_params(shift_cents, ps->quality);
    mod_delay_set_freq(&ps->mdL, freq);
    mod_delay_set_freq(&ps->mdR, freq);    
}

void pitch_shifter_set_quality(PitchShifter *ps, double quality)
{
    ps->quality = quality;
    double freq = get_md_freq_from_params(ps->shift_cents, quality);
    mod_delay_set_amp(&ps->mdL, quality);
    mod_delay_set_amp(&ps->mdR, quality);
    mod_delay_set_freq(&ps->mdL, freq);
    mod_delay_set_freq(&ps->mdR, freq);
}

float pitch_shifter_buf_apply(void *ps_v, float *restrict L, float *restrict R, int len, float input_amp)
{
    PitchShifter *ps = ps_v;
    if (L)
	mod_delay_buf(&ps->mdL, L, len);
    if (R) {
	mod_delay_buf(&ps->mdR, R, len);
	if (L) {
	    ps->mdR.phase = ps->mdL.phase;
	}
    }
    return input_amp;   
}

void pitch_shifter_clear(PitchShifter *ps)
{
    mod_delay_clear(&ps->mdL);
    mod_delay_clear(&ps->mdR);
}

static void pitch_shifter_set_shift(PitchShifter *ps, double cents_broad, int semitones, double addtl_cents)
{
    ps->shift_cents = cents_broad + 100 * semitones + addtl_cents;
    pitch_shifter_set_shift_amt(ps, ps->shift_cents);
}

static void shift_cents_dsp_cb(Endpoint *ep)
{
    PitchShifter *ps = ep->xarg1;
    double cents_broad = ep->current_write_val.double_v;
    int semitones = endpoint_safe_read(&ps->shift_semitones_ep, NULL).int_v;
    double adttl_cents = endpoint_safe_read(&ps->shift_fine_ep, NULL).double_v;

    pitch_shifter_set_shift(ps, cents_broad, semitones, adttl_cents);
}

static void shift_semitones_dsp_cb(Endpoint *ep)
{
    PitchShifter *ps = ep->xarg1;
    double cents_broad = endpoint_safe_read(&ps->shift_cents_ep, NULL).double_v;
    int semitones = ep->current_write_val.int_v;
    double adttl_cents = endpoint_safe_read(&ps->shift_fine_ep, NULL).double_v;
    
    pitch_shifter_set_shift(ps, cents_broad, semitones, adttl_cents);
}

static void shift_fine_dsp_cb(Endpoint *ep)
{
    PitchShifter *ps = ep->xarg1;
    double cents_broad = endpoint_safe_read(&ps->shift_cents_ep, NULL).double_v;
    int semitones = endpoint_safe_read(&ps->shift_semitones_ep, NULL).int_v;
    double adttl_cents = ep->current_write_val.double_v;
    
    pitch_shifter_set_shift(ps, cents_broad, semitones, adttl_cents);
}

static void quality_dsp_cb(Endpoint *ep)
{
    pitch_shifter_set_quality(ep->xarg1, ep->current_write_val.double_v);
}


void pitch_shifter_init(PitchShifter *ps)
{
    ps->shift_cents = 0;
    ps->quality = 0.15;
    mod_delay_init(
	&ps->mdL,
	MAX_PS_DELAY_LEN,
	ps->quality,
	0.0,
	2,
	OSC_SAW_DOWN);
    mod_delay_init(
	&ps->mdR,
	MAX_PS_DELAY_LEN,
	ps->quality,
	0.0,
	2,
	OSC_SAW_DOWN);

    endpoint_init(
	&ps->shift_cents_ep,
	&ps->shift_cents_internal,
	JDAW_DOUBLE,
	"shift_cents",
	"Broad (cents)",
	JDAW_THREAD_DSP,
	component_gui_cb, NULL, shift_cents_dsp_cb,
	ps, NULL, NULL, NULL);
    endpoint_set_allowed_range(
	&ps->shift_cents_ep,
	(Value){.double_v = -1200.0},
	(Value){.double_v = 1200.0});
    endpoint_set_default_value(&ps->shift_cents_ep, (Value){.double_v = 0.0});

    endpoint_init(
	&ps->shift_semitones_ep,
        &ps->shift_semitones,
	JDAW_INT,
	"shift_semitones",
	"Coarse (semitones)",
	JDAW_THREAD_DSP,
	component_gui_cb, NULL, shift_semitones_dsp_cb,
	ps, NULL, NULL, NULL);
    endpoint_set_allowed_range(&ps->shift_semitones_ep, (Value){.int_v = -12}, (Value){.int_v = 12});
    endpoint_set_default_value(&ps->shift_semitones_ep, (Value){.int_v = 0});

    endpoint_init(
	&ps->shift_fine_ep,
        &ps->shift_fine,
	JDAW_DOUBLE,
	"shift_fine",
	"Fine (cents)",
	JDAW_THREAD_DSP,
	component_gui_cb, NULL, shift_fine_dsp_cb,
	ps, NULL, NULL, NULL);
    endpoint_set_allowed_range(&ps->shift_fine_ep, (Value){.double_v = -100.0}, (Value){.double_v = 100.0});
    endpoint_set_default_value(&ps->shift_fine_ep, (Value){.double_v = 0});

			       
    endpoint_init(
	&ps->quality_ep,
	&ps->quality,
	JDAW_DOUBLE,
	"phase_fidelity_vs_freq_fidelity",
	"Phase fidelity vs freq fidelity",
	JDAW_THREAD_DSP,
	component_gui_cb, NULL, quality_dsp_cb,
	ps, NULL, NULL, NULL);
    endpoint_set_allowed_range(
	&ps->quality_ep,
	(Value){.double_v = 0.01},
	(Value){.double_v = 0.99});
    endpoint_set_default_value(&ps->quality_ep, (Value){.double_v = 0.15});

    api_endpoint_register(&ps->shift_cents_ep, &ps->effect->api_node);
    api_endpoint_register(&ps->shift_semitones_ep, &ps->effect->api_node);
    api_endpoint_register(&ps->shift_fine_ep, &ps->effect->api_node);
    api_endpoint_register(&ps->quality_ep, &ps->effect->api_node);    
}

void pitch_shifter_deinit(PitchShifter *ps)
{
    mod_delay_deinit(&ps->mdL);
    mod_delay_deinit(&ps->mdR);
}
