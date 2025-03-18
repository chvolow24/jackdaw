/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#ifndef JDAW_DSP_H
#define JDAW_DSP_H

#include <complex.h>
#include <stdio.h>
#include <stdint.h>
#include "endpoint.h"
#include "SDL.h"

#define DEFAULT_FILTER_LEN 128
#define DELAY_LINE_MAX_LEN_S 1

#define TAU (6.283185307179586476925286766559)
#define PI (3.1415926535897932384626433832795)


typedef struct track Track;

typedef enum filter_type {
    LOWPASS=0, HIGHPASS=1, BANDPASS=2, BANDCUT=3
} FilterType;

typedef struct fir_filter {
    bool active;
    bool initialized;
    FilterType type;
    double cutoff_freq_unscaled; /* For gui components and automations */
    double cutoff_freq; /* 0 < cutoff_freq < 0.5 */
    double bandwidth_unscaled; /* For gui components and automations */
    double bandwidth;
    double *impulse_response;
    double complex *frequency_response;
    double *frequency_response_mag;
    double *overlap_buffer_L;
    double *overlap_buffer_R;
    uint16_t impulse_response_len; /* Only modified in callbacks */
    uint16_t impulse_response_len_internal;
    /* uint16_t impulse_response_len_internal; */
    uint16_t frequency_response_len;
    uint16_t overlap_len;
    pthread_mutex_t lock;

    Track *track;

    Endpoint type_ep;
    Endpoint cutoff_ep;
    Endpoint bandwidth_ep;
    Endpoint impulse_response_len_ep;

    /* SDL_mutex *lock;audio.c */
} FIRFilter;

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

    /* SDL_mutex *lock; */
} DelayLine;

/* Initialize the dsp subsystem. All this does currently is to populate the nth roots of unity for n < ROU_MAX_DEGREE */
void init_dsp();

/* Init an empty FIR filter and allocate space for its buffers. Params MUST be initialized with 'set_filter_params'*/
void filter_init(FIRFilter *filter, Track *track, FilterType type, uint16_t impulse_response_len, uint16_t frequency_response_len);

/* Bandwidth param only required for band-pass and band-cut filters */
void filter_set_params(FIRFilter *filter, FilterType type,  double cutoff, double bandwidth);
void filter_set_params_hz(FIRFilter *filter, FilterType type, double cutoff_hz, double bandwidth_hz);

void filter_set_cutoff(FIRFilter *filter, double cutoff);
void filter_set_cutoff_hz(FIRFilter *filter, double cutoff);
void filter_set_bandwidth(FIRFilter *filter, double cutoff);
void filter_set_bandwidth_hz(FIRFilter *filter, double cutoff);
void filter_set_type(FIRFilter *filter, FilterType t);

void filter_set_impulse_response_len(FIRFilter *f, int new_len);

/* Destry a FIRFilter and associated memory */
void filter_deinit(FIRFilter *filter);

/* void apply_track_filter(Track *track, uint8_t channel, uint16_t chunk_size, float *sample_array); */
void apply_filter(FIRFilter *filter, Track *track, uint8_t channel, uint16_t chunk_size, float *sample_array);

void FFT(double *A, double complex *B, int n);
void get_real_component(double complex *A, double *B, int n);
void get_magnitude(double complex *A, double *B, int len);

void delay_line_init(DelayLine *dl, Track *track, uint32_t sample_rate);
void delay_line_set_params(DelayLine *dl, double amp, int32_t len);
void delay_line_clear(DelayLine *dl);

double dsp_scale_freq_to_hz(double freq_unscaled);


/* void track_add_default_filter(Track *track); */
#endif
