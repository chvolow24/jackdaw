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

typedef struct lop_delay {
    float *mem;
    int32_t init_len;
    int32_t len;
    float delay_coeff;
    float lop_coeff;
    float lop_mem_out;
    int32_t mem_index;
} LopDelay;

typedef struct allpass {
    float *mem;
    int32_t init_len;
    int32_t len;
    float coeff;
    int32_t mem_index;
} Allpass;

typedef struct allpass_group {
    int num_filters;
    Allpass *filters;

    /* Below values only relevant for the design described here:
       https://valhalladsp.com/2009/05/30/schroeder-reverbs-the-forgotten-algorithm/
    */
    float coeff;
    int32_t init_len;
    int32_t len;
    float *mem;
    int32_t mem_index;
} AllpassGroup;


void allpass_init(Allpass *ap, int32_t len, float coeff);
float allpass_sample(Allpass *ap, float in);

void allpass_group_init(AllpassGroup *ag, int num_filters, int32_t *lens_samples, float coeff);

void allpass_group_feedback_init(AllpassGroup *ag, int32_t len, float g);
void allpass_group_set_len(AllpassGroup *ag, float scalar);
/* void allpass_group_init_schroeder(AllpassGroup *ag); */
float allpass_group_sample(AllpassGroup *ag, float in);

/* https://valhalladsp.com/2009/05/30/schroeder-reverbs-the-forgotten-algorithm/ */
float allpass_group_feedback_sample(AllpassGroup *ag, float in);

void lop_delay_set_len(LopDelay *ld, double scale_init_len);
void allpass_group_clear(AllpassGroup *ag);
void allpass_group_deinit(AllpassGroup *ag);
void allpass_group_set_coeff(AllpassGroup *ag, float new);


float lop_delay_sample(LopDelay *ld, float in);
void lop_delay_clear(LopDelay *ld);
void lop_delay_init(LopDelay *ld, int32_t len, float delay_coeff, float lop_coeff);
void lop_delay_deinit(LopDelay *ld);

#endif
