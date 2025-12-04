/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    envelope_follower.h

    * peak-tracking envelope follower
    * one-sample memory
 *****************************************************************************************************************/

#ifndef JDAW_ENVELOPE_FOLLOWER_H
#define JDAW_ENVELOPE_FOLLOWER_H

#define ENV_F_STD_ATTACK_MSEC 0
#define ENV_F_STD_RELEASE_MSEC 200

#include <math.h>
#include <stdint.h>

typedef struct envelope_follower {
    float prev_out;
    double attack_coeff;
    double release_coeff;
} EnvelopeFollower;


void envelope_follower_set_times(EnvelopeFollower *e, int32_t attack_sframes, int32_t decay_sframes);
void envelope_follower_set_times_msec(EnvelopeFollower *e, double attack_msec, double decay_msec, double sample_rate);
float envelope_follower_sample(EnvelopeFollower *e, float in);


#endif
