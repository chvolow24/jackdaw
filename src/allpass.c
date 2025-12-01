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
    ap->mem_index = len - 1;
}

void lop_delay_init(LopDelay *ld, int32_t len, float delay_coeff, float lop_coeff)
{
    ld->mem = calloc(len, sizeof(float));
    ld->len = len;
    ld->delay_coeff = delay_coeff;
    ld->lop_coeff = lop_coeff;
    ld->lop_mem_out = 0.0f;
}

float allpass_sample(Allpass *ap, float in)
{
    float intermed = in - ap->coeff * ap->mem[ap->mem_index];
    float out = ap->coeff * intermed + ap->mem[ap->mem_index];
    ap->mem[ap->mem_index] = intermed;
    ap->mem_index--;
    if (ap->mem_index < 0) {
	ap->mem_index += ap->len;
    }
    return out;
}

float lop_delay_sample(LopDelay *ld, float in)
{
    float delay_out = in - ld->delay_coeff * ld->mem[ld->mem_index];
    float lop_out = ld->lop_coeff * delay_out + (1 - ld->lop_coeff) * ld->lop_mem_out;
    ld->lop_mem_out = lop_out;
    ld->mem[ld->mem_index] = lop_out;
    ld->mem_index--;
    if (ld->mem_index < 0) {
	ld->mem_index += ld->len;
    }

    return delay_out;
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
    static const int NUM_FILTERS = 3;
    static const int32_t LENS[] = {
    /* 	432 * 96, */
    /* 	15 * 96, */
    /* 	1037 * 96, */
	347 * 96,
	113 * 96,
	37 * 96
    };
    static const float COEFF = 0.7f;
    ag->filters = calloc(NUM_FILTERS, sizeof(Allpass));
    ag->num_filters = NUM_FILTERS;
    for (int i=0; i<NUM_FILTERS; i++) {
	allpass_init(ag->filters + i, LENS[i], COEFF);
	/* allpass_init(ag->filters + i, 1 + (rand() % 96000), COEFF); */
	/* allpass_init(ag->filters + i, LENS[i], COEFF); */
    }
}
