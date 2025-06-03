/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "envelope_follower.h"

void envelope_follower_set_times(EnvelopeFollower *e, int32_t attack_sframes, int32_t decay_sframes)
{
    e->attack_coeff = 1 - exp(-1.0 / attack_sframes);
    e->release_coeff = 1 - exp(-1.0 / decay_sframes);
}

void envelope_follower_set_times_msec(EnvelopeFollower *e, double attack_msec, double decay_msec, double sample_rate)
{
    e->attack_coeff = 1 - exp(-1.0 / (attack_msec * sample_rate / 1000));
    e->release_coeff = 1 - exp(-1.0 / (decay_msec * sample_rate / 1000));
}

float envelope_follower_sample(EnvelopeFollower *e, float in)
{
    float out;
    if (in > e->prev_out) {
	out =  fabs(in) * e->attack_coeff + (1 - e->attack_coeff) * e->prev_out;
    } else {
	out =  fabs(in) * e->release_coeff + (1 - e->release_coeff) * e->prev_out;
    }    
    e->prev_out = out;
    return out;
}

