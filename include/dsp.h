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
#include "project.h"

/* vv make shitty clangd work on charlie's computer vv */
#include "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/complex.h"
/* ^^                                              ^^ */



// typedef enum filter_type {
//     LOWPASS, HIGHPASS, BANDPASS, BANDCUT
// } FilterType;

typedef struct fir_filter {
    FilterType type;
    double cutoff_freq; // 0 < cutoff_freq < 0.5
    double bandwidth;
    double *impulse_response;
    double complex *frequency_response;
    uint16_t impulse_response_len;
    uint16_t frequency_response_len;
}FIRFilter;

/* Initialize the dsp subsystem. All this does currently is to populate the nth roots of unity for n < ROU_MAX_DEGREE */
void init_dsp();

/* Create an empty FIR filter and allocate space for its buffers. MUST be initialized with 'set_filter_params'*/
FIRFilter *create_FIR_filter(FilterType type, uint16_t impulse_response_len, uint16_t frequency_response_len);

/* Bandwidth param only required for band-pass and band-cut filters */
void set_FIR_filter_params(FIRFilter *filter, double cutoff, double bandwidth);

/* Destry a FIRFilter and associated memory */
void destroy_filter(FIRFilter *filter);

void apply_filter(FIRFilter *filter, uint16_t chunk_size, float *sample_array);

void process_clip_vol_and_pan(Clip *clip);
void process_track_vol_and_pan(Track *track);
void process_vol_and_pan(void);

#endif