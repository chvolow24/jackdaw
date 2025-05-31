/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    fir_filter.h

    * 
    * 
*****************************************************************************************************************/

#include <complex.h>
#include <stdio.h>
#include <stdint.h>
#include "endpoint.h"

#define DEFAULT_FILTER_LEN 128
#define DELAY_LINE_MAX_LEN_S 1


typedef struct track Track;
typedef struct effect Effect;

typedef enum filter_type {
    LOWPASS=0, HIGHPASS=1, BANDPASS=2, BANDCUT=3
} FilterType;

typedef struct fir_filter {
    /* bool active; */
    bool initialized;
    FilterType type;
    double cutoff_freq_unscaled; /* For gui components and automations */
    double cutoff_freq; /* 0 < cutoff_freq < 0.5 */
    double bandwidth_unscaled; /* For gui components and automations */
    double bandwidth;
    double *impulse_response;
    double complex *frequency_response;
    double *frequency_response_mag;
    float *overlap_buffer_L;
    float *overlap_buffer_R;
    double *output_freq_mag_L;
    double *output_freq_mag_R;
    /* float *freq_mag_L; */
    /* float *freq_mag_R; */
    uint16_t impulse_response_len; /* Only modified in callbacks */
    uint16_t impulse_response_len_internal;
    /* uint16_t impulse_response_len_internal; */
    uint16_t frequency_response_len;
    uint16_t overlap_len;
    /* pthread_mutex_t lock; */

    Track *track;

    Endpoint type_ep;
    Endpoint cutoff_ep;
    Endpoint bandwidth_ep;
    Endpoint impulse_response_len_ep;

    Effect *effect;

    /* SDL_mutex *lock;audio.c */
} FIRFilter;


/* Init an empty FIR filter and allocate space for its buffers. Params MUST be initialized with 'set_filter_params'*/
void filter_init(FIRFilter *filter, Track *track, FilterType type, uint16_t impulse_response_len, uint16_t frequency_response_len);

/* Bandwidth param only required for band-pass and band-cut filters */
void filter_set_params(FIRFilter *filter, FilterType type,  double cutoff, double bandwidth);
void filter_set_params_hz(FIRFilter *filter, FilterType type, double cutoff_hz, double bandwidth_hz);

void filter_set_cutoff(FIRFilter *filter, double cutoff);
void filter_set_cutoff_hz(FIRFilter *filter, double cutoff);
void filter_set_bandwidth(FIRFilter *filter, double cutoff);
void filter_set_bandwidth_hz(FIRFilter *filter, double cutoff);
void filter_set_type(FIRFilter *filter, FilterType t);

void filter_set_impulse_response_len(FIRFilter *f, int new_len);

void filter_set_arbitrary_IR(FIRFilter *filter, float *ir_in, int ir_len);

/* Destry a FIRFilter and associated memory */
void filter_deinit(FIRFilter *filter);

/* void apply_track_filter(Track *track, uint8_t channel, uint16_t chunk_size, float *sample_array); */
/* void apply_filter(FIRFilter *filter, Track *track, uint8_t channel, uint16_t chunk_size, float *sample_array); */
/* void filter_buf_apply(FIRFilter *f, float *buf, int len, int channel); */
float filter_buf_apply(void *f_v, float *buf, int len, int channel, float input_amp);
