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
#include "dsp.h"
#include "iir.h"
#include "waveform.h"

extern SDL_Color color_global_white;
extern Window *main_win;

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
static int reiss_2011(double freq, double amp, double bandwidth, double complex *dst)
{
    double cosw = cos(freq);
    double tanbdiv2 = tan(bandwidth / 2);
    double tansqbdiv2 = pow(tanbdiv2, 2.0);
    double sinsqw = pow(sin(freq), 2.0);

    double zero_term1 = cosw / (1 + amp * tanbdiv2);
    double pole_term1 = cosw / (1 + tanbdiv2);

    double sqrt_arg = sinsqw - (pow(amp, 2) * tansqbdiv2);
    if (sqrt_arg < 0.0) {
	fprintf(stderr, "ERROR! Cannot set at params\n");
	return -1;
    }
    /* double zero_term2_num = sqrt(sinsqw - (pow(amp, 2) * tansqbdiv2)); */

    double zero_term2_num = sqrt(sqrt_arg);
    double zero_term2_denom = 1 + amp * tanbdiv2;

    sqrt_arg = sinsqw - tansqbdiv2;
    if (sqrt_arg < 0.0) {
	fprintf(stderr, "ERROR! Cannot set at params\n");
	return -1;
    }

    double pole_term2_num = sqrt(sinsqw - tansqbdiv2);
    double pole_term2_denom = 1 + tanbdiv2;

    double complex zero_term2 = I * zero_term2_num / zero_term2_denom;
    double complex pole_term2 = I * pole_term2_num / pole_term2_denom;

    dst[0] = pole_term1 + pole_term2;
    dst[1] = zero_term1 + zero_term2;
    return 0;
}



const int freq_resp_len = 90000;
double freq_resp[freq_resp_len];
struct freq_plot *eqfp;
Layout *fp_container = NULL;
void iir_set_coeffs_peaknotch(IIRFilter *iir, double freq, double amp, double bandwidth)
{
    double complex pole_zero[2];
    if (reiss_2011(freq * PI, amp, bandwidth, pole_zero) < 0) {
	return;
    }

    
    for (int i=0; i<freq_resp_len; i++) {
	double prop = (double)i / freq_resp_len;
	double complex pole1 = pole_zero[0];
	double complex pole2 = conj(pole1);
	double complex zero1 = pole_zero[1];
	double complex zero2 = conj(zero1);

	double theta = PI * prop; 
	double complex z = cos(theta) + I * sin(theta);
	freq_resp[i] = ((zero1 - z) * (zero2 - z)) / ((pole1 - z) * (pole2 - z));

	int steps[] = {1};
	double *arrs[] = {freq_resp};
	SDL_Color *colors[] = {&color_global_white};
	if (!fp_container) {
	    fp_container = layout_add_child(main_win->layout);
	    layout_set_default_dims(fp_container);
	    layout_force_reset(fp_container);
	    eqfp = waveform_create_freq_plot(
		arrs,
		1,
		colors,
		steps,
		freq_resp_len,
		fp_container);

	} else {
	    /* waveform_reset_freq_plot(eqfp); */
	}

	
	    
	    
	
    }
    

    iir->A[0] = 1.0;
    iir->A[1] = -2 * creal(pole_zero[1]);
    iir->A[2] = pow(cabs(pole_zero[1]), 2);

    iir->B[0] = 2 * creal(pole_zero[0]);
    iir->B[1] = -1 * pow(cabs(pole_zero[0]), 2);


    fprintf(stderr, "IIR SET PEAKNOTCH from %f %f %f\n", freq, amp, bandwidth);
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
