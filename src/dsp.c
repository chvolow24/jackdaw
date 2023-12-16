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


#include <complex.h>
#include "project.h"
#include "gui.h"
#include "dsp.h"


#define TAU (6.283185307179586476925286766559)
#define PI (3.1415926535897932384626433832795)
#define ROU_MAX_DEGREE 25
double complex *roots_of_unity[ROU_MAX_DEGREE];

extern Project *proj;



static void init_roots_of_unity();

void init_dsp()
{
    init_roots_of_unity();
}

// void process_clip_vol_and_pan(Clip *clip)
// {
// //         Track *track = clip->track;
// //         float lpan, rpan, panctrlval;
// //         panctrlval = track->pan_ctrl->value;
// //         lpan = panctrlval < 0 ? 1 : 1 - panctrlval;
// //         rpan = panctrlval > 0 ? 1 : 1 + panctrlval;
// //         // uint8_t k=0;
// //         for (uint32_t j=0; j<clip->len_sframes * clip->channels; j+=2) {
// //             // pan = j%2==0 ? lpan : rpan;
// //             // if (k<20) {
// //             //     k++;
// //             //     fprintf(stderr, "\t\t->sample %d, pan value: %f\n", j, pan);
// //             // }
// //             clip->post_proc[j] = clip->pre_proc[j] * track->vol_ctrl->value * lpan;
// //             clip->post_proc[j+1] = clip->pre_proc[j+1] * track->vol_ctrl->value * rpan;
// //         }
// }
 
// void process_track_vol_and_pan(Track *track)
// {
//     // Clip *clip = NULL;
//     // float lpan, rpan, panctrlval;
//     // panctrlval = track->pan_ctrl->value;
//     // lpan = panctrlval < 0 ? 1 : 1 - panctrlval;
//     // rpan = panctrlval > 0 ? 1 : 1 + panctrlval;
//     for (uint8_t i=0; i<track->num_clips; i++) {
//         Clip *clip = track->clips[i];
//         process_clip_vol_and_pan(clip);
//         // uint8_t k=0;
//         // for (uint32_t j=0; j<clip->len_sframes * clip->channels; j+=2) {
//         //     // pan = j%2==0 ? lpan : rpan;
//         //     // if (k<20) {
//         //     //     k++;
//         //     //     fprintf(stderr, "\t\t->sample %d, pan value: %f\n", j, pan);
//         //     // }
//         //     clip->post_proc[j] = clip->pre_proc[j] * track->vol_ctrl->value * lpan;
//         //     clip->post_proc[j+1] = clip->pre_proc[j+1] * track->vol_ctrl->value * rpan;
//         // }
//     }

// }

// void process_vol_and_pan()
// {
//     if (!proj) {
//         fprintf(stderr, "Error: request to process vol and pan for nonexistent project.\n");
//         return;
//     }
//     Track *track = NULL;
//     for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
//         track = proj->tl->tracks[i];
//         process_track_vol_and_pan(track);
//     }
// }



/*****************************************************************************************************************
    FFT and helper functions
 *****************************************************************************************************************/


static double complex *gen_FFT_X(int n)
{
    double complex *X = (double complex *)malloc(sizeof(double complex) * n);
    double theta_increment = TAU / n;
    double theta = 0;
    for (int i=0; i<n; i++) {
        X[i] = cos(theta) + I * sin(theta);
        theta += theta_increment;
    }
    return X;
}

/* Generate the roots of unity to be used in FFT evaluation */
static void init_roots_of_unity()
{
    for (uint8_t i=0; i<ROU_MAX_DEGREE; i++) {
        roots_of_unity[i] = gen_FFT_X(pow(2, i));
    }
}

static double complex *FFT_inner(double *A, int n, int offset, int increment)
{
    double complex *B = (double complex *)malloc(sizeof(double complex) * n);

    if (n==1) {
        B[0] = A[offset] + 0 * I;
        return B;
    }

    int halfn = n>>1;
    int degree = log2(n);
    double complex *X = roots_of_unity[degree];
    int doubleinc = increment << 1;
    double complex *Beven = FFT_inner(A, halfn, offset, doubleinc);
    double complex *Bodd = FFT_inner(A, halfn, offset + increment, doubleinc);
    for (int k=0; k<halfn; k++) {
        double complex odd_part_by_x = Bodd[k] * conj(X[k]);
        B[k] = Beven[k] + odd_part_by_x;

        B[k + halfn] = Beven[k] - odd_part_by_x;

    }

    free(Beven);
    free(Bodd);
    return B;

}

// static double complex *FFT_float_inner(float *A, int n, int offset, int increment)
// {
//     double complex *B = (double complex *)malloc(sizeof(double complex) * n);

//     if (n==1) {
//         B[0] = A[offset] + 0 * I;
//         return B;
//     }

//     int halfn = n>>1;
//     int degree = log2(n);
//     double complex *X = roots_of_unity[degree];
//     int doubleinc = increment << 1;
//     double complex *Beven = FFT_float_inner(A, halfn, offset, doubleinc);
//     double complex *Bodd = FFT_float_inner(A, halfn, offset + increment, doubleinc);
//     for (int k=0; k<halfn; k++) {

//         double complex odd_part_by_x = Bodd[k] * conj(X[k]);
//         B[k] = Beven[k] + odd_part_by_x;

//         B[k + halfn] = Beven[k] - odd_part_by_x;

//     }

//     free(Beven);
//     free(Bodd);
//     return B;

// }

double complex *FFT(double *A, int n)
{

    double complex *B = FFT_inner(A, n, 0, 1);

    for (int k=0; k<n; k++) {
        B[k]/=n;
    }
    return B;
}

// double complex *FFT_float(float *A, int n)
// {

//     double complex *B = FFT_float_inner(A, n, 0, 1);

//     for (int k=0; k<n; k++) {
//         B[k]/=n;
//     }
//     return B;
// }


/* I'm not sure why using an "unscaled" version of the FFT appears to work when getting the frequency response
for the FIR filters defined below (e.g. "bandpass_IR()"). The normal, scaled version produces values that are
too small */
static double complex *FFT_unscaled(double *A, int n) 
{
    double complex *B = FFT_inner(A, n, 0, 1);
    return B;
}

static double complex *IFFT_inner(double complex *A, int n, int offset, int increment)
{
    double complex *B = (double complex *)malloc(sizeof(double complex) * n);

    if (n==1) {
        B[0] = A[offset];
        return B;
    }
    int halfn = n>>1;
    int degree = log2(n);
    double complex *X = roots_of_unity[degree];
    int doubleinc = increment << 1;
    double complex *Beven = IFFT_inner(A, halfn, offset, doubleinc);
    double complex *Bodd = IFFT_inner(A, halfn, offset + increment, doubleinc);
    for (int k=0; k<halfn; k++) {
        double complex odd_part_by_x = Bodd[k] * X[k];
        B[k] = Beven[k] + odd_part_by_x;
        B[k + halfn] = Beven[k] - odd_part_by_x;
    }
    free(Beven);
    free(Bodd);
    return B;

}

double complex *IFFT(double complex *A, int n)
{
    double complex *B = IFFT_inner(A, n, 0, 1);
    return B;
}

double complex *IFFT_real_input(double *A_real, int n)
{
    double complex A[n];
    for (int i=0; i<n; i++) {
        A[i] = A_real[i] + 0 * I;
    }
    return IFFT(A, n);
}


double *get_magnitude(double complex *A, int len) 
{
    double *B = (double *)malloc(sizeof(double) * len);
    for (int i=0; i<len; i++) {
        B[i] = cabs(A[i]);
    }
    return B;
}

/* Take a complex array and return a new, dynamically allocated array with only the real component */
double *get_real_component(double complex *A, int len)
{
    double *B = (double *)malloc(sizeof(double) * len);
    for (int i=0; i<len; i++) {
        // std::cout << "Complex: " << A[i].real() << " + " << A[i].imag() << std::endl;
        B[i] = creal(A[i]);
    }
    return B;
}

/* Hamming window function */
double hamming(int x, int lenw)
{
    return 0.54 - 0.46 * cos(TAU * x / lenw);
}




/*****************************************************************************************************************
   Impulse response functions

   Create sinc or sinc-like curves representing time-domain impulse responses. Pad with zeroes and FFT to get 
    the frequency response of the filters.
 *****************************************************************************************************************/



double lowpass_IR(int x, int offset, double cutoff)
{
    cutoff *= TAU;
    if (x-offset == 0) {
        return cutoff / PI;
    } else {
        return sin(cutoff * (x-offset))/(PI * (x-offset));
    }
}

double highpass_IR(int x, int offset, double cutoff)
{
    cutoff *= TAU;
    if (x-offset == 0) {
        return 1 - (cutoff / PI);
    } else {
        return (1.0 / (PI * (x-offset))) * (sin(PI * (x-offset)) - sin(cutoff * (x-offset)));
    }
}

double bandpass_IR(int x, int offset, double center_freq, double bandwidth) 
{
    
    double freq_lower = TAU * (center_freq - bandwidth/2);
    double freq_higher = TAU * (center_freq + bandwidth/2);
    if (freq_lower < 0) {
        freq_lower = 0;
    }
    if (freq_higher > PI) {
        freq_higher = PI;
    }
    if (x-offset == 0) {
        return (freq_higher - freq_lower) / PI;
    } else {
        return (1/(PI * (x-offset))) * (sin(freq_higher * (x-offset)) - sin(freq_lower * (x-offset)));
    }

}

double bandcut_IR(int x, int offset, double center_freq, double bandwidth) 
{
    double freq_lower = TAU * (center_freq - bandwidth/2);
    double freq_higher = TAU * (center_freq + bandwidth/2);
    if (freq_lower < 0) {
        freq_lower = 0;
    }
    if (freq_higher > PI) {
        freq_higher = PI;
    }
    // if (x-offset == 0) {
    //     return 0;
    // } else {
        return highpass_IR(x, offset, freq_higher / TAU) + lowpass_IR(x, offset, freq_lower / TAU);
    // }
}

/*****************************************************************************************************************
   Public filter creation / modification
 *****************************************************************************************************************/



/* Create an empty FIR filter and allocate space for its buffers. MUST be initialized with 'set_filter_params'*/
FIRFilter *create_FIR_filter(FilterType type, uint16_t impulse_response_len, uint16_t frequency_response_len) 
{
    FIRFilter *filter = malloc(sizeof(FIRFilter));
    if (!filter) {
        fprintf(stderr, "Error: unable to allocate space for FIR Filter\n");
        return NULL;
    }
    filter->type = type;
    filter->impulse_response_len = impulse_response_len;
    filter->frequency_response_len = frequency_response_len;
    filter->impulse_response = malloc(sizeof(double) * impulse_response_len);
    filter->frequency_response = NULL;
    filter->overlap_buffer_L = NULL;
    filter->overlap_buffer_R = NULL;
    filter->overlap_len = impulse_response_len - 1;
    return filter;
}

/* Bandwidth param only required for band-pass and band-cut filters */
void set_FIR_filter_params(FIRFilter *filter, double cutoff, double bandwidth)
{
    filter->cutoff_freq = cutoff;
    filter->bandwidth = bandwidth;
    double *ir = filter->impulse_response;
    uint16_t len = filter->impulse_response_len;
    uint16_t offset = len/2;
    switch (filter->type) {
        case LOWPASS:
            for (uint16_t i=0; i<len; i++) {
                ir[i] = lowpass_IR(i, offset, cutoff) * hamming(i, len);
            } 
            break;
        case HIGHPASS:
            for (uint32_t i=0; i<len; i++) {
                ir[i] = highpass_IR(i, offset, cutoff) * hamming(i, len);
            } 
            break;
        case BANDPASS:
            for (uint32_t i=0; i<len; i++) {
                ir[i] = bandpass_IR(i, offset, cutoff, bandwidth) * hamming(i, len);
            } 
            break;
        case BANDCUT:
            for (uint32_t i=0; i<len; i++) {
                ir[i] = bandcut_IR(i, offset, cutoff, bandwidth) * hamming(i, len);
            } 
            break;
    }

    double *IR_zero_padded = malloc(sizeof(double) * filter->frequency_response_len);
    for (uint16_t i=0; i<filter->frequency_response_len; i++) {
        if (i<len) {
            IR_zero_padded[i] = ir[i];
        } else {
            IR_zero_padded[i] = 0;
        }
    }
    filter->frequency_response = FFT_unscaled(IR_zero_padded, filter->frequency_response_len);
    free(IR_zero_padded);
}

void destroy_filter(FIRFilter *filter) 
{
    if (filter->frequency_response) {
        free(filter->frequency_response);
        filter->frequency_response = NULL;
    }
    if (filter->impulse_response) {
        free(filter->impulse_response);
        filter->impulse_response = NULL;
    }
    free(filter);
}




/* Destructive; replaces values in sample_array */
void apply_filter(FIRFilter *filter, uint8_t channel, uint16_t chunk_size, float *sample_array)
{

    double *overlap_buffer = channel == 0 ? filter->overlap_buffer_L : filter->overlap_buffer_R;
    if (filter->overlap_len != filter->impulse_response_len - 1) {
        filter->overlap_len = filter->impulse_response_len - 1;
        if (overlap_buffer) {
            free(overlap_buffer);
        }
    }
    if (!overlap_buffer) {
        overlap_buffer = malloc(sizeof(double) * filter->overlap_len);
        memset(overlap_buffer, '\0', sizeof(double) * filter->overlap_len);
        if (channel == 0) {
            filter->overlap_buffer_L = overlap_buffer;
        } else {
            filter->overlap_buffer_R = overlap_buffer;
        }
    }
    
    uint16_t padded_len = filter->frequency_response_len;
    double *padded_chunk = malloc(sizeof(double) * padded_len);
    for (uint16_t i=0; i<padded_len; i++) {
        if (i<chunk_size) {
            padded_chunk[i] = sample_array[i];
        } else {
            padded_chunk[i] = 0.0;
        }

    }
    double complex *freq_domain = FFT(padded_chunk, padded_len);
    free(padded_chunk);
    for (uint16_t i=0; i<padded_len; i++) {
        freq_domain[i] *= filter->frequency_response[i];
    }
    complex double *time_domain = IFFT(freq_domain, padded_len);
    free(freq_domain);
    double *real = get_real_component(time_domain, padded_len);
    free(time_domain);

    for (uint16_t i=0; i<chunk_size + filter->overlap_len; i++) {
        if (i<chunk_size) {
            sample_array[i] = real[i];
            if (i<filter->overlap_len) {
                sample_array[i] += overlap_buffer[i];
            }
        } else {
            overlap_buffer[i-chunk_size] = real[i];
        }
    }
}


void apply_track_filters(Track *track, uint8_t channel, uint16_t chunk_size, float *sample_array) 
{
    if (track->num_filters == 0) {
        return;
    }

    double *overlap_buffer = channel == 0 ? track->overlap_buffer_L : track->overlap_buffer_R;

    if (!overlap_buffer) {
        overlap_buffer = malloc(sizeof(double) *track->overlap_len);
        memset(overlap_buffer, '\0', sizeof(double) * track->overlap_len);
        if (channel == 0) {
            track->overlap_buffer_L = overlap_buffer;
        } else {
            track->overlap_buffer_R = overlap_buffer;
        }
    }
    
    uint16_t padded_len = proj->chunk_size_sframes * 2;
    double *padded_chunk = malloc(sizeof(double) * padded_len);
    for (uint16_t i=0; i<padded_len; i++) {
        if (i<chunk_size) {
            padded_chunk[i] = sample_array[i];
        } else {
            padded_chunk[i] = 0.0;
        }

    }
    double complex *freq_domain = FFT(padded_chunk, padded_len);
    free(padded_chunk);

    double complex *freq_response_sum = malloc(sizeof(double complex) * padded_len);
    memset(freq_response_sum, '\0', sizeof(double complex) * padded_len);
    for (uint8_t f=0; f<track->num_filters; f++) {
        FIRFilter *filter = track->filters[f];
        for (uint16_t i=0; i<filter->frequency_response_len; i++) {
            freq_response_sum[i] += filter->frequency_response[i];
        }
    }
    for (uint16_t i=0; i<padded_len; i++) {
        freq_domain[i] *= freq_response_sum[i];
    }
    complex double *time_domain = IFFT(freq_domain, padded_len);
    free(freq_domain);
    double *real = get_real_component(time_domain, padded_len);
    free(time_domain);

    for (uint16_t i=0; i<chunk_size + track->overlap_len; i++) {
        if (i<chunk_size) {
            sample_array[i] = real[i];
            if (i<track->overlap_len) {
                sample_array[i] += overlap_buffer[i];
            }
        } else {
            overlap_buffer[i-chunk_size] = real[i];
        }
    }
}