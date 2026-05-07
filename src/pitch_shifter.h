#ifndef JDAW_PITCH_SHIFTER_H
#define JDAW_PITCH_SHIFTER_H

#include "mod_delay.h"

typedef struct pitch_shifter {
    double shift_cents;
    double low_latency_vs_quality; // 0.0 =  minimize latency; 1 = maximize quality
    ModDelay mdL;
    ModDelay mdR;
} PitchShifter;





#endif
