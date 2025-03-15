/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    eq.c

    * Parametric EQ
    * 4 cascaded biquad IIR filter
 *****************************************************************************************************************/

#include "color.h"
#include "geometry.h"
#include "eq.h"
#include "iir.h"
#include "input.h"
#include "label.h"
#include "project.h"
#include "waveform.h"

extern Project *proj;
extern SDL_Color freq_L_color;
extern SDL_Color freq_R_color;
extern SDL_Color color_global_white;
extern SDL_Color color_global_grey;
SDL_Color grey_clear = {200, 200, 200, 100};

extern Window *main_win;

static SDL_Color EQ_CTRL_COLORS[] = {
    {255, 10, 10, 255},
    {255, 220, 0, 255},
    {10, 255, 10, 255},
    {0, 245, 190, 255},
    {225, 20, 155, 255},
    {10, 10, 255, 255},
};

static SDL_Color EQ_CTRL_COLORS_LIGHT[] = {
    {255, 10, 10, 100},
    {255, 220, 0, 100},
    {10, 255, 10, 100},
    {0, 245, 190, 100},
    {225, 20, 155, 100},
    {10, 10, 255, 100}
};



/* EQ glob_eq; */
static void eq_set_peak(EQ *eq, int filter_index, double freq_raw, double amp_raw, double bandwidth);


/* endpoint_init(Endpoint *ep, void *val, */
/* 	      ValType t, */
/* 	      const char *local_id, */
/* 	      const char *undo_str, */
/* 	      enum jdaw_thread owner_thread, */
/* 	      EndptCb gui_cb, */
/* 	      EndptCb proj_cb, */
/* 	      EndptCb dsp_cb, */
/* 	      void *xarg1, void *xarg2, void *xarg3, void *xarg4) -> int */

static void eq_dsp_cb(Endpoint *ep)
{
    EQ *eq = ep->xarg1;
    EQFilterCtrl *ctrl = ep->xarg2;
    eq_set_peak(eq, ctrl->index, ctrl->freq_amp_raw[0], ctrl->freq_amp_raw[1], ctrl->bandwidth_scalar * ctrl->freq_amp_raw[0]);
}

static void eq_gui_cb(Endpoint *ep)
{
    EQ *eq = ep->xarg1;
    EQFilterCtrl *ctrl = ep->xarg2;
    if (eq->fp) {
	ctrl->x = waveform_freq_plot_x_abs_from_freq(eq->fp, ctrl->freq_amp_raw[0]);
	ctrl->y = waveform_freq_plot_y_abs_from_amp(eq->fp, ctrl->freq_amp_raw[1], 0, true);
	Value v = {.double_pair_v = {ctrl->freq_amp_raw[0], ctrl->freq_amp_raw[1]}};
	label_move(ctrl->label, ctrl->x + 10 * main_win->dpi_scale_factor, ctrl->y - 10 * main_win->dpi_scale_factor);
	label_reset(ctrl->label, v);
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
    int hz = val.double_pair_v[0] * proj->sample_rate / 2;
    size_t written = snprintf(dst, dstsize, "%d Hz, ", hz);
    dstsize -= written;
    dst += written;
    Value amp = (Value){.double_v = val.double_pair_v[1]};
    label_amp_to_dbstr(dst, dstsize, amp, JDAW_DOUBLE);
    /* fprintf(stderr, "LABLE: %s\n", dst); */
}


EndptCb test;
void eq_init(EQ *eq)
{
    double nsub1;
    if (proj) {
	nsub1 = (double)proj->fourier_len_sframes / 2 - 1;
    } else {
	nsub1 = (double)DEFAULT_FOURIER_LEN_SFRAMES / 2 - 1;
    }
    iir_group_init(&eq->group, EQ_DEFAULT_NUM_FILTERS, 2, EQ_DEFAULT_CHANNELS); /* STEREO, 4 PEAK, BIQUAD */
    for (int i=0; i<EQ_DEFAULT_NUM_FILTERS; i++) {
	eq->ctrls[i].bandwidth_scalar = 0.15;
	eq->ctrls[i].freq_amp_raw[0] = pow(nsub1, 0.15 + 0.15 * i) / nsub1;
	eq->ctrls[i].freq_amp_raw[1] = 1.0;
	eq->ctrls[i].eq = eq;
	eq->ctrls[i].index = i;
	eq->ctrls[i].label = label_create(
	    EQ_CTRL_LABEL_BUFLEN,
	    main_win->layout,
	    eq_ctrl_label_fn,
	    eq->ctrls + i,
	    JDAW_DOUBLE,
	    main_win);
	
	    
	    
	endpoint_init(
	    &eq->ctrls[i].amp_ep,
	    &eq->ctrls[i].freq_amp_raw + 1,
	    JDAW_DOUBLE,
	    "eq_peak_amp",
	    "undo set eq filter peak",
	    JDAW_THREAD_DSP,
	    eq_gui_cb, NULL, eq_dsp_cb,
	    (void*)eq, (void*)(eq->ctrls + i), NULL, NULL);
	endpoint_init(
	    &eq->ctrls[i].freq_ep,
	    &eq->ctrls[i].freq_amp_raw,
	    JDAW_DOUBLE,
	    "eq_peak_freq",
	    "undo set eq filter freq",
	    JDAW_THREAD_DSP,
	    eq_gui_cb, NULL, eq_dsp_cb,
	    (void*)eq, (void*)(eq->ctrls + i), NULL, NULL);
	endpoint_init(
	    &eq->ctrls[i].bandwidth_scalar_ep,
	    &eq->ctrls[i].bandwidth_scalar,
	    JDAW_DOUBLE,
	    "eq_bandwidth_scalar",
	    "undo set eq filter bandwidth",
	    JDAW_THREAD_DSP,
	    eq_gui_cb, NULL, eq_dsp_cb,
	    (void*)eq, (void*)(eq->ctrls + i), NULL, NULL);
	endpoint_init(
	    &eq->ctrls[i].freq_amp_ep,
	    &eq->ctrls[i].freq_amp_raw,
	    JDAW_DOUBLE_PAIR,
	    "eq_freq_and_amp",
	    "undo set eq freq/amp",
	    JDAW_THREAD_DSP,
	    eq_gui_cb, NULL, eq_dsp_cb,
	    (void *)eq, (void *)(eq->ctrls + i), NULL, NULL);
    }
}

void eq_deinit(EQ *eq)
{
    iir_group_deinit(&eq->group);
}

static void eq_set_peak(EQ *eq, int filter_index, double freq_raw, double amp_raw, double bandwidth)
{
    static const double epsilon = 1e-9;
    if (fabs(amp_raw - 1.0) < epsilon) {
	eq->ctrls[filter_index].filter_active = false;
    } else {
	eq->ctrls[filter_index].filter_active = true;
    }
    IIRFilter *iir = eq->group.filters + filter_index;
    double bandwidth_scalar_adj;
    int ret = iir_set_coeffs_peaknotch(iir, freq_raw, amp_raw, bandwidth, &bandwidth_scalar_adj);
    /* int ret = iir_set_coeffs_shelving(iir, freq_raw, amp_raw, bandwidth) */
    if (ret == 1) {
	/* fprintf(stderr, "RESETTING BW SCALAR: %f->%f\n", eq->ctrls[filter_index].bandwidth_scalar, bandwidth_scalar_adj); */
	eq->ctrls[filter_index].bandwidth_scalar = bandwidth_scalar_adj;
    }
}


static void eq_set_filter_from_mouse(EQ *eq, int filter_index, SDL_Point mousep)
{
    eq->ctrls[filter_index].x = mousep.x;
    eq->ctrls[filter_index].y = mousep.y;
    switch(eq->group.filters[filter_index].type) {
    case IIR_PEAKNOTCH: {
	double freq_raw = waveform_freq_plot_freq_from_x_abs(eq->fp, mousep.x);
	double amp_raw;
	if (main_win->i_state & I_STATE_SHIFT) {
	    /* endpoint_write(&eq->ctrls[filter_index].amp_ep, (Value){.double_v = 0.0}, true, true, true, false); */
	    amp_raw = 1.0;
	    /* eq->ctrls[filter_index].y = waveform_freq_plot_y_abs_from_amp(eq->fp, 1.0, 0, true); */
	} else {
	    amp_raw = waveform_freq_plot_amp_from_x_abs(eq->fp, mousep.y, 0, true);
	}

	endpoint_start_continuous_change(&eq->ctrls[filter_index].freq_amp_ep, false, (Value){0}, JDAW_THREAD_DSP, (Value){.double_pair_v = {freq_raw, amp_raw}});
	endpoint_write(&eq->ctrls[filter_index].freq_amp_ep, (Value){.double_pair_v = {freq_raw, amp_raw}}, true, true, true, false);
	/* endpoint_start_continuous_change(&eq->ctrls[filter_index].amp_ep, false, (Value){0}, JDAW_THREAD_DSP, (Value){.double_v = amp_raw}); */
	/* endpoint_start_continuous_change(&eq->ctrls[filter_index].freq_ep, false, (Value){0}, JDAW_THREAD_DSP, (Value){.double_v = freq_raw}); */
	/* endpoint_write(&eq->ctrls[filter_index].amp_ep, (Value){.double_v = amp_raw}, true, true, true, false); */
	/* endpoint_write(&eq->ctrls[filter_index].freq_ep, (Value){.double_v = freq_raw}, true, true, true, false); */

    }
	break;
    default:
	break;
    }   
}


void eq_mouse_motion(EQFilterCtrl *ctrl, Window *win)
{
    EQ *eq = ctrl->eq;
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
	eq_set_filter_from_mouse(eq, ctrl->index, win->mousep);
    }

}

bool eq_mouse_click(EQ *eq, SDL_Point mousep)
{
    int click_tolerance = 20 * main_win->dpi_scale_factor;
    int min_dist = click_tolerance;
    int clicked_i;
    for (int i=0; i<EQ_DEFAULT_NUM_FILTERS; i++) {
	int x_dist = abs(mousep.x - eq->ctrls[i].x);
	if (x_dist < min_dist) {
	    min_dist = x_dist;
	    clicked_i = i;
	}
    }
    if (min_dist < click_tolerance) {
	eq_set_filter_from_mouse(eq, clicked_i, mousep);
	proj->dragged_component.component = eq->ctrls + clicked_i;
	proj->dragged_component.type = DRAG_EQ_FILTER_NODE;
	return true;
    }
    return false;
}

void eq_create_freq_plot(EQ *eq, Layout *container)
{
    int steps[] = {1, 1};
    double *arrs[] = {proj->output_L_freq, proj->output_R_freq};
    SDL_Color *colors[] = {&freq_L_color, &freq_R_color};

    eq->fp = waveform_create_freq_plot(
	arrs,
	2,
	colors,
	steps,
	proj->fourier_len_sframes / 2,
	container);

    eq->group.fp = eq->fp;
    for (int i=0; i<eq->group.num_filters; i++) {
	eq->group.filters[i].fp = eq->fp;
    }
    iir_group_update_freq_resp(&eq->group);
    waveform_freq_plot_add_linear_plot(eq->fp, IIR_FREQPLOT_RESOLUTION, eq->group.freq_resp, &color_global_white);
    eq->fp->linear_plot_ranges[0] = EQ_MAX_AMPLITUDE;
    eq->fp->linear_plot_mins[0] = 0.0;
    static double ones[] = {1.0, 1.0, 1,0};
    waveform_freq_plot_add_linear_plot(eq->fp, 3, ones, &grey_clear);
    eq->fp->linear_plot_ranges[1] = EQ_MAX_AMPLITUDE;
    eq->fp->linear_plot_mins[1] = 0.0;

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


/* void eq_tests_create() */
/* { */
/*     Layout *fp_cont = layout_add_child(main_win->layout); */
/*     layout_set_default_dims(fp_cont); */

/*     layout_force_reset(fp_cont); */
    
/*     eq_init(&glob_eq); */
/*     eq_create_freq_plot(&glob_eq, fp_cont); */

/*     /\* void iir_set_coeffs_peaknotch(IIRFilter *iir, double freq, double amp, double bandwidth) *\/ */
/*     eq_set_peak(&glob_eq, 0, 0.01, 2.0, 0.01); */
/*     eq_set_peak(&glob_eq, 1, 0.3, 0.03, 0.3); */
/* } */

double eq_sample(EQ *eq, double in, int channel)
{
    if (!eq->active) return in;

    for (int i=0; i<eq->group.num_filters; i++) {
	if (eq->ctrls[i].filter_active) {
	    in = iir_sample(eq->group.filters + i, in, channel);
	}
    }
    return in;
    /* return iir_group_sample(&eq->group, in, channel); */
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
	if (eq->active && eq->ctrls[i].filter_active) {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand((EQ_CTRL_COLORS_LIGHT + i)));
	    geom_fill_circle(main_win->rend, eq->ctrls[i].x - EQ_CTRL_RAD, eq->ctrls[i].y - EQ_CTRL_RAD, EQ_CTRL_RAD);
	    SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand((EQ_CTRL_COLORS + i)));
	} else {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_grey));
	}
	label_draw(eq->ctrls[i].label);
	
	geom_draw_circle(main_win->rend, eq->ctrls[i].x - EQ_CTRL_RAD, eq->ctrls[i].y - EQ_CTRL_RAD, EQ_CTRL_RAD);
	/* geom_draw_circle(main_win->rend, eq->ctrls[i].x - 2.0 * radmin1div2, eq->ctrls[i].y - 2.0 * radmin1div2, radmin1); */
    }
}
