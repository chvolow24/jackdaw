#include <math.h>
#include <stdlib.h>
#include "consts.h"
#include "mod_delay.h"
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
    int32_t mem_index;
    fprintf(stderr, "\n");
    float saved_in[len];
    memcpy(saved_in, buf_in, sizeof(float) * len);
    for (int t=0; t<md->num_taps; t++) {
	OscGeneric *osc = &md->taps[t].osc;
	float osc_buf[len];
	osc_generic_get_buf(osc, osc_buf, len);
	mem_index = md->mem_index;
	fprintf(stderr, "T %d, mem_index %d (osc 1,2 %f %f\n", t, mem_index, osc_buf[0], osc_buf[1]);
	for (int i=0; i<len; i++) {
	    double read_i = mem_index - md->center_samples - (md->center_samples - 1.0) * osc_buf[i];
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
	    if (t == md->num_taps - 1) {
		md->mem[mem_index] = buf_in[i];
	    }
	    
	    mem_index++;
	    if (mem_index >= floor(md->amp_samples)) {
		mem_index -= md->amp_samples;
	    }

	    if (osc->type == OSC_SAW_UP || osc->type == OSC_SAW_DOWN) {
		double window = 0.5 + cos(PI * osc_buf[i]) / 2.0;
		out *= window;
	    }
	    if (t==0) {
		buf_in[i] = out;
	    } else {
		buf_in[i] += out;
	    }
	}
	/* if (osc->type == OSC_SAW_UP || osc->type ==  OSC_SAW_DOWN) { */
	/*     for (int i=0; i<len; i++) { */
	/* 	double window = 0.5 + cos(PI * osc_buf[i]) / 2.0; */
	/* 	buf_in[i] *= window; */
	/*     } */
	/* } */
    }
    md->mem_index = mem_index;
    md->phase += md->phase_incr * len;
    while (md->phase > TAU) md->phase -= TAU;

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
	osc_init(
	    &md->taps[i].osc,
	    t,
	    init_freq,
	    &md->phase,
	    &md->phase_incr,
	    /* i == 0 ? NULL : &md->taps[0].osc.phase, */
	    /* i == 0 ? NULL : &md->taps[0].osc.phase_incr, */
	    TAU * i / num_taps);
	    
    }
    /* md->osc.type = OSC_SAW_UP; */
    /* md->osc.freq_hz = init_freq; */
    /* md->osc.phase_incr = init_freq * TAU / session_get_sample_rate(); */

    md->freq_hz = init_freq;
    md->phase_incr = init_freq * TAU / session_get_sample_rate();
}

