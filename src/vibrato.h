#ifndef JDAW_VIBRATO_H
#define JDAW_VIBRATO_H

#include "effect.h"
#include "endpoint.h"
#include "mod_delay.h"

typedef struct vibrato {
    Effect *effect;

    double freq_hz;
    double freq_ctrl; /* 0.0 - 1.0 */
    double depth; /* depth_ctrl ^ 2.0 */
    double depth_ctrl;

    ModDelay mdL;
    ModDelay mdR;

    Endpoint freq_hz_ep;
    Endpoint depth_ep;
} Vibrato;

void vibrato_init(Vibrato *vib);
float vibrato_buf_apply(void *vib_v, float *restrict L, float *restrict R, int len, float input_amp);
void vibrato_clear(Vibrato *vib);
void vibrato_deinit(Vibrato *vib);

#endif
