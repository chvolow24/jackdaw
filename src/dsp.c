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

void process_clip_vol_and_pan(Clip *clip)
{
//         Track *track = clip->track;
//         float lpan, rpan, panctrlval;
//         panctrlval = track->pan_ctrl->value;
//         lpan = panctrlval < 0 ? 1 : 1 - panctrlval;
//         rpan = panctrlval > 0 ? 1 : 1 + panctrlval;
//         // uint8_t k=0;
//         for (uint32_t j=0; j<clip->len_sframes * clip->channels; j+=2) {
//             // pan = j%2==0 ? lpan : rpan;
//             // if (k<20) {
//             //     k++;
//             //     fprintf(stderr, "\t\t->sample %d, pan value: %f\n", j, pan);
//             // }
//             clip->post_proc[j] = clip->pre_proc[j] * track->vol_ctrl->value * lpan;
//             clip->post_proc[j+1] = clip->pre_proc[j+1] * track->vol_ctrl->value * rpan;
//         }
}
 
void process_track_vol_and_pan(Track *track)
{
    // Clip *clip = NULL;
    // float lpan, rpan, panctrlval;
    // panctrlval = track->pan_ctrl->value;
    // lpan = panctrlval < 0 ? 1 : 1 - panctrlval;
    // rpan = panctrlval > 0 ? 1 : 1 + panctrlval;
    for (uint8_t i=0; i<track->num_clips; i++) {
        Clip *clip = track->clips[i];
        process_clip_vol_and_pan(clip);
        // uint8_t k=0;
        // for (uint32_t j=0; j<clip->len_sframes * clip->channels; j+=2) {
        //     // pan = j%2==0 ? lpan : rpan;
        //     // if (k<20) {
        //     //     k++;
        //     //     fprintf(stderr, "\t\t->sample %d, pan value: %f\n", j, pan);
        //     // }
        //     clip->post_proc[j] = clip->pre_proc[j] * track->vol_ctrl->value * lpan;
        //     clip->post_proc[j+1] = clip->pre_proc[j+1] * track->vol_ctrl->value * rpan;
        // }
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



double complex *FFT_inner(double *A, int n, int offset, int increment)
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
        double complex some = Bodd[k];
        some = X[k];

        double complex odd_part_by_x = Bodd[k] * conj(X[k]);
        B[k] = Beven[k] + odd_part_by_x;

        B[k + halfn] = Beven[k] - odd_part_by_x;

    }

    free(Beven);
    free(Bodd);
    return B;

}

double complex *FFT(double *A, int n)
{

    double complex *B = FFT_inner(A, n, 0, 1);

    for (int k=0; k<n; k++) {
    // if (logbool) {
    //     fprintf(stderr, "Input at %d: %f, unscaled output: %f, scaled output: %f\n", k, A[k], std::abs(B[k]), std::abs(B[k]) / n);
    // }
        B[k]/=n;
    }
    // if (logbool) {
    //     exit(1);
    // }

    return B;
}

double complex *FFT_unscaled(double *A, int n) 
{
    double complex *B = FFT_inner(A, n, 0, 1);
    return B;
}

double complex *IFFT_inner(double complex *A, int n, int offset, int increment)
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

double *get_real_component(double complex *A, int len)
{
    double *B = (double *)malloc(sizeof(double) * len);
    for (int i=0; i<len; i++) {
        // std::cout << "Complex: " << A[i].real() << " + " << A[i].imag() << std::endl;
        B[i] = creal(A[i]);
    }
    return B;
}




double hamming(int x, int lenw)
{
    return 0.54 - 0.46 * cos(TAU * x / lenw);
}




/* IMPULSE RESPONSE FUNCTIONS 
The below functions create sinc or sinc-like curves representing
time-domain impulse responses. Pad with zeroes and FFT to get 
the frequency response of the filters.
*/
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


// typedef struct fir_filter {
//     FilterType type;
//     double cutoff_freq; // 0 < cutoff_freq < 0.5
//     double bandwidth;
//     double *impulse_response;
//     double complex *frequency_response;
//     uint16_t impulse_response_len;
//     uint16_t frequency_response_len;
// }FIRFilter;

// /* Initialize the dsp subsystem. All this does currently is to populate the nth roots of unity for n < ROU_MAX_DEGREE */
// void init_dsp();

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
    filter->frequency_response = malloc(sizeof(complex double) * frequency_response_len);

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
            for (uint32_t i=0; i<len; i++) {
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
}