/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/

/*****************************************************************************************************************
    project_loop.c

    * main project animation loop
    * in-progress animations and updates
 *****************************************************************************************************************/


#include <time.h>
#include "SDL.h"
#include "audio_connection.h"
#include "components.h"
#include "draw.h"
#include "dsp.h"
#include "input.h"
#include "layout.h"
#include "modal.h"
#include "mouse.h"
#include "page.h"
#include "project.h"
#include "project_draw.h"
#include "screenrecord.h"
#include "status.h"
#include "timeline.h"
#include "transport.h"
#include "waveform.h"
#include "window.h"

#define MAX_MODES 8
#define STICK_DELAY_MS 500
#define PLAYSPEED_ADJUST_SCALAR_LARGE 0.1f
#define PLAYSPEED_ADJUST_SCALAR_SMALL 0.015f


#define TOP_MODE (main_win->modes[main_win->num_modes - 1])


#ifndef INSTALL_DIR
#define INSTALL_DIR "."
#endif


extern Window *main_win;
extern SDL_Color color_global_black;
extern SDL_Color color_global_white;


extern Project *proj;


/* static int timed_stop_update_track_vol_pan(void *data) */
/* { */
/*     fprintf(stdout, "OK.......\n"); */
/*     /\* int delay_ms = *((int *)data); *\/ */
/*     SDL_Delay(STICK_DELAY_MS); */
/*     fprintf(stdout, "Done!\n"); */
/*     stop_update_track_vol_pan(); */
/*     return 0; */
/* } */

static int timed_hide_slider_label(void *data)
{
    Slider *fs = (Slider *)data;
    if (fs->editing) {
	SDL_Delay(STICK_DELAY_MS);
	fs->editing = false;
    }
    return 0;
}

static void hide_slider_label(Slider *fs)
{
    SDL_CreateThread(timed_hide_slider_label, "hide_slider_label", fs);
}


static void stop_update_track_vol_pan()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    Track *trk = NULL;
    for (int i=0; i<tl->num_tracks; i++) {
	trk = tl->tracks[i];
	hide_slider_label(trk->vol_ctrl);
	hide_slider_label(trk->pan_ctrl);
        /* trk->vol_ctrl->editing = false; */
	/* trk->pan_ctrl->editing = false; */
    }
    proj->vol_changing = false;
    proj->pan_changing = false;
}


static void update_track_vol_pan()
{
    /* fprintf(stdout, "%p, %p\n", proj->vol_changing, proj->pan_changing); */
    if (!proj->vol_changing && !proj->pan_changing) {
	return;
    }
    Timeline *tl = proj->timelines[proj->active_tl_index];
    bool had_active_track = false;
    Track *selected_track = tl->tracks[tl->track_selector];
    /* if (proj->vol_changing) { */
    for (int i=0; i<tl->num_tracks; i++) {
	Track *trk = tl->tracks[i];
	if (trk->active) {
	    had_active_track = true;
	    if (proj->vol_changing) {
		/* hide_slider_label(trk->vol_ctrl); */
		trk->vol_ctrl->editing = true;
		if (proj->vol_up) {
		    track_increment_vol(trk);
		} else {
		    track_decrement_vol(trk);
		}
	    }
	    if (proj->pan_changing) {
		/* hide_slider_label */
		trk->pan_ctrl->editing = true;
		if (proj->pan_right) {
		    track_increment_pan(trk);
		} else {
		    track_decrement_pan(trk);
		}
	    }
	}
    }
    if (!had_active_track) {
	if (proj->vol_changing) {
	    if (proj->vol_up) {
		track_increment_vol(selected_track);
	    } else {
		track_decrement_vol(selected_track);
	    }
	    selected_track->vol_ctrl->editing = true;
	}
	if (proj->pan_changing) {
	    if (proj->pan_right) {
		track_increment_pan(selected_track);
	    } else {
		track_decrement_pan(selected_track);
	    }
	    selected_track->pan_ctrl->editing = true;
	}
    }
}

/* TODO: SDL bug workaround. I haven't been able to get this to work reliably cross-platform. */
/* https://discourse.libsdl.org/t/window-event-at-initiation-of-window-resize/50963/3 */
/* static int SDLCALL window_resize_callback(void *userdata, SDL_Event *event) */
/* { */
/*     if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_EXPOSED) { */
/* 	window_resize_passive(main_win, event->window.data1, event->window.data2); */
/* 	project_draw(); */
/*     } */
/*     return 0; */
/* } */

/* Modal *test_modal; */

extern SDL_Color color_global_grey;

//typedef void (SliderStrFn)(char *dst, size_t dstsize, void *value, ValType type);

static int rb_target_action(void *self_v, void *target)
{
    RadioButton *self = (RadioButton *)self_v;
    FIRFilter *f = (FIRFilter *)target;
    /* double cutoff = f->cutoff_freq; */
    FilterType t = (FilterType)(self->selected_item);
    /* fprintf(stdout, "SET FIR FILTER PARAMS %p\n", f); */
    set_FIR_filter_type(f, t);
    return 0;
    
}

static int slider_target_action(void *self_v, void *target)
{
    Slider *self = (Slider *)self_v;
    FIRFilter *f = (FIRFilter *)target;
    /* double cutoff = f->cutoff_freq; */
    double cutoff_unscaled = *(double *)(self->value);
    /* double cutoff_scaled = log(1.0 + cutoff_unscaled) / log(2.0f); */
    double cutoff_h = pow(10.0, log10((double)proj->sample_rate / 2) * cutoff_unscaled);
    /* /\* double cutoff_h = log(0.0001 + cutoff_unscaled) / log(1.0001f) * proj->sample_rate / 2; *\/ */
    /* fprintf(stdout, "unscaled: %f, %d\n", cutoff_unscaled, (int)cutoff_h); */
    /* FilterType t = f->type; */
    /* set_FIR_filter_params_h(f, t, cutoff_h, 500); */
    set_FIR_filter_cutoff_h(f, cutoff_h);
    return 0;
}

static int slider_bandwidth_target_action(void *self_v, void *target)
{
    Slider *self = (Slider *)self_v;
    FIRFilter *f = (FIRFilter *)target;
    double bandwidth_unscaled = *(double *)(self->value);
    double bandwidth_h = pow(10.0, log10((double)proj->sample_rate / 2) * bandwidth_unscaled);
    /* /\* double cutoff_h = log(0.0001 + cutoff_unscaled) / log(1.0001f) * proj->sample_rate / 2; *\/ */
    /* fprintf(stdout, "unscaled: %f, %d\n", cutoff_unscaled, (int)cutoff_h); */
    /* FilterType t = f->type; */
    /* set_FIR_filter_params_h(f, t, cutoff_h, 500); */
    set_FIR_filter_bandwidth_h(f, bandwidth_h);
    return 0;
}

static int slider_irlen_target_action(void *self_v, void *target)
{
    Slider *self = (Slider *)self_v;
    FIRFilter *f = (FIRFilter *)target;
    int val = *(int *)self->value;
    set_FIR_filter_impulse_response_len(f, val);

    if (proj->freq_domain_plot) waveform_destroy_freq_plot(proj->freq_domain_plot);

    Layout *freq_lt = layout_add_child(proj->layout);
    freq_lt->rect.w = 600 * main_win->dpi_scale_factor;
    freq_lt->rect.h = 300 * main_win->dpi_scale_factor;
    layout_set_values_from_rect(freq_lt);
    freq_lt->y.type = REVREL;
    freq_lt->y.value.intval = 0;
    layout_reset(freq_lt);
    layout_center_agnostic(freq_lt, true, false);

    double *arrays[] = {proj->output_L_freq, proj->output_R_freq, proj->timelines[0]->tracks[0]->fir_filter->frequency_response_mag};
    SDL_Color *colors[] = {&color_global_white, &color_global_white, &color_global_white};
    int steps[] = {1, 1, 2};
    proj->freq_domain_plot = waveform_create_freq_plot(arrays, 3, colors, steps, proj->fourier_len_sframes / 2, freq_lt);

    /* proj->freq_domain_plot->plots[2] = proj->timelines[0]->tracks[0]->fir_filter->frequency_response_mag; */
    return 0;
}

static void test_tabview()
{
    if (main_win->active_tab_view) {
	/* tabview_destroy(main_win->active_tab_view); */
	main_win->active_tab_view = NULL;
	return;
    }

    TabView *tv = tab_view_create("Some view", proj->layout, main_win);

    static SDL_Color page_colors[] = {
	{30, 80, 80, 255},
	{70, 40, 70, 255},
	{40, 40, 80, 255}
    };
    Page *page = tab_view_add_page(
	tv,
	"Track FIR Filter",
	INSTALL_DIR "/assets/layouts/some_name.xml",
	page_colors,
	main_win);

    tab_view_add_page(
	tv,
	"EQ",
	INSTALL_DIR "/assets/layouts/some_name.xml",
	page_colors + 1,
	main_win);
    tab_view_add_page(
	tv,
	"This is another tab",
	INSTALL_DIR "/assets/layouts/some_name.xml",
	page_colors + 2,
	main_win);

        
    PageElParams p;
    p.textbox_p.font = main_win->std_font;
    p.textbox_p.text_size = 12;
    p.textbox_p.set_str = "Bandwidth:";
    p.textbox_p.win = main_win;
    page_add_el(page, EL_TEXTBOX, p, "bandwidth_label");

    p.textbox_p.set_str = "Cutoff frequency:";
    p.textbox_p.win = main_win;
    PageEl* el = page_add_el(page, EL_TEXTBOX, p, "cutoff_label");


    
    static double freq_unscaled = 0;
    p.slider_p.create_label_fn = NULL;
    p.slider_p.style = SLIDER_FILL;
    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.value = &freq_unscaled;
    p.slider_p.val_type = JDAW_DOUBLE;
    p.slider_p.action = slider_target_action;
    p.slider_p.target = (void *)(proj->timelines[0]->tracks[0]->fir_filter);
    el = page_add_el(page, EL_SLIDER, p, "cutoff_slider");


    static double bandwidth_unscaled = 0;
    p.slider_p.action = slider_bandwidth_target_action;
    p.slider_p.target = (void *)(proj->timelines[0]->tracks[0]->fir_filter);
    p.slider_p.value = &bandwidth_unscaled;
    el = page_add_el(page, EL_SLIDER, p, "bandwidth_slider");

    static int ir_len = 20;
    p.slider_p.action = slider_irlen_target_action;
    p.slider_p.target = (void *)(proj->timelines[0]->tracks[0]->fir_filter);
    p.slider_p.value = &ir_len;
    p.slider_p.val_type = JDAW_INT;
    el = page_add_el(page, EL_SLIDER, p, "slider_ir_len");

    Slider *sl = (Slider *)el->component;
    Value min, max;
    min.int_v = 4;
    max.int_v = proj->fourier_len_sframes;
    slider_set_range(sl, min, max);
    

    /* 	int text_size; */
    /* 	SDL_Color *text_color; */
    /* 	void *target_enum; */
    /* 	void (*external_action)(void *); */
    /* 	const char **item_names; */
    /* 	uint8_t num_items; */
    /* }; */
    

    static const char * item_names[] = {
	"Lowpass",
	"Highpass",
	"Bandpass",
	"Bandcut"

    };

    p.radio_p.text_size = 14;
    p.radio_p.text_color = &color_global_black;
    /* p.radio_p.target_enum = NULL; */
    p.radio_p.action = rb_target_action;
    p.radio_p.item_names = item_names;
    p.radio_p.num_items = 4;
    p.radio_p.target = proj->timelines[0]->tracks[0]->fir_filter;
    
    el = page_add_el(page, EL_RADIO, p, "radio1");

    layout_force_reset(tv->layout);
    tab_view_activate(tv);
    FILE *f = fopen("test.xml", "w");
    layout_write(f, tv->layout, 0);
    fclose(f);
}

static void test_page_create()
{
    if (main_win->active_page) {
	page_close(main_win->active_page);
	return;
    }

    Layout *page_layout = layout_add_child(main_win->layout);
    /* layout_set_default_dims(page_layout); */
    page_layout->w.type = SCALE;
    page_layout->h.type = SCALE;
    page_layout->x.type = REL;
    page_layout->y.type = REL;
    page_layout->w.value.floatval = 0.8;
    page_layout->h.value.floatval = 0.8;
    layout_center_agnostic(page_layout, true, true);
    Page *page = page_create(
	"This is a page",
	INSTALL_DIR "/assets/layouts/some_name.xml",
	page_layout,
	&color_global_grey,
	main_win);

    
    PageElParams p;
    p.textbox_p.font = main_win->std_font;
    p.textbox_p.text_size = 12;
    p.textbox_p.set_str = "Bandwidth:";
    p.textbox_p.win = main_win;
    page_add_el(page, EL_TEXTBOX, p, "bandwidth_label");

    p.textbox_p.set_str = "Cutoff frequency:";
    p.textbox_p.win = main_win;
    PageEl* el = page_add_el(page, EL_TEXTBOX, p, "cutoff_label");


    
    static double freq_unscaled = 0;
    p.slider_p.create_label_fn = NULL;
    p.slider_p.style = SLIDER_FILL;
    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.value = &freq_unscaled;
    p.slider_p.val_type = JDAW_DOUBLE;
    p.slider_p.action = slider_target_action;
    p.slider_p.target = (void *)(proj->timelines[0]->tracks[0]->fir_filter);
    el = page_add_el(page, EL_SLIDER, p, "cutoff_slider");


    static double bandwidth_unscaled = 0;
    p.slider_p.action = slider_bandwidth_target_action;
    p.slider_p.target = (void *)(proj->timelines[0]->tracks[0]->fir_filter);
    p.slider_p.value = &bandwidth_unscaled;
    el = page_add_el(page, EL_SLIDER, p, "bandwidth_slider");

    static int ir_len = 20;
    p.slider_p.action = slider_irlen_target_action;
    p.slider_p.target = (void *)(proj->timelines[0]->tracks[0]->fir_filter);
    p.slider_p.value = &ir_len;
    p.slider_p.val_type = JDAW_INT;
    el = page_add_el(page, EL_SLIDER, p, "slider_ir_len");

    Slider *sl = (Slider *)el->component;
    Value min, max;
    min.int_v = 4;
    max.int_v = proj->fourier_len_sframes;
    slider_set_range(sl, min, max);
    

    /* 	int text_size; */
    /* 	SDL_Color *text_color; */
    /* 	void *target_enum; */
    /* 	void (*external_action)(void *); */
    /* 	const char **item_names; */
    /* 	uint8_t num_items; */
    /* }; */
    

    static const char * item_names[] = {
	"Lowpass",
	"Highpass",
	"Bandpass",
	"Bandcut"

    };

    p.radio_p.text_size = 14;
    p.radio_p.text_color = &color_global_black;
    /* p.radio_p.target_enum = NULL; */
    p.radio_p.action = rb_target_action;
    p.radio_p.item_names = item_names;
    p.radio_p.num_items = 4;
    p.radio_p.target = proj->timelines[0]->tracks[0]->fir_filter;
    
    el = page_add_el(page, EL_RADIO, p, "radio1");

    
    page_activate(page);
    
}

void loop_project_main()
{

    /* audioconn_close(proj->playback_conn); */
    while (1) {
    }

}
