#ifndef JDAW_PITCH_SHIFTER_H
#define JDAW_PITCH_SHIFTER_H

#include "mod_delay.h"

typedef struct pitch_shifter {
    double shift_cents;
    double low_latency_vs_quality; // 0.0 =  minimize latency; 1 = maximize quality
    ModDelay mdL;
    ModDelay mdR;
} PitchShifter;

void pitch_shifter_init(PitchShifter *ps);

void pitch_shifter_set_llvq(PitchShifter *ps, double llvq);
void pitch_shifter_set_shift_amt(PitchShifter *ps, double shift_cents);

float pitch_shifter_buf_apply(PitchShifter *ps, float *restrict L, float *restrict R, int len, float input_amp);





#endif
