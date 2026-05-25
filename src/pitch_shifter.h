#ifndef JDAW_PITCH_SHIFTER_H
#define JDAW_PITCH_SHIFTER_H

#include "effect.h"
#include "endpoint.h"
#include "mod_delay.h"

typedef struct pitch_shifter {
    Effect *effect;

    double shift_cents; // final sum
    double shift_cents_internal; // ep target, added to other shifters
    
    int shift_semitones;
    double shift_fine;
    double quality; // 0.0 = phase coherence; 1.0 = frequency coherence (less comb filtering)
    ModDelay mdL;
    ModDelay mdR;

    Endpoint shift_cents_ep;
    Endpoint shift_semitones_ep;
    Endpoint shift_fine_ep;
    Endpoint quality_ep;
    
    
} PitchShifter;

void pitch_shifter_init(PitchShifter *ps);

void pitch_shifter_set_quality(PitchShifter *ps, double quality);
void pitch_shifter_set_shift_amt(PitchShifter *ps, double shift_cents);
float pitch_shifter_buf_apply(void *ps_v, float *restrict L, float *restrict R, int len, float input_amp);
void pitch_shifter_clear(PitchShifter *ps);
void pitch_shifter_deinit(PitchShifter *ps);




#endif
