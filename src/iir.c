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
#include "project.h"
#include "iir.h"
#include "waveform.h"

extern SDL_Color color_global_white;
extern Window *main_win;
extern SDL_Color freq_L_color;
extern SDL_Color freq_R_color;

void iir_init(IIRFilter *f, int degree, int num_channels)
{
    f->degree = degree;
    f->num_channels = num_channels;
    f->A = calloc(degree + 1, sizeof(double));
    f->B = calloc(degree, sizeof(double));
    f->memIn = calloc(num_channels, sizeof(double *));
    f->memOut = calloc(num_channels, sizeof(double *));
    for (int i=0; i<num_channels; i++) {	    
	f->memIn[i] = calloc(degree, sizeof(double));
	f->memOut[i] = calloc(degree, sizeof(double));
    }
    f->A[0] = 1;
}

void iir_deinit(IIRFilter *f)
{
    if (f->A) {
	free(f->A);
	f->A = NULL;
    }
    if (f->B) {
	free(f->B);
	f->B = NULL;
    }
    if (f->memIn && f->memOut) {
	for (int i=0; i<f->num_channels; i++) {
	    free(f->memIn[i]);
	    free(f->memOut[i]);
	}
	free(f->memIn);
	free(f->memOut);
	f->memIn = NULL;
	f->memOut = NULL;
    }
}

void iir_set_coeffs(IIRFilter *f, double *A_in, double *B_in)
{
    memcpy(f->A, A_in, (f->degree + 1) * sizeof(double));
    memcpy(f->B, B_in, f->degree * sizeof(double));
}


/* Apply the filter */
double iir_sample(IIRFilter *f, double in, int channel)
{
    double out = in * f->A[0];
    for (int i=0; i<f->degree; i++) {
	out += f->A[i + 1] * f->memIn[channel][i];
	out += f->B[i] * f->memOut[channel][i];
    }
    memmove(f->memIn[channel] + 1, f->memIn[channel], sizeof(double) * (f->degree - 1));
    memmove(f->memOut[channel] + 1, f->memOut[channel], sizeof(double) * (f->degree - 1));
    f->memIn[channel][0] = in;
    f->memOut[channel][0] = out;
	   
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
	fprintf(stderr, "SINSQ: %f; pow amp2 tansqbdiv2: %f\n", sinsqw, (pow(amp, 2) * tansqbdiv2));
	/* fprintf(stderr, "ERROR! Cannot set at params\n"); */
	return -1;
    }
    /* double zero_term2_num = sqrt(sinsqw - (pow(amp, 2) * tansqbdiv2)); */

    double zero_term2_num = sqrt(sqrt_arg);
    double zero_term2_denom = 1 + amp * tanbdiv2;

    sqrt_arg = sinsqw - tansqbdiv2;
    if (sqrt_arg < 0.0) {
	fprintf(stderr, "SECOND: SINSQ: %f; tansqbdiv2: %f\n", sinsqw, tansqbdiv2);
	/* fprintf(stderr, "ERROR! Cannot set at params\n"); */
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



extern Project *proj;
const int freq_resp_len = DEFAULT_FOURIER_LEN_SFRAMES / 2;
const int FRLENSCAL = 1;
double freq_resp[freq_resp_len * FRLENSCAL];
struct freq_plot *eqfp;
Layout *fp_container = NULL;

void waveform_update_logscale(struct logscale *la, double *array, int num_items, int step, SDL_Rect *container);
void iir_set_coeffs_peaknotch(IIRFilter *iir, double freq, double amp, double bandwidth)
{
    double complex pole_zero[2];
    int safety = 0;
    while (reiss_2011(freq * PI, amp, bandwidth, pole_zero) < 0) {
	if (safety < 2) {
	    fprintf(stderr, "\t->Adj %f->\n", bandwidth);
	}
	bandwidth *= 0.9;
	safety++;
	if (safety > 1000) {
	    fprintf(stderr, "ABORT ABORT ABORT!\n");
	    exit(0);
	}
    }

    /* Track *t = timeline_selected_track(proj->timelines[proj->active_tl_index]); */
   
    for (int i=0; i<freq_resp_len * FRLENSCAL; i++) {
	double prop = (double)i / (freq_resp_len * FRLENSCAL);
	double complex pole1 = pole_zero[0];
	double complex pole2 = conj(pole1);
	double complex zero1 = pole_zero[1];
	double complex zero2 = conj(zero1);

	double theta = PI * prop; 
	double complex z = cos(theta) + I * sin(theta);
	freq_resp[i] = ((zero1 - z) * (zero2 - z)) / ((pole1 - z) * (pole2 - z));
	/* if (i<5) { */
	/*     fprintf(stderr, "FREQ RESP %d: %f\n", i, freq_resp[i]); */
	/* } */
    }
    int steps[] = {1, 1, 1};
    double *arrs[] = {freq_resp, proj->output_L_freq, proj->output_R_freq};
    SDL_Color *colors[] = {&color_global_white, &freq_L_color, &freq_R_color};
    if (!fp_container) {
	fp_container = layout_add_child(main_win->layout);
	layout_set_default_dims(fp_container);
	layout_force_reset(fp_container);
	eqfp = waveform_create_freq_plot(
	    arrs,
	    3,
	    colors,
	    steps,
	    freq_resp_len,
	    fp_container);

	waveform_update_logscale(eqfp->plots[0], freq_resp, freq_resp_len * FRLENSCAL, 1, &eqfp->container->rect);
	logscale_set_range(eqfp->plots[0], 0.0, 20.0);
	/* eqfp->num_items = freq_resp_len_long; */
	/* waveform_reset_freq_plot(eqfp); */

    }
    

    iir->A[0] = 1.0;
    iir->A[1] = -2 * creal(pole_zero[1]);
    iir->A[2] = pow(cabs(pole_zero[1]), 2);

    iir->B[0] = 2 * creal(pole_zero[0]);
    iir->B[1] = -1 * pow(cabs(pole_zero[0]), 2);


    /* fprintf(stderr, "IIR SET PEAKNOTCH from %f %f %f\n", freq, amp, bandwidth); */
    /* fprintf(stderr, "%f, %f, %f\n", iir->A[0], iir->A[1], iir->A[2]); */
    /* fprintf(stderr, "%f, %f\n", iir->B[0], iir->B[1]); */

    /* coeffs from pole and zero (assume */    
}

/* void iir_filter_tests() */
/* { */
/*     IIRFilter f; */
/*     double freq = 0.1; */
/*     double bandwidth = 0.01; */
/*     double amp = 10.0; */
/*     iir_init(&f, 2); */
/*     iir_set_coeffs_peaknotch(&f, freq, amp, bandwidth); */
/* } */

void iir_group_init(IIRGroup *group, int num_filters, int degree, int num_channels)
{
    group->filters = calloc(num_filters, sizeof(IIRFilter));
    for (int i=0; i<num_filters; i++) {
	iir_init(group->filters + i, degree, num_channels);
    }
}

void iir_group_deinit(IIRGroup *group)
{
    for (int i=0; i<group->num_filters; i++) {
	iir_deinit(group->filters + i);
    }
    free(group->filters);
    group->filters = NULL;
}

double iir_group_sample(IIRGroup *group, double in, int channel)
{
    for (int i=0; i<group->num_filters; i++) {
	in = iir_sample(group->filters + i, in, channel);
    }
    return in;
}

