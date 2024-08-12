/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
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

#ifndef JDAW_DSP_H
#define JDAW_DSP_H

#include <stdio.h>
#include <complex.h>
#include "SDL.h"

#define DEFAULT_FILTER_LEN 128

typedef struct track Track;

typedef enum filter_type {
    LOWPASS=0, HIGHPASS=1, BANDPASS=2, BANDCUT=3
} FilterType;

typedef struct fir_filter {
    FilterType type;
    double cutoff_freq; // 0 < cutoff_freq < 0.5
    double bandwidth;
    double *impulse_response;
    double complex *frequency_response;
    double *frequency_response_mag;
    double *overlap_buffer_L;
    double *overlap_buffer_R;
    uint16_t impulse_response_len;
    uint16_t frequency_response_len;
    uint16_t overlap_len;
    SDL_mutex *lock;
} FIRFilter;

typedef struct delay_line {
    int32_t len;
    double amp;
    int32_t stereo_offset;
    
    int32_t pos_L;
    int32_t pos_R;
    double *buf_L;
    double *buf_R;
    SDL_mutex *lock;
} DelayLine;

/* Initialize the dsp subsystem. All this does currently is to populate the nth roots of unity for n < ROU_MAX_DEGREE */
void init_dsp();

/* Create an empty FIR filter and allocate space for its buffers. MUST be initialized with 'set_filter_params'*/
FIRFilter *filter_create(FilterType type, uint16_t impulse_response_len, uint16_t frequency_response_len);

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
void filter_destroy(FIRFilter *filter);

/* void apply_track_filter(Track *track, uint8_t channel, uint16_t chunk_size, float *sample_array); */
void apply_filter(FIRFilter *filter, Track *track, uint8_t channel, uint16_t chunk_size, float *sample_array);
// void process_clip_vol_and_pan(Clip *clip);
/* void process_track_vol_and_pan(Track *track); */
/* void process_vol_and_pan(void); */

void FFT(double *A, double complex *B, int n);
void get_real_component(double complex *A, double *B, int n);
void get_magnitude(double complex *A, double *B, int len);

void delay_line_init(DelayLine *dl);
void delay_line_set_params(DelayLine *dl, double amp, int32_t len);
void delay_line_clear(DelayLine *dl);
#endif
