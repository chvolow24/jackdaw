#include <math.h>
#include "mod_delay.h"
#include "pitch_shifter.h"
#include "session.h"
#define MAX_PS_DELAY_LEN (session_get_sample_rate() / 2)

static double get_md_freq_from_params(double shift_cents, double amp)
{
    double freq_ratio = pow(2.0, shift_cents / 100.0);
    double freq = 2.0 * (freq_ratio - 1.0) / amp;
    return freq;
}


void pitch_shifter_init(PitchShifter *ps)
{
    ps->shift_cents = 0;
    ps->low_latency_vs_quality = 0.15;
    mod_delay_init(
	&ps->mdL,
	MAX_PS_DELAY_LEN,
	ps->low_latency_vs_quality,
	0.0,
	2,
	OSC_SAW_DOWN);
    mod_delay_init(
	&ps->mdL,
	MAX_PS_DELAY_LEN,
	ps->low_latency_vs_quality,
	0.0,
	2,
	OSC_SAW_DOWN); 
}

void pitch_shifter_set_shift_amt(PitchShifter *ps, double shift_cents)
{
    ps->shift_cents = shift_cents;
    double freq = get_md_freq_from_params(shift_cents, ps->low_latency_vs_quality);
    /* mod_delay_set_amp( */
    mod_delay_set_freq(&ps->mdL, freq);
    mod_delay_set_freq(&ps->mdR, freq);    
}

void pitch_shifter_set_llvq(PitchShifter *ps, double llvq)
{
    ps->low_latency_vs_quality = llvq;
    double freq = get_md_freq_from_params(ps->shift_cents, llvq);
    mod_delay_set_amp(&ps->mdL, llvq);
    mod_delay_set_amp(&ps->mdR, llvq);
    mod_delay_set_freq(&ps->mdL, freq);
    mod_delay_set_freq(&ps->mdR, freq);    
}
