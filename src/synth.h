#ifndef JDAW_SYNTH_H
#define JDAW_SYNTH_H

#include <stdint.h>
#include "adsr.h"
#include "iir.h"
#include "midi_io.h"
/* #include "consts.h" */

#define SYNTH_NUM_VOICES 32
#define SYNTH_NUM_BASE_OSCS 4
#define SYNTH_MAX_DETUNE_OSCS 5
#define SYNTHVOICE_NUM_OSCS (SYNTH_NUM_BASE_OSCS * SYNTH_MAX_DETUNE_OSCS) /* 4 base oscillators, up to 5 per base for detune */

/* #define SYNTH_EVENT_BUF_SIZE 512 */

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
    Synth *synth;
    bool available;
    ADSRState amp_env[2]; /* L and R */
} SynthVoice;

/* typedef struct adsr { */
/*     int32_t a; /\* Sample frames *\/ */
/*     int32_t d; /\* Sample frames *\/ */
/*     float s; /\* raw amplitude *\/ */
/*     int32_t r; /\* Sample frames *\/ */
/* } ADSR; */

struct detune_cfg {
    int num_voices; /* always even -- central frequency and sidebands on either side */
    float cents; /* divergence from central pitch of first voice, or between center pitches if even */
    float relative_amp; /* 1.0 for same amp as main voice */
    float stereo_spread; /* 0.0 all centered; 1.0 full range */

};
typedef struct osc_cfg OscCfg;
typedef struct osc_cfg {
    bool active;
    WaveShape type;
    float amp;
    float pan;
    int octave;
    int tune_coarse; /* semitones */
    float tune_fine; /* cents, -50-50 */

    /* Detune info */
    struct detune_cfg detune;

    /* If modulation pointers are not null, do not add audio data directly from this osc */
    OscCfg *mod_freq_of;
    OscCfg *mod_amp_of;

    OscCfg *freq_mod_by;
    OscCfg *amp_mod_by;
    
} OscCfg;

typedef struct synth {    
    SynthVoice voices[SYNTH_NUM_VOICES];    
    OscCfg base_oscs[SYNTH_NUM_BASE_OSCS];    
    ADSRParams amp_env;

    /* Filter */
    IIRFilter filter;
    ADSRParams filter_env;
    
    bool monitor;
    bool allow_voice_stealing;
} Synth;

/* int synth_create_virtual_device(Synth *s); */
/* void synth_init_defaults(Synth *s); */
Synth *synth_create();
/* void synth_add_buf(Synth *s, float *buf, int channel, int32_t len, int32_t tl_start); */
/* void synth_add_buf(Synth *s, float *buf, int channel, int32_t len, int32_t tl_start, bool send_immediate, float step); */
void synth_feed_midi(Synth *s, PmEvent *events, int num_events, int32_t tl_start, bool send_immediate);
void synth_add_buf(Synth *s, float *buf, int channel, int32_t len, float step);
void synth_close_all_notes(Synth *s);

#endif


/*

 |  + + + + | + + + + | + + + + | + + + + |

 */
