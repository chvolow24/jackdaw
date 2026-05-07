#include <math.h>
#include <stdlib.h>
#include "consts.h"
#include "dsp_utils.h"
#include "mod_delay.h"
#include "osc.h"
#include "session.h"

/* float mod_delay_sample(ModDelay *md, float in) */
/* { */
/*     double read_i = md->mem_index - md->center_samples - (md->center_samples - 1.0) * sin(md->phase); */
/*     if (md->amp_samples > 1) { */
/* 	while (read_i > md->amp_samples) { */
/* 	    read_i -= md->amp_samples; */
/* 	} */
/* 	while (read_i < 0) { */
/* 	    read_i += md->amp_samples; */
/* 	} */
/*     } */
/*     int32_t left_i = floor(read_i); */
/*     double ldiff = read_i - left_i; */
/*     int32_t right_i; */
/*     if (left_i >= floor(md->amp_samples) - 1) { */
/* 	right_i = 0; */
/*     } else { */
/* 	right_i = left_i + 1; */
/*     } */

/*     float left = md->mem[left_i]; */
/*     float right = md->mem[right_i]; */
/*     float m = right - left; */
/*     float out = left + m * ldiff; */
    
/*     md->mem[md->mem_index] = in; */
/*     md->mem_index++; */
/*     if (md->mem_index >= floor(md->amp_samples)) { */
/* 	md->mem_index -= md->amp_samples; */
/*     } */
/*     md->phase += md->phase_incr; */
/*     if (md->phase >= TAU) md->phase -= TAU; */
    
/*     return out; */
/* } */

void mod_delay_buf(ModDelay *md, float *restrict buf_in, int len)
{
    /* int while_iters = 0; */
    /* clock_t c = clock(); */
    float osc_bufs[md->num_taps][len];
    for (int t=0; t<md->num_taps; t++) {
	osc_generic_get_buf(&md->taps[t].osc, osc_bufs[t], len);
    }
    
    float outbuf[len];
    for (int i=0; i<len; i++) {
	for (int t=0; t<md->num_taps; t++) {
	    ModDelayTap *tap = md->taps + t;
	    double read_i = md->mem_index - md->center_samples - (md->center_samples - 1.0) * osc_bufs[t][i];
	    /* fprintf(stderr, "MEM INDEX %d, center %f, osc_buf %f\n", tap->mem_index, md->center_samples, osc_buf[i]); */
	    if (md->amp_samples > 1) {
		while (read_i > md->amp_samples) {
		    read_i -= md->amp_samples;
		}
		while (read_i < 0) {
		    read_i += md->amp_samples;
		}
	    }
	    int32_t left_i = floor(read_i);
	    double ldiff = read_i - left_i;
	    int32_t right_i;
	    if (left_i >= floor(md->amp_samples) - 1) {
		right_i = 0;
	    } else {
		right_i = left_i + 1;
	    }

	    float left = md->mem[left_i];
	    float right = md->mem[right_i];
	    float m = right - left;
	    float out = left + m * ldiff;
	    md->mem[md->mem_index] = buf_in[i];

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
	if (md->mem_index >= floor(md->amp_samples)) {
	    md->mem_index -= md->amp_samples;
	}
    }
    memcpy(buf_in, outbuf, len * sizeof(float));
    md->phase += md->phase_incr * len;
    while (md->phase < 0) {
	md->phase += TAU;
    }
    while (md->phase > TAU) {
	/* while_iters++; */
	md->phase -= TAU;
    }
    /* double time_ms = (double)(clock() - c) / CLOCKS_PER_SEC; */
    /* fprintf(stderr, "TIME: %f, While iters %d\n", time_ms, while_iters); */

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
	    TAU * (double)i / num_taps);
	    
    }
    /* md->osc.type = OSC_SAW_UP; */
    /* md->osc.freq_hz = init_freq; */
    /* md->osc.phase_incr = init_freq * TAU / session_get_sample_rate(); */

    md->freq_hz = init_freq;
    md->phase_incr = init_freq * TAU / session_get_sample_rate();
}

void mod_delay_set_amp(ModDelay *md, float new_amp)
{
    md->amp = new_amp;
    md->amp_samples = new_amp * md->max_len;
    md->center_samples = md->amp_samples / 2;
}

void mod_delay_set_freq(ModDelay *md, float new_freq_hz)
{
    md->freq_hz = new_freq_hz;
    md->phase_incr = new_freq_hz * TAU / session_get_sample_rate();

}

