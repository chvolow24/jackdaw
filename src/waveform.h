/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

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
    pthread_mutex_t *related_obj_lock;
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
