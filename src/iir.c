/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    iir.c

    * Set IIR filter coefficients
    * Sample- or buffer-wise application
 *****************************************************************************************************************/

#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iir.h"

void iir_init(IIRFilter *f, int degree)
{
    f->degree = degree;
    f->A = calloc(degree + 1, sizeof(double));
    f->B = calloc(degree, sizeof(double));
    f->memIn = calloc(degree, sizeof(double));
    f->memOut = calloc(degree, sizeof(double));
    f->A[0] = 1;
}

void iir_deinit(IIRFilter *f)
{
    if (f->A) free(f->A);
    if (f->B) free(f->B);
}

void iir_set_coeffs(IIRFilter *f, double *A_in, double *B_in)
{
    memcpy(f->A, A_in, (f->degree + 1) * sizeof(double));
    memcpy(f->B, B_in, f->degree * sizeof(double));
}


/* Apply the filter */
double iir_sample(IIRFilter *f, double in)
{
    double out = in * f->A[0];
    for (int i=0; i<f->degree; i++) {
	out += f->A[i + 1] * f->memIn[i];
	out += f->B[i] * f->memOut[i];
    }
    memmove(f->memIn + 1, f->memIn, sizeof(double) * (f->degree - 1));
    memmove(f->memOut + 1, f->memOut, sizeof(double) * (f->degree - 1));
    f->memIn[0] = in;
    f->memOut[0] = out;
    /* fprintf(stderr, "%f -> %f\n", in, out); */
    /* for (int i=0; i<f->degree; i++) { */
    /* 	fprintf(stderr, "%f, ", f->memIn[i]); */
    /* } */
    /* fprintf(stderr, "\n"); */
    /* for (int i=0; i<f->degree; i++) { */
    /* 	fprintf(stderr, "%f, ", f->memOut[i]); */
    /* } */
    /* fprintf(stderr, "\n"); */
	   
    return out;
}



/* SET COEFFS */

/* Joshua D. Reiss, IEEE TRANSACTIONS ON AUDIO, SPEECH, AND LANGUAGE PROCESSING, VOL. 19, NO. 6, AUGUST 2011  */
static void reiss_2011(double freq, double amp, double bandwidth, double complex *dst)
{
    double cosw = cos(freq);
    double tanbdiv2 = tan(bandwidth / 2
	);
    double tansqbdiv2 = pow(tanbdiv2, 2.0);
    double sinsqw = pow(sin(freq), 2.0);

    double zero_term1 = cosw / (1 + amp * tanbdiv2);
    double pole_term1 = cosw / (1 + tanbdiv2);

    double zero_term2_num = sqrt(sinsqw - (pow(amp, 2) * tansqbdiv2));
    double zero_term2_denom = 1 + amp * tanbdiv2;

    double pole_term2_num = sqrt(sinsqw - tansqbdiv2);
    double pole_term2_denom = 1 + tanbdiv2;

    double complex zero_term2 = I * zero_term2_num / zero_term2_denom;
    double complex pole_term2 = I * pole_term2_num / pole_term2_denom;

    dst[0] = pole_term1 + pole_term2;
    dst[1] = zero_term1 + zero_term2;
}


void iir_set_coeffs_peaknotch(IIRFilter *iir, double freq, double amp, double bandwidth)
{
    double complex pole_zero[2];
    reiss_2011(freq, amp, bandwidth, pole_zero);

    iir->A[0] = 1.0;
    iir->A[1] = -2 * creal(pole_zero[1]);
    iir->A[2] = pow(cabs(pole_zero[1]), 2);

    iir->B[0] = 2 * creal(pole_zero[0]);
    iir->B[1] = -1 * pow(cabs(pole_zero[0]), 2);


    fprintf(stderr, "IIR SET PEAKNOTCH\n");
    fprintf(stderr, "%f, %f, %f\n", iir->A[0], iir->A[1], iir->A[2]);
    fprintf(stderr, "%f, %f\n", iir->B[0], iir->B[1]);

    /* coeffs from pole and zero (assume */    
}

void iir_filter_tests()
{
    IIRFilter f;
    double freq = 0.1;
    double bandwidth = 0.01;
    double amp = 10.0;
    iir_init(&f, 2);
    iir_set_coeffs_peaknotch(&f, freq, amp, bandwidth);
}
