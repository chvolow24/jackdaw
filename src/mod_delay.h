#ifndef JDAW_MOD_DELAY_H
#define JDAW_MOD_DELAY_H

#include <stdint.h>
#include "osc.h"

#define MAX_MOD_DELAY_TAPS 8

typedef struct mod_delay ModDelay;

typedef struct mod_delay_tap {
    ModDelay *parent;
    OscGeneric osc;
} ModDelayTap;

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
    /* OscGeneric osc; */
    
    int num_taps;
    ModDelayTap taps[MAX_MOD_DELAY_TAPS];
    
    double phase;
} ModDelay;

void mod_delay_buf(ModDelay *md, float *restrict buf_in, int len);
float mod_delay_sample(ModDelay *md, float in);
void mod_delay_init(ModDelay *md, int32_t max_len, double init_amp, double init_freq, int num_taps, OscType t);
#endif
