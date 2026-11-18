/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2026 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <stdlib.h>
#include "SDL_stdinc.h"
#include "atomic.h"
#include "color.h"
#include "consts.h"
#include "dev.h"
#include "dsp_utils.h"
#include "endpoint.h"
#include "geometry.h"
#include "eq.h"
#include "iir.h"
#include "input.h"
#include "label.h"
#include "logscale.h"
#include "project.h"
#include "session.h"
#include "session_endpoint_ops.h"
#include "value.h"
#include "waveform.h"


#define DEFAULT_BANDWIDTH_SCALAR 3.0

extern Window *main_win;
extern struct colors colors;



SDL_Color grey_clear = {200, 200, 200, 100};


SDL_Color EQ_CTRL_COLORS[] = {
    {255, 10, 10, 255},
    {255, 220, 0, 255},
    {10, 255, 10, 255},
    {0, 245, 190, 255},
    {225, 20, 155, 255},
    {10, 10, 255, 255},
};

SDL_Color EQ_CTRL_COLORS_LIGHT[] = {
    {255, 10, 10, 100},
    {255, 220, 0, 100},
    {10, 255, 10, 100},
    {0, 245, 190, 100},
    {225, 20, 155, 100},
    {10, 10, 255, 100}
};

static void eq_dsp_cb(Endpoint *ep)
{
    EQ *eq = ep->xarg1;
    EQFilterCtrl *ctrl = ep->xarg2;
    double bw_preferred = endpoint_read(&ctrl->bandwidth_preferred_ep, NULL).double_v;
    ctrl->bandwidth_scalar = bw_preferred;
    eq_set_filter_from_ctrl(eq, ctrl - eq->ctrls);
}

static void eq_freq_dsp_cb(Endpoint *ep)
{
    EQ *eq = ep->xarg1;
    EQFilterCtrl *ctrl = ep->xarg2;
    int flen = eq->effect->effect_chain->proj->fourier_len_sframes / 2;
    ctrl->freq_amp_raw[0] = pow(flen, ctrl->freq_exp) / flen;
    eq_dsp_cb(ep);
}

/* static void eq_bandwidth_scalar_dsp_cb(Endpoint *ep) */
/* { */
/*     /\* EQ *eq = ep->xarg1; *\/ */
/*     EQFilterCtrl *ctrl = ep->xarg2; */
/*     ctrl->bandwidth_preferred = ctrl->bandwidth_scalar; */
/*     eq_dsp_cb(ep); */

/* } */

static void eq_gui_cb(Endpoint *ep)
{
    EQ *eq = ep->xarg1;
    EQFilterCtrl *ctrl = ep->xarg2;
    if (eq->fp) {
        Value freq_amp_val = endpoint_read(&ctrl->freq_amp_ep, NULL);
        double freq_amp[2];
        memcpy(freq_amp, freq_amp_val.double_pair_v, 2 * sizeof(double));
        
	ctrl->x = logscale_x_abs(&eq->fp->x_axis, freq_amp[0] * eq->fp->x_axis.max_scaled);
        ctrl->y = waveform_freq_plot_y_abs_from_amp(eq->fp, freq_amp[1], 0, true);
	Value v = {.double_pair_v = {freq_amp[0], freq_amp[1]}};
	if (ep->display_label) {
	    label_move(ctrl->label, ctrl->x + 10 * main_win->dpi_scale_factor, ctrl->y - 10 * main_win->dpi_scale_factor);
	    label_reset(ctrl->label, v);
	}
    }
    int stale[] = {0, 0, 0, 0, 0, 0};
    for (int i=0; i<EQ_DEFAULT_NUM_FILTERS; i++) {
        if (eq->group.filters[i].freq_resp_stale) {
            stale[i] = 1;
        }
    }
    iir_group_update_freq_resp(&eq->group);
}

static void selected_filter_active_gui_cb(Endpoint *ep)
{
    EQ *eq = ep->xarg1;
    iir_group_update_freq_resp(&eq->group);
}


void eq_ctrl_label_fn(char *dst, size_t dstsize, Value val, ValType type)
{
    /* label_freq_raw_to_hz(dst, dstsize, freq, JDAW_DOUBLE); */
    /* Session *session = session_get(); */
    int hz = val.double_pair_v[0] * session_get_sample_rate() / 2;
    size_t written = snprintf(dst, dstsize, "%d Hz, ", hz);
    dstsize -= written;
    dst += written;
    Value amp = (Value){.double_v = val.double_pair_v[1]};
    label_amp_to_dbstr(dst, dstsize, amp, JDAW_DOUBLE);
}

static void eq_set_filter_active(EQ *eq, int index, bool active)
{
    astrr(&eq->ctrls[index].filter_active, active);
    astrr(&eq->group.filters[index].bypass, !active);
    if (!active && eq->fp) {
        iir_set_neutral_freq_resp(&eq->group.filters[index]);
    }
    astrr(&eq->group.filters[index].freq_resp_stale, true);
}

static void selected_filter_active_dsp_cb(Endpoint *ep)
{
    EQ *eq = ep->xarg1;
    uint8_t val = endpoint_read(ep, NULL).uint8_v;
    bool active = val & 0xF0;
    uint8_t index = val & 0x0F;
    eq_set_filter_active(eq, index, active);    
}


void eq_init(EQ *eq)
{
    eq->output_freq_mag_len = eq->effect->effect_chain->chunk_len_sframes * 2;
    switch (eq->output_freq_mag_len) {
    case 64:
        shared_double64_init(&eq->output_freq_mag_L.sd64);
        shared_double64_init(&eq->output_freq_mag_R.sd64);
        break;
    case 128:
        shared_double128_init(&eq->output_freq_mag_L.sd128);
        shared_double128_init(&eq->output_freq_mag_R.sd128);
        break;
    case 256:
        shared_double256_init(&eq->output_freq_mag_L.sd256);
        shared_double256_init(&eq->output_freq_mag_R.sd256);
        break;
    case 512:
        shared_double512_init(&eq->output_freq_mag_L.sd512);
        shared_double512_init(&eq->output_freq_mag_R.sd512);
        break;
    case 1024:
        shared_double1024_init(&eq->output_freq_mag_L.sd1024);
        shared_double1024_init(&eq->output_freq_mag_R.sd1024);
        break;
    case 2048:
        shared_double2048_init(&eq->output_freq_mag_L.sd2048);
        shared_double2048_init(&eq->output_freq_mag_R.sd2048);
        break;
    case 4096:
        shared_double4096_init(&eq->output_freq_mag_L.sd4096);
        shared_double4096_init(&eq->output_freq_mag_R.sd4096);
        break;
    case 8192:
        shared_double8192_init(&eq->output_freq_mag_L.sd8192);
        shared_double8192_init(&eq->output_freq_mag_R.sd8192);
        break;        
    }

    double nsub1 = (double)eq->effect->effect_chain->chunk_len_sframes - 1;
    iir_group_init(&eq->group, EQ_DEFAULT_NUM_FILTERS, 2, EQ_DEFAULT_CHANNELS); /* STEREO, BIQUAD */

    eq->group.filters[0].type = IIR_LOWSHELF;
    eq->group.filters[EQ_DEFAULT_NUM_FILTERS - 1].type = IIR_HIGHSHELF;

    endpoint_init(
	&eq->selected_filter_active_ep,
	&eq->selected_filter_active_raw,
	JDAW_UINT8,
	"selected_filter_active",
	"",
        JDAW_THREAD_DSP,
	selected_filter_active_gui_cb, NULL, selected_filter_active_dsp_cb,
	eq, NULL, NULL, NULL);
    endpoint_set_default_value(&eq->selected_filter_active_ep, (Value){.int_v = 0});

    static char api_ctrl_node_names[EQ_DEFAULT_NUM_FILTERS][12];
		  		  
    for (int i=0; i<EQ_DEFAULT_NUM_FILTERS; i++) {
	EQFilterCtrl *ctrl = eq->ctrls + i;
	ctrl->bandwidth_scalar = DEFAULT_BANDWIDTH_SCALAR;
	ctrl->bandwidth_preferred = DEFAULT_BANDWIDTH_SCALAR;
	ctrl->freq_amp_raw[0] = pow(nsub1, 0.15 + 0.15 * i) / nsub1;
	ctrl->freq_amp_raw[1] = 1.0;
	ctrl->eq = eq;
	ctrl->index = i;
	ctrl->label = label_create(
	    EQ_CTRL_LABEL_BUFLEN,
	    main_win->layout,
	    eq_ctrl_label_fn,
	    ctrl,
	    JDAW_DOUBLE,
	    main_win);

	snprintf(api_ctrl_node_names[i], 12, "Filter %d", i + 1);
	api_node_register(&ctrl->api_node, &eq->effect->api_node, NULL, api_ctrl_node_names[i]);
	    
	endpoint_init(
	    &ctrl->amp_ep,
	    &ctrl->freq_amp_raw[1],
	    JDAW_DOUBLE,
	    "amp",
	    "Amp",
	    JDAW_THREAD_DSP,
	    eq_gui_cb, NULL, eq_dsp_cb,
	    (void*)eq, (void*)(ctrl), NULL, NULL);
	endpoint_set_allowed_range(
	    &ctrl->amp_ep,
	    (Value){.double_v = 0.0},
	    (Value){.double_v = 20.0});
	endpoint_set_default_value(
	    &ctrl->amp_ep,
	    (Value){.double_v = 1.0});
        api_endpoint_register(&ctrl->amp_ep, &ctrl->api_node);

	endpoint_init(
	    &ctrl->freq_ep,
	    &ctrl->freq_exp,
	    JDAW_DOUBLE,
	    "freq",
	    "Freq",
	    JDAW_THREAD_DSP,
	    eq_gui_cb, NULL, eq_freq_dsp_cb,
	    (void*)eq, (void*)(ctrl), NULL, NULL);
	endpoint_set_allowed_range(
	    &ctrl->freq_ep,
	    (Value){.double_v = 0.0},
	    (Value){.double_v = 0.999});
	endpoint_set_default_value(
	    &ctrl->freq_ep,
	    (Value){.double_v = 0.1});
        api_endpoint_register(&ctrl->freq_ep, &ctrl->api_node);
	
	/* endpoint_init( */
	/*     &ctrl->bandwidth_scalar_ep, */
	/*     &ctrl->bandwidth_scalar, */
	/*     JDAW_DOUBLE, */
	/*     "bandwidth_scalar", */
	/*     "bandwidth_scalar_raw", */
	/*     JDAW_THREAD_DSP, */
	/*     eq_gui_cb, NULL, eq_bandwidth_scalar_dsp_cb, */
	/*     (void*)eq, (void*)(ctrl), NULL, NULL); */
	endpoint_init(
	    &ctrl->freq_amp_ep,
	    &ctrl->freq_amp_raw,
	    JDAW_DOUBLE_PAIR,
	    "freq_and_amp",
	    "Freq and Amp",
	    JDAW_THREAD_DSP,
	    eq_gui_cb, NULL, eq_dsp_cb,
	    (void *)eq, (void *)(ctrl), NULL, NULL);
	api_endpoint_register(&ctrl->freq_amp_ep, &ctrl->api_node);
	ctrl->freq_amp_ep.automatable = false;
	ctrl->freq_amp_ep.do_not_serialize = true;

	endpoint_init(
	    &ctrl->bandwidth_preferred_ep,
	    &ctrl->bandwidth_preferred,
	    JDAW_DOUBLE,
	    "bandwidth_preferred",
	    "Bandwidth",
	    JDAW_THREAD_DSP,
	    eq_gui_cb, NULL, eq_dsp_cb,
	    (void*)eq, (void*)(ctrl), NULL, NULL);
	endpoint_set_allowed_range(
	    &ctrl->bandwidth_preferred_ep,
	    (Value){.double_v = 0.001},
	    (Value){.double_v = 5.0});
	endpoint_set_default_value(
	    &ctrl->bandwidth_preferred_ep,
	    (Value){.double_v = 0.4});
        api_endpoint_register(&ctrl->bandwidth_preferred_ep, &ctrl->api_node);

        endpoint_init(
            &ctrl->filter_type_ep,
            &ctrl->filter_type,
            JDAW_INT,
            "filter_type",
            "Filter type",
            JDAW_THREAD_DSP,
            eq_gui_cb, NULL, eq_dsp_cb,
            (void *)eq, (void *)ctrl, NULL, NULL);
        endpoint_set_allowed_range(
            &ctrl->filter_type_ep,
            (Value){.int_v = 0},
            (Value){.int_v = NUM_IIR_FILTER_TYPES - 1});
        endpoint_set_default_value(
	    &ctrl->filter_type_ep,
            (Value){.int_v = (int)(
                    i == 0 ? IIR_LOWSHELF :
                    i == EQ_DEFAULT_NUM_FILTERS - 1 ? IIR_HIGHSHELF :
                    IIR_PEAKNOTCH
                    )});
        endpoint_write_default(&ctrl->filter_type_ep);
        api_endpoint_register(&ctrl->filter_type_ep, &ctrl->api_node);
    }
}

void eq_deinit(EQ *eq)
{
    for (int i=0; i<eq->group.num_filters; i++) {
	EQFilterCtrl *ctrl = eq->ctrls + i;
	label_destroy(ctrl->label);
    }
    iir_group_deinit(&eq->group);
    if (eq->fp) waveform_destroy_freq_plot(eq->fp);
    
}

void eq_select_ctrl(EQ *eq, int index)
{
    uint8_t current_val = endpoint_read(&eq->selected_filter_active_ep, NULL).uint8_v;
    uint8_t new_val = (current_val & 0xF0) | ((uint8_t)index);
    endpoint_write(&eq->selected_filter_active_ep, (Value){.uint8_v = new_val}, true, true, true, false);
    /* eq->selected_ctrl = index; */
    /* endpoint_write(&eq->selected_filter_active_ep, (Value){.bool_v = eq->ctrls[eq->selected_ctrl].filter_active}, true, true, true, true); */
    /* eq->selected_filter_active = eq->ctrls[eq->selected_ctrl].filter_active; */
}

/* Called in:
   - dot_jdaw.c [TODO: remove!]
   - eq_dsp_cb
*/
void eq_set_filter_from_ctrl(EQ *eq, int index)
{
    EQFilterCtrl *ctrl = eq->ctrls + index;
    IIRFilter *filter = eq->group.filters + index;
    filter->type = ctrl->filter_type;

    double freq_raw = ctrl->freq_amp_raw[0];
    double amp_raw = ctrl->freq_amp_raw[1];
    static const double epsilon = 1e-9;
    if (fabs(amp_raw - 1.0) < epsilon) {
        uint8_t value = 0x00 | index;
        endpoint_write(&eq->selected_filter_active_ep, (Value){.uint8_v = value}, true, true, true, false);
    } else {
        uint8_t value = 0xF0 | index;
        endpoint_write(&eq->selected_filter_active_ep, (Value){.uint8_v = value}, true, true, true, false);
    }
    switch(filter->type) {
    case IIR_PEAKNOTCH: {
	double bandwidth_scalar_adj;
	double bandwidth = ctrl->bandwidth_scalar * freq_raw;
	int ret = iir_set_coeffs_peaknotch(filter, freq_raw, amp_raw, bandwidth, &bandwidth_scalar_adj);
	if (ret == 1) {
	    eq->ctrls[index].bandwidth_scalar = bandwidth_scalar_adj;
	}
    }
	break;
    case IIR_LOWSHELF:
	iir_set_coeffs_lowshelf(filter, freq_raw, amp_raw);
	break;
    case IIR_HIGHSHELF: {
	iir_set_coeffs_highshelf(filter, freq_raw, amp_raw);
	break;
    }
    default:
	break;
    }
}

/* Called from button action */
void eq_set_filter_type(EQ *eq, IIRFilterType t)
{
    uint8_t sel_filter_active = endpoint_read(&eq->selected_filter_active_ep, NULL).uint8_v;
    EQFilterCtrl *ctrl = eq->ctrls + (sel_filter_active & 0x0F);
    endpoint_write(&ctrl->filter_type_ep, (Value){.int_v = t}, true, true, true, true);
}

static void eq_set_filter_from_mouse(EQ *eq, int filter_index, SDL_Point mousep)
{
    eq->ctrls[filter_index].x = mousep.x;
    eq->ctrls[filter_index].y = mousep.y;
    double freq_hz = logscale_val_from_x_abs(&eq->fp->x_axis, mousep.x);
    double freq_raw =  freq_hz / eq->fp->x_axis.max_scaled;
    double amp_raw;
    if (main_win->i_state & I_STATE_SHIFT) {
        amp_raw = 1.0;
    } else {
        amp_raw = waveform_freq_plot_amp_from_y_abs(eq->fp, mousep.y, 0, true);
    }

    endpoint_start_continuous_change(&eq->ctrls[filter_index].freq_amp_ep, false, (Value){0}, JDAW_THREAD_DSP, (Value){.double_pair_v = {freq_raw, amp_raw}});
    endpoint_write(&eq->ctrls[filter_index].freq_amp_ep, (Value){.double_pair_v = {freq_raw, amp_raw}}, true, true, true, false);
}

void eq_mouse_motion(EQFilterCtrl *ctrl, Window *win)
{
    EQ *eq = ctrl->eq;
    if (!eq->fp) return;
    SDL_Rect rect = eq->fp->container->rect;
    SDL_Point write_point = win->mousep;
    if (write_point.x < rect.x)  write_point.x = rect.x;
    else if (write_point.x > rect.x + rect.w) write_point.x = rect.x + rect.w - 1;
    if (write_point.y < rect.y) write_point.y = rect.y;
    else if (write_point.y > rect.y + rect.h) write_point.y = rect.y + rect.h - 1;
    if (win->i_state & I_STATE_CMDCTRL) {
	double new_bw_scalar = ctrl->bandwidth_scalar + (double)win->current_event->motion.yrel / 100.0;
	if (new_bw_scalar < 0.001) new_bw_scalar = 0.005;
	if (!ctrl->bandwidth_preferred_ep.changing) {
            endpoint_start_continuous_change(&ctrl->bandwidth_preferred_ep, false, (Value){0}, JDAW_THREAD_DSP, (Value){.double_v = new_bw_scalar});
	} else {
            endpoint_write(&ctrl->bandwidth_preferred_ep, (Value){.double_v = new_bw_scalar}, true, true, true, false);
	}
    } else {
	eq_set_filter_from_mouse(eq, ctrl->index, write_point);
    }
}

bool eq_mouse_click(EQ *eq, SDL_Point mousep)
{
    Session *session = session_get();
    int click_tolerance = 20 * main_win->dpi_scale_factor;
    int min_dist = click_tolerance;
    int clicked_i = -1;
    for (int i=0; i<EQ_DEFAULT_NUM_FILTERS; i++) {
	int x_dist = abs(mousep.x - eq->ctrls[i].x);
	if (x_dist < min_dist) {
	    min_dist = x_dist;
	    clicked_i = i;
	}
    }
    if (clicked_i < 0) return false;
    
    if (min_dist < click_tolerance) {
	eq_set_filter_from_mouse(eq, clicked_i, mousep);
	session->dragged_component.component = eq->ctrls + clicked_i;
	session->dragged_component.type = DRAG_EQ_FILTER_NODE;
	eq_select_ctrl(eq, clicked_i);
	return true;
    }
    return false;
}

void eq_create_freq_plot(EQ *eq, Layout *container)
{
    SharedDoubleMax8192 *arrs[] = {&eq->output_freq_mag_L, &eq->output_freq_mag_R};
    int lens[] = {eq->effect->effect_chain->chunk_len_sframes, eq->effect->effect_chain->chunk_len_sframes};
    
    SDL_Color *plot_colors[] = {&colors.freq_L, &colors.freq_R};

    eq->fp = waveform_create_freq_plot(
	NULL, NULL, 0,
	NULL, NULL, 0,
        arrs, lens, 2,
	NULL, NULL, plot_colors,
	20, (double)eq->effect->effect_chain->proj->sample_rate / 2,
	container);
    waveform_reset_freq_plot(eq->fp);

    eq->group.fp = eq->fp;

    iir_group_add_freqplot(&eq->group, eq->fp);
    
    iir_group_update_freq_resp(&eq->group);
    waveform_freq_plot_add_linear_plot(eq->fp, IIR_FREQPLOT_RESOLUTION, eq->group.freq_resp, &colors.white);
    eq->fp->linear_plot_ranges[0] = EQ_MAX_AMPLITUDE;
    eq->fp->linear_plot_mins[0] = 0.0;
    static double ones[] = {1.0, 1.0, 1,0};
    waveform_freq_plot_add_linear_plot(eq->fp, 3, ones, &grey_clear);
    eq->fp->linear_plot_ranges[1] = EQ_MAX_AMPLITUDE;
    eq->fp->linear_plot_mins[1] = 0.0;

    /* TODO: investigate duplicate call */
    iir_group_update_freq_resp(&eq->group);
    
    for (int i=0; i<EQ_DEFAULT_NUM_FILTERS; i++) {
	eq->ctrls[i].x = waveform_freq_plot_x_abs_from_freq(eq->fp, eq->ctrls[i].freq_amp_raw[0]);
	eq->ctrls[i].y = waveform_freq_plot_y_abs_from_amp(eq->fp, eq->ctrls[i].freq_amp_raw[1], 0, true);
    }
}

void eq_destroy_freq_plot(EQ *eq)
{
    waveform_destroy_freq_plot(eq->fp);
    eq->fp = NULL;
    for (int i=0; i<eq->group.num_filters; i++) {
	eq->group.filters[i].fp = NULL;
    }
    eq->group.fp = NULL;
}

static double eq_sample(EQ *eq, double in, int channel)
{
    /* if (!eq->active) return in; */
    for (int i=0; i<eq->group.num_filters; i++) {
	if (aldr(&eq->ctrls[i].filter_active)) {
	    in = iir_sample(eq->group.filters + i, in, channel);
	}
    }
    return in;
}

float eq_buf_apply(void *eq_v, float *restrict buf, int len, int channel, float input_amp)
{
    
    static float amp_epsilon = 1e-7f;
    EQ *eq = eq_v;

    if (input_amp < amp_epsilon) {
	eq_advance(eq, channel);
	return input_amp;
    }
    float output_amp = 0.0;
    for (int i=0; i<len; i++) {
	buf[i] = eq_sample(eq, buf[i], channel);
	output_amp += fabs(buf[i]);
    }

    /* IFF Freq Plot is onscreen, reset frequency magnitude spectrum */
    if (aldr(&eq->effect->page_onscreen)) {
        
	/* Zero-pad the input */
	float fft_input[len * 2];
	memset(fft_input + len, '\0', len * sizeof(float));
	memcpy(fft_input, buf, len * sizeof(float));
        
	/* Apply hamming window to non-zero input,
	   scaling up to account for amplitude reduction */
	for (int i=0; i<len; i++) {
	    fft_input[i] *= HAMMING_SCALAR * hamming(i, len);
	}
        
	double complex freq[len * 2];
	FFTf(fft_input, freq, len * 2);
        SharedDoubleMax8192 *sdu = channel == 0 ? &eq->output_freq_mag_L : &eq->output_freq_mag_R;
        double dst[len * 2];
        get_magnitude(freq, dst, len * 2);
        switch (len) {
        case 64: {
            double64 d;
            memcpy(d.arr, dst, len * sizeof(double));
            shared_double64_write(&sdu->sd64, d);
        }
            break;
        case 128: {
            double128 d;
            memcpy(d.arr, dst, len * sizeof(double));
            shared_double128_write(&sdu->sd128, d);
        }
            break;
        case 256: {
            double256 d;
            memcpy(d.arr, dst, len * sizeof(double));
            shared_double256_write(&sdu->sd256, d);
        }
            break;
        case 512: {
            double512 d;
            memcpy(d.arr, dst, len * sizeof(double));
            shared_double512_write(&sdu->sd512, d);
        }
            break;
        case 1024: {
            double1024 d;
            memcpy(d.arr, dst, len * sizeof(double));
            shared_double1024_write(&sdu->sd1024, d);
        }
            break;
        case 2048: {
            double2048 d;
            memcpy(d.arr, dst, len * sizeof(double));
            shared_double2048_write(&sdu->sd2048, d);
        }
            break;
        case 4096: {
            double4096 d;
            memcpy(d.arr, dst, len * sizeof(double));
            shared_double4096_write(&sdu->sd4096, d);
        }
            break;
        case 8192: {
            double8192 d;
            memcpy(d.arr, dst, len * sizeof(double));
            shared_double8192_write(&sdu->sd8192, d);
        }
            break;
        }
    }

    
    return output_amp;
}

void eq_advance(EQ *eq, int channel)
{
    for (int i=0; i<eq->group.num_filters; i++) {
	if (aldr(&eq->ctrls[i].filter_active)) {
	    iir_advance(eq->group.filters + i, channel);
	}
    }

}

float eq_buf_apply_stereo(void *eq_v, float *restrict L, float *restrict R, int len, float input_amp)
{
    float output_amp = 0.0f;
    if (L)
	output_amp += eq_buf_apply(eq_v, L, len, 0, input_amp);
    if (R)
	output_amp += eq_buf_apply(eq_v, R, len, 1, input_amp);
    return output_amp;
}

void eq_draw(EQ *eq)
{
    waveform_draw_freq_plot(eq->fp);
    for (int i=0; i<EQ_DEFAULT_NUM_FILTERS; i++) {
	if (endpoint_read(&eq->effect->active_ep, NULL).bool_v && aldr(&eq->ctrls[i].filter_active)) {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand((EQ_CTRL_COLORS_LIGHT + i)));
	    geom_fill_circle(main_win->rend, eq->ctrls[i].x - EQ_CTRL_RAD, eq->ctrls[i].y - EQ_CTRL_RAD, EQ_CTRL_RAD);
	    SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand((EQ_CTRL_COLORS + i)));
	} else {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.grey));
	}
	label_draw(eq->ctrls[i].label);
	
	geom_draw_circle(main_win->rend, eq->ctrls[i].x - EQ_CTRL_RAD, eq->ctrls[i].y - EQ_CTRL_RAD, EQ_CTRL_RAD);
        uint8_t sel_filter_active = endpoint_read(&eq->selected_filter_active_ep, NULL).uint8_v;
        uint8_t sel_filter_index = sel_filter_active & 0x0F;
	if (i == sel_filter_index) {
	    SDL_SetRenderDrawColor(main_win->rend, 255, 255, 255, 255);
	    geom_draw_circle(main_win->rend, eq->ctrls[i].x - EQ_SEL_CTRL_RAD, eq->ctrls[i].y - EQ_SEL_CTRL_RAD, EQ_SEL_CTRL_RAD);
	}
    }
}


void eq_clear(EQ *eq)
{
    iir_group_clear(&eq->group);
}
