/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    allpass.h

    * Schroeder allpass filters
    * Direct Form II implementation
    * for use in reverb
    * reference: https://ccrma.stanford.edu/~jos/pasp/Allpass_Two_Combs.html
 *****************************************************************************************************************/


#ifndef JDAW_ALLPASS_H
#define JDAW_ALLPASS_H

#include <stdint.h>

typedef struct allpass {
    float *mem;
    int32_t len;
    float coeff;
    int32_t mem_index;
} Allpass;

typedef struct allpass_group {
    int num_filters;
    Allpass *filters;
} AllpassGroup;


void allpass_init(Allpass *ap, int32_t len, float coeff);
float allpass_sample(Allpass *ap, float in);

void allpass_group_init_schroeder(AllpassGroup *ag);
float allpass_group_sample(AllpassGroup *ag, float in);

#endif
