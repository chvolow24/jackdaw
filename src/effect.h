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

typedef struct effect {
    void *obj;
    EffectType type;
    float (*buf_apply)(void *effect, float *input, int len, int channel, float input_amp);
    bool operate_on_empty_buf;
    Page *page;
    Track *track;
    char name[MAX_NAMELENGTH];
    APINode api_node;
} Effect;
const char *effect_type_str(EffectType type);

/* Directly add effect to track */
Effect *track_add_effect(Track *t, EffectType type);

/* Higher-level function, called from userfn to create modal */
void track_add_new_effect(Track *track);
    
float effect_chain_buf_apply(Effect **e, int num_effects, float *buf, int len, int channel, float input_amp);
void effect_destroy(Effect *e);

#endif
