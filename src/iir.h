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

#define IIR_FREQPLOT_RESOLUTION 1000

typedef struct iir_filter {
    int degree;
    int num_channels;
    double *A; /* input delay coeffs */
    double *B; /* output delay coeffs */
    double **memIn;
    double **memOut;

    struct freq_plot *fp;
    double freq_resp[IIR_FREQPLOT_RESOLUTION];
    
    /* double complex *pole_zero; */

} IIRFilter;

typedef struct iir_group {
    int num_filters;
    IIRFilter *filters;

    struct freq_plot *fp;
    double freq_resp[IIR_FREQPLOT_RESOLUTION];
} IIRGroup;


void iir_init(IIRFilter *f, int degree, int num_channels);
void iir_deinit(IIRFilter *f);
void iir_set_coeffs(IIRFilter *f, double *A_in, double *B_in);
double iir_sample(IIRFilter *f, double in, int channel);
void iir_set_coeffs_peaknotch(IIRFilter *iir, double freq, double amp, double bandwidth);


void iir_group_init(IIRGroup *group, int num_filters, int degree, int num_channels);
void iir_group_deinit(IIRGroup *group);
double iir_group_sample(IIRGroup *group, double in, int channel);
void iir_group_update_freq_resp(IIRGroup *group);

#endif
