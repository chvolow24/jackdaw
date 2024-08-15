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
#include "textbox.h"
#include "userfn.h"
#include "waveform.h"

#ifndef INSTALL_DIR
#define INSTALL_DIR "."
#endif

#define LAYOUT_DIR INSTALL_DIR "/assets/layouts/"
#define FIR_FILTER_LT_PATH LAYOUT_DIR "track_settings_fir_filter.xml"
#define DELAY_LINE_LT_PATH LAYOUT_DIR "track_settings_delay_line.xml"

#define LABEL_STD_FONT_SIZE 12
#define RADIO_STD_FONT_SIZE 14

extern Project *proj;
extern Window *main_win;

extern SDL_Color color_global_white;
extern SDL_Color color_global_black;
extern SDL_Color color_global_light_grey;
extern SDL_Color freq_L_color;
extern SDL_Color freq_R_color;

static struct freq_plot *current_fp;
Waveform *current_waveform;

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

static int slider_cutoff_target_action(void *self_v, void *target)
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
    
    /* slider_reset(self); */
    return 0;
}

static int toggle_delay_line_target_action(void *self_v, void *target)
{
    DelayLine *dl = (DelayLine *)target;
    delay_line_clear(dl);
    return 0;
}

static int slider_deltime_target_action(void *self_v, void *target)
{
    DelayLine *dl = (DelayLine *)target;
    Slider *self = (Slider *)self_v;
    int32_t val_msec = *(int32_t *)self->value;
    int32_t len_sframes = (int32_t)((double)val_msec * proj->sample_rate / 1000);
    delay_line_set_params(dl, dl->amp, len_sframes);
    /* slider_reset(self); */
    return 0;
}

static int delay_set_offset(void *self_v, void *target)
{
    DelayLine *dl = (DelayLine *)target;
    double offset_prop = *((double *)((Slider *)self_v)->value);
    int32_t offset = offset_prop * dl->len;
    SDL_LockMutex(dl->lock);
    dl->stereo_offset = offset;
    SDL_UnlockMutex(dl->lock);
    return 0;
}

/* typedef void (SliderStrFn)(char *dst, size_t dstsize, void *value, ValType type); */
static void create_hz_label(char *dst, size_t dstsize, void *value, ValType type)
{
    double raw = *(double *)value;
    double hz = pow(10.0, log10((double)proj->sample_rate / 2) * raw);
    snprintf(dst, dstsize, "%.0f Hz", hz);
    /* snprintf(dst, dstsize, "hi"); */
    dst[dstsize - 1] = '\0';
}

static void create_msec_label(char *dst, size_t dstsize, void *value, ValType type)
{
    int32_t msec = *(int32_t *)value;
    snprintf(dst, dstsize, "%d ms", msec);
}

static int previous_track(void *self_v, void *target)
{
    user_tl_track_selector_up(NULL);
    return 0;
}

static int next_track(void *self_v, void *target)
{
    user_tl_track_selector_down(NULL);
    return 0;
}

static void create_track_selection_area(Page *page, Track *track)
{
    PageElParams p;
    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.win = main_win;
    p.textbox_p.text_size = 16;
    p.textbox_p.set_str = track->name;
    PageEl *el = page_add_el(page, EL_TEXTBOX, p, "track_name");
    Textbox *tb = el->component;
    textbox_set_align(tb, CENTER_LEFT);
    textbox_size_to_fit_h(tb, 5);
    /* layout_reset(tb->layout->parent); */
    textbox_reset_full(tb);

    p.button_p.set_str = "p↑";
    p.button_p.font = main_win->mono_bold_font;
    p.button_p.win = main_win;
    p.button_p.text_size = 16;
    p.button_p.background_color = &color_global_light_grey;
    p.button_p.text_color = &color_global_black;
    p.button_p.action = previous_track;
    el = page_add_el(page, EL_BUTTON, p, "track_previous");

    p.button_p.set_str = "n↓";
    p.button_p.action = next_track;
    el = page_add_el(page, EL_BUTTON, p, "track_next");
}

void settings_track_tabview_set_track(TabView *tv, Track *track)
{

    FIRFilter *f = track->fir_filter;
    for (int i=0; i<tv->num_tabs; i++) {
	page_destroy(tv->tabs[i]);
	textbox_destroy(tv->labels[i]);
    }
    tv->num_tabs = 0;
    
    /* static SDL_Color page_colors[] = { */
    /* 	{30, 80, 80, 255}, */
    /* 	{40, 40, 80, 255}, */
    /* 	{70, 40, 70, 255} */
    /* }; */

    static SDL_Color page_colors[] = {
	{30, 80, 80, 255},
	{50, 50, 80, 255},
	{70, 40, 70, 255}
    };

    Page *page = tab_view_add_page(
	tv,
	"FIR Filter",
	FIR_FILTER_LT_PATH,
	page_colors,
	&color_global_white,
	main_win);

    PageElParams p;

    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = LABEL_STD_FONT_SIZE;
    p.textbox_p.set_str = "Bandwidth:";
    p.textbox_p.win = page->win;
    PageEl *el = page_add_el(page, EL_TEXTBOX, p, "bandwidth_label");

    Textbox *tb = el->component;
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Cutoff / center frequency:";
    p.textbox_p.win = main_win;
    el = page_add_el(page, EL_TEXTBOX, p, "cutoff_label");
    tb = el->component;
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);
    
    p.textbox_p.set_str = "Impulse response length (\"sharpness\")";
    el = page_add_el(page, EL_TEXTBOX, p, "irlen_label");
    tb=el->component;
    textbox_set_trunc(tb, false);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Enable FIR filter";
    el = page_add_el(page, EL_TEXTBOX, p, "toggle_label");
    tb=el->component;
    textbox_set_trunc(tb, false);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.toggle_p.value = &track->fir_filter_active;
    p.toggle_p.target = NULL;
    p.toggle_p.action = NULL;
    page_add_el(page, EL_TOGGLE, p, "toggle_filter_on");

    
    static double freq_unscaled;
    freq_unscaled = unscale_freq(f->cutoff_freq);
    p.slider_p.create_label_fn = create_hz_label;
    p.slider_p.style = SLIDER_TICK;
    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.value = &freq_unscaled;
    p.slider_p.val_type = JDAW_DOUBLE;
    p.slider_p.action = slider_cutoff_target_action;
    p.slider_p.target = (void *)(f);
    el = page_add_el(page, EL_SLIDER, p, "cutoff_slider");


    static double bandwidth_unscaled;
    bandwidth_unscaled = unscale_freq(f->bandwidth);
    p.slider_p.action = slider_bandwidth_target_action;
    p.slider_p.target = (void *)(f);
    p.slider_p.value = &bandwidth_unscaled;
    el = page_add_el(page, EL_SLIDER, p, "bandwidth_slider");

    static int ir_len = 20;
    if (track->fir_filter_active) ir_len = f->impulse_response_len;
    p.slider_p.action = slider_irlen_target_action;
    /* p.slider_p.target = */
    p.slider_p.target = (void *)f;
    p.slider_p.value = &ir_len;
    p.slider_p.val_type = JDAW_INT;
    p.slider_p.create_label_fn = slider_std_labelmaker;
    el = page_add_el(page, EL_SLIDER, p, "irlen_slider");

    
    Slider *sl = (Slider *)el->component;
    Value min, max;
    min.int_v = 4;
    max.int_v = proj->fourier_len_sframes;
    slider_set_range(sl, min, max);
    slider_reset(sl);
    

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

    p.radio_p.text_size = RADIO_STD_FONT_SIZE;
    p.radio_p.text_color = &color_global_white;
    /* p.radio_p.target_enum = NULL; */
    p.radio_p.action = rb_target_action;
    p.radio_p.item_names = item_names;
    p.radio_p.num_items = 4;
    p.radio_p.target = f;
    
    el = page_add_el(page, EL_RADIO, p, "filter_type");
    RadioButton *radio = el->component;
    radio->selected_item = (uint8_t)f->type;

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
    SDL_Color *plot_colors[] = {&freq_L_color, &freq_R_color, &color_global_white};
    p.freqplot_p.arrays = arrays;
    p.freqplot_p.colors =  plot_colors;
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

    create_track_selection_area(page, track);


    page = tab_view_add_page(
	tv,
	"Delay line",
	DELAY_LINE_LT_PATH,
	page_colors + 1,
	&color_global_white,
	main_win);

    /* create_track_selection_area(page, track); */

    p.toggle_p.value = &track->delay_line_active;
    p.toggle_p.action = toggle_delay_line_target_action;
    p.toggle_p.target = (void *)(&track->delay_line);
    el = page_add_el(page, EL_TOGGLE, p, "toggle_delay");

    p.textbox_p.set_str = "Delay line on";
    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = LABEL_STD_FONT_SIZE;
    p.textbox_p.win = main_win;
    tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "toggle_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Delay time (ms)";
    tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "del_time_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);
    
    p.textbox_p.set_str = "Delay amplitude";
    tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "del_amp_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Stereo offset";
    tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "stereo_offset_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);


    static int32_t delay_msec;
    delay_msec = (int32_t)((double)track->delay_line.len / proj->sample_rate * 1000.0);
    
    p.slider_p.create_label_fn = NULL;
    p.slider_p.style = SLIDER_TICK;
    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.value = &delay_msec;
    p.slider_p.val_type = JDAW_INT32;
    p.slider_p.action = slider_deltime_target_action;
    p.slider_p.target = (void *)(&track->delay_line);
    p.slider_p.create_label_fn = create_msec_label;
    el = page_add_el(page, EL_SLIDER, p, "del_time_slider");
    min.int32_v = 1;
    max.int32_v = 1000;
    slider_set_range((Slider *)el->component, min, max);
    slider_reset((Slider *)el->component);
    
    p.slider_p.value = &track->delay_line.amp;
    p.slider_p.val_type = JDAW_DOUBLE;
    p.slider_p.create_label_fn = slider_std_labelmaker;
    p.slider_p.action = NULL;
    p.slider_p.target = NULL;
    el = page_add_el(page, EL_SLIDER, p, "del_amp_slider");

    static double offset;
    offset = (double)abs(track->delay_line.stereo_offset) / track->delay_line.len;
    p.slider_p.value = &offset;
    p.slider_p.val_type = JDAW_DOUBLE;
    p.slider_p.action = delay_set_offset;
    p.slider_p.target = &track->delay_line;
    el = page_add_el(page, EL_SLIDER, p, "stereo_offset_slider");
    slider_reset(el->component);

    /* p.waveform_p.len = track->delay_line.len; */
    /* p.waveform_p.num_channels = 2; */
    /* void *channels[2] = {(void *)&(track->delay_line.buf_L), (void *)&(track->delay_line.buf_R)}; */
    /* p.waveform_p.channels =channels; */
    /* p.waveform_p.background_color = &color_global_black; */
    /* p.waveform_p.plot_color = &color_global_white; */
    /* p.waveform_p.type = JDAW_DOUBLE; */

    /* el = page_add_el(page, EL_WAVEFORM, p, "waveform"); */
    /* current_waveform = el->component; */


    create_track_selection_area(page, track);

	
    page = tab_view_add_page(
	tv,
	"EQ (coming soon)",
	DELAY_LINE_LT_PATH,
	page_colors + 2,
	&color_global_white,
	main_win);
    
    
    
}

TabView *settings_track_tabview_create(Track *track)
{

    TabView *tv = tab_view_create("Track Settings", proj->layout, main_win);

    settings_track_tabview_set_track(tv, track);
 
    layout_force_reset(tv->layout);
    return tv;
 
}
