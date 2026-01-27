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
    NUM_EFFECT_TYPES
} EffectType;
typedef struct effect_chain EffectChain;
typedef struct project Project;
typedef struct effect {
    void *obj;
    EffectType type;
    float (*buf_apply)(void *effect, float *input, int len, int channel, float input_amp);
    bool operate_on_empty_buf;
    Page *page;
    EffectChain *effect_chain;
    /* Track *track; */
    char name[MAX_NAMELENGTH];
    bool active;
    Endpoint active_ep;
    APINode api_node;
} Effect;

typedef struct effect_chain {
    Effect **effects;
    int num_effects;
    int effects_alloc_len;
    int num_effects_per_type[NUM_EFFECT_TYPES];
    pthread_mutex_t effect_chain_lock;
    int32_t chunk_len_sframes;
    Project *proj; /* For link through to audio settings */
    APINode *api_node; /* Allocated on parent object, e.g. track or synth */
    const char *obj_name;
} EffectChain;

void effect_chain_init(EffectChain *ec, Project *proj, APINode *api_node, const char *obj_name, int32_t chunk_len_sframes);
void effect_chain_deinit(EffectChain *ec);
Effect *effect_chain_add_effect(EffectChain *ec, EffectType type);

const char *effect_type_str(EffectType type);

/* LEGACY FUNCTION: included for backwards compatibility */
Effect *track_add_effect(Track *t, EffectType type);

/* Creates modal to select effect type before adding. obj_name used in modal header */
void effect_add(EffectChain *ec, const char *obj_name);

float effect_chain_buf_apply(EffectChain *ec, float *buf, int len, int channel, float input_amp);
void effect_delete(Effect *e, bool from_undo);
void effect_destroy(Effect *e);

#endif
