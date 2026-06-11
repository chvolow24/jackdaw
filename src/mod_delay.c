#include <math.h>
#include <stdlib.h>
#include "consts.h"
#include "mod_delay.h"
#include "osc.h"
#include "session.h"

#define MD_SPACER_SAMPLES 1

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
    int32_t total_rb_len;
    if (md->dst_amp_samples > 0) {
	total_rb_len = MD_SPACER_SAMPLES * 2 + ceil(md->dst_amp_samples);
	int32_t old_len = MD_SPACER_SAMPLES * 2 + ceil(md->amp_samples);
	double ratio = md->amp_samples / md->dst_amp_samples;
	md->mem_index /= ratio;
	md->amp_samples = md->dst_amp_samples;
	float new[total_rb_len];
	for (int32_t i=0; i<total_rb_len; i++) {
	    double old_i = (double)i * old_len / total_rb_len;
	    int32_t left_i = floor(old_i);
	    float left = md->mem[left_i];
	    float right = md->mem[left_i + 1];
	    float diff_left = old_i - left_i;
	    new[i] = left + (right - left) * diff_left;;
	}
	memcpy(md->mem, new, sizeof(new));

	md->center_samples = md->dst_center_samples;
	md->dst_amp_samples = -1;
    } else {
	total_rb_len = MD_SPACER_SAMPLES * 2 + ceil(md->amp_samples);
    }
    for (int i=0; i<len; i++) {
	for (int t=0; t<md->num_taps; t++) {
	    ModDelayTap *tap = md->taps + t;
	    double read_i_rel = MD_SPACER_SAMPLES + md->center_samples * (1.0 - osc_bufs[t][i]);
	    double read_i = md->mem_index + read_i_rel;
 	    while (read_i >= total_rb_len) {
		read_i -= total_rb_len;
	    }
	    while (read_i < 0) {
		read_i += total_rb_len;
	    }
	    
	    int32_t left_i = floor(read_i);
	    int32_t right_i = left_i + 1;	    
 
	    while (right_i >= total_rb_len) {
		right_i -= total_rb_len;
	    }

	    
	    double ldiff = read_i - left_i;
	    double left = md->mem[left_i];
	    double right = md->mem[right_i];
	    double m = right - left;
	    double out = left + m * ldiff;

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
	if (md->mem_index >= total_rb_len) {
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

    md->freq_hz = init_freq;
    md->phase_incr = init_freq * TAU / session_get_sample_rate();
}

void mod_delay_set_amp(ModDelay *md, double new_amp)
{

    if (new_amp * md->max_len + 2 * MD_SPACER_SAMPLES >= md->max_len) {
	new_amp = ((double)md->max_len - 2 * MD_SPACER_SAMPLES - 1) / md->max_len;
    }

    if (new_amp < 0.0) new_amp = 0.0;
    md->amp = new_amp;
    
    md->dst_amp_samples = new_amp * md->max_len;
    md->dst_center_samples = md->dst_amp_samples / 2;
}


void mod_delay_set_freq(ModDelay *md, double new_freq_hz)
{
    md->freq_hz = new_freq_hz;
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
