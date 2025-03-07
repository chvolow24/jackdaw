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

#include "eq.h"
#include "project.h"
#include "waveform.h"

extern Project *proj;
extern SDL_Color freq_L_color;
extern SDL_Color freq_R_color;
extern SDL_Color color_global_white;

extern Window *main_win;


EQ glob_eq;

void eq_init(EQ *eq)
{
    iir_group_init(&eq->group, DEFAULT_NUM_FILTERS, 2, DEFAULT_CHANNELS); /* STEREO, 4 PEAK, BIQUAD */
}

void eq_set_peak(EQ *eq, int filter_index, double freq_raw, double amp_raw, double bandwidth)
{
    IIRFilter *iir = eq->group.filters + filter_index;
    iir_set_coeffs_peaknotch(iir, freq_raw, amp_raw, bandwidth);
    iir_group_update_freq_resp(&eq->group);
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
}


void eq_tests_create()
{
    Layout *fp_cont = layout_add_child(main_win->layout);
    layout_set_default_dims(fp_cont);

    layout_force_reset(fp_cont);
    
    eq_init(&glob_eq);
    eq_create_freq_plot(&glob_eq, fp_cont);

    /* void iir_set_coeffs_peaknotch(IIRFilter *iir, double freq, double amp, double bandwidth) */
    eq_set_peak(&glob_eq, 0, 0.01, 2.0, 0.01);
    eq_set_peak(&glob_eq, 1, 0.3, 0.03, 0.3);
}
