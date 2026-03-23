/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    effect.h

    * abstract representation of effects
    * each effect contains a pointer to the specific effect object
 *****************************************************************************************************************/


#ifndef JDAW_EFFECT_H
#define JDAW_EFFECT_H

#include "api.h"
#include "compressor.h"
#include "eq.h"
#include "saturation.h"


/* #define NUM_EFFECT_TYPES 5 */
typedef struct track Track;

typedef enum effect_type {
    EFFECT_EQ,
    EFFECT_FIR_FILTER,
    EFFECT_DELAY,
    EFFECT_SATURATION,
    EFFECT_COMPRESSOR,
    EFFECT_REVERB,
    NUM_EFFECT_TYPES
} EffectType;

typedef enum effect_channel_mode {
    EFFECT_CH_MODE_STEREO,
    EFFECT_CH_MODE_L,
    EFFECT_CH_MODE_R,
    EFFECT_CH_MODE_MID,
    EFFECT_CH_MODE_SIDE,
    EFFECT_CH_MODE_MID_SIDE,
    EFFECT_NUM_CH_MODES
} EffectChannelMode;

#define EFFECT_CH_MODE_DO_ENCODE(ch_mode) (ch_mode > 2)

typedef struct effect_chain EffectChain;
typedef struct project Project;
typedef struct effect {
    void *obj;
    EffectType type;
    EffectChannelMode channel_mode;
    Endpoint channel_mode_ep;
    float (*buf_apply)(void *effect, float *restrict buf_L, float *restrict buf_R, int len, float input_amp);
    bool operate_on_empty_buf;
    /* DO NOT ACCESS outside main thread */
    Page *page;
    /* Safe to access outside main thread */
    _Atomic bool page_onscreen;
    EffectChain *effect_chain;
    /* Track *track; */
    char name[MAX_NAMELENGTH];
    bool active;
    Endpoint active_ep;
    APINode api_node;
} Effect;

typedef struct effect_chain {
    bool initialized;
    Effect **effects;
    int num_effects;
    int effects_alloc_len;
    int num_effects_per_type[NUM_EFFECT_TYPES];
    bool blocked_types[NUM_EFFECT_TYPES];
    pthread_mutex_t effect_chain_lock;
    int32_t chunk_len_sframes;
    Project *proj; /* For link through to audio settings */
    APINode api_node;
    /* APINode *api_node; /\* Allocated on parent object, e.g. track or synth *\/ */
    const char *obj_name;
    bool mid_side_encoded;
} EffectChain;

void effect_chain_init(EffectChain *ec, Project *proj, APINode *parent_node, const char *obj_name, int32_t chunk_len_sframes);
void effect_chain_deinit(EffectChain *ec);
void effect_chain_block_type(EffectChain *ec, EffectType type);
Effect *effect_chain_add_effect(EffectChain *ec, EffectType type);

const char *effect_type_str(EffectType type);

/* LEGACY FUNCTION: included for backwards compatibility */
Effect *track_add_effect(Track *t, EffectType type);

/* Creates modal to select effect type before adding. obj_name used in modal header */
void effect_add(EffectChain *ec, const char *obj_name);

float effect_chain_buf_apply(EffectChain *ec, float *restrict L, float *restrict R, int len, float input_amp);
void effect_chain_silence(EffectChain *ec);
void effect_delete(Effect *e, bool from_undo);
void effect_destroy(Effect *e);

#endif
