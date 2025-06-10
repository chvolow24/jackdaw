#ifndef JDAW_ADSR_H
#define JDAW_ADSR_H

#include <stdbool.h>
#include <stdint.h>

enum adsr_stage {
    ADSR_A,
    ADSR_D,
    ADSR_S,
    ADSR_R,
    ADSR_UNINIT,
    ADSR_OVERRUN
};

typedef struct adsr_params {
    int32_t a; /* sample frames */
    int32_t d; /* sample frames */
    float s; /* raw amp */
    int32_t r; /* sample_frames */

    float *a_ramp;
    float *d_ramp;

    float ramp_exp;
    /* float *r_ramp; */
    /* r ramp not pre-calculated bc start amp not known*/
} ADSRParams;

typedef struct adsr_state {    
    enum adsr_stage current_stage;
    int32_t env_remaining;
    float release_start_env;
    ADSRParams *params;
    bool has_release_start;
    int32_t samples_before_release_start;
} ADSRState;


void adsr_set_params(ADSRParams *p, int32_t a, int32_t d, float s, int32_t r);
void adsr_get_chunk(ADSRState *adsr, float *dst, int dst_len);
void adsr_apply_chunk(ADSRState *adsr, float *buf, int buf_len);





#endif
