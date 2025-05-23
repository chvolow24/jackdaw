/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    iir.h

    * Define types related to infinite impulse response filters
    * Filters are stateful
    * Interface for parameter adjustment and sample- or buffer-wise application
 *****************************************************************************************************************/

#ifndef JDAW_IIR_H
#define JDAW_IIR_H

#include <complex.h>
#include <stdbool.h>
#include <stdint.h>

#define IIR_FREQPLOT_RESOLUTION 1600
#define IIR_MAX_DEGREE 10

typedef enum iir_filter_type {
    IIR_LOWSHELF=0,
    IIR_PEAKNOTCH=1,
    IIR_HIGHSHELF=2,
} IIRFilterType;

typedef struct iir_filter {
    IIRFilterType type;
    int degree;
    int num_channels;
    double *A; /* input delay coeffs */
    double *B; /* output delay coeffs */
    double **memIn;
    double **memOut;

    struct freq_plot *fp;
    double freq_resp[IIR_FREQPLOT_RESOLUTION];

    bool freq_resp_stale;
    /* double complex pole_zero[2]; */
    double complex poles[IIR_MAX_DEGREE];
    uint8_t num_poles;
    double complex zeros[IIR_MAX_DEGREE];
    uint8_t num_zeros;

    double complex normalization_constant;

} IIRFilter;

typedef struct iir_group {
    int num_filters;
    IIRFilter *filters;
    struct freq_plot *fp;
    double freq_resp[IIR_FREQPLOT_RESOLUTION];
} IIRGroup;


void iir_init(IIRFilter *f, int degree, int num_channels);
void iir_deinit(IIRFilter *f);
void iir_set_neutral_freq_resp(IIRFilter *iir);
void iir_set_coeffs(IIRFilter *f, double *A_in, double *B_in);
double iir_sample(IIRFilter *f, double in, int channel);
void iir_buf_apply(IIRFilter *f, float *buf, int len, int channel);
/* void iir_set_coeffs_peaknotch(IIRFilter *iir, double freq, double amp, double bandwidth); */
int iir_set_coeffs_peaknotch(IIRFilter *iir, double freq, double amp, double bandwidth, double *legal_bandwidth_scalar);
/* int iir_set_coeffs_lowpass(IIRFilter *iir, double freq); */
/* int iir_set_coeffs_highpass(IIRFilter *iir, double freq); */
int iir_set_coeffs_lowshelf(IIRFilter *iir, double freq, double amp);
int iir_set_coeffs_highshelf(IIRFilter *iir, double freq, double amp);
int iir_set_coeffs_highshelf_double(IIRFilter *iir1, IIRFilter *iir2, double freq, double amp);


void iir_group_init(IIRGroup *group, int num_filters, int degree, int num_channels);
void iir_group_deinit(IIRGroup *group);
double iir_group_sample(IIRGroup *group, double in, int channel);
void iir_group_update_freq_resp(IIRGroup *group);
void iir_group_clear(IIRGroup *group);
void iir_advance(IIRFilter *f, int channel);

#endif
