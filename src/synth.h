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

#define SYNTH_NUM_VOICES 24
#define SYNTH_NUM_BASE_OSCS 4
#define SYNTH_MAX_UNISON_OSCS 7
#define SYNTHVOICE_NUM_OSCS (SYNTH_NUM_BASE_OSCS * SYNTH_MAX_UNISON_OSCS) /* 4 base oscillators, up to 5 per base for detune */
#define MAX_OSC_BUF_LEN 8192

/* #define SYNTH_EVENT_BUF_SIZE 512 */

typedef enum wave_shape {
    WS_SINE,
    WS_SQUARE,
    WS_TRI,
    WS_SAW,
    WS_OTHER,
    NUM_WAVE_SHAPES
} WaveShape;

/* float WAV_TABLES[NUM_WAVE_SHAPES]; */

typedef struct osc Osc;
typedef struct osc_cfg OscCfg;
typedef struct synth_voice SynthVoice;
typedef struct osc {
    float buf[MAX_OSC_BUF_LEN];
    bool active;
    OscCfg *cfg;
    SynthVoice *voice;
    WaveShape type;
    double phase;
    double sample_phase_incr;
    double freq;
    float amp;
    float pan;
    Osc *freq_modulator;
    Osc *amp_modulator;
    
    float detune_cents; /* Unison voice detune */
    float tune_cents; /* Octave + coarse + fine */
    float pan_offset; /* Unison stereo spread */
    float freq_last_sample;

    double sample_phase_incr_addtl; /* pitch bend */

    float last_out_tri;
    float last_in;
    IIRFilter tri_dc_blocker;
    /* int reset_params_ctr; */
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
    ADSRState noise_amt_env[2];

    bool note_off_deferred;

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
typedef struct osc_cfg {
    bool active;
    WaveShape type;
    float amp;
    /* float amp_unscaled; */
    float pan;
    int octave;
    int tune_coarse; /* semitones */
    float tune_fine; /* cents, -50-50 */
    
    bool fix_freq;
    float fixed_freq;
    float fixed_freq_unscaled;

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
    Endpoint fix_freq_ep;
    Endpoint fixed_freq_ep;
    Endpoint amp_ep;
    Endpoint pan_ep;
    Endpoint octave_ep;
    Endpoint tune_coarse_ep;
    Endpoint tune_fine_ep;

    /* For serialization only */
    int fmod_target; 
    Endpoint fmod_target_ep;
    int amod_target;
    Endpoint amod_target_ep;

    char *active_id;
    char *type_id;
    char *amp_id;
    char *pan_id;
    char *octave_id;
    char *tune_coarse_id;
    char *tune_fine_id;
    char *fmod_target_dropdown_id;
    char *amod_target_dropdown_id;
    char *fix_freq_id;
    char *fixed_freq_id;
    
    /* Endpoint  */    
    APINode api_node;
    
} OscCfg;

typedef struct synth {
    char preset_name[MAX_NAMELENGTH];
    SynthVoice voices[SYNTH_NUM_VOICES];    
    OscCfg base_oscs[SYNTH_NUM_BASE_OSCS];
    APINode api_node;
    ADSRParams amp_env;
    bool sync_phase;
    float noise_amt;
    bool noise_apply_env;
    ADSRParams noise_amt_env;
    
    /* Filter */
    bool filter_active;
    float resonance;
    float base_cutoff_unscaled;
    float base_cutoff;
    float pitch_amt;
    float vel_amt;
    float env_amt;
    APINode filter_node;
    APINode noise_node;
    bool use_amp_env;
    ADSRParams filter_env;
    Endpoint filter_active_ep;
    Endpoint base_cutoff_ep;
    Endpoint pitch_amt_ep;
    Endpoint vel_amt_ep;
    Endpoint env_amt_ep;
    Endpoint resonance_ep;
    Endpoint use_amp_env_ep;
    Endpoint sync_phase_ep;
    Endpoint noise_amt_ep;
    Endpoint noise_apply_env_ep;
    
    bool monitor;
    bool allow_voice_stealing;
    Page *osc_page; /* For GUI callback targeting */
    Page *amp_env_page;
    Page *filter_page;
    Page *noise_page;

    Track *track;

    float vol;
    float pan;
    Endpoint vol_ep;
    Endpoint pan_ep;

    bool pedal_depressed;

    SynthVoice *deferred_offs[SYNTH_NUM_VOICES];
    uint8_t num_deferred_offs;
    /* bool deferred_note_offs[128]; */

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
void synth_clear_all(Synth *s);

/* Return 0 for success, 1 for unset (carrier NULL), < 0 for error */
int synth_set_freq_mod_pair(Synth *s, OscCfg *carrier_cfg, OscCfg *modulator_cfg);

/* Return 0 for success, 1 for unset (carrier NULL), < 0 for error */
int synth_set_amp_mod_pair(Synth *s, OscCfg *carrier_cfg, OscCfg *modulator_cfg);

void synth_destroy(Synth *s);

void synth_write_preset_file(const char *filepath, Synth *s);
void synth_read_preset_file(const char *filepath, Synth *s);

#endif
