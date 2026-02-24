#ifndef JDAW_SCHROEDER_H
#define JDAW_SCHROEDER_H

#include "allpass.h"

#define MAX_PARALLEL_LOP_DELAYS 16

typedef struct schroeder {
    AllpassGroup series_aps[2];
    LopDelay parallel_lop_delays[2][MAX_PARALLEL_LOP_DELAYS];
    int num_parallel_lop_delays;
} Schroeder;

void schroeder_init_freeverb(Schroeder *sch);
float schroeder_buf_apply(void *sch_v, float *restrict in_L, float *restrict in_R, int len, float input_amp);

/* TESTIG ONLY */
void schroeder_toggle_do_allpass();
void schroeder_toggle_do_lop_delay();

#endif
