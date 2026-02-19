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

#define MAX_LINEAR_PLOTS 10
#define SFPP_THRESHOLD 15.5

#include "logscale.h"
#include "textbox.h"
#include "value.h"

/* struct logscale { */
/*     double *array; */
/*     double min; */
/*     double range; */
/*     int num_items; */
/*     int step; */
/*     SDL_Rect *container; */
/*     int *x_pos_cache; */
/*     SDL_Color *color; */
/* }; */


struct freq_plot {
    Logscale x_axis;
    /* bool scale_y_axis; */
    /* Logscale y_axis; */
    float **farrays;
    double **darrays;
    int num_farrays;
    int num_darrays;
    int *farray_lens;
    int *darray_lens;
    SDL_Color **fcolors;
    SDL_Color **dcolors;
    
    double *linear_plots[MAX_LINEAR_PLOTS];
    int num_linear_plots;
    int linear_plot_lens[MAX_LINEAR_PLOTS];
    SDL_Color *linear_plot_colors[MAX_LINEAR_PLOTS];
    double linear_plot_mins[MAX_LINEAR_PLOTS];
    double linear_plot_ranges[MAX_LINEAR_PLOTS];
    
    Layout *container;
    int num_tics;
    int *tic_cache;
    Textbox *labels[128];
    int num_labels;
    pthread_mutex_t *related_obj_lock;
};

/* DEPRECATED: use "waveform_draw_all_channels_generic" instead */
void waveform_draw_all_channels(float **channels, uint8_t num_channels, uint32_t buflen, SDL_Rect *rect);
/* void waveform_draw_freq_domain(struct logscale *la); */
/* struct logscale *waveform_create_logscale(double *array, int num_items, SDL_Rect *container, SDL_Color *color); */
/* void waveform_destroy_logscale(struct logscale *la); */


struct freq_plot *waveform_create_freq_plot(
    double **darrays,
    int *darray_lens,
    int num_darrays,
    float **farrays,
    int *farray_lens,
    int num_farrays,
    SDL_Color **darray_colors,
    SDL_Color **farray_colors,
    double min_freq_hz,
    double max_freq_hz,
    Layout *container);
void waveform_reset_freq_plot(struct freq_plot *fp);
void waveform_destroy_freq_plot(struct freq_plot *fp);
void waveform_draw_freq_plot(struct freq_plot *fp);
/* void waveform_draw_all_channels_generic(void **channels, ValType type, uint8_t num_channels, uint32_t buflen, SDL_Rect *rect); */
/* void waveform_draw_all_channels_generic(void **channels, ValType type, uint8_t num_channels, uint32_t buflen, SDL_Rect *rect, int min_x, int max_x); */
void waveform_draw_all_channels_generic(void **channels, ValType type, uint8_t num_channels, uint32_t buflen, SDL_Rect *rect, int min_x, int max_x, double sfpp, SDL_Color *color, float gain);

int waveform_freq_plot_x_abs_from_freq(struct freq_plot *fp, double freq_raw);
int waveform_freq_plot_y_abs_from_amp(struct freq_plot *fp, double amp, int arr_i, bool linear_plot);
double waveform_freq_plot_freq_from_x_abs(struct freq_plot *fp, int abs_x);
double waveform_freq_plot_amp_from_y_abs(struct freq_plot *fp, int abs_y, int arr_i, bool linear_plot);
/* void waveform_freq_plot_add_linear_plot(struct freq_plot *fp, int len, SDL_Color *color, double calculate_point(double input, void *xarg), void *xarg); */
/* void waveform_freq_plot_update_linear_plot(struct freq_plot *fp, double calculate_point(double input, void *xarg), void *xarg); */
void waveform_freq_plot_add_linear_plot(struct freq_plot *fp, int len, double *arr, SDL_Color *color);
/* void logscale_set_range(struct logscale *l, double min, double max); */
#endif
