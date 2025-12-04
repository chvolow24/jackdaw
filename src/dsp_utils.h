/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    dsp_utils.h

    * Fast Fourier Transform (FFT) implementation for float and double
    * Buffer-wise arithmetic
    * Window function(s)
*****************************************************************************************************************/


#ifndef JDAW_DSP_UTILS_H
#define JDAW_DSP_UTILS_H

#include <complex.h>
#include <stdio.h>
#include <stdint.h>

/* Initialize the dsp subsystem. All this does currently is to populate the nth roots of unity for n < ROU_MAX_DEGREE */
void init_dsp();


void FFT(double *restrict A, double complex *restrict B, int n);
void FFT_unscaled(double *restrict A, double complex *restrict B, int n);
void IFFT(double complex *restrict A, double complex *restrict B, int n);
void FFTf(float *restrict A, double complex *restrict B, int n);
void get_real_component(double complex *restrict A, double *restrict B, int n);
void get_real_componentf(double complex *restrict A, float *restrict B, int len);
void get_magnitude(double complex *restrict A, double *restrict B, int len);
double hamming(int x, int lenw);

/* Input range 0:1. Return frequenct in Hz from 1 - Nyquist */
double dsp_scale_freq_to_hz(double freq_unscaled);
/* Input range 0:1. Return logscaled value 0:1, where 1 = Nyquist */
double dsp_scale_freq(double freq_unscaled);


/* Add "b" into "a" */
void float_buf_add(float *restrict a, float *restrict b, int len);

/* Add "a" and "b" into "sum" */
void float_buf_add_to(float *restrict a, float *restrict b, float *restrict sum, int len);

/* Multiply "b" into "a" */
void float_buf_mult(float *restrict a, float *restrict b, int len);

/* Multiply all elments of "a" by "by" */
void float_buf_mult_const(float *restrict a, float by, int len);

/* Multiply "a" and "b" into "product" */
void float_buf_mult_to(float *restrict a, float *restrict b, float *restrict product, int len);

/* Convert a linear pan parameter value into a multiplier, depending on channel */
float pan_scale(float pan, int channel);

/* db FS */
float amp_to_db(float amp);
float db_to_amp(float db);

#endif
