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
#include "endpoint.h"
#include "envelope_follower.h"

typedef struct effect Effect;

typedef struct compressor {
    EnvelopeFollower ef[2];
    float gain_scalar[2];
    float env[2];
    /* bool active; */
    float threshold;
    float m; /* Equal to one over the ratio */
    float makeup_gain;
    
    Effect *effect;

    float attack_time;
    float release_time;
    float ratio;
    /* Endpoint  active_ep; */
    Endpoint attack_time_ep;
    Endpoint release_time_ep;
    Endpoint threshold_ep;
    Endpoint ratio_ep;
    Endpoint makeup_gain_ep;
} Compressor;

/* Per-sample operation */
/* static inline float compressor_sample_gain_reduction(Compressor *c, float in); */
void compressor_init(Compressor *c);
float compressor_buf_apply(void *compressor_v, float *buf, int len, int channel, float input_amp);
void compressor_set_times_msec(Compressor *c, double attack_msec, double release_msec, double sample_rate);
void compressor_set_threshold(Compressor *c, float thresh);
void compressor_set_m(Compressor *c, float m);

void compressor_draw(Compressor *c, SDL_Rect *target);
#endif
