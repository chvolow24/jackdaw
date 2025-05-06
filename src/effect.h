#ifndef JDAW_EFFECT_H
#define JDAW_EFFECT_H

#include "compressor.h"
#include "eq.h"
#include "dsp.h"
/* #include "project.h" */
#include "saturation.h"

/* #define NUM_EFFECT_TYPES 5 */
typedef struct track Track;

typedef enum effect_type {
    EFFECT_EQ=0,
    EFFECT_FIR_FILTER=1,
    EFFECT_DELAY=2,
    EFFECT_SATURATION=3,
    EFFECT_COMPRESSOR=4
} EffectType;

typedef struct effect {
    void *obj;
    EffectType type;
    float (*buf_apply)(void *effect, float *input, int len, int channel, float input_amp);
    bool operate_on_empty_buf;
    Page *page;
    Track *track;
} Effect;
const char *effect_type_str(EffectType type);

/* Directly add effect to track */
Effect *track_add_effect(Track *t, EffectType type);

/* Higher-level function, called from userfn to create modal */
void track_add_new_effect(Track *track);
    
float effect_chain_buf_apply(Effect *e, int num_effects, float *buf, int len, int channel, float input_amp);


#endif
