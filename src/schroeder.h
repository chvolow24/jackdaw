#ifndef JDAW_SCHROEDER_H
#define JDAW_SCHROEDER_H

#include "allpass.h"
#include "effect.h"
#include "endpoint.h"

/* #define MAX_PARALLEL_LOP_DELAYS 16 */
#define SCHROEDER_NUM_PARALLEL_LOP_DELAYS 8
#define MAX_PREDELAY_SFRAMES 22050

typedef struct schroeder {
    Effect *effect;    
    float wet;
    float stereo_spread; // range 0->1
    float panscale_left; // (0.5f + stereo_spread / 2)
    float panscale_right; // (0.5f - stereo_spread / 2)
    float lop_coeff;
    float lop_delay_coeff;
    float allpass_coeff;
    AllpassGroup series_aps[2];
    LopDelay parallel_lop_delays[2][SCHROEDER_NUM_PARALLEL_LOP_DELAYS];
    int32_t predelay_sframes;
    

    float lop_delay_coeff_raw; // lop_delay_coeff = exp(delay_len_scalar * ln(lop_delay_coeff_raw));
    float decay_time; // lop_delay_coeff_raw = 0.5 + (sqrt(decay_time)) / 2
    float brightness; // lop_coeff = brightness
    float delay_len_scalar; // range 0->1
    float predelay_msec;
    Endpoint decay_time_ep;
    Endpoint brightness_ep;
    Endpoint stereo_spread_ep;
    Endpoint delay_len_scalar_ep;
    Endpoint predelay_ep;
    Endpoint wet_ep;
    /* int num_parallel_lop_delays;     */
    int32_t predelay_index;
    int32_t predelay_len;
    float predelay_buf[2][MAX_PREDELAY_SFRAMES];
} Schroeder;

void schroeder_init_freeverb(Schroeder *sch);
float schroeder_buf_apply(void *sch_v, float *restrict in_L, float *restrict in_R, int len, float input_amp);

void schroeder_clear(Schroeder *sch);
void schroeder_deinit(Schroeder *sch);



/* TESTIG ONLY */
/* void schroeder_toggle_do_allpass(); */
/* void schroeder_toggle_do_lop_delay(); */

/* void schroeder_set_lop_coeff(Schroeder *sch, float new); */
/* void schroeder_set_lop_delay_coeff(Schroeder *sch, float new); */
/* void schroeder_set_allpass_coeff(Schroeder *sc, float new); */

/* void schroeder_incr_lop_coeff(Schroeder *sch); */
/* void schroeder_decr_lop_coeff(Schroeder *sch); */
/* void schroeder_incr_lop_delay_coeff(Schroeder *sch); */
/* void schroeder_decr_lop_delay_coeff(Schroeder *sch); */
/* void schroeder_incr_allpass_coeff(Schroeder *sch); */
/* void schroeder_decr_allpass_coeff(Schroeder *sch); */
/* void schroeder_incr_dry(Schroeder *sch); */
/* void schroeder_decr_dry(Schroeder *sch); */
/* void schroeder_incr_stereo_spread(Schroeder *sch); */
/* void schroeder_decr_stereo_spread(Schroeder *sch); */

#endif
