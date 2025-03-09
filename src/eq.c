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
#include "project.h"
#include "waveform.h"

extern Project *proj;
extern SDL_Color freq_L_color;
extern SDL_Color freq_R_color;
extern SDL_Color color_global_white;

extern Window *main_win;

static SDL_Color EQ_CTRL_COLORS[] = {
    {255, 0, 0, 255},
    {255, 240, 0, 255},
    {0, 255, 0, 255},
    {255, 0, 155, 255}
};

static SDL_Color EQ_CTRL_COLORS_LIGHT[] = {
    {255, 0, 0, 100},
    {255, 240, 0, 100},
    {0, 255, 0, 100},
    {255, 0, 155, 100}
};



/* EQ glob_eq; */
static void eq_set_peak(EQ *eq, int filter_index, double freq_raw, double amp_raw, double bandwidth);

void eq_init(EQ *eq)
{
    double nsub1 = (double)proj->fourier_len_sframes / 2 - 1;
    iir_group_init(&eq->group, EQ_DEFAULT_NUM_FILTERS, 2, EQ_DEFAULT_CHANNELS); /* STEREO, 4 PEAK, BIQUAD */
    for (int i=0; i<EQ_DEFAULT_NUM_FILTERS; i++) {
	eq->ctrls[i].bandwidth_scalar = 1.0;
	eq->ctrls[i].freq_raw = pow(nsub1, 0.2 + 0.2 * i) / nsub1;
	eq->ctrls[i].amp_raw = 1.0;
	eq->ctrls[i].eq = eq;
	eq->ctrls[i].index = i;
    }
}

void eq_deinit(EQ *eq)
{
    iir_group_deinit(&eq->group);
}

static void eq_set_peak(EQ *eq, int filter_index, double freq_raw, double amp_raw, double bandwidth)
{
    IIRFilter *iir = eq->group.filters + filter_index;
    double bandwidth_scalar_adj;
    int ret = iir_set_coeffs_peaknotch(iir, freq_raw, amp_raw, bandwidth, &bandwidth_scalar_adj);
    if (ret == 1) {
	/* fprintf(stderr, "RESETTING BW SCALAR: %f->%f\n", eq->ctrls[filter_index].bandwidth_scalar, bandwidth_scalar_adj); */
	eq->ctrls[filter_index].bandwidth_scalar = bandwidth_scalar_adj;
    }
    iir_group_update_freq_resp(&eq->group);
}


static void eq_set_filter_from_mouse(EQ *eq, int filter_index, SDL_Point mousep)
{
    eq->ctrls[filter_index].x = mousep.x;
    eq->ctrls[filter_index].y = mousep.y;
    switch(eq->group.filters[filter_index].type) {
    case IIR_PEAKNOTCH: {
	double freq_raw = waveform_freq_plot_freq_from_x_abs(eq->fp, mousep.x);
	double amp_raw = waveform_freq_plot_amp_from_x_abs(eq->fp, mousep.y, 0, true);
	eq->ctrls[filter_index].freq_raw = freq_raw;
	eq->ctrls[filter_index].amp_raw = amp_raw;
	eq_set_peak(eq, filter_index, freq_raw, amp_raw, eq->ctrls[filter_index].bandwidth_scalar * freq_raw);
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
	ctrl->bandwidth_scalar += (double)win->current_event->motion.yrel / 100.0;
	SDL_Point p = {ctrl->x, ctrl->y};
	eq_set_filter_from_mouse(eq, ctrl->index, p);
    } else {
	eq_set_filter_from_mouse(eq, ctrl->index, win->mousep);
    }

}

bool eq_mouse_click(EQ *eq, SDL_Point mousep)
{
    int click_tolerance = 10 * main_win->dpi_scale_factor;
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
    waveform_freq_plot_add_linear_plot(eq->fp, IIR_FREQPLOT_RESOLUTION, eq->group.freq_resp, &color_global_white);
    eq->fp->linear_plot_ranges[0] = EQ_MAX_AMPLITUDE;
    eq->fp->linear_plot_mins[0] = 0.0;
    iir_group_update_freq_resp(&eq->group);
    for (int i=0; i<EQ_DEFAULT_NUM_FILTERS; i++) {
	eq->ctrls[i].x = waveform_freq_plot_x_abs_from_freq(eq->fp, eq->ctrls[i].freq_raw);
	eq->ctrls[i].y = waveform_freq_plot_y_abs_from_amp(eq->fp, eq->ctrls[i].amp_raw, 0, true);
    }
}

void eq_destroy_freq_plot(EQ *eq)
{
    waveform_destroy_freq_plot(eq->fp);
    eq->fp = NULL;
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
    return iir_group_sample(&eq->group, in, channel);
}

void eq_draw(EQ *eq)
{

    static const int raddiv2 = (double)EQ_CTRL_RAD / 2;
    waveform_draw_freq_plot(eq->fp);
    for (int i=0; i<EQ_DEFAULT_NUM_FILTERS; i++) {
	SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand((EQ_CTRL_COLORS_LIGHT + i)));
        geom_fill_circle(main_win->rend, eq->ctrls[i].x - raddiv2 * main_win->dpi_scale_factor, eq->ctrls[i].y - raddiv2 * main_win->dpi_scale_factor, EQ_CTRL_RAD);
	SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand((EQ_CTRL_COLORS + i)));
	geom_draw_circle(main_win->rend, eq->ctrls[i].x - raddiv2 * main_win->dpi_scale_factor, eq->ctrls[i].y - raddiv2 * main_win->dpi_scale_factor, EQ_CTRL_RAD);
	/* geom_draw_circle(main_win->rend, eq->ctrls[i].x - 2.0 * radmin1div2, eq->ctrls[i].y - 2.0 * radmin1div2, radmin1); */
    }
}
