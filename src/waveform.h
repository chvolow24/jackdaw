/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software iso
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/


/*****************************************************************************************************************
    waveform.h

    * algorithm to draw waveforms inside rectangles
    * incl. multi-channel audio
 *****************************************************************************************************************/


#ifndef JDAW_WAVEFORM_H
#define JDAW_WAVEFORM_H

#include "textbox.h"
#include "value.h"
#include "window.h"

struct logscale {
    double *array;
    int num_items;
    int step;
    SDL_Rect *container;
    int *x_pos_cache;
    SDL_Color *color;
};

struct freq_plot {
    struct logscale **plots;
    double **arrays;
    SDL_Color **colors;
    int *steps;
    int num_plots;
    int num_items;
    Layout *container;
    int num_tics;
    int *tic_cache;
    Textbox *labels[128];
    int num_labels;
};

void waveform_draw_all_channels(float **channels, uint8_t num_channels, uint32_t buflen, SDL_Rect *rect);
void waveform_draw_freq_domain(struct logscale *la);
/* struct logscale *waveform_create_logscale(double *array, int num_items, SDL_Rect *container, SDL_Color *color); */
/* void waveform_destroy_logscale(struct logscale *la); */


struct freq_plot *waveform_create_freq_plot(
    double **arrays,
    int num_arrays,
    SDL_Color **colors,
    int *steps,
    int num_items,
    Layout *container);
void waveform_reset_freq_plot(struct freq_plot *fp);
void waveform_destroy_freq_plot(struct freq_plot *fp);
void waveform_draw_freq_plot(struct freq_plot *fp);
/* void waveform_draw_all_channels_generic(void **channels, ValType type, uint8_t num_channels, uint32_t buflen, SDL_Rect *rect); */
void waveform_draw_all_channels_generic(void **channels, ValType type, uint8_t num_channels, uint32_t buflen, SDL_Rect *rect, int min_x, int max_x);
#endif
