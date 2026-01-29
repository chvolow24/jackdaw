/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "color.h"
#include "consts.h"
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

/* static void eq_filter_selection_gui_cb(Endpoint *ep) */
/* { */
/*     EQ *eq = ep->xarg1; */
/*     int sel = endpoint_safe_read(ep, NULL).int_v; */
/*     TabView *tv = main_win->active_tabview; */
/*     if (!tv) return; */
/*     Page *page = tv->tabs[tv->current_tab]; */
/*     char id[12]; */
/*     snprintf(id, 12, "filter_tab%d\n", sel); */
/*     PageEl *el = page_get_el_by_id(page, id); */
/*     if (!el) { */
/* 	fprintf(stderr, "Error: cannot find tab"); */
/*     } */
    

/* } */


static void eq_dsp_cb(Endpoint *ep)
{
    EQ *eq = ep->xarg1;
    EQFilterCtrl *ctrl = ep->xarg2;

    ctrl->bandwidth_scalar = ctrl->bandwidth_preferred;
    eq_set_filter_from_ctrl(eq, ctrl - eq->ctrls);
    /* eq_set_peak(eq, ctrl->index, ctrl->freq_amp_raw[0], ctrl->freq_amp_raw[1], ctrl->bandwidth_scalar * ctrl->freq_amp_raw[0]); */
}

static void eq_freq_dsp_cb(Endpoint *ep)
{
    EQ *eq = ep->xarg1;
    EQFilterCtrl *ctrl = ep->xarg2;
    int flen = eq->effect->effect_chain->proj->fourier_len_sframes / 2;
    ctrl->freq_amp_raw[0] = pow(flen, ctrl->freq_exp) / flen;
    eq_dsp_cb(ep);
}

static void eq_bandwidth_scalar_dsp_cb(Endpoint *ep)
{
    /* EQ *eq = ep->xarg1; */
    EQFilterCtrl *ctrl = ep->xarg2;
    ctrl->bandwidth_preferred = ctrl->bandwidth_scalar;
    eq_dsp_cb(ep);

}

static void eq_gui_cb(Endpoint *ep)
{
    EQ *eq = ep->xarg1;
    EQFilterCtrl *ctrl = ep->xarg2;
    if (eq->fp) {
	ctrl->x = logscale_x_abs(&eq->fp->x_axis, ctrl->freq_amp_raw[0] * eq->fp->x_axis.max_scaled);
	/* ctrl->y = waveform_amp_from_ */
	/* ctrl->x = waveform_freq_plot_x_abs_from_freq(eq->fp, ctrl->freq_amp_raw[0]); */
	/* ctrl->y = waveform_freq_plot_y_abs_from_amp(eq->fp, ctrl->freq_amp_raw[1], 0, true); */
	Value v = {.double_pair_v = {ctrl->freq_amp_raw[0], ctrl->freq_amp_raw[1]}};
	if (ep->display_label) {
	    label_move(ctrl->label, ctrl->x + 10 * main_win->dpi_scale_factor, ctrl->y - 10 * main_win->dpi_scale_factor);
	    label_reset(ctrl->label, v);
	}
    }
    iir_group_update_freq_resp(&eq->group);
}
/* void eq_freq_dsp_cb(Endpoint *ep) */
/* { */
/*     EQ *eq = ep->xarg1; */
/*     EQFilterCtrl *ctrl = ep->xarg2; */
    
/* } */


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
    /* fprintf(stderr, "LABLE: %s\n", dst); */
}


/* EndptCb test; */

void eq_toggle_selected_filter_active(EQ *eq);

static void selected_filter_active_cb(Endpoint *ep)
{
    EQ *eq = ep->xarg1;
    eq_toggle_selected_filter_active(eq);
}

void eq_init(EQ *eq)
{
    eq->output_freq_mag_L = calloc(eq->effect->effect_chain->chunk_len_sframes * 2, sizeof(double));
    eq->output_freq_mag_R = calloc(eq->effect->effect_chain->chunk_len_sframes * 2, sizeof(double));

    double nsub1 = (double)eq->effect->effect_chain->chunk_len_sframes - 1;
    /* if (session->proj_initialized) { */
    /* 	nsub1 = (double)session->proj.fourier_len_sframes / 2 - 1; */
    /* } else { */
    /* 	nsub1 = (double)DEFAULT_FOURIER_LEN_SFRAMES / 2 - 1; */
    /* } */
    iir_group_init(&eq->group, EQ_DEFAULT_NUM_FILTERS, 2, EQ_DEFAULT_CHANNELS); /* STEREO, BIQUAD */

    eq->group.filters[0].type = IIR_LOWSHELF;
    eq->group.filters[EQ_DEFAULT_NUM_FILTERS - 1].type = IIR_HIGHSHELF;
    /* endpoint_init(&eq->selected_ctrl_ep, */
    /* 		  &eq->selected_ctrl, */
    /* 		  JDAW_INT, */
    /* 		  "", */
    /* 		  "", */
    /* 		  JDAW_THREAD_DSP, */
    /* 		  eq_filter_selection_gui_cb, */
    /* 		  NULL, */
    /* 		  NULL, */
    /* 		  eq, NULL, NULL, NULL); */

    endpoint_init(
	&eq->selected_filter_active_ep,
	&eq->selected_filter_active,
	JDAW_BOOL,
	"selected_filter_active",
	"",
        JDAW_THREAD_DSP,
	NULL, NULL, selected_filter_active_cb,
	eq, NULL, NULL, NULL);
    eq->selected_filter_active_ep.block_undo = true;
	
		  

    static char api_ctrl_node_names[EQ_DEFAULT_NUM_FILTERS][12];
		  		  
    for (int i=0; i<EQ_DEFAULT_NUM_FILTERS; i++) {
	EQFilterCtrl *ctrl = eq->ctrls + i;
	ctrl->bandwidth_scalar = DEFAULT_BANDWIDTH_SCALAR;
	ctrl->bandwidth_preferred = DEFAULT_BANDWIDTH_SCALAR;
	ctrl->freq_amp_raw[0] = pow(nsub1, 0.15 + 0.15 * i) / nsub1;
	fprintf(stderr, "CTRL %d freq: %f\n", i, ctrl->freq_amp_raw[0]);
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

	/* static char buf[255]; */
	/* snprintf(buf, 255, "EQ filter %d amp", i + 1); */
	/* char *display_name = strdup(buf); */
	/* ctrl->amp_ep_display_name = display_name; */


	snprintf(api_ctrl_node_names[i], 12, "Filter %d", i + 1);

	api_node_register(&ctrl->api_node, &eq->effect->api_node, NULL, api_ctrl_node_names[i]);

	    
	endpoint_init(
	    &ctrl->amp_ep,
	    &ctrl->freq_amp_raw[1],
	    JDAW_DOUBLE,
	    "amp",
	    "Amp",
	    /* display_name, */
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

	/* memset(buf, '\0', 255); */
	/* snprintf(buf, 255, "EQ filter %d freq", i + 1); */
	/* display_name = strdup(buf); */
	/* ctrl->freq_ep_display_name = display_name; */

	endpoint_init(
	    &ctrl->freq_ep,
	    &ctrl->freq_exp,
	    JDAW_DOUBLE,
	    "freq",
	    "Freq",
	    /* display_name, */
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
	
	endpoint_init(
	    &ctrl->bandwidth_scalar_ep,
	    &ctrl->bandwidth_scalar,
	    JDAW_DOUBLE,
	    "bandwidth_scalar",
	    "bandwidth_scalar_raw",
	    /* "EQ filter bandwidth", */
	    JDAW_THREAD_DSP,
	    eq_gui_cb, NULL, eq_bandwidth_scalar_dsp_cb,
	    (void*)eq, (void*)(ctrl), NULL, NULL);
	endpoint_init(
	    &ctrl->freq_amp_ep,
	    &ctrl->freq_amp_raw,
	    JDAW_DOUBLE_PAIR,
	    "freq_and_amp",
	    "Freq and Amp",
	    /* "EQ filter freq/amp", */
	    JDAW_THREAD_DSP,
	    eq_gui_cb, NULL, eq_dsp_cb,
	    (void *)eq, (void *)(ctrl), NULL, NULL);


	/* snprintf(buf, 255, "EQ filter %d bandwidth", i + 1); */
	/* display_name = strdup(buf); */
	/* ctrl->bandwidth_ep_display_name = display_name; */

	endpoint_init(
	    &ctrl->bandwidth_preferred_ep,
	    &ctrl->bandwidth_preferred,
	    JDAW_DOUBLE,
	    "bandwidth_preferred",
	    "Bandwidth",
	    /* display_name, */
	    JDAW_THREAD_DSP,
	    eq_gui_cb, NULL, eq_dsp_cb,
	    (void*)eq, (void*)(ctrl), NULL, NULL);
	endpoint_set_allowed_range(
	    &ctrl->bandwidth_preferred_ep,
	    (Value){.double_v = 0.001},
	    (Value){.double_v = 1.0});
	endpoint_set_default_value(
	    &ctrl->bandwidth_preferred_ep,
	    (Value){.double_v = 0.4});


	/* snprintf(buf, 255, "EQ filter %d active", i + 1); */
	/* display_name = strdup(buf); */
	/* ctrl->filter_active_ep_display_name = display_name; */

	/* endpoint_init( */
	/*     &ctrl->filter_active_ep, */
	/*     &ctrl->filter_active, */
	/*     JDAW_BOOL, */
	/*     "filter_active", */
	/*     display_name, */
	/*     JDAW_THREAD_DSP, */
	/*     NULL, NULL, NULL, */
	/*     NULL, NULL, NULL, NULL); */


	/* if (i < 2) { */
	api_endpoint_register(&ctrl->freq_ep, &ctrl->api_node);
	api_endpoint_register(&ctrl->amp_ep, &ctrl->api_node);
	api_endpoint_register(&ctrl->bandwidth_preferred_ep, &ctrl->api_node);
	/* api_endpoint_register(&ctrl->filter_active_ep, &ctrl->api_node); */
	/* api_endpoint_register(&ctrl->filter_active_ep, &ctrl->api_node); */
	/* } */


    }
}

void eq_deinit(EQ *eq)
{
    free(eq->output_freq_mag_L);
    free(eq->output_freq_mag_R);
    for (int i=0; i<eq->group.num_filters; i++) {
	EQFilterCtrl *ctrl = eq->ctrls + i;
	label_destroy(ctrl->label);
	/* if (ctrl->freq_ep_display_name) free(ctrl->freq_ep_display_name); */
	/* if (ctrl->amp_ep_display_name) free(ctrl->amp_ep_display_name); */
	/* if (ctrl->bandwidth_ep_display_name) free(ctrl->bandwidth_ep_display_name); */
	/* if (ctrl->filter_active_ep_display_name) free(ctrl->filter_active_ep_display_name); */
    }
    iir_group_deinit(&eq->group);
    if (eq->fp) waveform_destroy_freq_plot(eq->fp);
    
}

/* static void eq_set_peak(EQ *eq, int filter_index, double freq_raw, double amp_raw, double bandwidth) */
/* { */
/*     static const double epsilon = 1e-9; */
/*     if (fabs(amp_raw - 1.0) < epsilon) { */
/* 	eq->ctrls[filter_index].filter_active = false; */
/*     } else { */
/* 	eq->ctrls[filter_index].filter_active = true; */
/*     } */
/*     IIRFilter *iir = eq->group.filters + filter_index; */
/*     double bandwidth_scalar_adj; */
/*     int ret; */
/*     ret = iir_set_coeffs_peaknotch(iir, freq_raw, amp_raw, bandwidth, &bandwidth_scalar_adj); */
/*     /\* if (filter_index > 0) *\/ */
/*     /\* else *\/ */
/*     /\* 	ret = iir_set_coeffs_lowshelf(iir, freq_raw, amp_raw); *\/ */
/*     if (ret == 1) { */
/* 	/\* fprintf(stderr, "RESETTING BW SCALAR: %f->%f\n", eq->ctrls[filter_index].bandwidth_scalar, bandwidth_scalar_adj); *\/ */
/* 	eq->ctrls[filter_index].bandwidth_scalar = bandwidth_scalar_adj; */
/*     } */
/* } */

void eq_select_ctrl(EQ *eq, int index)
{
    eq->selected_ctrl = index;
    endpoint_write(&eq->selected_filter_active_ep, (Value){.bool_v = eq->ctrls[eq->selected_ctrl].filter_active}, true, true, true, false);
    /* eq->selected_filter_active = eq->ctrls[eq->selected_ctrl].filter_active; */
}

void eq_toggle_selected_filter_active(EQ *eq)
{
    bool *b = &eq->ctrls[eq->selected_ctrl].filter_active;
    *b = eq->selected_filter_active;
    /* eq->selected_filter_active = *b; */
    if (!*b) {
	eq->group.filters[eq->selected_ctrl].bypass = true;
	iir_set_neutral_freq_resp(eq->group.filters + eq->selected_ctrl);
    } else {
	/* eq_set_filter_from_ctrl(eq, eq->selected_ctrl); */
	eq->group.filters[eq->selected_ctrl].bypass = false;
	eq->group.filters[eq->selected_ctrl].freq_resp_stale = true;
    }
    iir_group_update_freq_resp(&eq->group);
}


void eq_set_filter_from_ctrl(EQ *eq, int index)
{
    EQFilterCtrl *ctrl = eq->ctrls + index;
    IIRFilter *filter = eq->group.filters + index;

    double freq_raw = ctrl->freq_amp_raw[0];
    double amp_raw = ctrl->freq_amp_raw[1];
    static const double epsilon = 1e-9;
    if (fabs(amp_raw - 1.0) < epsilon) {
	eq->ctrls[index].filter_active = false;
	eq->selected_filter_active = false;
	/* eq_select_ctrl(eq, index); */
    } else {
	eq->ctrls[index].filter_active = true;
	eq->selected_filter_active = true;
	/* eq_select_ctrl(eq, index); */
    }

    switch(filter->type) {
    case IIR_PEAKNOTCH: {

	double bandwidth_scalar_adj;
	double bandwidth = ctrl->bandwidth_scalar * freq_raw;
	int ret = iir_set_coeffs_peaknotch(filter, freq_raw, amp_raw, bandwidth, &bandwidth_scalar_adj);
	/* if (filter_index > 0) */
	/* else */
	/* 	ret = iir_set_coeffs_lowshelf(iir, freq_raw, amp_raw); */
	if (ret == 1) {
	    /* fprintf(stderr, "RESETTING BW SCALAR: %f->%f\n", eq->ctrls[filter_index].bandwidth_scalar, bandwidth_scalar_adj); */
	    eq->ctrls[index].bandwidth_scalar = bandwidth_scalar_adj;
	}
	/* eq_set_peak(eq, index, ctrl->freq_amp_raw[0], ctrl->freq_amp_raw[1], ctrl->freq_amp_raw[0] * ctrl->bandwidth_scalar); */
	/* iir_set_coeffs_peaknotch(filter, ctrl->freq_amp_raw[0], ctrl->freq_amp_raw[1], ctrl->freq_amp_raw[0] * ctrl->bandwidth_scalar, NULL); */
    }
	break;
    case IIR_LOWSHELF:
	iir_set_coeffs_lowshelf(filter, freq_raw, amp_raw);
	break;
    case IIR_HIGHSHELF: {
	/* IIRFilter *filter2 = eq->group.filters + index + 1; */
	/* iir_set_coeffs_highshelf_double(filter, filter2, freq_raw, amp_raw); */
	iir_set_coeffs_highshelf(filter, freq_raw, amp_raw);
	break;
    }
    default:
	break;
    }
}

void eq_set_filter_type(EQ *eq, IIRFilterType t)
{
    eq->group.filters[eq->selected_ctrl].type = t;    
    eq_set_filter_from_ctrl(eq, eq->selected_ctrl);
    iir_group_update_freq_resp(&eq->group);
}

static void eq_set_filter_from_mouse(EQ *eq, int filter_index, SDL_Point mousep)
{
    eq->ctrls[filter_index].x = mousep.x;
    eq->ctrls[filter_index].y = mousep.y;
    fprintf(stderr, "MOUSE X: %d (%f)\n", mousep.x, (double)(mousep.x - eq->fp->container->rect.x) / eq->fp->container->rect.w);
    double freq_hz = logscale_val_from_x_abs(&eq->fp->x_axis, mousep.x);
    double freq_raw =  freq_hz / eq->fp->x_axis.max_scaled;
    fprintf(stderr, "\tFreq hz: %f, raw: %f\n", freq_hz, freq_raw);
    /* fprintf(stderr, "\tFreq hz: %f\n", freq_hz); */
    /* double freq_raw = waveform_freq_plot_freq_from_x_abs(eq->fp, mousep.x); */
    double amp_raw;
    /* switch(eq->group.filters[filter_index].type) { */
    /* case IIR_PEAKNOTCH: { */
	if (main_win->i_state & I_STATE_SHIFT) {
	    /* endpoint_write(&eq->ctrls[filter_index].amp_ep, (Value){.double_v = 0.0}, true, true, true, false); */
	    amp_raw = 1.0;
	    /* eq->ctrls[filter_index].y = waveform_freq_plot_y_abs_from_amp(eq->fp, 1.0, 0, true); */
	} else {
	    amp_raw = waveform_freq_plot_amp_from_y_abs(eq->fp, mousep.y, 0, true);
	}

	endpoint_start_continuous_change(&eq->ctrls[filter_index].freq_amp_ep, false, (Value){0}, JDAW_THREAD_DSP, (Value){.double_pair_v = {freq_raw, amp_raw}});
	endpoint_write(&eq->ctrls[filter_index].freq_amp_ep, (Value){.double_pair_v = {freq_raw, amp_raw}}, true, true, true, false);
	/* endpoint_start_continuous_change(&eq->ctrls[filter_index].amp_ep, false, (Value){0}, JDAW_THREAD_DSP, (Value){.double_v = amp_raw}); */
	/* endpoint_start_continuous_change(&eq->ctrls[filter_index].freq_ep, false, (Value){0}, JDAW_THREAD_DSP, (Value){.double_v = freq_raw}); */
	/* endpoint_write(&eq->ctrls[filter_index].amp_ep, (Value){.double_v = amp_raw}, true, true, true, false); */
	/* endpoint_write(&eq->ctrls[filter_index].freq_ep, (Value){.double_v = freq_raw}, true, true, true, false); */

    /* } */
    /* 	break; */
    /* default: */
    /* 	break; */
    /* }    */
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
	if (!ctrl->bandwidth_scalar_ep.changing) {
	    endpoint_start_continuous_change(&ctrl->bandwidth_scalar_ep, false, (Value){0}, JDAW_THREAD_DSP, (Value){.double_v = new_bw_scalar});
	} else {
	    endpoint_write(&ctrl->bandwidth_scalar_ep, (Value){.double_v = new_bw_scalar}, true, true, true, false);
	}
	/* SDL_Point p = {ctrl->x, ctrl->y}; */
	/* eq_set_filter_from_mouse(eq, ctrl->index, p); */
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
    /* eq->selected_ctrl = clicked_i; */
    /* endpoint_write(&eq->selected_ctrl_ep, (Value){.int_v = clicked_i}, true, true, true, false); */
    
    /* eq->selected_ctrl = clicked_i; */
    
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
    double *arrs[] = {eq->output_freq_mag_L, eq->output_freq_mag_R};
    int lens[] = {eq->effect->effect_chain->chunk_len_sframes, eq->effect->effect_chain->chunk_len_sframes};
    /* double *arrs[] = {proj->output_L_freq, proj->output_R_freq}; */
    SDL_Color *fcolors[] = {&colors.freq_L, &colors.freq_R};

    /* Session *session = session_get(); */
    eq->fp = waveform_create_freq_plot(
	arrs,
	lens,
	2,
	NULL, NULL, 0,
	fcolors, NULL,
	40, (double)eq->effect->effect_chain->proj->sample_rate / 2,
	container);
    waveform_reset_freq_plot(eq->fp);

    eq->group.fp = eq->fp;

    iir_group_add_freqplot(&eq->group, eq->fp);
    /* for (int i=0; i<eq->group.num_filters; i++) { */
    /* 	eq->group.filters[i].fp = eq->fp; */
    /* } */
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
	if (eq->ctrls[i].filter_active) {
	    in = iir_sample(eq->group.filters + i, in, channel);
	}
    }
    return in;
}

float eq_buf_apply(void *eq_v, float *buf, int len, int channel, float input_amp)
{
    
    static float amp_epsilon = 1e-7f;
    EQ *eq = eq_v;


/*    if (!eq->active) {
	return input_amp;
	} else */
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
    if (eq->effect->page && eq->effect->page->onscreen) {
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
	double *dst = channel == 0 ? eq->output_freq_mag_L : eq->output_freq_mag_R;
	get_magnitude(freq, dst, len * 2);
    }

    
    return output_amp;
}

void eq_advance(EQ *eq, int channel)
{
    for (int i=0; i<eq->group.num_filters; i++) {
	if (eq->ctrls[i].filter_active) {
	    iir_advance(eq->group.filters + i, channel);
	}
    }

}

void eq_draw(EQ *eq)
{
    waveform_draw_freq_plot(eq->fp);
    for (int i=0; i<EQ_DEFAULT_NUM_FILTERS; i++) {
	if (eq->effect->active && eq->ctrls[i].filter_active) {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand((EQ_CTRL_COLORS_LIGHT + i)));
	    geom_fill_circle(main_win->rend, eq->ctrls[i].x - EQ_CTRL_RAD, eq->ctrls[i].y - EQ_CTRL_RAD, EQ_CTRL_RAD);
	    SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand((EQ_CTRL_COLORS + i)));
	} else {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.grey));
	}
	label_draw(eq->ctrls[i].label);
	
	geom_draw_circle(main_win->rend, eq->ctrls[i].x - EQ_CTRL_RAD, eq->ctrls[i].y - EQ_CTRL_RAD, EQ_CTRL_RAD);
	if (i == eq->selected_ctrl) {
	    SDL_SetRenderDrawColor(main_win->rend, 255, 255, 255, 255);
	    geom_draw_circle(main_win->rend, eq->ctrls[i].x - EQ_SEL_CTRL_RAD, eq->ctrls[i].y - EQ_SEL_CTRL_RAD, EQ_SEL_CTRL_RAD);
	}
	/* geom_draw_circle(main_win->rend, eq->ctrls[i].x - 2.0 * radmin1div2, eq->ctrls[i].y - 2.0 * radmin1div2, radmin1); */
    }
}


void eq_clear(EQ *eq)
{
    iir_group_clear(&eq->group);
}
