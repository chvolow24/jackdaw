#ifndef JDAW_OSC_H
#define JDAW_OSC_H

#include <stdbool.h>

/* Note:
   synth.h does not use this module, and instead has its own
   internal oscillator implementations. May replace later.
*/

typedef enum osc_type {
    OSC_SINE,
    OSC_SQUARE,
    OSC_TRI,
    OSC_SAW_UP,
    OSC_SAW_DOWN
} OscType;

typedef struct osc_generic {
    OscType type;
    double phase;
    double phase_incr; // computed from freq
    double *external_phase; // if not NULL, internal phase and phase incr are ignored
    double *external_phase_incr;
    double phase_shift; // relative to external    
    double freq_hz;
} OscGeneric;

void osc_init(
    OscGeneric *osc,
    OscType type,
    double freq_hz,
    double *external_phase, // If NULL, external phase not tracked
    double *external_phase_incr,
    double phase_shift
);
    
    
    

void osc_generic_get_buf(OscGeneric *osc, float *restrict buf, int len);

#endif
