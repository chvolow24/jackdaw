#ifndef JDAW_SYNTH_H
#define JDAW_SYNTH_H

#include <stdint.h>
#include "midi_io.h"
#include "consts.h"

#define SYNTH_NUM_VOICES 32
#define SYNTHVOICE_NUM_OSCS 40 /* 4 base oscillators, up to 5 per base for detune */

typedef enum wave_shape {
    WS_SINE,
    WS_SQUARE,
    WS_TRI,
    WS_SAW,
    WS_OTHER,
    NUM_WAVE_SHAPES
} WaveShape;

float WAV_TABLES[NUM_WAVE_SHAPES];

typedef struct osc {
    WaveShape type;
    double phase[2];
    double sample_phase_incr;
    double freq;
    float amp;
    float pan;
} Osc;

typedef struct synth Synth;

typedef struct synth_voice {
    Osc oscs[SYNTHVOICE_NUM_OSCS];
    uint8_t note_val;
    uint8_t velocity;
    int32_t note_start_rel[2]; /* relative to start of current chunk */
    int32_t note_end_rel[2]; /* relative to start of current chunk */
    Synth *synth;
    bool available;
    uint8_t amp_env_stage[2];
    int32_t env_remaining[2];
    float release_start_env[2];
    float last_env[2];
} SynthVoice;

typedef struct adsr {
    int32_t a; /* Sample frames */
    int32_t d; /* Sample frames */
    float s; /* raw amplitude */
    int32_t r; /* Sample frames */
} ADSR;

typedef struct synth {
    SynthVoice voices[SYNTH_NUM_VOICES];
    uint8_t num_oscs;
    ADSR amp_env;
    PmEvent events[128];
    int num_events;
    
    bool monitor;
} Synth;

/* int synth_create_virtual_device(Synth *s); */
/* void synth_init_defaults(Synth *s); */
Synth *synth_create();
void synth_add_buf(Synth *s, float *buf, int channel, int32_t len, int32_t tl_start);

#endif
