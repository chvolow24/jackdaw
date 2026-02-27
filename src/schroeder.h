#ifndef JDAW_SCHROEDER_H
#define JDAW_SCHROEDER_H

#include "allpass.h"

#define MAX_PARALLEL_LOP_DELAYS 16

typedef struct schroeder {
    float dry;
    float stereo_spread;
    float lop_coeff;
    float lop_delay_coeff;
    float allpass_coeff;
    AllpassGroup series_aps[2];
    LopDelay parallel_lop_delays[2][MAX_PARALLEL_LOP_DELAYS];
    int num_parallel_lop_delays;
} Schroeder;

void schroeder_init_freeverb(Schroeder *sch);
float schroeder_buf_apply(void *sch_v, float *restrict in_L, float *restrict in_R, int len, float input_amp);



/* TESTIG ONLY */
void schroeder_toggle_do_allpass();
void schroeder_toggle_do_lop_delay();

void schroeder_set_lop_coeff(Schroeder *sch, float new);
void schroeder_set_lop_delay_coeff(Schroeder *sch, float new);
void schroeder_set_allpass_coeff(Schroeder *sc, float new);

void schroeder_incr_lop_coeff(Schroeder *sch);
void schroeder_decr_lop_coeff(Schroeder *sch);
void schroeder_incr_lop_delay_coeff(Schroeder *sch);
void schroeder_decr_lop_delay_coeff(Schroeder *sch);
void schroeder_incr_allpass_coeff(Schroeder *sch);
void schroeder_decr_allpass_coeff(Schroeder *sch);
void schroeder_incr_dry(Schroeder *sch);
void schroeder_decr_dry(Schroeder *sch);
void schroeder_incr_stereo_spread(Schroeder *sch);
void schroeder_decr_stereo_spread(Schroeder *sch);

#endif
