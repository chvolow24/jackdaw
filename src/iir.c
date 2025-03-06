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
#include "page.h"
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
    /* f->pole_zero = calloc(degree, sizeof(double complex)); */
    for (int i=0; i<num_channels; i++) {	    
	f->memIn[i] = calloc(degree, sizeof(double));
	f->memOut[i] = calloc(degree, sizeof(double));
    }
    f->A[0] = 1;
    for (int i=0; i<IIR_FREQPLOT_RESOLUTION; i++) {
	f->freq_resp[i] = 1.0;
    }
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
    if (!isnormal(out)) out = 0.0;
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
	/* fprintf(stderr, "SINSQ: %f; pow amp2 tansqbdiv2: %f\n", sinsqw, (pow(amp, 2) * tansqbdiv2)); */
	/* fprintf(stderr, "ERROR! Cannot set at params\n"); */
	return -1;
    }
    /* double zero_term2_num = sqrt(sinsqw - (pow(amp, 2) * tansqbdiv2)); */

    double zero_term2_num = sqrt(sqrt_arg);
    double zero_term2_denom = 1 + amp * tanbdiv2;
    double pole_term2_denom = 1 + tanbdiv2;
    
    const static double epsilon = 0.001;
    if (fabs(zero_term2_denom) < epsilon) return -2;
    if (fabs(pole_term2_denom) < epsilon) return -2;

    
    sqrt_arg = sinsqw - tansqbdiv2;
    if (sqrt_arg < 0.0) {
	/* fprintf(stderr, "SECOND: SINSQ: %f; tansqbdiv2: %f\n", sinsqw, tansqbdiv2); */
	/* fprintf(stderr, "ERROR! Cannot set at params\n"); */
	return -1;
    }

    double pole_term2_num = sqrt(sinsqw - tansqbdiv2);


    double complex zero_term2 = I * zero_term2_num / zero_term2_denom;
    double complex pole_term2 = I * pole_term2_num / pole_term2_denom;

    double complex pole = pole_term1 + pole_term2;
    double complex zero = zero_term1 + zero_term2;

    fprintf(stderr, "polezero: %f+%fi, %f+%fi\n", creal(pole), cimag(pole), creal(zero), cimag(zero));
    if (cabs(pole) >= 1.0) {
	fprintf(stderr, "ERROR ERROR ERROR! Pole outside unit Coicle!\n");
	/* exit(1); */
	return -3;
    }
    
    dst[0] = pole;
    dst[1] = zero;
    
    return 0;
}



void waveform_update_logscale(struct logscale *la, double *array, int num_items, int step, SDL_Rect *container);

static double biquad_amp_from_freq(double freq_raw, double complex *pole_zero)
{
    double complex pole1 = pole_zero[0];
    double complex pole2 = conj(pole1);
    double complex zero1 = pole_zero[1];
    double complex zero2 = conj(zero1);

    double theta = PI * freq_raw;
    double complex z = cos(theta) + I * sin(theta);
    return ((zero1 - z) * (zero2 - z)) / ((pole1 - z) * (pole2 - z));

}

void iir_set_coeffs_peaknotch(IIRFilter *iir, double freq, double amp, double bandwidth)
{
    double complex pole_zero[2];
    int safety = 0;
    int test = 0;
    /* fprintf(stderr, "COEFFS: %f %f %f || %f %f\n", iir->A[0], iir->A[1], iir->A[2], iir->B[0], iir->B[1]); */
    /* fprintf(stderr, "MEM: %f %f || %f %f\n", iir->memIn[0][0], iir->memIn[0][1], iir->memOut[0][0], iir->memOut[0][1]); */
    
    while ((test = reiss_2011(freq * PI, amp, bandwidth, pole_zero)) < 0) {
	if (test == -1) {
	    /* fprintf(stderr, "IMAG!!!!\n"); */
	} else if (test == -2) {
	    /* fprintf(stderr, "INF!!!!\n"); */
	    return;
	} else if (test == -3) {
	    /* fprintf(stderr, "POLE ERROR!\n"); */
	    return;
	}
	    
	bandwidth *= 0.9;
	safety++;
	if (safety > 5000) {
	    fprintf(stderr, "ABORT: IIR parameters unstable, cannot be recovered\n");
	    exit(0);
	}
    }

    if (iir->fp) {
	for (int i=0; i<IIR_FREQPLOT_RESOLUTION; i++) {
	    double prop = (double) i / IIR_FREQPLOT_RESOLUTION;
	    int nsub1 = iir->fp->num_items - 1;
	    double input = pow(nsub1, prop) / nsub1;
	    iir->freq_resp[i] = biquad_amp_from_freq(input, pole_zero);
	}
    }
    /* iir->pole_zero[0] = pole_zero[0]; */
    /* iir->pole_zero[1] = pole_zero[1]; */

    /* DO FREQ PLOTTING */
    /* int steps[] = {1, 1}; */
    /* double *arrs[] = {proj->output_L_freq, proj->output_R_freq}; */
    /* SDL_Color *colors[] = {&freq_L_color, &freq_R_color}; */
    /* if (!fp_container) { */
    /* 	fp_container = layout_add_child(main_win->layout); */
    /* 	layout_set_default_dims(fp_container); */
    /* 	layout_force_reset(fp_container); */
    /* 	eqfp = waveform_create_freq_plot( */
    /* 	    arrs, */
    /* 	    2, */
    /* 	    colors, */
    /* 	    steps, */
    /* 	    freq_resp_len, */
    /* 	    fp_container); */

    /* } */
    

    iir->A[0] = 1.0;
    iir->A[1] = -2 * creal(pole_zero[1]);
    iir->A[2] = pow(cabs(pole_zero[1]), 2);

    iir->B[0] = 2 * creal(pole_zero[0]);
    iir->B[1] = -1 * pow(cabs(pole_zero[0]), 2);

    /* if (iir->fp) { */
    /* 	waveform_freq_plot_add_linear_plot(iir->fp, IIR_FREQPLOT_RESOLUTION, &color_global_white, amp_from_freq, pole_zero); */
    /* } */
    /* if (eqfp && eqfp->num_linear_plots == 0) { */
    /* 	waveform_freq_plot_add_linear_plot(eqfp, IIR_FREQPLOT_RESOLUTION, &color_global_white, amp_from_freq, pole_zero); */
    /* 	eqfp->linear_plot_mins[0] = 0.0; */
    /* 	eqfp->linear_plot_ranges[0] = 20.0; */
    /* } else if (eqfp) { */
    /* 	waveform_freq_plot_update_linear_plot(eqfp, amp_from_freq, pole_zero); */
    /* } */



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
    group->num_filters = num_filters;
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

void iir_group_update_freq_resp(IIRGroup *group)
{
    /* fprintf(stderr, "UPDATE freq resp\n"); */
    if (!group->fp) return;
    for (int i=0; i<IIR_FREQPLOT_RESOLUTION; i++) {
	group->freq_resp[i] = group->filters[0].freq_resp[i];
	for (int f=1; f<group->num_filters; f++) {
	    group->freq_resp[i] *= group->filters[f].freq_resp[i];
	}
    }
}
