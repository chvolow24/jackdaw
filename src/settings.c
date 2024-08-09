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
    settings.c

    * create structs required for settings pages
    * swap out params for settings pages
 *****************************************************************************************************************/


#include "dsp.h"
#include "page.h"
#include "project.h"
#include "waveform.h"

#ifndef INSTALL_DIR
#define INSTALL_DIR "."
#endif

#define LAYOUT_DIR INSTALL_DIR "/assets/layouts/"
#define FIR_FILTER_LT_PATH LAYOUT_DIR "some_name.xml"

extern Project *proj;
extern Window *main_win;

extern SDL_Color color_global_white;
extern SDL_Color freq_L_color;
extern SDL_Color freq_R_color;



static double unscale_freq(double scaled)
{
    return log(scaled * proj->sample_rate) / log(proj->sample_rate);
}

static int rb_target_action(void *self_v, void *target)
{
    RadioButton *self = (RadioButton *)self_v;
    FIRFilter *f = (FIRFilter *)target;
    /* double cutoff = f->cutoff_freq; */
    FilterType t = (FilterType)(self->selected_item);
    /* fprintf(stdout, "SET FIR FILTER PARAMS %p\n", f); */
    filter_set_type(f, t);
    return 0;
}

static int slider_target_action(void *self_v, void *target)
{
    Slider *self = (Slider *)self_v;
    FIRFilter *f = (FIRFilter *)target;
    double cutoff_unscaled = *(double *)(self->value);
    double cutoff_h = pow(10.0, log10((double)proj->sample_rate / 2) * cutoff_unscaled);
    filter_set_cutoff_hz(f, cutoff_h);
    return 0;
}

static int slider_bandwidth_target_action(void *self_v, void *target)
{
    Slider *self = (Slider *)self_v;
    FIRFilter *f = (FIRFilter *)target;
    double bandwidth_unscaled = *(double *)(self->value);
    double bandwidth_h = pow(10.0, log10((double)proj->sample_rate / 2) * bandwidth_unscaled);
    filter_set_bandwidth_hz(f, bandwidth_h);
    return 0;
}

static struct freq_plot *current_fp;
static int slider_irlen_target_action(void *self_v, void *target)
{
    Slider *self = (Slider *)self_v;
    FIRFilter *f = (FIRFilter *)target;
    int val = *(int *)self->value;
    filter_set_impulse_response_len(f, val);


    /* Need to reset arrays in freq plot */
    /* TODO: find a better way to do this */
    current_fp->arrays[2] = f->frequency_response_mag;
    waveform_reset_freq_plot(current_fp);

    
    slider_reset(self);
    return 0;
}

void settings_track_tabview_set_track(TabView *tv, Track *track)
{
    for (int i=0; i<tv->num_tabs; i++) {
	page_destroy(tv->tabs[i]);
	textbox_destroy(tv->labels[i]);
    }
    tv->num_tabs = 0;
    
    static SDL_Color page_colors[] = {
	{30, 80, 80, 255},
	{70, 40, 70, 255},
	{40, 40, 80, 255}
    };
    
   Page *page = tab_view_add_page(
	tv,
	"FIR Filter",
	FIR_FILTER_LT_PATH,
	page_colors,
	&color_global_white,
	main_win);

    /* tab_view_add_page( */
    /* 	tv, */
    /* 	"[EQ]", */
    /* 	INSTALL_DIR "/assets/layouts/some_name.xml", */
    /* 	page_colors + 1, */
    /* 	&color_global_white, */
    /* 	main_win); */
    /* tab_view_add_page( */
    /* 	tv, */
    /* 	"[This is another tab]", */
    /* 	INSTALL_DIR "/assets/layouts/some_name.xml", */
    /* 	page_colors + 2, */
    /* 	&color_global_white, */
    /* 	main_win); */

        
    PageElParams p;
    p.textbox_p.font = main_win->std_font;
    p.textbox_p.text_size = 12;
    p.textbox_p.set_str = "Bandwidth:";
    p.textbox_p.win = main_win;
    PageEl *el = page_add_el(page, EL_TEXTBOX, p, "bandwidth_label");

    Textbox *tb = el->component;
    textbox_set_align(tb, CENTER_LEFT);
    
    p.textbox_p.set_str = "Cutoff frequency:";
    p.textbox_p.win = main_win;
    el = page_add_el(page, EL_TEXTBOX, p, "cutoff_label");
    tb = el->component;
    textbox_set_align(tb, CENTER_LEFT);
    
    p.textbox_p.set_str = "Impulse response length (\"sharpness\")";
    el = page_add_el(page, EL_TEXTBOX, p, "irlen_label");
    tb=el->component;
    textbox_set_trunc(tb, false);
    textbox_set_align(tb, CENTER_LEFT);

    
    static double freq_unscaled;
    freq_unscaled = unscale_freq(track->fir_filter->cutoff_freq);
    p.slider_p.create_label_fn = NULL;
    p.slider_p.style = SLIDER_TICK;
    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.value = &freq_unscaled;
    p.slider_p.val_type = JDAW_DOUBLE;
    p.slider_p.action = slider_target_action;
    p.slider_p.target = (void *)(track->fir_filter);
    el = page_add_el(page, EL_SLIDER, p, "cutoff_slider");


    static double bandwidth_unscaled;
    bandwidth_unscaled = unscale_freq(track->fir_filter->bandwidth);
    p.slider_p.action = slider_bandwidth_target_action;
    p.slider_p.target = (void *)(track->fir_filter);
    p.slider_p.value = &bandwidth_unscaled;
    el = page_add_el(page, EL_SLIDER, p, "bandwidth_slider");


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
    p.radio_p.text_color = &color_global_white;
    /* p.radio_p.target_enum = NULL; */
    p.radio_p.action = rb_target_action;
    p.radio_p.item_names = item_names;
    p.radio_p.num_items = 4;
    p.radio_p.target = track->fir_filter;
    
    el = page_add_el(page, EL_RADIO, p, "radio1");
    RadioButton *radio = el->component;
    radio->selected_item = (uint8_t)track->fir_filter->type;

    FIRFilter *f = track->fir_filter;

    if (!track->buf_L_freq_mag) track->buf_L_freq_mag = calloc(f->frequency_response_len, sizeof(double));
    if (!track->buf_R_freq_mag) track->buf_R_freq_mag = calloc(f->frequency_response_len, sizeof(double));
    double *arrays[3] = {
	track->buf_L_freq_mag,
	track->buf_R_freq_mag,
	/* proj->output_L_freq, */
	/* proj->output_R_freq, */
	f->frequency_response_mag
    };
	
    int steps[] = {1, 1, 1};
    SDL_Color *colors[] = {&freq_L_color, &freq_R_color, &color_global_white};
    p.freqplot_p.arrays = arrays;
    p.freqplot_p.colors =  colors;
    p.freqplot_p.steps = steps;
    p.freqplot_p.num_items = f->frequency_response_len / 2;
    p.freqplot_p.num_arrays = 3;
    /* p.freqplot_p. */
    el = page_add_el(page, EL_FREQ_PLOT, p, "freq_plot");


    /* TODO:
       This freq plot needs to be accessible to the slider's action
       so it can reset the array pointer when the FIR filter's freq
       response buffer is reallocated. Find a better way to do this
       than storing in a global var, maybe. */
    current_fp = el->component;

    
    static int ir_len = 20;
    if (track->fir_filter_active) ir_len = f->impulse_response_len;
    p.slider_p.action = slider_irlen_target_action;
    p.slider_p.style = SLIDER_TICK;
    /* p.slider_p.target = */
    p.slider_p.target = (void *)f;
    p.slider_p.value = &ir_len;
    p.slider_p.val_type = JDAW_INT;
    p.slider_p.create_label_fn = NULL;
    p.slider_p.orientation = SLIDER_HORIZONTAL;
    el = page_add_el(page, EL_SLIDER, p, "slider_ir_len");

    Slider *sl = (Slider *)el->component;
    Value min, max;
    min.int_v = 4;
    max.int_v = proj->fourier_len_sframes;
    slider_set_range(sl, min, max);
    slider_reset(sl);
    
    
}

TabView *settings_track_tabview_create(Track *track)
{

    TabView *tv = tab_view_create("Track Settings", proj->layout, main_win);

    settings_track_tabview_set_track(tv, track);
 
    layout_force_reset(tv->layout);
    return tv;
 
}
