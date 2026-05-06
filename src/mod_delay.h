#ifndef JDAW_MOD_DELAY_H
#define JDAW_MOD_DELAY_H

#include <stdint.h>

typedef struct mod_delay {
    
    /* Ring buf */
    float *mem;
    int32_t mem_index;
    int32_t max_len;

    /* Params */
    double center_samples; // (calc'd) amp_samples / 2
    double phase_incr; // (calc'd)
    double amp_samples; // (calc'd) amp * max_len
    double amp; // (calc'd) prop of max_len
    double amp_msec; // exposed
    double freq_hz; // exposed
    
    double phase;
} ModDelay;

float mod_delay_sample(ModDelay *md, float in);
void mod_delay_init(ModDelay *md, int32_t max_len, double init_amp, double init_freq);

#endif
