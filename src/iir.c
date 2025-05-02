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
    f->normalization_constant = 1.0;
    f->memIn = calloc(num_channels, sizeof(double *));
    f->memOut = calloc(num_channels, sizeof(double *));
    f->type = IIR_PEAKNOTCH;
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

    memmove(f->memIn[channel] + 1, f->memIn[channel], sizeof(double) * (f->degree - 1));
    memmove(f->memOut[channel] + 1, f->memOut[channel], sizeof(double) * (f->degree - 1));
				    
    f->memIn[channel][0] = in;
    f->memOut[channel][0] = out;
    
    return out;
}

void iir_buf_apply(IIRFilter *f, float *buf, int len, int channel)
{
    for (int i=0; i<len; i++) {
	buf[i] = iir_sample(f, buf[i], channel);
    }
}

void iir_advance(IIRFilter *f, int channel)
{
    memmove(f->memIn[channel] + 1, f->memIn[channel], sizeof(double) * (f->degree - 1));
    memmove(f->memOut[channel] + 1, f->memOut[channel], sizeof(double) * (f->degree - 1));
    f->memIn[channel][0] = 0.0;
    f->memOut[channel][0] = 0.0;
}

void iir_clear(IIRFilter *f)
{
    for (int i=0; i<f->num_channels; i++) {
	memset(f->memIn[i], '\0', f->degree * sizeof(double));
	memset(f->memOut[i], '\0', f->degree * sizeof(double));
    }
}



/* SET COEFFS */


/* Joshua D. Reiss, IEEE TRANSACTIONS ON AUDIO, SPEECH, AND LANGUAGE PROCESSING, VOL. 19, NO. 6, AUGUST 2011  */
int reiss_2011(double freq, double amp, double bandwidth, double complex *pole_dst, double complex *zero_dst)
{
    double cosw = cos(freq);
    double tanbdiv2 = tan(bandwidth / 2);
    double tansqbdiv2 = pow(tanbdiv2, 2.0);
    double sinsqw = pow(sin(freq), 2.0);

    double zero_term1 = cosw / (1 + amp * tanbdiv2);
    double pole_term1 = cosw / (1 + tanbdiv2);

    double sqrt_arg = sinsqw - (pow(amp, 2) * tansqbdiv2);
    if (sqrt_arg < 0.0) {
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
	return -1;
    }

    double pole_term2_num = sqrt(sinsqw - tansqbdiv2);


    double complex zero_term2 = I * zero_term2_num / zero_term2_denom;
    double complex pole_term2 = I * pole_term2_num / pole_term2_denom;

    double complex pole = pole_term1 + pole_term2;
    double complex zero = zero_term1 + zero_term2;

    if (cabs(pole) >= 1.0) {
	return -3;
    }
    
    pole_dst[0] = pole;
    zero_dst[0] = zero;
    
    return 0;
}



void waveform_update_logscale(struct logscale *la, double *array, int num_items, int step, SDL_Rect *container);

static double biquad_amp_from_freq(double freq_raw, double complex *pole, double complex *zero, double complex *normalization_constant)
{
    double complex pole1 = pole[0];
    double complex pole2 = conj(pole1);
    double complex zero1 = zero[0];
    double complex zero2 = conj(zero1);

    double theta = PI * freq_raw;
    double complex z = cos(theta) + I * sin(theta);
    double complex ret = *normalization_constant * ((zero1 - z) * (zero2 - z)) / ((pole1 - z) * (pole2 - z));
    return cabs(ret);
}

static double double_biquad_amp_from_freq(double freq_raw, double complex *pole, double complex *zero, double complex *normalization_constant)
{
    double complex pole1 = pole[0];
    double complex pole2 = conj(pole1);
    double complex pole3 = pole[1];
    double complex pole4 = conj(pole3);
    
    double complex zero1 = zero[0];
    double complex zero2 = conj(zero1);
    double complex zero3 = zero[1];
    double complex zero4 = conj(zero3);

    double theta = PI * freq_raw;
    double complex z = cos(theta) + I * sin(theta);
    double complex ret = *normalization_constant *
	((zero1 - z) * (zero2 - z) * (zero3 - z) * (zero4 - z)) /
	((pole1 - z) * (pole2 - z) * (pole3 - z) * (pole4 - z));
    return cabs(ret);
}


static void iir_reset_freq_resp(IIRFilter *iir)
{
    if (iir->num_poles == 1) {
	for (int i=0; i<IIR_FREQPLOT_RESOLUTION; i++) {
	    double prop = (double) i / IIR_FREQPLOT_RESOLUTION;
	    int nsub1 = iir->fp->num_items - 1;
	    double input = pow(nsub1, prop) / nsub1;
	    iir->freq_resp[i] = biquad_amp_from_freq(input, iir->poles, iir->zeros, &iir->normalization_constant);
	    iir->freq_resp_stale = false;
	}
    } else if (iir->num_poles == 2) {
	for (int i=0; i<IIR_FREQPLOT_RESOLUTION; i++) {
	    double prop = (double) i / IIR_FREQPLOT_RESOLUTION;
	    int nsub1 = iir->fp->num_items - 1;
	    double input = pow(nsub1, prop) / nsub1;
	    iir->freq_resp[i] = double_biquad_amp_from_freq(input, iir->poles, iir->zeros, &iir->normalization_constant);
	    iir->freq_resp_stale = false;
	}
    }
}

void iir_set_neutral_freq_resp(IIRFilter *iir)
{
    for (int i=0; i<IIR_FREQPLOT_RESOLUTION; i++) {
	iir->freq_resp[i] = 1 + 0*I;
    }
}

/* void double_biquad_normalize_and_set_coeffs(IIRFilter *iir, double complex norm_freq, double norm_amp) */
/* { */
/*     static double epsilon = 1e-9; */

/*     double complex H = */
/* 	((norm_freq - iir->zeros[0]) * (norm_freq - conj(iir->zeros[0])) * (norm_freq - iir->zeros[1]) * (norm_freq - conj(iir->zeros[1]))) */
/* 	/ ((norm_freq - iir->poles[0]) * (norm_freq - conj(iir->poles[0])) * (norm_freq - iir->poles[1]) * (norm_freq - conj(iir->poles[1]))); */

/*     if (cabs(H) < epsilon)  */
/* 	iir->normalization_constant = 1.0; */
/*     else  */
/* 	iir->normalization_constant = norm_amp / H; */

/*     double req0 = creal(iir->zeros[0]); */
/*     double req1 = creal(iir->zeros[1]); */
/*     double rep0 = creal(iir->poles[0]); */
/*     double rep1 = creal(iir->poles[1]); */

/*     double magq0 = pow(cabs(iir->zeros[0]), 2.0); */
/*     double magq1 = pow(cabs(iir->zeros[1]), 2.0); */
/*     double magp0 = pow(cabs(iir->poles[0]), 2.0); */
/*     double magp1 = pow(cabs(iir->poles[1]), 2.0); */
/*     /\* fprint(stderr, "NORM CONST: %f\n", creal(iir->normalization_constant)); *\/ */
/*     /\* fprintf(stderr, "NORM %f + %fi (mag %f)\n", creal(iir->normalization_constant), cimag(iir->normalization_constant), cabs(iir->normalization_constant)); *\/ */
    
/*     iir->A[0] = iir->normalization_constant; */
/*     iir->A[1] = (-2 * req0 - 2 * req1) * iir->normalization_constant; */
/*     iir->A[2] = (-1 * magq0 + 4 * req0 * req1 -  magq1) * iir->normalization_constant; */
/*     iir->A[3] = (magq0 * 2 * req1 + magq1 * 2 * req0) * iir->normalization_constant; */
/*     iir->A[4] = (magq0 * magq1) * iir->normalization_constant; */

/*     iir->B[0] = (2 * rep0 + 2 * rep1); */
/*     iir->B[1] = magp0 - 4 * rep0 * rep1 + magp1; */
/*     iir->B[2] = -1 * magp0 * 2 * rep1 - magp1 * 2 * rep0; */
/*     iir->B[3] = -1 * magp0 * magp1; */


/*     /\* iir->B[0] = 2 * creal(iir->poles[0]); *\/ */
/*     /\* iir->B[1] = -1 * pow(cabs(iir->poles[0]), 2); *\/ */

/* } */

void biquad_normalize_and_set_coeffs(IIRFilter *iir, double complex norm_freq, double norm_amp)
{
    static double epsilon = 1e-9;

    double complex H = ((norm_freq - iir->zeros[0]) * (norm_freq - conj(iir->zeros[0]))) / ((norm_freq - iir->poles[0]) * (norm_freq - conj(iir->poles[0])));

    if (cabs(H) < epsilon) 
	iir->normalization_constant = 1.0;
    else 
	iir->normalization_constant = norm_amp / H;

    /* fprint(stderr, "NORM CONST: %f\n", creal(iir->normalization_constant)); */
    /* fprintf(stderr, "NORM %f + %fi (mag %f)\n", creal(iir->normalization_constant), cimag(iir->normalization_constant), cabs(iir->normalization_constant)); */
    
    iir->A[0] = iir->normalization_constant;
    iir->A[1] = -2 * creal(iir->zeros[0]) * iir->normalization_constant;
    iir->A[2] = pow(cabs(iir->zeros[0]), 2) * iir->normalization_constant;

    iir->B[0] = 2 * creal(iir->poles[0]);
    iir->B[1] = -1 * pow(cabs(iir->poles[0]), 2);
}

int iir_set_coeffs_lowpass(IIRFilter *iir, double freq)
{
    freq *= PI;
    double freq_prewarp = 2 * tan(freq / 2);

    double complex exp_term = cexp(I * 3 * PI / 4);
    double complex p0 = (2 + freq_prewarp * exp_term) / (2 - freq_prewarp * exp_term);
    /* double complex p1 = conj(p0); */

    iir->poles[0] = p0;
    iir->zeros[0] = -1;
    iir->num_poles = 1;
    iir->num_zeros = 1;

    biquad_normalize_and_set_coeffs(iir, 1, 1);

    if (iir->fp) {
	iir_reset_freq_resp(iir);
    } else {
	iir->freq_resp_stale = true;
    }
    return 0;
}

int iir_set_coeffs_highpass(IIRFilter *iir, double freq)
{
    freq *= PI;
    double freq_prewarp = 2 * tan(freq / 2);

    double complex exp_term = cexp(I * 3 * PI / 4);
    double complex p0 = (2 + freq_prewarp * exp_term) / (2 - freq_prewarp * exp_term);
    /* double complex p1 = conj(p0); */

    iir->poles[0] = p0;
    iir->zeros[0] = 1;
    iir->num_poles = 1;
    iir->num_zeros = 1;

    biquad_normalize_and_set_coeffs(iir, 0, 1);
    
    if (iir->fp) {
	iir_reset_freq_resp(iir);
    } else {
	iir->freq_resp_stale = true;
    }
    return 0;
}


int iir_set_coeffs_lowshelf(IIRFilter *iir, double freq, double amp)
{
    static const double epsilon = 1e-9;
    freq *= PI;
    double freq_prewarp = 2 * tan(freq / 2);

    double complex exp_term = cexp(I * 3 * PI / 4);
    double complex p0 = (2 + freq_prewarp * exp_term) / (2 - freq_prewarp * exp_term);
    double complex z0;
    if (fabs(amp) - 0.0 > epsilon) {   
	double complex scaled_exp = freq_prewarp * exp_term * sqrt(amp);
	z0 = (2 + scaled_exp) / (2 - scaled_exp);
    } else {
	z0 = 1; /* Highpass case */
    }
    iir->poles[0] = p0;
    iir->zeros[0] = z0;
    iir->num_poles = 1;
    iir->num_zeros = 1;

    /* fprintf(stderr, "\nfreq: %f; amp: %f; pole: %f+%fi, zero: %f+%fi\n", freq, amp, creal(p0), cimag(p0), creal(z0), cimag(z0)); */
    biquad_normalize_and_set_coeffs(iir, 1, amp);
    /* fprintf(stderr, "norm: %f+%fi\n", creal(iir->normalization_constant), cimag(iir->normalization_constant)); */


    if (iir->fp) {
	iir_reset_freq_resp(iir);
    } else {
	iir->freq_resp_stale = true;
    }
    return 0;
}


int iir_set_coeffs_highshelf_double_butterworth(IIRFilter *iir1, IIRFilter *iir2, double freq, double amp)
{
    /* static const double epsilon = 1e-9; */
    freq *= PI;
    double freq_prewarp = 2 * tan(freq / 2);

    double complex exp_term1 = cexp(I * 5 * PI / 8);
    double complex exp_term2 = cexp(I * 7 * PI / 8);
    double complex p0 = (2 + freq_prewarp * exp_term1) / (2 - freq_prewarp * exp_term1);
    double complex p1 = (2 + freq_prewarp * exp_term2) / (2 - freq_prewarp * exp_term2);
    double complex z0 = -1;
    double complex z1 = -1;
    /* if (fabs(amp) - 0.0 > epsilon) {	 */
    /* 	double complex scaled_exp = freq_prewarp * exp_term / sqrt(amp); */
    /* 	z0 = (2 + scaled_exp) / (2 - scaled_exp); */
    /* } else { /\* Lowpass case *\/ */
    /* 	z0 = -1; */
    /* } */
    iir1->poles[0] = p0;
    iir2->poles[0] = p1;
    iir1->zeros[0] = z0;
    iir2->zeros[0] = z1;
    iir1->num_poles = 1;
    iir1->num_zeros = 1;
    iir2->num_poles = 1;
    iir2->num_zeros = 1;


    /* fprintf(stderr, "\nfreq: %f; amp: %f; pole: %f+%fi, zero: %f+%fi\n", freq, amp, creal(p0), cimag(p0), creal(z0), cimag(z0)); */
    biquad_normalize_and_set_coeffs(iir1, 1, 1);
    biquad_normalize_and_set_coeffs(iir2, 1, 1);
    /* double_biquad_normalize_and_set_coeffs(iir, 1, 1); */
    /* fprintf(stderr, "norm: %f+%fi\n", creal(iir->normalization_constant), cimag(iir->normalization_constant)); */
    if (iir1->fp) {
	iir_reset_freq_resp(iir1);
    } else {
	iir1->freq_resp_stale = true;
    }
    if (iir2->fp) {
	iir_reset_freq_resp(iir2);
    } else {
	iir2->freq_resp_stale = true;
    }

    return 0;
}

int iir_set_coeffs_highshelf_double(IIRFilter *iir1, IIRFilter *iir2, double freq, double amp)
{
    static const double epsilon = 1e-9;
    freq *= PI;
    double freq_prewarp = 2 * tan(freq / 2);

    double ripple = amp;
    double complex G1 = 1 * csqrt((-2 + csqrt((2 * ripple + 2 * I) / ripple))/4);
    double complex G2 = 1 * csqrt((-2 - csqrt((2 * ripple + 2 * I) / ripple))/4);
    double complex p0 = (2 - freq_prewarp * G1) / (2 + freq_prewarp * G1);
    double complex p1 = (2 - freq_prewarp * G2) / (2 + freq_prewarp * G2);
    double complex z0 = -1;
    double complex z1 = -1;
    /* if (fabs(amp) - 0.0 > epsilon) {	 */
    /* 	double complex scaled_exp = freq_prewarp * exp_term / sqrt(amp); */
    /* 	z0 = (2 + scaled_exp) / (2 - scaled_exp); */
    /* } else { /\* Lowpass case *\/ */
    /* 	z0 = -1; */
    /* } */
    iir1->poles[0] = p0;
    iir2->poles[0] = p1;
    iir1->zeros[0] = z0;
    iir2->zeros[0] = z1;
    iir1->num_poles = 1;
    iir1->num_zeros = 1;
    iir2->num_poles = 1;
    iir2->num_zeros = 1;


    /* fprintf(stderr, "\nfreq: %f; amp: %f; pole: %f+%fi, zero: %f+%fi\n", freq, amp, creal(p0), cimag(p0), creal(z0), cimag(z0)); */
    biquad_normalize_and_set_coeffs(iir1, 1, 1);
    biquad_normalize_and_set_coeffs(iir2, 1, 1);
    /* double_biquad_normalize_and_set_coeffs(iir, 1, 1); */
    /* fprintf(stderr, "norm: %f+%fi\n", creal(iir->normalization_constant), cimag(iir->normalization_constant)); */
    if (iir1->fp) {
	iir_reset_freq_resp(iir1);
    } else {
	iir1->freq_resp_stale = true;
    }
    if (iir2->fp) {
	iir_reset_freq_resp(iir2);
    } else {
	iir2->freq_resp_stale = true;
    }

    return 0;
}


int iir_set_coeffs_highshelf(IIRFilter *iir, double freq, double amp)
{
    static const double epsilon = 1e-9;
    freq *= PI;
    double freq_prewarp = 2 * tan(freq / 2);

    double complex exp_term = cexp(I * 3 * PI / 4);
    double complex p0 = (2 + freq_prewarp * exp_term) / (2 - freq_prewarp * exp_term);
    double complex z0;
    if (fabs(amp) - 0.0 > epsilon) {	
	double complex scaled_exp = freq_prewarp * exp_term / sqrt(amp);
	z0 = (2 + scaled_exp) / (2 - scaled_exp);
    } else { /* Lowpass case */
	z0 = -1;
    }
    iir->poles[0] = p0;
    iir->zeros[0] = z0;
    iir->num_poles = 1;
    iir->num_zeros = 1;

    /* fprintf(stderr, "\nfreq: %f; amp: %f; pole: %f+%fi, zero: %f+%fi\n", freq, amp, creal(p0), cimag(p0), creal(z0), cimag(z0)); */
    biquad_normalize_and_set_coeffs(iir, 1, 1);
    /* fprintf(stderr, "norm: %f+%fi\n", creal(iir->normalization_constant), cimag(iir->normalization_constant)); */
    if (iir->fp) {
	iir_reset_freq_resp(iir);
    } else {
	iir->freq_resp_stale = true;
    }
    return 0;
}

int iir_set_coeffs_lowpass_chebyshev(IIRFilter *iir, double freq, double amp)
{
    /* static const double epsilon = 1e-9; */
    double ripple = amp;
    freq *= PI;
    double freq_prewarp = 2 * tan(freq / 2);

    double complex exp_term = csqrt(-2.0 + (2.0 / ripple) * I);
    double complex exp_term_scaled = exp_term * freq_prewarp;
    double complex p0 = (2 - exp_term_scaled) / (2 + exp_term_scaled);    
    /* double complex exp_term = cexp(I * 3 * PI / 4); */
    /* double complex p0 = (2 + freq_prewarp * exp_term) / (2 - freq_prewarp * exp_term); */
    /* double complex z0; */
    /* if (fabs(amp) - 0.0 > epsilon) {	 */
    /* 	double complex scaled_exp = freq_prewarp * exp_term / sqrt(amp); */
    /* 	z0 = (2 + scaled_exp) / (2 - scaled_exp); */
    /* } else { /\* Lowpass case *\/ */
    /* 	z0 = -1; */
    /* } */
    iir->poles[0] = p0;
    iir->zeros[0] = -1;
    iir->num_poles = 1;
    iir->num_zeros = 1;

    /* fprintf(stderr, "\nfreq: %f; amp: %f; pole: %f+%fi, zero: %f+%fi\n", freq, amp, creal(p0), cimag(p0), creal(z0), cimag(z0)); */
    biquad_normalize_and_set_coeffs(iir, 1, 1);
    /* fprintf(stderr, "norm: %f+%fi\n", creal(iir->normalization_constant), cimag(iir->normalization_constant)); */
    if (iir->fp) {
	iir_reset_freq_resp(iir);
    } else {
	iir->freq_resp_stale = true;
    }
    return 0;
}





int iir_set_coeffs_peaknotch(IIRFilter *iir, double freq, double amp, double bandwidth, double *legal_bandwidth_scalar)
{
    /* double complex pole_zero[2]; */
    int safety = 0;
    int test = 0;
    int ret = 0;
    
    iir->num_poles = 1;
    iir->num_zeros = 1;
    while ((test = reiss_2011(freq * PI, amp, bandwidth, iir->poles, iir->zeros)) < 0) {
	if (test == -1) {
	    /* fprintf(stderr, "IMAG!!!!\n"); */
	} else if (test == -2) {
	    /* fprintf(stderr, "INF!!!!\n"); */
	    return -1;
	} else if (test == -3) {
	    /* fprintf(stderr, "POLE ERROR!\n"); */
	    return -3;
	}
	    
	bandwidth *= 0.9;
	safety++;
	if (safety > 5000) {
	    fprintf(stderr, "ABORT: IIR parameters unstable, cannot be recovered\n");
	    exit(0);
	}
	ret = 1;
    }
    if (ret == 1 && legal_bandwidth_scalar) {
	*legal_bandwidth_scalar = bandwidth / freq;
    }

    biquad_normalize_and_set_coeffs(iir, 1, 1);
    
    if (iir->fp) {
	iir_reset_freq_resp(iir);
    } else {
	iir->freq_resp_stale = true;
    }
    /* iir->normalization_constant = 1.0; */

    return ret;
}


/* int iir_set_coeffs_shelving(IIRFilter *iir, double freq_raw, double amp_raw, double Q) */
/* { */

/*     fprintf(stderr, "FREQ: %f, AMP: %f Q: %f\n", freq_raw, amp_raw, Q); */
/*     double K = tan(PI * freq_raw); */
/*     double V0 = amp_raw; */
/*     double root2 = 1/Q; */
/*     double b0 = (1 + sqrt(V0)*root2*K + V0*(pow(K, 2))) / (1 + root2*K + pow(K, 2)); */
/*     double b1 =             (2 * (V0*pow(K, 2) - 1) ) / (1 + root2*K + pow(K, 2)); */
/*     double b2 = (1 - sqrt(V0)*root2*K + V0*pow(K, 2)) / (1 + root2*K + pow(K, 2)); */
/*     double a1 =                (2 * (pow(K, 2) - 1) ) / (1 + root2*K + pow(K, 2)); */
/*     double a2 =             (1 - root2*K + pow(K, 2)) / (1 + root2*K + pow(K, 2)); */

/*     iir->A[0] = 1.0; */
/*     iir->A[1] = a1; */
/*     iir->A[2] = a2; */
/*     iir->B[0] = b0; */
/*     iir->B[1] = b1; */
/*     if (iir->fp) { */
/* 	iir_reset_freq_resp(iir); */
/* 	/\* for (int i=0; i<IIR_FREQPLOT_RESOLUTION; i++) { *\/ */
/* 	/\*     double prop = (double) i / IIR_FREQPLOT_RESOLUTION; *\/ */
/* 	/\*     int nsub1 = iir->fp->num_items - 1; *\/ */
/* 	/\*     double input = pow(nsub1, prop) / nsub1; *\/ */
/* 	/\*     iir->freq_resp[i] = biquad_amp_from_freq(input, iir->pole_zero); *\/ */
/* 	/\* } *\/ */
/*     } else { */
/* 	iir->freq_resp_stale = true; */
/*     } */

/*     /\* iir->B[2] = b2; *\/ */
/*     return 0; */
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
    if (!group->fp) return;
    for (int i=0; i<group->num_filters; i++) {
	if (group->filters[i].freq_resp_stale) {
	    iir_reset_freq_resp(group->filters + i);
	}
    }
    for (int i=0; i<IIR_FREQPLOT_RESOLUTION; i++) {
	group->freq_resp[i] = group->filters[0].freq_resp[i];
	for (int f=1; f<group->num_filters; f++) {
	    group->freq_resp[i] *= group->filters[f].freq_resp[i];
	}
    }
}

void iir_group_clear(IIRGroup *group)
{
    for (int i=0; i<group->num_filters; i++) {
	iir_clear(group->filters + i);
    }
}
