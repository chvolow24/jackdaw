/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    delay_line.h

    * 
    * 
 *****************************************************************************************************************/


#ifndef DELAY_LINE_H
#define DELAY_LINE_H

#include "dsp_utils.h"

typedef struct delay_line {
    bool active;
    bool initialized;
    int32_t len_msec; /* For endpoint / GUI components */
    int32_t len; /* Sample frames */
    double amp;
    double stereo_offset;
    int32_t max_len;
    
    /* int32_t read_pos; */
    int32_t pos_L;
    int32_t pos_R;
    double *buf_L;
    double *buf_R;
    double *cpy_buf;
    pthread_mutex_t lock;

    Track *track;

    Endpoint len_ep;
    Endpoint amp_ep;
    Endpoint stereo_offset_ep;

    Effect *effect;

    /* SDL_mutex *lock; */
} DelayLine;

void delay_line_init(DelayLine *dl, Track *track, uint32_t sample_rate);
void delay_line_set_params(DelayLine *dl, double amp, int32_t len);
void delay_line_clear(DelayLine *dl);
float delay_line_buf_apply(void *dl_v, float *buf, int len, int channel, float input_amp);

#endif
