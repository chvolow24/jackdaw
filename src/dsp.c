/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    dsp.c

    * Digital signal processing
    * Clip buffers are run through functions here to populate post-proc buffer
 *****************************************************************************************************************/


#include <complex.h>
#include <stdint.h>
#include "endpoint_callbacks.h"
#include "project.h"
#include "pthread.h"
#include "dsp.h"


#define ROU_MAX_DEGREE 14
double complex *roots_of_unity[ROU_MAX_DEGREE];

/* extern Project *proj; */

static void init_roots_of_unity();

void init_dsp()
{
    init_roots_of_unity();
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

void FFT(double *A, double complex *B, int n)
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


void FFTf(float *A, double complex *B, int n)
{
    FFT_innerf(A, B, n, 0, 1);

    for (int k=0; k<n; k++) {
        B[k]/=n;
    }
}


/* I'm not sure why using an "unscaled" version of the FFT appears to work when getting the frequency response
for the FIR filters defined below (e.g. "bandpass_IR()"). The normal, scaled version produces values that are
too small. */
static void FFT_unscaled(double *A, double complex *B, int n) 
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

static void IFFT(double complex *A, double complex *B, int n)
{
    IFFT_inner(A, B, n, 0, 1);
}

/* static void IFFT_real_input(double *A_real, double complex *B, int n) */
/* { */
/*     double complex A[n]; */
/*     for (int i=0; i<n; i++) { */
/*         A[i] = A_real[i] + 0 * I; */
/*     } */
/*     IFFT(A, B, n); */
/* } */


void get_magnitude(double complex *restrict A, double *restrict B, int len) 
{
    /* double *B = (double *)malloc(sizeof(double) * len); */
    for (int i=0; i<len; i++) {
        B[i] = cabs(A[i]);
    }
}

void get_real_component(double complex *restrict A, double *restrict B, int len)
{
    /* double *B = (double *)malloc(sizeof(double) * len); */
    for (int i=0; i<len; i++) {
        // std::cout << "Complex: " << A[i].real() << " + " << A[i].imag() << std::endl;
        B[i] = creal(A[i]);
    }
}
void get_real_componentf(double complex *restrict A, float *restrict B, int len)
{
    /* double *B = (double *)malloc(sizeof(double) * len); */
    for (int i=0; i<len; i++) {
        // std::cout << "Complex: " << A[i].real() << " + " << A[i].imag() << std::endl;
        B[i] = creal(A[i]);
    }
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



static double lowpass_IR(int x, int offset, double cutoff)
{
    cutoff *= TAU;
    if (x-offset == 0) {
        return cutoff / PI;
    } else {
        return sin(cutoff * (x-offset))/(PI * (x-offset));
    }
}

static double highpass_IR(int x, int offset, double cutoff)
{
    cutoff *= TAU;
    if (x-offset == 0) {
        return 1 - (cutoff / PI);
    } else {
        return (1.0 / (PI * (x-offset))) * (sin(PI * (x-offset)) - sin(cutoff * (x-offset)));
    }
}

static double bandpass_IR(int x, int offset, double center_freq, double bandwidth) 
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

static double bandcut_IR(int x, int offset, double center_freq, double bandwidth) 
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
static void FIR_filter_alloc_buffers(FIRFilter *filter)
{
    pthread_mutex_lock(&filter->lock);

    int max_irlen = filter->track->tl->proj->fourier_len_sframes;
    if (!filter->impulse_response)
	filter->impulse_response = calloc(1, sizeof(double) * max_irlen);

    if (!filter->frequency_response)
	filter->frequency_response = calloc(1, sizeof(double complex) * filter->frequency_response_len);

    if (!filter->frequency_response_mag)
	filter->frequency_response_mag = calloc(1, sizeof(double) * filter->frequency_response_len);

    /* filter->overlap_len =  = 1; */
    if (!filter->overlap_buffer_L)
	filter->overlap_buffer_L = calloc(1, sizeof(float) * max_irlen - 1);
    if (!filter->overlap_buffer_R)
	filter->overlap_buffer_R = calloc(1, sizeof(float) * max_irlen - 1);

    if (!filter->output_freq_mag_L)
	filter->output_freq_mag_L = calloc(filter->frequency_response_len, sizeof(double));

    if (!filter->output_freq_mag_R)
	filter->output_freq_mag_R = calloc(filter->frequency_response_len, sizeof(double));


	
    /* SDL_LockMutex(filter->lock); */
    /* if (filter->impulse_response) free(filter->impulse_response); */
    /* filter->impulse_response = calloc(1, sizeof(double) * filter->impulse_response_len); */
    /* if (filter->frequency_response) free(filter->frequency_response); */
    /* filter->frequency_response = calloc(1, sizeof(double complex) * filter->frequency_response_len); */
    /* if (filter->frequency_response_mag) free(filter->frequency_response_mag); */
    /* filter->frequency_response_mag = calloc(1, sizeof(double) * filter->frequency_response_len); */
    /* filter->overlap_len = filter->impulse_response_len - 1; */
    /* if (filter->overlap_buffer_L) free(filter->overlap_buffer_L); */
    /* filter->overlap_buffer_L = calloc(1, sizeof(double) * filter->overlap_len); */
    /* if (filter->overlap_buffer_R) free(filter->overlap_buffer_R); */
    /* filter->overlap_buffer_R = calloc(1, sizeof(double) * filter->overlap_len); */
    pthread_mutex_unlock(&filter->lock);
    /* pthread_mutex_unlock(&filter->lock); */

}
void filter_init(FIRFilter *filter, Track *track, FilterType type, uint16_t impulse_response_len, uint16_t frequency_response_len) 
{
    pthread_mutex_init(&filter->lock, NULL);
    filter->type = type;
    filter->track = track;
    filter->impulse_response_len_internal = impulse_response_len;
    filter->impulse_response_len = impulse_response_len;
    filter->frequency_response_len = frequency_response_len;
    FIR_filter_alloc_buffers(filter);

    filter->cutoff_freq = 0.02;
    filter->bandwidth = 0.1;
    filter_set_impulse_response_len(filter, impulse_response_len);
    /* filter_set_params(filter, LOWPASS, 0.02, 0.1); */
    
    endpoint_init(
	&filter->cutoff_ep,
	&filter->cutoff_freq_unscaled,
	JDAW_DOUBLE,
	"filter_cutoff_freq",
	"Filter cutoff",
	JDAW_THREAD_DSP,
	filter_cutoff_gui_cb, NULL, filter_cutoff_dsp_cb,
        filter, NULL, NULL, NULL);
    endpoint_set_allowed_range(
	&filter->cutoff_ep,
	(Value){.double_v = 0.0},
	(Value){.double_v = 1.0});
    endpoint_set_default_value(&filter->cutoff_ep, (Value){.double_v = 0.1});
    endpoint_set_label_fn(&filter->cutoff_ep, label_freq_raw_to_hz);
    api_endpoint_register(&filter->cutoff_ep, &filter->effect->api_node);

    endpoint_init(
	&filter->bandwidth_ep,
	&filter->bandwidth_unscaled,
	JDAW_DOUBLE,
	"filter_bandwidth",
	"Filter bandwidth",
	JDAW_THREAD_DSP,
	filter_bandwidth_gui_cb, NULL, filter_bandwidth_dsp_cb,
	filter, NULL, NULL, NULL);
    endpoint_set_allowed_range(
	&filter->bandwidth_ep,
	(Value){.double_v = 0.0},
	(Value){.double_v = 1.0});
    endpoint_set_default_value(&filter->bandwidth_ep, (Value){.double_v = 0.1});
    endpoint_set_label_fn(&filter->bandwidth_ep, label_freq_raw_to_hz);
    api_endpoint_register(&filter->bandwidth_ep, &filter->effect->api_node);

    endpoint_init(
	&filter->impulse_response_len_ep,
	&filter->impulse_response_len_internal,
	JDAW_UINT16,
	"filter_irlen",
	"Filter impulse resp len",
	JDAW_THREAD_DSP,
	filter_irlen_gui_cb, NULL, filter_irlen_dsp_cb,
	filter, NULL, NULL, NULL);
    endpoint_set_allowed_range(
	&filter->impulse_response_len_ep,
	(Value){.uint16_v = 4},
	(Value){.uint16_v = filter->track->tl->proj->fourier_len_sframes});
    /* endpoint_set_default_value(&filter->impulse_response_len_ep, (Value){.double_v = 0.1}); */
    /* endpoint_set_label_fn(&filter->impulse_response_len_ep, label_msec); */
    /* api_endpoint_register(&filter->impulse_response_len_ep, &filter->effect->api_node); */


    endpoint_init(
	&filter->type_ep,
	&filter->type,
	JDAW_INT,
	"type",
	"Filter type",
	JDAW_THREAD_DSP,
	filter_type_gui_cb, NULL, filter_type_dsp_cb,
	filter, NULL, NULL, NULL);

    
    filter_set_params_hz(filter, LOWPASS, 1000, 1000); /* Defaults */

    
    filter->initialized = true;

}

extern pthread_t DSP_THREAD_ID;
extern Project *proj;

/* Bandwidth param only required for band-pass and band-cut filters */
void filter_set_params(FIRFilter *filter, FilterType type, double cutoff, double bandwidth)
{
    /* DSP_THREAD_ONLY_WHEN_ACTIVE("filter_set_params"); */
    pthread_mutex_lock(&filter->lock);
    filter->type = type;
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

    /* double *IR_zero_padded = malloc(sizeof(double) * filter->frequency_response_len); */
    double IR_zero_padded[filter->frequency_response_len];
    for (uint16_t i=0; i<filter->frequency_response_len; i++) {
        if (i<len) {
            IR_zero_padded[i] = ir[i];
        } else {
            IR_zero_padded[i] = 0;
        }
    }
    /* if (!filter->frequency_response) { */
    /* 	fprintf(stdout, "Allocating freq response\n"); */
    /* 	filter->frequency_response = malloc(sizeof(double complex) * filter->frequency_response_len); */
    /* } */


    FFT_unscaled(IR_zero_padded, filter->frequency_response, filter->frequency_response_len);
    get_magnitude(filter->frequency_response, filter->frequency_response_mag, filter->frequency_response_len);
    pthread_mutex_unlock(&filter->lock);
    /* free(IR_zero_padded); */
}

void filter_set_arbitrary_IR(FIRFilter *filter, float *ir_in, int ir_len)
{
    filter_set_impulse_response_len(filter, ir_len);
    pthread_mutex_lock(&filter->lock);
    double ir_zero_padded[filter->frequency_response_len];
    for (int i=0; i<filter->frequency_response_len; i++) {
	if (i < ir_len) {
	    ir_zero_padded[i] = ir_in[i] * hamming(i, ir_len);
	} else {
	    ir_zero_padded[i] = 0;
	}
    }
    FFT_unscaled(ir_zero_padded, filter->frequency_response, filter->frequency_response_len);
    get_magnitude(filter->frequency_response, filter->frequency_response_mag, filter->frequency_response_len);
    pthread_mutex_unlock(&filter->lock);

}

void filter_set_params_hz(FIRFilter *filter, FilterType type, double cutoff_hz, double bandwidth_hz)
{
    double cutoff = cutoff_hz / (double)filter->track->tl->proj->sample_rate;
    double bandwidth = bandwidth_hz / (double)filter->track->tl->proj->sample_rate;;
    filter_set_params(filter, type, cutoff, bandwidth);
}

void filter_set_cutoff(FIRFilter *f, double cutoff)
{
    FilterType t = f->type;
    double bandwidth = f->bandwidth;
    filter_set_params(f, t, cutoff, bandwidth);

}
void filter_set_cutoff_hz(FIRFilter *f, double cutoff_hz)
{
    FilterType t = f->type;
    double bandwidth = f->bandwidth;
    double cutoff = cutoff_hz / (double)f->track->tl->proj->sample_rate;
    filter_set_params(f, t, cutoff, bandwidth);
}
void filter_set_bandwidth(FIRFilter *f, double bandwidth)
{
    FilterType t = f->type;
    double cutoff = f->cutoff_freq;
    filter_set_params(f, t, cutoff, bandwidth);
}
void filter_set_bandwidth_hz(FIRFilter *f, double bandwidth_h)
{
    FilterType t = f->type;
    double bandwidth = bandwidth_h / f->track->tl->proj->sample_rate;
    double cutoff = f->cutoff_freq;
    filter_set_params(f, t, cutoff, bandwidth);
}

void filter_set_impulse_response_len(FIRFilter *f, int new_len)
{
    /* DSP_THREAD_ONLY_WHEN_ACTIVE("filter_set_params"); */
    f->impulse_response_len = new_len;
    f->overlap_len = new_len - 1;
    double cutoff = f->cutoff_freq;
    double bandwidth = f->bandwidth;
    FilterType t = f->type;
    filter_set_params(f, t, cutoff, bandwidth);
    memset(f->overlap_buffer_L, '\0', f->overlap_len * sizeof(float));
    memset(f->overlap_buffer_R, '\0', f->overlap_len * sizeof(float));
}

void filter_set_type(FIRFilter *f, FilterType t)
{
    double cutoff = f->cutoff_freq;
    double bandwidth = f->bandwidth;
    filter_set_params(f, t, cutoff, bandwidth);
}

void filter_deinit(FIRFilter *filter) 
{
    if (filter->frequency_response) {
        free(filter->frequency_response);
        filter->frequency_response = NULL;
    }
    if (filter->impulse_response) {
        free(filter->impulse_response);
        filter->impulse_response = NULL;
    }
    if (filter->overlap_buffer_L) {
	free(filter->overlap_buffer_L);
    }
    if (filter->overlap_buffer_R) {
	free(filter->overlap_buffer_R);
    }
    if (filter->frequency_response_mag) {
	free(filter->frequency_response_mag);
    }
    if (filter->output_freq_mag_L) {
	free(filter->output_freq_mag_L);
    }
    if (filter->output_freq_mag_R) {
	free(filter->output_freq_mag_R);
    }
    /* if (filter->lock) { */
    pthread_mutex_destroy(&filter->lock);
	/* SDL_DestroyMutex(filter->lock); */
    /* } */
    /* free(filter); */
}




/* Destructive; replaces values in sample_array. Legacy fn called by new standard prototype filter_buf_apply */
static void apply_filter(FIRFilter *filter, Track *track, uint8_t channel, uint16_t chunk_size, float *sample_array)
{
    pthread_mutex_lock(&filter->lock);
    /* SDL_LockMutex(filter->lock); */
    float *overlap_buffer = channel == 0 ? filter->overlap_buffer_L : filter->overlap_buffer_R;
    uint16_t padded_len = filter->frequency_response_len;
    float padded_chunk[padded_len];
    memset(padded_chunk + chunk_size, '\0', (padded_len - chunk_size) * sizeof(float));
    memcpy(padded_chunk, sample_array, chunk_size * sizeof(float));

    double complex freq_domain[padded_len];

    FFTf(padded_chunk,freq_domain, padded_len);
    
    /* Apply filter via multiplication in freq domain */
    for (uint16_t i=0; i<padded_len; i++) {
        freq_domain[i] *= filter->frequency_response[i];
    }

    /* Populate magnitude buffers for display in freq plot.
       To avoid slowdown, the same frequency response that is used in
       the application of the filter is used here, without windowing or
       compensatory scaling. This frequency plot will therefore look
       more jagged than the one displayed on an EQ effect */
    if (filter->effect && filter->effect->page && filter->effect->page->onscreen) {
	double *dst = channel == 0 ? filter->output_freq_mag_L : filter->output_freq_mag_R;
	get_magnitude(freq_domain, dst, padded_len);
    }

    double complex time_domain[padded_len];
    IFFT(freq_domain, time_domain, padded_len);
    float real[padded_len];
    get_real_componentf(time_domain, real, padded_len);

    memcpy(sample_array, real, chunk_size * sizeof(float));
    for (int i=0; i<filter->overlap_len; i++) {
        sample_array[i] += overlap_buffer[i];
    }

    memcpy(overlap_buffer, real + chunk_size, filter->overlap_len * sizeof(float));
    pthread_mutex_unlock(&filter->lock);
}

/* Apply a FIR filter to a float buffer */
float filter_buf_apply(void *f_v, float *buf, int len, int channel, float input_amp)
{
    FIRFilter *f = f_v;
    if (!f->active) return input_amp;
    apply_filter(f, f->track, channel, len, buf);
    float output_amp = 0.0;
    for (int i=0; i<len; i++) {
	output_amp += fabs(buf[i]);
    }
    
    return output_amp;
}

/* void ___apply_track_filter(Track *track, uint8_t channel, uint16_t chunk_size, float *sample_array)  */
/* { */
/*     if (!track->fir_filter_active == 0) { */
/*         return; */
/*     } */
/*     uint16_t padded_len = proj->chunk_size_sframes * 2; */

/*     double padded_chunk[padded_len]; */
/*     for (uint16_t i=0; i<padded_len; i++) { */
/* 	if (i<chunk_size) { */
/* 	    padded_chunk[i] = sample_array[i]; */
/* 	} else { */
/* 	    padded_chunk[i] = 0.0; */
/* 	} */

/*     } */
/*     double complex freq_domain[padded_len]; */
/*     FFT(padded_chunk, freq_domain, padded_len); */
/*     /\* free(padded_chunk); *\/ */

/*     FIRFilter *filter = track->fir_filter; */
/*     pthread_mutex_lock(&filter->lock); */
/*     /\* double complex freq_response_sum[filter->frequency_response_len]; *\/ */
	
/*     /\* memset(freq_response_sum, '\0', sizeof(double complex) * filter->frequency_response_len); *\/ */

/*     double *overlap_buffer = channel == 0 ? filter->overlap_buffer_L : filter->overlap_buffer_R; */

/*     /\* if (!overlap_buffer) { *\/ */
/*     /\*     overlap_buffer = malloc(sizeof(double) * filter->overlap_len); *\/ */
/*     /\*     memset(overlap_buffer, '\0', sizeof(double) * filter->overlap_len); *\/ */
/*     /\*     if (channel == 0) { *\/ */
/*     /\* 	filter->overlap_buffer_L = overlap_buffer; *\/ */
/*     /\*     } else { *\/ */
/*     /\* 	filter->overlap_buffer_R = overlap_buffer; *\/ */
/*     /\*     } *\/ */
/*     /\* } *\/ */
    


/*     /\* for (uint16_t i=0; i<filter->frequency_response_len; i++) { *\/ */
/*     /\* 	freq_response_sum[i] += filter->frequency_response[i]; *\/ */
/*     /\* } *\/ */
/*     for (uint16_t i=0; i<filter->frequency_response_len; i++) { */
/* 	freq_domain[i] *= filter->frequency_response[i]; */
/*     } */
/*     pthread_mutex_unlock(&filter->lock); */
/*     double complex time_domain[padded_len]; */
/*     IFFT(freq_domain, time_domain, padded_len); */
/*     /\* free(freq_domain); *\/ */
/*     double real[padded_len]; */
/*     get_real_component(time_domain, real, padded_len); */
/*     /\* free(time_domain); *\/ */

/*     for (uint16_t i=0; i<chunk_size + filter->overlap_len; i++) { */
/* 	if (i<chunk_size) { */
/* 	    sample_array[i] = real[i]; */
/* 	    if (i<filter->overlap_len) { */
/* 		sample_array[i] += overlap_buffer[i]; */
/* 	    } */
/* 	} else { */
/* 	    overlap_buffer[i-chunk_size] = real[i]; */
/* 	} */
/*     } */
/*     /\* for (uint8_t f=0; f<track->num_filters; f++) { *\/ */
/*     /\*     FIRFilter *filter = track->filters[f]; *\/ */

/*     /\* } *\/ */

/* } */

/* void track_add_default_filter(Track *track) */
/* { */
/*     int ir_len = track->tl->proj->fourier_len_sframes/4; */
/*     filter_init(&track->fir_filter, track, LOWPASS, ir_len, track->tl->proj->fourier_len_sframes * 2); */
/*     track->fir_filter.track = track; */
/*     filter_set_params_hz(&track->fir_filter, LOWPASS, 1000, 1000); */
/*     track->fir_filter_active = true; */
/* } */


/*****************************************************************************************************************
   Delay lines
 *****************************************************************************************************************/


void delay_line_init(DelayLine *dl, Track *track, uint32_t sample_rate)
{
    if (dl->buf_L) {
	fprintf(stderr, "ERROR: attempt to reinitialize delay line.\n");
	return;
    }
    dl->track = track;
    dl->pos_L = 0;
    dl->pos_R = 0;
    dl->amp = 0.0;
    dl->len = 5000;
    dl->stereo_offset = 0;
    pthread_mutex_init(&dl->lock, NULL);
    /* dl->lock = SDL_CreateMutex(); */
    dl->max_len = DELAY_LINE_MAX_LEN_S * sample_rate;
    dl->buf_L = calloc(dl->max_len, sizeof(double));
    dl->buf_R = calloc(dl->max_len, sizeof(double));
    dl->cpy_buf = calloc(dl->max_len, sizeof(double));

    endpoint_init(
	&dl->len_ep,
	&dl->len_msec,
	JDAW_INT16,
	"delay_line_len",
	"Delay time",
	JDAW_THREAD_DSP,
	delay_line_len_gui_cb, NULL, delay_line_len_dsp_cb,
	/* NULL, NULL, delay_line_len_dsp_cb, */
	(void *)dl, NULL, NULL, NULL);
    endpoint_set_allowed_range(&dl->len_ep, (Value){.int16_v=0}, (Value){.int16_v=1000});
    endpoint_set_label_fn(&dl->len_ep, label_msec);
    api_endpoint_register(&dl->len_ep, &dl->effect->api_node);

    endpoint_init(
	&dl->amp_ep,
	&dl->amp,
	JDAW_DOUBLE,
	"delay_line_amp",
	"Delay amp",
	JDAW_THREAD_DSP,
	/* delay_line_amp_gui_cb, NULL, NULL, */
	delay_line_amp_gui_cb, NULL, NULL,
	(void *)dl, NULL, NULL, NULL);
    endpoint_set_allowed_range(&dl->amp_ep, (Value){.double_v=0.0}, (Value){.double_v=0.99});
    endpoint_set_label_fn(&dl->amp_ep, label_amp_to_dbstr);
    api_endpoint_register(&dl->amp_ep, &dl->effect->api_node);
    
    endpoint_init(
	&dl->stereo_offset_ep,
	&dl->stereo_offset,
	JDAW_DOUBLE,
	"delay_line_stereo_offset",
	"Delay stereo offset",
	JDAW_THREAD_DSP,
	delay_line_stereo_offset_gui_cb, NULL, NULL,
	NULL, NULL, NULL, NULL);
    endpoint_set_allowed_range(&dl->stereo_offset_ep, (Value){.double_v=0.0}, (Value){.double_v=1.0});
    /* endpoint_set_label_fn(&dl->stereo_offset_ep, label_amp_); */
    api_endpoint_register(&dl->stereo_offset_ep, &dl->effect->api_node);
}

/*

  Delay line reduce length:
    - write from current pos
    - write every 1 sample
    - read every >1 samples

  Delay line increase length:
    - write from current pos
    - write every 1 sample
    - read every <1 sample
 */

static inline void del_read_into_buffer_resize(DelayLine *dl, double *read_from, double *read_to, int32_t *read_pos, int32_t len)
{
    for (int32_t i=0; i<len; i++) {
	/* double read_pos_d = (double)*read_pos; */
	double read_pos_d = dl->len * ((double)i / len);
	int32_t read_i = (int32_t)(round(read_pos_d));
	if (read_i < 0) read_i = 0;
	read_to[i] = read_from[read_i];
	/* read_pos_d += read_step; */
	if (read_pos_d > dl->len) {
	    read_pos_d -= dl->len;
	}
    }
}

void delay_line_set_params(DelayLine *dl, double amp, int32_t len)
{
    /* fprintf(stderr, "Set line len: %d\n", len); */
    /* if (!dl->buf_L) { */
    /* 	delay_line_init(dl, sample_rate); */
    /* } */
    pthread_mutex_lock(&dl->lock);
    /* if (len > proj->sample_rate) { */
    /* 	fprintf(stderr, "UH OH: len = %d\n", len); */
    /* 	exit(1); */
    /* 	return; */
    /* } */
    if (dl->len != len) {
	/* double new_buf[len]; */
	double *new_buf = dl->cpy_buf;
	/* double read_step = (double)dl->len / len; */
	/* double read_step = 1.0; */
	del_read_into_buffer_resize(dl, dl->buf_L, new_buf,  &dl->pos_L, len);
	memcpy(dl->buf_L, new_buf, len * sizeof(double));
	del_read_into_buffer_resize(dl, dl->buf_R, new_buf, &dl->pos_R, len);
	memcpy(dl->buf_R, new_buf, len * sizeof(double));
	double pos_L_prop = (double)dl->pos_L / dl->len;
	double pos_R_prop = (double)dl->pos_R / dl->len;
	dl->pos_L = pos_L_prop * len;
	dl->pos_R = pos_R_prop * len;
	dl->len = len;
	/* dl->pos_L = 0; */
	/* dl->pos_R = 0; */
    }
    dl->amp = amp;
    pthread_mutex_unlock(&dl->lock);
}

float delay_line_buf_apply(void *dl_v, float *buf, int len, int channel, float input_amp)
{
    DelayLine *dl = dl_v;
    float output_amp = 0.0f;
    if (!dl->active) return output_amp;
    pthread_mutex_lock(&dl->lock);
    double *del_line = channel == 0 ? dl->buf_L : dl->buf_R;
    int32_t *del_line_pos = channel == 0 ? &dl->pos_L : &dl->pos_R;
    for (int i=0; i<len; i++) {
	double track_sample = buf[i];
	int32_t pos = *del_line_pos;
	if (channel == 0) {
	    pos -= dl->stereo_offset * dl->len;
	    if (pos < 0) {
		pos += dl->len;
	    }
	}
	buf[i] += del_line[pos];
	output_amp += fabs(buf[i]);
	del_line[*del_line_pos] += track_sample;
	del_line[*del_line_pos] *= dl->amp;

	/* clip delay line */
	if (del_line[*del_line_pos] > 1.0) del_line[*del_line_pos] = 1.0;	
	else if (del_line[*del_line_pos] < -1.0) del_line[*del_line_pos] = -1.0;
		
	if (*del_line_pos + 1 >= dl->len) {
	    *del_line_pos = 0;
	} else {
	    (*del_line_pos)++;
	}
    }
    pthread_mutex_unlock(&dl->lock);
    return output_amp;
}

void delay_line_clear(DelayLine *dl)
{
    memset(dl->buf_L, '\0', dl->len * sizeof(double));
    memset(dl->buf_R, '\0', dl->len * sizeof(double));
}


extern Project *proj;
double dsp_scale_freq_to_hz(double freq_unscaled)
{
    return pow(10.0, log10((double)proj->sample_rate / 2) * freq_unscaled);
}
