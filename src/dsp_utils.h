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
double dsp_scale_freq_to_hz(double freq_unscaled);


void float_buf_add(float *restrict a, float *restrict b, int len);
void float_buf_add_to(float *restrict a, float *restrict b, float *restrict sum, int len);
void float_buf_mult(float *restrict a, float *restrict b, int len);
void float_buf_mult_to(float *restrict a, float *restrict b, float *restrict product, int len);


#endif
