/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    settings.c

    * create structs required for settings pages
    * swap out params for settings pages
 *****************************************************************************************************************/

#include <pthread.h>
#include "assets.h"
#include "dsp.h"
#include "eq.h"
#include "label.h"
#include "page.h"
#include "project.h"
#include "textbox.h"
#include "timeline.h"
#include "userfn.h"
#include "waveform.h"

#ifndef INSTALL_DIR
#define INSTALL_DIR "."
#endif

#define LABEL_STD_FONT_SIZE 12
#define RADIO_STD_FONT_SIZE 14

extern Project *proj;
extern Window *main_win;

extern SDL_Color color_global_white;
extern SDL_Color color_global_black;
extern SDL_Color color_global_light_grey;
extern SDL_Color color_global_quickref_button_blue;
extern SDL_Color freq_L_color;
extern SDL_Color freq_R_color;

/* static struct freq_plot *current_fp; */
Waveform *current_waveform;

static double unscale_freq(double scaled)
{
    return log(scaled * proj->sample_rate) / log(proj->sample_rate);
}
static int toggle_delay_line_target_action(void *self_v, void *target)
{
    DelayLine *dl = (DelayLine *)target;
    delay_line_clear(dl);
    return 0;
}

static int toggle_saturation_gain_comp(void *self_v, void *target)
{
    Saturation *s = (Saturation *)target;
    /* fprintf(stderr, "WRITING TYPE: %d\n", s->type); */
    endpoint_write(&s->gain_comp_ep, (Value){.bool_v = s->do_gain_comp}, true, true, true, true);
    return 0;

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
    PageEl *el = page_add_el(page, EL_TEXTBOX, p, "track_settings_name_tb", "track_name");
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
    el = page_add_el(page, EL_BUTTON, p, "track_settings_prev_track", "track_previous");

    p.button_p.set_str = "n↓";
    p.button_p.action = next_track;
    el = page_add_el(page, EL_BUTTON, p, "track_settings_next_track", "track_next");
}

void settings_track_tabview_set_track(TabView *tv, Track *track)
{
    /* if (!track->fir_filter.frequency_response) { */
    /* 	Project *proj_loc = track->tl->proj; */
    /* 	int ir_len = proj_loc->fourier_len_sframes/4; */
    /* 	if (!track->fir_filter.initialized) */
    /* 	    filter_init(&track->fir_filter, track, LOWPASS, ir_len, proj_loc->fourier_len_sframes * 2); */
    /* 	track->fir_filter_active = false; */
    /* } */
    /* if (!track->delay_line.buf_L) { */
    /* 	delay_line_init(&track->delay_line, track, proj->sample_rate); */
    /* 	/\* delay_line_set_params(&track->delay_line, 0.3, 10000); *\/ */
    /* } */

    FIRFilter *f = &track->fir_filter;

    tabview_clear_all_contents(tv);

    static SDL_Color page_colors[] = {
	{30, 80, 80, 255},
	{50, 50, 80, 255},
	/* {70, 40, 70, 255} */
	{43, 43, 55, 255},
	{100, 40, 40, 255},
    };

    Page *page = tab_view_add_page(
	tv,
	"Equalizer",
	EQ_LT_PATH,
	page_colors + 2,
	&color_global_white,
	main_win);


    PageElParams p;
    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = LABEL_STD_FONT_SIZE;
    p.textbox_p.win = main_win;

    p.textbox_p.set_str = "Enable EQ";
    PageEl *el = page_add_el(page, EL_TEXTBOX, p, "track_settings_eq_toggle_label", "toggle_label");
    Textbox *tb = el->component;
    textbox_set_trunc(tb, false);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);


    p.toggle_p.value = &track->eq.active;
    p.toggle_p.target = NULL;
    p.toggle_p.action = NULL;
    page_add_el(page, EL_TOGGLE, p, "track_settings_eq_toggle", "toggle_eq_on");
    

    p.eq_plot_p.eq = &track->eq;
    page_add_el(page, EL_EQ_PLOT, p, "track_settings_eq_plot", "eq_plot");

    p.textarea_p.font = main_win->mono_bold_font;
    p.textarea_p.color = color_global_white;
    p.textarea_p.text_size = 12;
    p.textarea_p.win = main_win;
    p.textarea_p.value = "Click and drag the circles to set peaks or notches.\n \nHold cmd or ctrl and drag up or down to set the filter bandwidth.\n \nAdditional filter types (shelving, lowpass, highpass) will be added in future versions of jackdaw.";
    page_add_el(page, EL_TEXTAREA, p, "track_settings_eq_desc", "description");

    create_track_selection_area(page, track);


    page = tab_view_add_page(
	tv,
	"FIR Filter",
	FIR_FILTER_LT_PATH,
	page_colors,
	&color_global_white,
	main_win);

    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = LABEL_STD_FONT_SIZE;
    p.textbox_p.set_str = "Bandwidth:";
    p.textbox_p.win = page->win;
    el = page_add_el(page, EL_TEXTBOX, p, "track_settings_filter_bandwidth_label", "bandwidth_label");

    tb = el->component;
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Cutoff / center frequency:";
    p.textbox_p.win = main_win;
    el = page_add_el(page, EL_TEXTBOX, p, "track_settings_filter_cutoff_label",  "cutoff_label");
    tb = el->component;
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);
    
    p.textbox_p.set_str = "Impulse response length (\"sharpness\")";
    el = page_add_el(page, EL_TEXTBOX, p, "track_settings_filter_irlen_label", "irlen_label");
    tb=el->component;
    textbox_set_trunc(tb, false);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Enable FIR filter";
    el = page_add_el(page, EL_TEXTBOX, p, "track_settings_filter_toggle_label", "toggle_label");
    tb=el->component;
    textbox_set_trunc(tb, false);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.toggle_p.value = &track->fir_filter.active;
    p.toggle_p.target = NULL;
    p.toggle_p.action = NULL;
    page_add_el(page, EL_TOGGLE, p, "track_settings_filter_toggle", "toggle_filter_on");
    
    f->cutoff_freq_unscaled = unscale_freq(f->cutoff_freq);
    p.slider_p.create_label_fn = label_freq_raw_to_hz;
    p.slider_p.style = SLIDER_TICK;
    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.min = (Value){.double_v = 0.0};
    p.slider_p.max = (Value){.double_v = 1.0};
    p.slider_p.ep = &track->fir_filter.cutoff_ep;
    el = page_add_el(page, EL_SLIDER, p, "track_settings_filter_cutoff_slider", "cutoff_slider");

    f->bandwidth_unscaled = unscale_freq(f->bandwidth);
    p.slider_p.ep = &track->fir_filter.bandwidth_ep;
    el = page_add_el(page, EL_SLIDER, p, "track_settings_filter_bandwidth_slider", "bandwidth_slider");

    p.slider_p.ep = &track->fir_filter.impulse_response_len_ep;

    p.slider_p.min = (Value){.int_v = 4};
    p.slider_p.max = (Value){.int_v = proj->fourier_len_sframes};
    p.slider_p.create_label_fn = NULL;
    el = page_add_el(page, EL_SLIDER, p, "track_settings_filter_irlen_slider",  "irlen_slider");    
    Slider *sl = (Slider *)el->component;
    sl->disallow_unsafe_mode = true;
    slider_reset(sl);

    static const char * item_names[] = {
	"Lowpass",
	"Highpass",
	"Bandpass",
	"Bandcut"

    };
    
    p.radio_p.text_size = RADIO_STD_FONT_SIZE;
    p.radio_p.text_color = &color_global_white;
    p.radio_p.ep = &f->type_ep;
    p.radio_p.item_names = item_names;
    p.radio_p.num_items = 4;
    
    el = page_add_el(page, EL_RADIO, p, "track_settings_filter_type_radio", "filter_type");
    RadioButton *radio = el->component;
    radio->selected_item = (uint8_t)f->type;

    if (!track->buf_L_freq_mag) track->buf_L_freq_mag = calloc(f->frequency_response_len, sizeof(double));
    if (!track->buf_R_freq_mag) track->buf_R_freq_mag = calloc(f->frequency_response_len, sizeof(double));
    
    double *arrays[3] = {
	track->buf_L_freq_mag,
	track->buf_R_freq_mag,
	f->frequency_response_mag
    };	
    int steps[] = {1, 1, 1};
    SDL_Color *plot_colors[] = {&freq_L_color, &freq_R_color, &color_global_white};
    p.freqplot_p.arrays = arrays;
    p.freqplot_p.colors =  plot_colors;
    p.freqplot_p.steps = steps;
    p.freqplot_p.num_items = f->frequency_response_len / 2;
    p.freqplot_p.num_arrays = 3;

    el = page_add_el(page, EL_FREQ_PLOT, p, "track_settings_filter_freq_plot", "freq_plot");
    struct freq_plot *plot = el->component;
    plot->related_obj_lock = &f->lock;

    create_track_selection_area(page, track);

    page = tab_view_add_page(
	tv,
	"Delay line",
	DELAY_LINE_LT_PATH,
	page_colors + 1,
	&color_global_white,
	main_win);

    p.toggle_p.value = &track->delay_line.active;
    p.toggle_p.action = toggle_delay_line_target_action;
    p.toggle_p.target = (void *)(&track->delay_line);
    el = page_add_el(page, EL_TOGGLE, p, "track_settings_delay_toggle", "toggle_delay");

    p.textbox_p.set_str = "Delay line on";
    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = LABEL_STD_FONT_SIZE;
    p.textbox_p.win = main_win;
    tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "track_settings_delay_toggle_label", "toggle_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Delay time (ms)";
    tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "track_settings_delay_time_label", "del_time_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);
    
    p.textbox_p.set_str = "Delay amplitude";
    tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "track_settings_delay_amp_label", "del_amp_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Stereo offset";
    tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "track_settings_delay_stereo_offset", "del_stereo_offset_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);
    
    p.slider_p.create_label_fn = NULL;
    p.slider_p.style = SLIDER_TICK;
    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.ep = &track->delay_line.len_ep;
    p.slider_p.min = (Value){.int32_v = 1};
    p.slider_p.max = (Value){.int32_v = 1000};
    p.slider_p.create_label_fn = label_msec;
    el = page_add_el(page, EL_SLIDER, p, "track_settings_delay_time_slider", "del_time_slider");

    sl = el->component;
    sl->disallow_unsafe_mode = true;
    slider_reset(sl);
    
    p.slider_p.create_label_fn = NULL;
    p.slider_p.ep = &track->delay_line.amp_ep;
    p.slider_p.min = (Value){.double_v = 0.0};
    p.slider_p.max = (Value){.double_v = 0.99};

    el = page_add_el(page, EL_SLIDER, p, "track_settings_delay_amp_slider", "del_amp_slider");

    p.slider_p.ep = &track->delay_line.stereo_offset_ep;
    p.slider_p.max = (Value){.double_v = 0.0};
    p.slider_p.max = (Value){.double_v = 1.0};

    el = page_add_el(page, EL_SLIDER, p, "track_settings_delay_stereo_offset_slider", "del_stereo_offset_slider");
    sl = el->component;
    slider_reset(sl);
    sl->disallow_unsafe_mode = true;
    
    create_track_selection_area(page, track);

    page = tab_view_add_page(
	tv,
	"Saturation",
	SATURATION_LT_PATH,
	page_colors + 3,
	&color_global_white,
	main_win);

    p.toggle_p.value = &track->saturation.active;
    p.toggle_p.action = NULL;
    p.toggle_p.target = NULL;
    el = page_add_el(page, EL_TOGGLE, p, "track_settings_saturation_toggle", "toggle_saturation");

    p.textbox_p.set_str = "Saturation on";
    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = LABEL_STD_FONT_SIZE;
    p.textbox_p.win = main_win;
    tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "track_settings_saturation_toggle_label", "toggle_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);


    p.toggle_p.value = &track->saturation.do_gain_comp;
    p.toggle_p.action = toggle_saturation_gain_comp;
    p.toggle_p.target = &track->saturation;
    el = page_add_el(page, EL_TOGGLE, p, "track_settings_saturation_gain_comp_toggle", "toggle_gain_comp");

    p.textbox_p.set_str = "Gain compensation";
    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = LABEL_STD_FONT_SIZE;
    p.textbox_p.win = main_win;
    tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "track_settings_saturation_toggle_label", "toggle_gain_comp_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Gain";
    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = LABEL_STD_FONT_SIZE;
    p.textbox_p.win = main_win;
    tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "track_settings_saturation_gain_label", "amp_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    
    p.slider_p.ep = &track->saturation.gain_ep;
    p.slider_p.min = (Value){.double_v = 1.0};
    p.slider_p.max = (Value){.double_v = SATURATION_MAX_GAIN};
    p.slider_p.create_label_fn = label_amp_to_dbstr;
    p.slider_p.style = SLIDER_FILL;
    p.slider_p.orientation = SLIDER_HORIZONTAL;
    el = page_add_el(page, EL_SLIDER, p, "track_settings_saturation_gain_slider", "amp_slider");
    sl = el->component;
    slider_reset(sl);

    static const char * saturation_type_names[] = {
	"Hyperbolic (tanh)",
	"Exponential"
    };
    
    p.radio_p.text_size = RADIO_STD_FONT_SIZE;
    p.radio_p.text_color = &color_global_white;
    p.radio_p.ep = &track->saturation.type_ep;
    p.radio_p.item_names = saturation_type_names;
    p.radio_p.num_items = 2;
    
    el = page_add_el(page, EL_RADIO, p, "track_settings_saturation_type", "type_radio");
    radio = el->component;
    radio_button_reset_from_endpoint(radio);
    /* radio->selected_item = (uint8_t)0; */


    create_track_selection_area(page, track);
    /* sl->disallow_unsafe_mode = true; */
}

TabView *settings_track_tabview_create(Track *track)
{
    TabView *tv = tabview_create("Track Settings", proj->layout, main_win);
    settings_track_tabview_set_track(tv, track);
    layout_force_reset(tv->layout);
    return tv;
 
}


static void click_track_populate_settings_internal(ClickSegment *s, TabView *tv, bool set_from_cfg);
static void reset_settings_page_subdivs(ClickSegment *s, TabView *tv, const char *selected_el_id)
{
    /* ClickTrack *tt = s->track; */
    click_track_populate_settings_internal(s, tv, false);
    Page *p = tv->tabs[0];
    page_select_el_by_id(p, selected_el_id);
}

txt_int_range_completion(1, 13)
static int num_beats_completion(Text *txt, void *s_v)
{

    txt_int_range_completion_1_13(txt, NULL);
    TabView *tv = main_win->active_tabview;
    if (!tv) {
	fprintf(stderr, "Error: no tabview on num beats completion\n");
	exit(1);
    }
    ClickSegment *s = (ClickSegment *)s_v;
    if (atoi(txt->display_value) != s->cfg.num_beats) {
	reset_settings_page_subdivs(s_v, tv, "click_segment_num_beats_value");
    }
    return 0;
}

static ClickSegment *click_segment_copy(ClickSegment *s)
{
    ClickSegment *cpy = calloc(1, sizeof(ClickSegment));
    memcpy(cpy, s, sizeof(ClickSegment));
    return cpy;
}

NEW_EVENT_FN(undo_redo_set_segment_params, "undo/redo edit click segment")
    ClickSegment *s = (ClickSegment *)obj1;
    s = click_track_get_segment_at_pos(s->track, s->start_pos);
    self->obj1 = s;
    ClickSegment *cpy = (ClickSegment *)obj2;
    enum ts_end_bound_behavior ebb = val1.int_v;

    ClickSegment *redo_cpy = click_segment_copy(s);
    click_segment_set_config(s, -1, cpy->cfg.bpm, cpy->cfg.num_beats, cpy->cfg.beat_subdiv_lens, ebb);
    click_segment_destroy(cpy);
    self->obj2 = redo_cpy;
    s->track->tl->needs_redraw = true;
}


static int time_sig_submit_button_action(void *self, void *s_v)
{
    ClickSegment *s = (ClickSegment *)s_v;
    ClickSegment *cpy = click_segment_copy(s);
    ClickTrack *tt = s->track;

    int num_beats = atoi(tt->num_beats_str);
    int tempo = atoi(tt->tempo_str);
    uint8_t subdivs[num_beats];
    for (int i=0; i<num_beats; i++) {
	subdivs[i] = atoi(tt->subdiv_len_strs[i]);
    }
    click_segment_set_config(s, -1, tempo, atoi(tt->num_beats_str), subdivs, tt->end_bound_behavior);
    TabView *tv = main_win->active_tabview;
    tabview_close(tv);
    tt->tl->needs_redraw = true;

    Value ebb = {.int_v = tt->end_bound_behavior};
    user_event_push(
	&proj->history,
	undo_redo_set_segment_params,
	undo_redo_set_segment_params,
	NULL, NULL,
	s,
	cpy,
	ebb, ebb,
	ebb, ebb,
	0, 0,
	false, true);
    return 0;
}

static void draw_time_sig(void *tt_v, void *rect_v)
{
    ClickTrack *tt = (ClickTrack *)tt_v;
    SDL_Rect *rect = (SDL_Rect *)rect_v;
    int num_beats = atoi(tt->num_beats_str);
    int subdivs[num_beats];
    int num_atoms = 0;
    for (int i=0; i<num_beats; i++) {
	subdivs[i] = atoi(tt->subdiv_len_strs[i]);
	num_atoms += subdivs[i];
    }
    SDL_SetRenderDrawColor(main_win->rend, 255, 255, 255, 255);
    SDL_RenderDrawLine(main_win->rend, rect->x, rect->y + rect->h, rect->x + rect->w, rect->y + rect->h);
    SDL_RenderDrawLine(main_win->rend, rect->x, rect->y, rect->x, rect->y + rect->h);
    SDL_RenderDrawLine(main_win->rend, rect->x + rect->w, rect->y, rect->x + rect->w, rect->y + rect->h);
    int beat_i = 0;
    int subdiv_i = 1;
    for (int i=1; i<num_atoms; i++) {
	int x = rect->x + rect->w * i / num_atoms;
	float h_prop = 0.75;
	if (subdiv_i != 0) {
	    if (subdivs[beat_i] % 2 == 0 && subdiv_i % 2 == 0) {
		h_prop = 0.4;
	    } else {
		h_prop = 0.2;
	    }
	}
	SDL_RenderDrawLine(main_win->rend, x, rect->y + rect->h, x, rect->y + rect->h - rect->h * h_prop);
	if (subdiv_i >= subdivs[beat_i] - 1) {
	    subdiv_i = 0;
	    beat_i++;
	} else {
	    subdiv_i++;
	}

    }
}

txt_int_range_completion(1, 9)

static int segment_next_action(void *self, void *targ)
{
    ClickSegment *s = (ClickSegment *)targ;
    click_track_populate_settings_internal(s->next, main_win->active_tabview, true);
    Page *p = main_win->active_tabview->tabs[0];
    page_select_el_by_id(p, "segment_right");

    /* reset_settings_page_subdivs(s->next, main_win->active_tabview, "segment_right"); */
    return 0;
}

static int segment_prev_action(void *self, void *targ)
{
    ClickSegment *s = (ClickSegment *)targ;
    click_track_populate_settings_internal(s->prev, main_win->active_tabview, true);
    Page *p = main_win->active_tabview->tabs[0];
    page_select_el_by_id(p, "segment_left");
    /* reset_settings_page_subdivs(s->prev, main_win->active_tabview, "segment_left"); */
    return 0;
}

static void click_track_populate_settings_internal(ClickSegment *s, TabView *tv, bool set_from_cfg)
{

    ClickTrack *tt = s->track;
    /* TempoSegment *s = click_track_get_segment_at_pos(tt, tt->tl->play_pos_sframes); */
    
    static SDL_Color page_colors[] = {
	{40, 40, 80, 255},
	{50, 50, 80, 255},
	{70, 40, 70, 255}
    };

    tabview_clear_all_contents(tv);
    
    Page *page = tab_view_add_page(
	tv,
	"Click track config",
	CLICK_TRACK_SETTINGS_LT_PATH,
	page_colors,
	&color_global_white,
	main_win);
    if (tv->current_tab >= tv->num_tabs) {
	tv->current_tab = 0;
    }
    
    layout_force_reset(page->layout);

    PageElParams p;
    PageEl *el;
    Textbox *tb;


    
    p.textbox_p.font = main_win->mono_font;
    p.textbox_p.text_size = 14;
    p.textbox_p.win = page->win;

    p.textbox_p.set_str = "Track name:";
    el = page_add_el(page, EL_TEXTBOX, p, "track_name_label", "track_name_label");
    tb = (Textbox *)el->component;
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Num beats:";
    el = page_add_el(page, EL_TEXTBOX, p, "num_beats_label", "num_beats_label");
    tb = (Textbox *)el->component;
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Subdivisions:";
    el = page_add_el(page, EL_TEXTBOX, p, "beat_subdiv_label", "beat_subdiv_label");
    tb = (Textbox *)el->component;
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Tempo (bpm):";
    el = page_add_el(page, EL_TEXTBOX, p, "tempo_label", "tempo_label");
    tb = (Textbox *)el->component;
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);


    int num_beats;
    if (set_from_cfg) {
	num_beats = s->cfg.num_beats;
	snprintf(tt->num_beats_str, 3, "%d", s->cfg.num_beats);
    } else {
	num_beats = atoi(tt->num_beats_str);
    }
    if (num_beats > MAX_BEATS_PER_BAR) {
	num_beats = MAX_BEATS_PER_BAR;
	snprintf(tt->num_beats_str, 3, "%d", num_beats);
	char errstr[128];
	snprintf(errstr, 128, "Cannot exceed %d beats per bar", num_beats);
	status_set_errstr(errstr);
    }
    p.textentry_p.font = main_win->mono_bold_font;
    p.textentry_p.text_size = 14;


    p.textentry_p.value_handle = tt->name;
    p.textentry_p.validation = NULL;
    p.textentry_p.completion = NULL;
    p.textentry_p.buf_len = MAX_NAMELENGTH;
    el = page_add_el(page, EL_TEXTENTRY, p, "track_name", "track_name");
    /* Layout *name_lt = el->layout; */
    /* layout_size_to_fit_children_v(name_lt, false, 1); */
    /* layout_center_agnostic(name_lt, false, true); */
    textentry_reset(el->component);

    
    p.textentry_p.value_handle = tt->num_beats_str;
    p.textentry_p.buf_len = 3;    
    p.textentry_p.validation = txt_integer_validation;
    p.textentry_p.completion = num_beats_completion;
    p.textentry_p.completion_target = (void *)s;
    /* p.textbox_p.set_str = tt->num_beats_str; */
    /* p.textbox_p.font = main_win->mono_font; */
    el = page_add_el(page, EL_TEXTENTRY, p, "click_segment_num_beats_value", "num_beats_value");
    /* Layout *num_beats_lt = el->layout; */
    /* layout_size_to_fit_children_v(num_beats_lt, false, 1); */
    /* layout_center_agnostic(num_beats_lt, false, true); */
    textentry_reset(el->component);


    Layout *subdiv_area = layout_get_child_by_name_recursive(page->layout, "beat_subdiv_values");
    for (int i=0; i<num_beats; i++) {
	int subdivs;
	if (set_from_cfg) {
	    subdivs = s->cfg.beat_subdiv_lens[i];
	} else {
	    subdivs = atoi(tt->subdiv_len_strs[i]);
	}
	snprintf(tt->subdiv_len_strs[i], 2, "%d", subdivs);
	Layout *child_l = layout_add_child(subdiv_area);
	child_l->x.type = STACK;
	child_l->h.type = SCALE;
	child_l->h.value = 1.0;
	/* child_l->y.value = 5; */
	/* child_l->h.type = PAD; */
	child_l->w.value = 23;
	
	char name[64];
	snprintf(name, 64, "beat_subdiv_lt_%d", i);
	layout_set_name(child_l, name);
	layout_force_reset(subdiv_area);
	
	p.textentry_p.value_handle = tt->subdiv_len_strs[i];
	p.textentry_p.buf_len = 2;
	p.textentry_p.text_size = 14;
	p.textentry_p.validation = txt_integer_validation;
	p.textentry_p.completion = txt_int_range_completion_1_9;
	p.textentry_p.completion_target = NULL;
	char buf[255];
	snprintf(buf, 255, "beat_%d_subdiv_len", i);
	el = page_add_el(page, EL_TEXTENTRY, p, buf, name);
	/* layout_size_to_fit_children_v(el->layout, false, 2); */
	/* layout_center_agnostic(el->layout, false, true); */

	/* ((TextEntry *)el->component)->tb->text->max_len = 2; */
	textentry_reset(el->component);
	if (i != num_beats - 1) {
	    Layout *child_r = layout_copy(child_l, child_l->parent);
	    child_r->w.value *= 0.75;
	    /* Layout *child_r = layout_add_child(child); */
	    snprintf(name, 64, "plus %d", i);
	    layout_set_name(child_r, name);
	    layout_force_reset(subdiv_area);
	    PageElParams pt;
	    pt.textbox_p.font = main_win->mono_bold_font;
	    pt.textbox_p.text_size = 14;
	    pt.textbox_p.set_str = "+";
	    pt.textbox_p.win = page->win;

	    el = page_add_el(page, EL_TEXTBOX, pt, "", name);
	    /* textbox_set_pad(el->component, 24, 0); */
	    textbox_set_align(el->component, CENTER);
	    textbox_reset_full(el->component);
	}
    }


    /* Add canvas */
    Layout *time_sig_area = layout_get_child_by_name_recursive(page->layout, "time_signature_area");
    Layout *canvas_lt = layout_get_child_by_name_recursive(time_sig_area, "time_sig_canvas");
    p.canvas_p.draw_fn = draw_time_sig;
    p.canvas_p.draw_arg1 = (void *)tt;
    p.canvas_p.draw_arg2 = (void *)&canvas_lt->rect;
    el = page_add_el(
	page,
	EL_CANVAS,
	p,
	"time_sig_canvas",
	"time_sig_canvas"
	);


    /* Add tempo */
    snprintf(tt->tempo_str, 5, "%d", s->cfg.bpm);
    p.textentry_p.font = main_win->mono_bold_font;
    p.textentry_p.text_size = 14;
    p.textentry_p.value_handle = tt->tempo_str;
    p.textentry_p.buf_len = 5;
    p.textentry_p.validation = txt_integer_validation;
    p.textentry_p.completion = NULL;
    /* p.textentry_p.completion = num_beats_completion; */
    /* p.textentry_p.completion_target = (void *)s; */
    /* p.textbox_p.set_str = tt->num_beats_str; */
    /* p.textbox_p.font = main_win->mono_font; */
    el = page_add_el(page, EL_TEXTENTRY, p, "tempo_value", "tempo_value");
    /* Layout *tempo_lt = el->layout; */
    /* tempo_lt->w.value = 50; */
    /* layout_size_to_fit_children_v(tempo_lt, false, 1); */
    /* layout_center_agnostic(tempo_lt, false, true); */
    /* /\* num_beats_lt->w.value = 50; *\/ */
    /* layout_size_to_fit_children_v(num_beats_lt, false, 2); */
    /* layout_center_agnostic(num_beats_lt, false, true); */
    textentry_reset(el->component);
    




    /* Add end-bound behavior radio */
    if (s->next) {
	char opt1[64];
	char opt2[64];
	char timestr[64];
	timecode_str_at(tt->tl, timestr, 64, s->next->start_pos);
	snprintf(opt1, 64, "Fixed end pos (%s)", timestr);

	if (s->cfg.dur_sframes * s->num_measures == s->next->start_pos - s->start_pos) {
	    snprintf(opt2, 64, "Fixed num measures (%d)", s->num_measures);
	} else {
	    snprintf(opt2, 64, "Fixed num measures (%f)", (float)(s->next->start_pos - s->start_pos)/s->cfg.dur_sframes);
	}
	char *options[] = {opt1, opt2};

	p.radio_p.ep = &tt->end_bound_behavior_ep;
	/* p.radio_p.action = tempo_rb_action; */
	/* p.radio_p.target = (void *)&tt->end_bound_behavior; */
	p.radio_p.num_items = 2;
	p.radio_p.text_size = 14;
	p.radio_p.text_color = &color_global_white;
	p.radio_p.item_names = (const char **)options;

	el = page_add_el(
	    page,
	    EL_RADIO,
	    p,
	    "click_segment_ebb_radio",
	    "ebb_radio"
	    );

	radio_button_reset_from_endpoint(el->component);
	layout_reset(el->layout);
	layout_size_to_fit_children_v(el->layout, true, 0);
	layout_force_reset(el->layout);
	/* te->tb->text->max_len = TEMPO_STRLEN; */
    }


    /* layout_force_reset(page->layout); */

    layout_size_to_fit_children_v(time_sig_area, true, 0);

    /* Add submit button */
    p.button_p.action = time_sig_submit_button_action;
    p.button_p.target = (void *)s;
    p.button_p.font = main_win->mono_bold_font;
    p.button_p.text_color = &color_global_black;
    p.button_p.text_size = 14;
    p.button_p.background_color = &color_global_light_grey;
    p.button_p.win = main_win;
    p.button_p.set_str = "Submit";
    el = page_add_el(
	page,
	EL_BUTTON,
	p,
	"time_signature_submit_button",
	"time_signature_submit");
    /* layout_center_agnostic(el->layout, true, false); */
    textbox_set_align(((Button *)el->component)->tb, CENTER);
    textbox_reset_full(((Button *)el->component)->tb);



    char label[255];
    if (s->next) {	
	snprintf(label, 255, "Segment from m%d to m%d", s->first_measure_index, s->next->first_measure_index);
    } else {
	snprintf(label, 255, "Segment from m%d to ∞", s->first_measure_index);
    }
    /* timecode_str_at(tt->tl, label + offset, 255 - offset, s->start_pos); */
    
    /* offset = strlen(label); */
    /* offset += snprintf(label + offset, 255 - offset, " - "); */
    /* timecode_str_at(tt->tl, label + offset, 255 - offset, s->end_pos); */

    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = 14;
    p.textbox_p.set_str = label;
    p.textbox_p.win = page->win;

    el = page_add_el(page, EL_TEXTBOX, p, "", "segment_label");
    tb = (Textbox *)el->component;
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);
    layout_size_to_fit_children_h(el->layout, true, 0);

    /* Add next/previous segment navigators */
    if (s->prev || s->next) {
	p.button_p.font = main_win->mono_font;
	p.button_p.win = page->win;
	p.button_p.target = NULL;
	p.button_p.text_color = &color_global_white;
	p.button_p.text_size = 14;
	p.button_p.background_color = &color_global_quickref_button_blue;
	p.button_p.target = s;

    }
    if (s->prev) {
	p.button_p.set_str = "←";
	p.button_p.action = segment_prev_action;
	page_add_el(page, EL_BUTTON, p, "segment_left", "segment_left");

    }
    if (s->next) {
	p.button_p.set_str = "→";
	p.button_p.action = segment_next_action;
	page_add_el(page, EL_BUTTON, p, "segment_right", "segment_right");
    }
    
    page_reset(page);
}

void click_track_populate_settings_tabview(ClickTrack *tt, TabView *tv)
{
    ClickSegment *s = click_track_get_segment_at_pos(tt, tt->tl->play_pos_sframes);
    click_track_populate_settings_internal(s, tv, true);
}

void timeline_click_track_edit(Timeline *tl)
{
    ClickTrack *tt = timeline_selected_click_track(tl);
    if (!tt) return;

    TabView *tv = tabview_create("Track Settings", proj->layout, main_win);
    click_track_populate_settings_tabview(tt, tv);

    tabview_activate(tv);
    tl->needs_redraw = true;    
}
