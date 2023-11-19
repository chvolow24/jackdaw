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

/*****************************************************************************************************************
    dsp.c

    * Digital signal processing
    * Clip buffers are run through functions here to populate post-proc buffer
 *****************************************************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <complex.h>
#include "project.h"
#include "gui.h"

#define TAU (6.283185307179586476925286766559)
#define ROU_MAX_DEGREE 20

extern Project *proj;

complex double *roots_of_unity[ROU_MAX_DEGREE];


void process_clip_vol_and_pan(Clip *clip)
{
        Track *track = clip->track;
        float lpan, rpan, panctrlval;
        panctrlval = track->pan_ctrl->value;
        lpan = panctrlval < 0 ? 1 : 1 - panctrlval;
        rpan = panctrlval > 0 ? 1 : 1 + panctrlval;
        // uint8_t k=0;
        for (uint32_t j=0; j<clip->len_sframes * clip->channels; j+=2) {
            // pan = j%2==0 ? lpan : rpan;
            // if (k<20) {
            //     k++;
            //     fprintf(stderr, "\t\t->sample %d, pan value: %f\n", j, pan);
            // }
            clip->post_proc[j] = clip->pre_proc[j] * track->vol_ctrl->value * lpan;
            clip->post_proc[j+1] = clip->pre_proc[j+1] * track->vol_ctrl->value * rpan;
        }
}
 
void process_track_vol_and_pan(Track *track)
{
    for (uint8_t i=0; i<track->num_clips; i++) {
        Clip *clip = track->clips[i];
        process_clip_vol_and_pan(clip);
    }

}

void process_vol_and_pan()
{
    if (!proj) {
        fprintf(stderr, "Error: request to process vol and pan for nonexistent project.\n");
        return;
    }
    Track *track = NULL;
    for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
        track = proj->tl->tracks[i];
        process_track_vol_and_pan(track);
    }
}






/* Fourier Transform (FFT) */


static void init_roots_of_unity(void);

/* Public function to initialize FFT components */
void init_dsp()
{
    init_roots_of_unity();
}

/* Generate the log2(n)th roots of unity */
static double complex *gen_FFT_X(int n)
{
    double complex *X = malloc(sizeof(double complex) * n);
    double theta_increment = TAU / n;
    double theta = 0;
    for (int i=0; i<n; i++) {
        X[i] = (cos(theta) + I * (sin(theta)));
        theta += theta_increment;
    }
    return X;
}

/* Generate the roots of unity to be used in FFT evaluation */
static void init_roots_of_unity()
{
    fprintf(stderr, "Calculating roots of unity...\n");
    for (uint8_t i=0; i<ROU_MAX_DEGREE; i++) {
        fprintf(stderr, "\t -> degree %d / %d\n", i, ROU_MAX_DEGREE);
        roots_of_unity[i] = gen_FFT_X(pow(2, i));
    }
}

/* Get the kth element of the FFT on arrray A */
static double complex FFTk(double *A, double complex *X, int orig_n, int n, int k, int offset, int increment)
{
    if (n == 1) {
        return A[offset] + 0 * I;
    } else {
        int new_inc = increment * 2;
        int halfn = n >> 1;
        int newk = (k << 1) % orig_n;
        return FFTk(A, X, orig_n, halfn, newk, offset, new_inc) + conj(X[k]) * FFTk(A, X, orig_n, halfn, newk, offset + increment, new_inc);
    }
}

/* Evaluate the FFT on an array of doubles */
double complex *FFT(double *A, int n)
{
    int degree = log2(n);
    double complex *X = roots_of_unity[degree];
    double complex *B = malloc(sizeof(double complex) * n);
    for (int k=0; k<n; k++) {
        B[k] = FFTk(A, X, n, n, k, 0, 1) / n;
    }
    return B;
}

/* Evaluate the FFT on an array of signed 16-bit integers */
double complex *FFT_int16(int16_t *A, int n)
{
    double converted[n];
    for (int i=0; i<n; i++) {
        converted[i] = (double) A[i];
    }
    double complex *B = FFT(converted, n);
    return B;
}

/* Get the kth element of the IFFT on complex array B */
static double complex IFFTk(double complex *B, double complex *X, int orig_n, int n, int k, int offset, int increment)
{
    if (n == 1) {
        return B[offset] + 0 * I;
    } else {
        int new_inc = increment * 2;
        int halfn = n / 2;
        int newk = (k*2) % orig_n;
        return IFFTk(B, X, orig_n, halfn, newk, offset, new_inc) + X[k] * IFFTk(B, X, orig_n, halfn, newk, offset + increment, new_inc);
    }
}

/* Evaluate the IFFT on complex array B */
double complex *IFFT(double complex *B, int n)
{
    int degree = log2(n);
    double complex *X = roots_of_unity[degree];
    double complex *A = malloc(sizeof(double complex) * n);
    for (int k=0; k<n; k++) {
        A[k] = IFFTk(B, X, n, n, k, 0, 1);
    }
    return A;
}