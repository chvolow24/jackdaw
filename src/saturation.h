/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    saturation.h

    * tanh waveshaping saturator effect
 *****************************************************************************************************************/


#ifndef JDAW_SATURATION_H
#define JDAW_SATURATION_H

#include <math.h>
#include <stdbool.h>
#include "endpoint.h"

#define SATURATION_MAX_GAIN 40.0

typedef enum saturation_type {
    SAT_TANH=0,
    SAT_EXPONENTIAL=1

} SaturationType;

typedef struct effect Effect;

typedef struct saturation Saturation;
typedef struct saturation {
    /* bool active; */
    SaturationType type;
    double gain;
    bool do_gain_comp;
    double gain_comp_val;
    double symmetry;
    Endpoint gain_ep;
    Endpoint symmetry_ep;
    Endpoint gain_comp_ep;
    Endpoint type_ep;
    double (*sample_fn)(Saturation *s, double in);
    Track *track;

    Effect *effect;
} Saturation;


void saturation_init(Saturation *s);
void saturation_set_gain(Saturation *s, double gain);
void saturation_set_type(Saturation *s, SaturationType t);
/* double saturation_sample(Saturation *s, double in); */
float saturation_buf_apply(void *saturation_v, float *restrict buf, int len, int channel_unused, float input_amp);
/* void saturation_buf_apply(Saturation *s, float *buf, int len, int channel_unused); */


#endif
