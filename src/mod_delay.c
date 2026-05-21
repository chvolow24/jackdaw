#include <math.h>
#include <stdlib.h>
#include "consts.h"
#include "dsp_utils.h"
#include "mod_delay.h"
#include "osc.h"
#include "session.h"

void mod_delay_buf(ModDelay *md, float *restrict buf_in, int len)
{

    float osc_bufs[md->num_taps][len];
    for (int t=0; t<md->num_taps; t++) {
	osc_generic_get_buf(&md->taps[t].osc, osc_bufs[t], len);
    }
    /* After osc applies linear ramp across length of its buf, reset phase incr on parent */
    if (md->phase_incr != md->dst_phase_incr) {
	md->phase_incr = md->dst_phase_incr;
    }

    float outbuf[len];

    const float amp_change_coeff = 0.0008;
    for (int i=0; i<len; i++) {

	/* Move amp_samples and center_samples in a lowpassed way
	 Signald by md->dst_amp_samples > 0 (normal == -1) */
	if (md->dst_amp_samples > 0) {
	    md->amp_samples = amp_change_coeff * md->dst_amp_samples + (1 - amp_change_coeff) * md->amp_samples;
	    md->center_samples = amp_change_coeff * md->dst_center_samples + (1 - amp_change_coeff) * md->center_samples;
	    if (fabs(md->amp_samples - md->dst_amp_samples) < 1.0) {
		md->amp_samples = md->dst_amp_samples;
		md->center_samples = md->dst_center_samples;
		md->dst_amp_samples = -1;
	    }
	}
	for (int t=0; t<md->num_taps; t++) {
	    ModDelayTap *tap = md->taps + t;
	    double read_i = md->mem_index - md->center_samples - md->center_samples * osc_bufs[t][i];
	    if (md->amp_samples > 1) {
		while (read_i >= md->amp_samples) {
		    read_i -= md->amp_samples;
		}
		while (read_i < 0) {
		    read_i += md->amp_samples;
		}
	    }
	    
	    int32_t left_i = floor(read_i);
	    double ldiff = read_i - left_i;
	    int32_t right_i;
	    if (left_i >= floor(md->amp_samples)) {
		right_i = 0;
	    } else {
		right_i = left_i + 1;
	    }
	    
	    float left = md->mem[left_i];
	    float right = md->mem[right_i];
	    float m = right - left;
	    float out = left + m * ldiff;

	    if (tap->osc.type == OSC_SAW_UP || tap->osc.type == OSC_SAW_DOWN) {
		double window = 0.5 + cos(PI * osc_bufs[t][i]) / 2.0;
		out *= window;
	    }
	    if (t==0) {
		outbuf[i] = out;
	    } else {
		outbuf[i] += out;
	    }
	}
	
	md->mem[md->mem_index] = buf_in[i];
	md->mem_index++;
	/* Mem is valid up through (incl) floor(md->amp_samples) */
	if (md->mem_index > floor(md->amp_samples)) {
	    md->mem_index = 0;
	}
    }
    memcpy(buf_in, outbuf, len * sizeof(float));
    md->phase += md->phase_incr * len;
    
    while (md->phase < 0) {
	md->phase += TAU;
    }
    while (md->phase > TAU) {
	md->phase -= TAU;
    }
}



void mod_delay_init(ModDelay *md, int32_t max_len, double init_amp, double init_freq, int num_taps, OscType t)
{
    md->mem = calloc(max_len, sizeof(float));
    md->mem_index = 0;
    md->max_len = max_len;
    md->amp = init_amp;
    md->amp_samples = init_amp * max_len;
    md->center_samples = md->amp_samples / 2.0;
    
    /* md->osc.phase = 0.0; */
    md->num_taps = num_taps;
    for (int i=0; i<num_taps; i++) {
	/* md->taps[i].mem = calloc(max_len, sizeof(float)); */
	/* md->taps[i].mem_index = 0; */
	osc_init(
	    &md->taps[i].osc,
	    t,
	    init_freq,
	    &md->phase,
	    &md->phase_incr,
	    &md->dst_phase_incr,
	    TAU * (double)i / num_taps);
	    
    }
    /* md->osc.type = OSC_SAW_UP; */
    /* md->osc.freq_hz = init_freq; */
    /* md->osc.phase_incr = init_freq * TAU / session_get_sample_rate(); */

    md->freq_hz = init_freq;
    md->phase_incr = init_freq * TAU / session_get_sample_rate();
}

void mod_delay_set_amp(ModDelay *md, double new_amp)
{
/*     double ratio = new_amp / md->amp; */
/*     fprintf(stderr, "AMP %f->%f\n", md->amp, new_amp); */
/*     fprintf(stderr, "MEMIND: %d->%d\n", md->mem_index, (int32_t)(md->mem_index * ratio)); */
    md->amp = new_amp;
    /* int32_t new_mem_index = md->mem_index * ratio; */
    /* md->mem_index = new_mem_index; */
    md->dst_amp_samples = new_amp * md->max_len;
    md->dst_center_samples = md->dst_amp_samples / 2;
}

void mod_delay_set_freq(ModDelay *md, double new_freq_hz)
{
    md->freq_hz = new_freq_hz;
    /* md->phase_incr = new_freq_hz * TAU / session_get_sample_rate(); */
    md->dst_phase_incr = new_freq_hz * TAU / session_get_sample_rate();

}

void mod_delay_clear(ModDelay *md)
{
    memset(md->mem, 0, md->max_len * sizeof(float));
    if (md->dst_amp_samples > 0) {
	md->amp_samples = md->dst_amp_samples;
	md->center_samples = md->dst_center_samples;
	md->dst_amp_samples = -1;
    }
    if (md->dst_phase_incr != md->phase_incr) {
	md->phase_incr = md->dst_phase_incr;
    }
    md->phase = 0.0;
    for (int i=0; i<md->num_taps; i++) {
	md->taps[i].osc.phase = 0.0;
    }
}

void mod_delay_deinit(ModDelay *md)
{
    free(md->mem);
}
