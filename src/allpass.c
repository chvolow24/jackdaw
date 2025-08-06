/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "allpass.h"

void allpass_init(Allpass *ap, int32_t len, float coeff)
{
    ap->mem = calloc(len, sizeof(float));
    ap->len = len;
    ap->coeff = coeff;
}

float allpass_sample(Allpass *ap, float in)
{
    float intermed = in - ap->coeff * ap->mem[ap->mem_index];
    float out = ap->coeff * intermed + ap->mem[ap->mem_index];
    ap->mem[ap->mem_index] = intermed;
    ap->mem_index--;
    if (ap->mem_index < 0) ap->mem_index += ap->len;
    return out;
}

float allpass_group_sample(AllpassGroup *ag, float in)
{
    for (int i=0; i<ag->num_filters; i++) {
	in = allpass_sample(ag->filters + i, in);
    }
    return in;
}

void allpass_group_init_schroeder(AllpassGroup *ag)
{
    memset(ag, '\0', sizeof(AllpassGroup));
    static const int NUM_FILTERS = 150;
    /* static const int32_t LENS[] = { */
    /* 	432 * 96, */
    /* 	15 * 96, */
    /* 	1037 * 96 */
    /* 	347 * 96, */
    /* 	113 * 96, */
    /* 	37 * 96 */
    /* }; */
    static const float COEFF = 0.7f;
    ag->filters = calloc(NUM_FILTERS, sizeof(Allpass));
    ag->num_filters = NUM_FILTERS;
    for (int i=0; i<NUM_FILTERS; i++) {
	/* allpass_init(ag->filters + i, LENS[i], COEFF);	 */
	allpass_init(ag->filters + i, (rand() % 1000), COEFF);
    }
}
