/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    synth.h

    * Jackdaw's built-in polyphonic synth
    * fundamentally constructed from four "base oscillators," an amplitude envelope, and a resonant filter
    * base oscillators (type OscCfg) control up to SYNTH_MAX_UNISON_OSCS literal oscillators per voice
    * unison oscs under auspices of base oscillator are at voice->oscs[base_i + N * SYNTH_NUM_BASE_OSCS],
      where N<SYNTH_MAX_UNISON_OSCS
    * base oscs can point at eachother for frequency or amplitude modulation
    * (in which case unison voices are ignored)
    * ADSR envelopes described in adsr.h
 *****************************************************************************************************************/


#ifndef JDAW_SYNTH_H
#define JDAW_SYNTH_H

#include <stdint.h>
#include "adsr.h"
#include "api.h"
#include "endpoint.h"
#include "iir.h"
#include "midi_io.h"
/* #include "consts.h" */

#define SYNTH_NUM_VOICES 32
#define SYNTH_NUM_BASE_OSCS 4
#define SYNTH_MAX_UNISON_OSCS 5
#define SYNTHVOICE_NUM_OSCS (SYNTH_NUM_BASE_OSCS * SYNTH_MAX_UNISON_OSCS) /* 4 base oscillators, up to 5 per base for detune */

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

typedef struct osc Osc;
typedef struct osc {
    WaveShape type;
    double phase[2];
    double sample_phase_incr;
    double freq;
    float amp;
    float pan;
    Osc *freq_modulator;
    Osc *amp_modulator;

    float last_out[2];
    float last_in[2];
} Osc;

typedef struct synth Synth;

typedef struct synth_voice {
    Osc oscs[SYNTHVOICE_NUM_OSCS];
    uint8_t note_val;
    uint8_t velocity;
    Synth *synth;
    bool available;
    ADSRState amp_env[2]; /* L and R */
    ADSRState filter_env[2];

    IIRFilter filter;
} SynthVoice;

/* typedef struct adsr { */
/*     int32_t a; /\* Sample frames *\/ */
/*     int32_t d; /\* Sample frames *\/ */
/*     float s; /\* raw amplitude *\/ */
/*     int32_t r; /\* Sample frames *\/ */
/* } ADSR; */

struct unison_cfg {
    int num_voices; /* always even -- central frequency and sidebands on either side */
    float detune_cents; /* divergence from central pitch of first voice, or between center pitches if even */
    float relative_amp; /* 1.0 for same amp as main voice */
    float stereo_spread; /* 0.0 all centered; 1.0 full range */
    Endpoint num_voices_ep;
    Endpoint detune_cents_ep;
    Endpoint relative_amp_ep;
    Endpoint stereo_spread_ep;
    
    char *num_voices_id;
    char *detune_cents_id;
    char *relative_amp_id;
    char *stereo_spread_id;


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
    struct unison_cfg unison;

    /* If modulation pointers are not null, do not add audio data directly from this osc */
    OscCfg *mod_freq_of;
    int fmod_dropdown_reset;
    OscCfg *mod_amp_of;
    int amod_dropdown_reset;

    OscCfg *freq_mod_by;
    OscCfg *amp_mod_by;

    Endpoint active_ep;
    Endpoint type_ep;
    Endpoint amp_ep;
    Endpoint pan_ep;
    Endpoint octave_ep;
    Endpoint tune_coarse_ep;
    Endpoint tune_fine_ep;

    char *active_id;
    char *type_id;
    char *amp_id;
    char *pan_id;
    char *octave_id;
    char *tune_coarse_id;
    char *tune_fine_id;
    
    /* Endpoint  */    
    APINode api_node;
    
} OscCfg;

typedef struct synth {
    SynthVoice voices[SYNTH_NUM_VOICES];    
    OscCfg base_oscs[SYNTH_NUM_BASE_OSCS];
    APINode api_node;
    ADSRParams amp_env;

    /* Filter */
    bool filter_active;
    float freq_scalar;
    float resonance;
    float velocity_freq_scalar;
    APINode filter_node;
    bool use_amp_env;
    ADSRParams filter_env;
    Endpoint filter_active_ep;
    Endpoint freq_scalar_ep;
    Endpoint resonance_ep;
    Endpoint velocity_freq_scalar_ep;
    Endpoint use_amp_env_ep;
    
    bool monitor;
    bool allow_voice_stealing;
    Page *osc_page; /* For GUI callback targeting */
    Page *amp_env_page;
    Page *filter_page;

    Track *track;

    float vol;
    float pan;
    Endpoint vol_ep;
    Endpoint pan_ep;

    IIRFilter dc_blocker; /* internal use only */
} Synth;

/* int synth_create_virtual_device(Synth *s); */
/* void synth_init_defaults(Synth *s); */
Synth *synth_create(Track *track);
/* void synth_add_buf(Synth *s, float *buf, int channel, int32_t len, int32_t tl_start); */
/* void synth_add_buf(Synth *s, float *buf, int channel, int32_t len, int32_t tl_start, bool send_immediate, float step); */
void synth_feed_midi(Synth *s, PmEvent *events, int num_events, int32_t tl_start, bool send_immediate);
void synth_add_buf(Synth *s, float *buf, int channel, int32_t len, float step);
void synth_close_all_notes(Synth *s);

/* Return 0 for success, 1 for unset (carrier NULL), < 0 for error */
int synth_set_freq_mod_pair(Synth *s, OscCfg *carrier_cfg, OscCfg *modulator_cfg);

/* Return 0 for success, 1 for unset (carrier NULL), < 0 for error */
int synth_set_amp_mod_pair(Synth *s, OscCfg *carrier_cfg, OscCfg *modulator_cfg);

#endif
