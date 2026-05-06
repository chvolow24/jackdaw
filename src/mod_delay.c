#include <math.h>
#include <stdlib.h>
#include "consts.h"
#include "mod_delay.h"
#include "session.h"

void breakfn();
float mod_delay_sample(ModDelay *md, float in)
{
    breakfn();
    double read_i = md->mem_index - md->center_samples - (md->center_samples - 1.0) * sin(md->phase);
    /* int num = 0; */
    if (md->amp_samples > 1) {
	while (read_i > md->amp_samples) {
	    read_i -= md->amp_samples;
	    /* num++; */
	}
	while (read_i < 0) {
	    read_i += md->amp_samples;
	    /* num++; */
	}
    }
    /* if (num == 2) exit(1); */
    int32_t left_i = floor(read_i);
    double ldiff = read_i - left_i;
    int32_t right_i;
    if (left_i >= floor(md->amp_samples) - 1) {
	right_i = 0;
	fprintf(stderr, "read i: %f, l:r %d, %d\n", read_i, left_i, right_i);
    } else {
	right_i = left_i + 1;
    }
    static int mod = 0;
    double diff = md->mem_index - read_i;
    if (diff > 0 && diff < 0.1 && mod % 5 == 0) {
	fprintf(stderr, "diff %f\n", diff);
	fprintf(stderr, "PHASE %1.2f mem index: %03d, read_i %f\n", md->phase, md->mem_index, read_i);
	fprintf(stderr, "\t->L R? %d %d\n", left_i, right_i);
    }
    mod++;
    /* fprintf(stderr, "%f -> %f\n", md->phase, (sin(md->phase) + 1)/ 2); */
    /* fprintf(stderr, "%f %f\n", md->phase, sin(md->phase)); */
    float left = md->mem[left_i];
    float right = md->mem[right_i];
    float m = right - left;
    float out = left + m * ldiff;
    /* fprintf(stderr, "mem_i: %d, %f Read pair: %d, %d\n", md->mem_index, read_i, left_i, right_i); */
    /* fprintf(stderr, "Amp samples: %f\n\tmemindex %d sin %f read index %f pair %d %d out %f\n", md->amp_samples, md->mem_index, sin(md->phase), read_i, left_i, right_i, out); */
    /* if (fabs(out) < 1e-9) { */
    /* 	fprintf(stderr, "\tOUT == 0: left %f, right %f, m %f\n", left, right, m); */
    /* } */
    
    /* fprintf(stderr, "Set mem %d %f\n", md->mem_index, in); */
    md->mem[md->mem_index] = in;
    md->mem_index++;
    if (md->mem_index >= floor(md->amp_samples)) {
	md->mem_index -= md->amp_samples;
    }
    md->phase += md->phase_incr;
    if (md->phase >= TAU) md->phase -= TAU;
    
    return out;
}

void mod_delay_init(ModDelay *md, int32_t max_len, double init_amp, double init_freq)
{
    md->mem = calloc(max_len, sizeof(float));
    md->mem_index = 0;
    md->max_len = max_len;
    md->amp = init_amp;
    md->amp_samples = init_amp * max_len;
    md->center_samples = md->amp_samples / 2.0;

    md->freq_hz = init_freq;
    double bandwidth_samples = session_get_sample_rate() / init_freq;
    md->phase_incr = TAU / bandwidth_samples;
    
}

