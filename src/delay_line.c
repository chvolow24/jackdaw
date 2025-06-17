/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "consts.h"
#include "delay_line.h"
#include "endpoint_callbacks.h"
#include "session.h"

void delay_line_set_params(DelayLine *dl, double amp, int32_t len);

void delay_line_len_dsp_cb(Endpoint *ep)
{
    Session *session = session_get();
    double sample_rate = session->proj_initialized ? session->proj.sample_rate : DEFAULT_SAMPLE_RATE;
    DelayLine *dl = (DelayLine *)ep->xarg1;
    int16_t val_msec = endpoint_safe_read(ep, NULL).int16_v;
    int32_t len_sframes = (int32_t)((double)val_msec * sample_rate / 1000.0);
    delay_line_set_params(dl, dl->amp, len_sframes);
    /* session_queue_callback(proj, ep, secondary_delay_line_gui_cb, JDAW_THREAD_MAIN); */
}

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
    /* pthread_mutex_init(&dl->lock, NULL); */
    /* dl->lock = SDL_CreateMutex(); */
    dl->max_len = DELAY_LINE_MAX_LEN_S * sample_rate;
    dl->buf_L = calloc(dl->max_len, sizeof(double));
    dl->buf_R = calloc(dl->max_len, sizeof(double));
    dl->cpy_buf = calloc(dl->max_len, sizeof(double));

    endpoint_init(
	&dl->len_ep,
	&dl->len_msec,
	JDAW_INT16,
	"len",
	"Time (ms)",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, delay_line_len_dsp_cb,
	/* NULL, NULL, delay_line_len_dsp_cb, */
	(void *)dl, NULL, &dl->effect->page, "track_settings_delay_time_slider");
    endpoint_set_allowed_range(&dl->len_ep, (Value){.int16_v=0}, (Value){.int16_v=1000});
    endpoint_set_label_fn(&dl->len_ep, label_msec);
    api_endpoint_register(&dl->len_ep, &dl->effect->api_node);

    endpoint_init(
	&dl->amp_ep,
	&dl->amp,
	JDAW_DOUBLE,
	"amp",
	"Amp",
	JDAW_THREAD_DSP,
	/* delay_line_amp_gui_cb, NULL, NULL, */
	page_el_gui_cb, NULL, NULL,
	/* delay_line_amp_gui_cb, NULL, NULL, */
	(void *)dl, NULL, &dl->effect->page, "track_settings_delay_amp_slider");
    endpoint_set_allowed_range(&dl->amp_ep, (Value){.double_v=0.0}, (Value){.double_v=0.99});
    endpoint_set_label_fn(&dl->amp_ep, label_amp_to_dbstr);
    api_endpoint_register(&dl->amp_ep, &dl->effect->api_node);
    
    endpoint_init(
	&dl->stereo_offset_ep,
	&dl->stereo_offset,
	JDAW_DOUBLE,
	"stereo_offset",
	"Stereo offset",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	/* delay_line_stereo_offset_gui_cb, NULL, NULL, */
	NULL, NULL, &dl->effect->page, "track_settings_delay_stereo_offset_slider");
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
    /* pthread_mutex_lock(&dl->lock); */
    /* if (len > session->proj.sample_rate) { */
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
    /* pthread_mutex_unlock(&dl->lock); */
}

float delay_line_buf_apply(void *dl_v, float *buf, int len, int channel, float input_amp)
{
    DelayLine *dl = dl_v;
    float output_amp = 0.0f;
    /* if (!dl->active) return input_amp; */
    /* pthread_mutex_lock(&dl->lock); */
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
    /* pthread_mutex_unlock(&dl->lock); */
    return output_amp;
}

void delay_line_clear(DelayLine *dl)
{
    memset(dl->buf_L, '\0', dl->len * sizeof(double));
    memset(dl->buf_R, '\0', dl->len * sizeof(double));
}


void delay_line_deinit(DelayLine *dl)
{
    if (dl->buf_L) free(dl->buf_L);
    if (dl->buf_R) free(dl->buf_R);
    if (dl->cpy_buf) free(dl->cpy_buf);	
}
