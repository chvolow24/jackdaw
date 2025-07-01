/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "consts.h"
#include "dsp_utils.h"
/* #include "endpoint_callbacks.h" */
#include "project.h"
#include "session.h"


#define ROU_MAX_DEGREE 14
double complex *roots_of_unity[ROU_MAX_DEGREE];


/*****************************************************************************************************************
    Buffer arithmetic
 *****************************************************************************************************************/


static void init_roots_of_unity();

void init_dsp()
{
    init_roots_of_unity();
}

void float_buf_add(float *restrict a, float *restrict b, int len)
{
    for (int i=0; i<len; i++) {
	a[i] += b[i];
    }
}

void float_buf_add_to(float *restrict a, float *restrict b, float *restrict sum, int len)
{
    for (int i=0; i<len; i++) {
	sum[i] = a[i] + b[i];
    }
}

void float_buf_mult(float *restrict a, float *restrict b, int len)
{
    for (int i=0; i<len; i++) {
	a[i] *= b[i];
    }    
}

void float_buf_mult_to(float *restrict a, float *restrict b, float *restrict product, int len)
{
    for (int i=0; i<len; i++) {
	product[i] = a[i] * b[i];
    }        
}
void float_buf_mult_const(float *restrict a, float by, int len)
{
    for (int i=0; i<len; i++) {
	a[i] *= by;
    }
}



/*****************************************************************************************************************
    FFT and helper functions
 *****************************************************************************************************************/

/* 6.283185 */
static double complex *gen_FFT_X(int n)
{
    double complex *X = (double complex *)malloc(sizeof(double complex) * n);
    for (int i=0; i<n; i++) {
	double theta = TAU * i / n;
        X[i] = cos(theta) + I * sin(theta);
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


static void FFT_inner(double *restrict A, double complex *restrict B, int n, int offset, int increment)
{
    if (n==1) {
        B[0] = A[offset] + 0 * I;
        return;
    }

    int halfn = n>>1;
    int degree = log2(n);
    double complex *X = roots_of_unity[degree];
    int doubleinc = increment << 1;
    double complex Beven[halfn];
    double complex Bodd[halfn];
    FFT_inner(A, Beven, halfn, offset, doubleinc);
    FFT_inner(A, Bodd, halfn, offset + increment, doubleinc);
    for (int k=0; k<halfn; k++) {
        double complex odd_part_by_x = Bodd[k] * conj(X[k]);
        B[k] = Beven[k] + odd_part_by_x;
        B[k + halfn] = Beven[k] - odd_part_by_x;
    }
}

void FFT(double *restrict A, double complex *restrict B, int n)
{

    FFT_inner(A, B, n, 0, 1);

    for (int k=0; k<n; k++) {
        B[k]/=n;
    }
}

static void FFT_innerf(float *restrict A, double complex *restrict B, int n, int offset, int increment)
{
    if (n==1) {
        B[0] = A[offset] + 0 * I;
        return;
    }

    int halfn = n>>1;
    int degree = log2(n);
    double complex *X = roots_of_unity[degree];
    int doubleinc = increment << 1;
    double complex Beven[halfn];
    double complex Bodd[halfn];
    FFT_innerf(A, Beven, halfn, offset, doubleinc);
    FFT_innerf(A, Bodd, halfn, offset + increment, doubleinc);
    for (int k=0; k<halfn; k++) {
        double complex odd_part_by_x = Bodd[k] * conj(X[k]);
        B[k] = Beven[k] + odd_part_by_x;
        B[k + halfn] = Beven[k] - odd_part_by_x;
    }
}


void FFTf(float *restrict A, double complex *restrict B, int n)
{
    FFT_innerf(A, B, n, 0, 1);

    for (int k=0; k<n; k++) {
        B[k]/=n;
    }
}


/* I'm not sure why using an "unscaled" version of the FFT appears to work when getting the frequency response
for the FIR filters defined below (e.g. "bandpass_IR()"). The normal, scaled version produces values that are
too small. */
void FFT_unscaled(double *restrict A, double complex *restrict B, int n) 
{
    FFT_inner(A, B, n, 0, 1);
}

static double complex *IFFT_inner(double complex *restrict A, double complex *restrict B, int n, int offset, int increment)
{
    /* double complex *B = (double complex *)malloc(sizeof(double complex) * n); */

    if (n==1) {
        B[0] = A[offset];
        return B;
    }
    int halfn = n>>1;
    int degree = log2(n);
    double complex *X = roots_of_unity[degree];
    int doubleinc = increment << 1;
    double complex Beven[halfn];
    IFFT_inner(A, Beven, halfn, offset, doubleinc);
    double complex Bodd[halfn];
    IFFT_inner(A, Bodd, halfn, offset + increment, doubleinc);
    for (int k=0; k<halfn; k++) {
        double complex odd_part_by_x = Bodd[k] * X[k];
        B[k] = Beven[k] + odd_part_by_x;
        B[k + halfn] = Beven[k] - odd_part_by_x;
    }
    return B;

}

void IFFT(double complex *restrict A, double complex *restrict B, int n)
{
    IFFT_inner(A, B, n, 0, 1);
}


void get_magnitude(double complex *restrict A, double *restrict B, int len) 
{
    for (int i=0; i<len; i++) {
        B[i] = cabs(A[i]);
    }
}

void get_real_component(double complex *restrict A, double *restrict B, int len)
{
    for (int i=0; i<len; i++) {
        B[i] = creal(A[i]);
    }
}
void get_real_componentf(double complex *restrict A, float *restrict B, int len)
{
    for (int i=0; i<len; i++) {
        B[i] = creal(A[i]);
    }
}


/* Hamming window function */
double hamming(int x, int lenw)
{    
    return 0.54 - 0.46 * cos(TAU * x / lenw);
}

extern Project *proj;
double dsp_scale_freq_to_hz(double freq_unscaled)
{
    Session *session = session_get();
    double sample_rate = session->proj_initialized ? session->proj.sample_rate : DEFAULT_SAMPLE_RATE;
    return pow(10.0, log10(sample_rate / 2.0) * freq_unscaled);
}



float pan_scale(float pan, int channel)
{
    return
	channel == 0 ?
	pan <= 0.5 ? 1.0f : (1.0f - pan) * 2.0f :
	pan >= 0.5 ? 1.0 : pan * 2.0f;    
}
