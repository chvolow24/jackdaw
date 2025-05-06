/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    compressor.h

    * basic dynamic range compression
 *****************************************************************************************************************/

#ifndef JDAW_COMPRESSOR_H
#define JDAW_COMPRESSOR_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "SDL.h"
#include "envelope_follower.h"

typedef struct effect Effect;

typedef struct compressor {
    EnvelopeFollower ef;
    float threshold;
    float m; /* Equal to one over the ratio */
    float makeup_gain;
    float gain_reduction;
    float env;
    
    Effect *effect;
    /* double m_static; /\* Equal to one over the ratio *\/ */
    /* double threshold; */
    /* int32_t attack_sframes; */
    /* int32_t decay_sframes; */
    
    /* double m_temp; */
    /* double m_incr; */
    /* int32_t attack_ctr; */
    /* int32_t decay_ctr; */
    /* bool in_attack; */
    /* bool in_decay; */
} Compressor;

/* Per-sample operation */
/* static inline float compressor_sample_gain_reduction(Compressor *c, float in); */
float compressor_buf_apply(void *compressor_v, float *buf, int len, int channel, float input_amp);
void compressor_set_times_msec(Compressor *c, double attack_msec, double release_msec, double sample_rate);
void compressor_set_threshold(Compressor *c, float thresh);
void compressor_set_m(Compressor *c, float m);

void compressor_draw(Compressor *c, SDL_Rect *target);
#endif
