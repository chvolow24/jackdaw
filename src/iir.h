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

typedef struct iir_filter {
    int degree;
    double *A; /* input delay coeffs */
    double *B; /* output delay coeffs */
    double *memIn;
    double *memOut;
} IIRFilter;


void iir_init(IIRFilter *f, int degree);
void iir_deinit(IIRFilter *f);
void iir_set_coeffs(IIRFilter *f, double *A_in, double *B_in);
double iir_sample(IIRFilter *f, double in);

void iir_set_coeffs_peaknotch(IIRFilter *iir, double freq, double amp, double bandwidth);

#endif
