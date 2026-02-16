/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
   Impulse response functions

   Create sinc or sinc-like curves representing time-domain impulse responses. Pad with zeroes and FFT to get 
    the frequency response of the filters.
 *****************************************************************************************************************/

#include <stdlib.h>
#include "consts.h"
#include "dsp_utils.h"
#include "endpoint_callbacks.h"
#include "fir_filter.h"
#include "page.h"
#include "project.h"

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


/* Endpoint callbacks */
static void filter_cutoff_dsp_cb(Endpoint *ep)
{
    FIRFilter *f = (FIRFilter *)ep->xarg1;
    Value cutoff = endpoint_safe_read(ep, NULL);
    double cutoff_hz = dsp_scale_freq_to_hz(cutoff.double_v);
    filter_set_cutoff_hz(f, cutoff_hz);
    /* fprintf(stderr, "DSP callback\n"); */
}

void filter_bandwidth_dsp_cb(Endpoint *ep)
{
    FIRFilter *f = (FIRFilter *)ep->xarg1;
    Value bandwidth = endpoint_safe_read(ep, NULL);
    double bandwidth_hz = dsp_scale_freq_to_hz(bandwidth.double_v);
    filter_set_bandwidth_hz(f, bandwidth_hz);
}


void filter_irlen_dsp_cb(Endpoint *ep)
{
    FIRFilter *f = (FIRFilter *)ep->xarg1;
    Value irlen_val = endpoint_safe_read(ep, NULL);
    filter_set_impulse_response_len(f, irlen_val.uint16_v);
}


void filter_type_dsp_cb(Endpoint *ep)
{
    FIRFilter *f = (FIRFilter *)ep->xarg1;
    Value val = endpoint_safe_read(ep, NULL);
    FilterType t = (FilterType)val.int_v;
    filter_set_type(f, t);
    
}



/* Create an empty FIR filter and allocate space for its buffers. MUST be initialized with 'set_filter_params'*/
static void FIR_filter_alloc_buffers(FIRFilter *filter)
{
    /* pthread_mutex_lock(&filter->lock); */

    int max_irlen = filter->chunk_len;
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
    /* pthread_mutex_unlock(&filter->lock); */
    /* pthread_mutex_unlock(&filter->lock); */

}
void filter_init(FIRFilter *filter, FilterType type, uint16_t impulse_response_len, uint16_t frequency_response_len, uint16_t chunk_len) 
{
    /* pthread_mutex_init(&filter->lock, NULL); */
    filter->type = type;
    filter->impulse_response_len_internal = impulse_response_len;
    filter->impulse_response_len = impulse_response_len;
    filter->frequency_response_len = frequency_response_len;
    filter->chunk_len = chunk_len;
    FIR_filter_alloc_buffers(filter);

    filter->cutoff_freq = 0.02;
    filter->bandwidth = 0.1;
    filter_set_impulse_response_len(filter, impulse_response_len);

    /* filter_set_params(filter, LOWPASS, 0.02, 0.1); */
    
    endpoint_init(
	&filter->cutoff_ep,
	&filter->cutoff_freq_unscaled,
	JDAW_DOUBLE,
	"freq",
	"Cutoff or center freq",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, filter_cutoff_dsp_cb,
	/* filter_cutoff_gui_cb, NULL, filter_cutoff_dsp_cb, */
        filter, NULL, &filter->effect->page, "track_settings_filter_cutoff_slider");
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
	"bandwidth",
	"Bandwidth",
	JDAW_THREAD_DSP,
	/* filter_bandwidth_gui_cb, NULL, filter_bandwidth_dsp_cb, */
	page_el_gui_cb, NULL, filter_bandwidth_dsp_cb,
	filter, NULL, &filter->effect->page, "track_settings_filter_bandwidth_slider");
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
	"IR Length",
	JDAW_THREAD_DSP,
	/* filter_irlen_gui_cb, NULL, filter_irlen_dsp_cb, */
	page_el_gui_cb, NULL, filter_irlen_dsp_cb,
	filter, NULL, &filter->effect->page, "track_settings_filter_irlen_slider");
    endpoint_set_allowed_range(
	&filter->impulse_response_len_ep,
	(Value){.uint16_v = 4},
	(Value){.uint16_v = filter->chunk_len});
    /* endpoint_set_default_value(&filter->impulse_response_len_ep, (Value){.double_v = 0.1}); */
    /* endpoint_set_label_fn(&filter->impulse_response_len_ep, label_msec); */
    /* api_endpoint_register(&filter->impulse_response_len_ep, &filter->effect->api_node); */


    endpoint_init(
	&filter->type_ep,
	&filter->type,
	JDAW_INT,
	"type",
	"Type",
	JDAW_THREAD_DSP,
	/* filter_type_gui_cb, NULL, filter_type_dsp_cb, */
	page_el_gui_cb, NULL, filter_type_dsp_cb,
	filter, NULL, &filter->effect->page, "track_settings_filter_type_radio");

    
    filter_set_params_hz(filter, LOWPASS, 1000, 1000); /* Defaults */

    
    filter->initialized = true;

}

extern pthread_t DSP_THREAD_ID;
extern Project *proj;

/* Bandwidth param only required for band-pass and band-cut filters */
void filter_set_params(FIRFilter *filter, FilterType type, double cutoff, double bandwidth)
{
    /* DSP_THREAD_ONLY_WHEN_ACTIVE("filter_set_params"); */
    /* pthread_mutex_lock(&filter->lock); */
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
    /* pthread_mutex_unlock(&filter->lock); */
    /* free(IR_zero_padded); */
}

void filter_set_arbitrary_IR(FIRFilter *filter, float *ir_in, int ir_len)
{
    filter_set_impulse_response_len(filter, ir_len);
    /* pthread_mutex_lock(&filter->lock); */
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
    /* pthread_mutex_unlock(&filter->lock); */

}

void filter_set_params_hz(FIRFilter *filter, FilterType type, double cutoff_hz, double bandwidth_hz)
{
    double cutoff = cutoff_hz / (double)filter->effect->effect_chain->proj->sample_rate;
    double bandwidth = bandwidth_hz / (double)filter->effect->effect_chain->proj->sample_rate;;
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
    double cutoff = cutoff_hz / (double)f->effect->effect_chain->proj->sample_rate;
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
    double bandwidth = bandwidth_h / f->effect->effect_chain->proj->sample_rate;
    double cutoff = f->cutoff_freq;
    filter_set_params(f, t, cutoff, bandwidth);
}

void filter_set_impulse_response_len(FIRFilter *f, int new_len)
{
    /* DSP_THREAD_ONLY_WHEN_ACTIVE("filter_set_params"); */
    if (new_len > f->effect->effect_chain->chunk_len_sframes) {
	fprintf(stderr, "RESETTING IR LEN TO MATCH EC\n");
	new_len = f->effect->effect_chain->chunk_len_sframes;
    }
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
    /* pthread_mutex_destroy(&filter->lock); */
	/* SDL_DestroyMutex(filter->lock); */
    /* } */
    /* free(filter); */
}




/* Destructive; replaces values in sample_array. Legacy fn called by new standard prototype filter_buf_apply */
static void apply_filter(FIRFilter *filter, uint8_t channel, uint16_t chunk_size, float *sample_array)
{
    /* pthread_mutex_lock(&filter->lock); */
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
    /* pthread_mutex_unlock(&filter->lock); */
}

/* Apply a FIR filter to a float buffer */
float filter_buf_apply(void *f_v, float *restrict buf, int len, int channel, float input_amp)
{
    FIRFilter *f = f_v;
    /* if (!f->active) return input_amp; */
    apply_filter(f, channel, len, buf);
    float output_amp = 0.0;
    for (int i=0; i<len; i++) {
	output_amp += fabs(buf[i]);
    }
    
    return output_amp;
}

