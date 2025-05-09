/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    effect_pages.c

    * horrible procedural construction of GUI pages for effects
    * this is why they invented HTML
 *****************************************************************************************************************/


#include "color.h"
#include "geometry.h"
#include "project.h"
#include "waveform.h"

#define LABEL_STD_FONT_SIZE 12
#define RADIO_STD_FONT_SIZE 14

extern Window *main_win;
extern Project *proj;

extern SDL_Color EQ_CTRL_COLORS[];
extern SDL_Color EQ_CTRL_COLORS_LIGHT[];

extern SDL_Color color_global_black;
extern SDL_Color color_global_white;
extern SDL_Color color_global_light_grey;
extern SDL_Color color_global_white;
extern SDL_Color freq_L_color;
extern SDL_Color freq_R_color;

SDL_Color filter_selected_clr = {50, 50, 200, 255};
SDL_Color filter_selected_inactive = {100, 100, 100, 100};

extern Symbol *SYMBOL_TABLE[];

static SDL_Color page_colors[] = {
    {30, 80, 80, 255},
    {50, 50, 80, 255},
    /* {70, 40, 70, 255} */
    {43, 43, 55, 255},
    {100, 40, 40, 255},
    {94, 58, 61, 255},
};

Page *add_eq_page(EQ *eq, Track *t, TabView *tv);
Page *add_fir_filter_page(FIRFilter *f, Track *t, TabView *tv);
Page *add_delay_page(DelayLine *dl, Track *t, TabView *tv);
Page *add_saturation_page(Saturation *s, Track *t, TabView *tv);
Page *add_compressor_page(Compressor *c, Track *t, TabView *tv);

Page *effect_add_page(Effect *e, TabView *tv)
{
    Track *t = e->track;
    Page *p = NULL;
    switch(e->type) {
    case EFFECT_EQ:
	p = add_eq_page(e->obj, t, tv);
	break;
    case EFFECT_FIR_FILTER:
	p = add_fir_filter_page(e->obj, t, tv);
	break;
    case EFFECT_DELAY:
	p = add_delay_page(e->obj, t, tv);
	break;
    case EFFECT_SATURATION:
	p = add_saturation_page(e->obj, t, tv);
	break;
    case EFFECT_COMPRESSOR:
	p = add_compressor_page(e->obj, t, tv);
	break;
    default:
	break;
    }
    p->linked_obj = e;
    p->linked_obj_type = PAGE_EFFECT;
    e->page = p;
    return p;
}

TabView *track_effects_tabview_create(Track *track)
{
    TabView *tv = tabview_create("Track Effects", proj->layout, main_win);
    for (int i=0; i<track->num_effects; i++) {
	effect_add_page(track->effects[i], tv);
    }
    tv->related_array = &track->effects;
    tv->related_array_el_size = sizeof(Effect *);
    return tv;

}

void filter_tabs_canvas_draw(void *draw_arg1, void *draw_arg2);
static void compressor_canvas_draw(void *draw_arg1, void *draw_arg2);
bool filter_tabs_onclick(SDL_Point p, Canvas *self, void *draw_arg1, void *draw_arg2);
int filter_active_toggle(void *self, void *target);
void filter_type_selector_canvas_draw(void *draw_arg1, void *draw_arg2);
int filter_type_button_action(void *self, void *target);
static void create_track_selection_area(Page *page, Track *track);
static int next_track(void *self_v, void *target);
static int previous_track(void *self_v, void *target);
static double unscale_freq(double scaled);
static int toggle_delay_line_target_action(void *self_v, void *target);
static int toggle_saturation_gain_comp(void *self_v, void *target);

Page *add_eq_page(EQ *eq, Track *track, TabView *tv)
{
    Page *page = tab_view_add_page(
	tv,
	eq->effect->name,
	/* "Equalizer", */
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

    p.toggle_p.value = &eq->active;
    p.toggle_p.target = NULL;
    p.toggle_p.action = NULL;
    page_add_el(page, EL_TOGGLE, p, "track_settings_eq_toggle", "toggle_eq_on");
    

    p.eq_plot_p.eq = eq;
    page_add_el(page, EL_EQ_PLOT, p, "track_settings_eq_plot", "eq_plot");

    /* p.textarea_p.font = main_win->mono_bold_font; */
    /* p.textarea_p.color = color_global_white; */
    /* p.textarea_p.text_size = 12; */
    /* p.textarea_p.win = main_win; */
    /* p.textarea_p.value = "Click and drag the circles to set peaks or notches.\n \nHold cmd or ctrl and drag up or down to set the filter bandwidth.\n \nAdditional filter types (shelving, lowpass, highpass) will be added in future versions of jackdaw."; */
    /* page_add_el(page, EL_TEXTAREA, p, "track_settings_eq_desc", "description"); */

    Layout *filter_tabs = layout_get_child_by_name_recursive(page->layout, "filter_tabs");
    
    p.canvas_p.draw_arg1 = eq;
    p.canvas_p.draw_arg2 = filter_tabs;
    p.canvas_p.draw_fn = filter_tabs_canvas_draw;
    el = page_add_el(
	page,
	EL_CANVAS,
	p,
	"",
	"filter_tabs");
    ((Canvas *)el->component)->on_click = filter_tabs_onclick;

    char lt_name[2];
    /* static const char *tab_ids[] = */
    /* 	{"filter_tab1", "filter_tab2", "filter_tab3", "filter_tab4", "filter_tab5", "filter_tab6"}; */
    for (int i=0; i<EQ_DEFAULT_NUM_FILTERS; i++) {
	Layout *tab_lt = layout_add_child(filter_tabs);
	tab_lt->h.type = SCALE;
	tab_lt->h.value = 1.0;
	tab_lt->w.value = 50.0;
	tab_lt->x.type = STACK;
	tab_lt->x.value = 0;
	snprintf(lt_name, 2, "%d", i);
	/* p.button_p. */
	p.textbox_p.font = main_win->mono_bold_font;
	p.textbox_p.text_size = 14;
	p.textbox_p.set_str = lt_name;
	p.textbox_p.win = main_win;
	layout_set_name(tab_lt, lt_name);
	layout_reset(tab_lt);
	el = page_add_el(
	    page,
	    EL_TEXTBOX,
	    p,
	    "",
	    lt_name);
	Textbox *tab_tb = el->component;
	textbox_set_border(tab_tb, EQ_CTRL_COLORS + i, 1);

    }
    /* memset(&p, '\0', sizeof(p)); */

    p.toggle_p.action = filter_active_toggle;
    p.toggle_p.target = eq;
    p.toggle_p.value = &eq->selected_filter_active;
    page_add_el(page, EL_TOGGLE, p, "", "filter_active_toggle");

    p.textbox_p.font = main_win->mono_font;
    p.textbox_p.text_size = 14;
    p.textbox_p.set_str = "Selected filter active";
    p.textbox_p.win = main_win;

    el = page_add_el(page, EL_TEXTBOX, p, "", "filter_active_toggle_label");
    tb = el->component;
    textbox_set_trunc(tb, false);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);


    Layout *button_container = layout_get_child_by_name_recursive(page->layout, "type_selector");

    p.canvas_p.draw_arg1 = eq;
    p.canvas_p.draw_arg2 = button_container;
    p.canvas_p.draw_fn = filter_type_selector_canvas_draw;
    el = page_add_el(
	page,
	EL_CANVAS,
	p,
	"",
	"type_selector");
    /* ((Canvas *)el->component)->on_click = filter_tabs_onclick; */


    Layout *button_lt = layout_add_child(button_container);
    layout_set_name(button_lt, "lowshelf_btn");
    button_lt->x.type = STACK;
    button_lt->x.value = 0.0;
    button_lt->w.value = SYMBOL_STD_DIM * SYMBOL_FILTER_DIM_SCALAR_H;
    button_lt->h.value = SYMBOL_STD_DIM * SYMBOL_FILTER_DIM_SCALAR_V;
    layout_reset(button_lt);
    p.sbutton_p.action = filter_type_button_action;
    p.sbutton_p.target = eq;
    p.sbutton_p.s = SYMBOL_TABLE[5];
    p.sbutton_p.background_color = NULL;
    el = page_add_el(
	page,
	EL_SYMBOL_BUTTON,
	p,
	"lowshelf_btn",
	"lowshelf_btn");
    ((SymbolButton *)el->component)->stashed_val.int_v = IIR_LOWSHELF;


    button_lt = layout_add_child(button_container);
    layout_set_name(button_lt, "peaknotch_btn");
    button_lt->x.type = STACK;
    button_lt->x.value = 20.0;
    button_lt->w.value = SYMBOL_STD_DIM * SYMBOL_FILTER_DIM_SCALAR_H;
    button_lt->h.value = SYMBOL_STD_DIM * SYMBOL_FILTER_DIM_SCALAR_V;
    layout_reset(button_lt);
    p.sbutton_p.action = filter_type_button_action;
    p.sbutton_p.target = eq;
    p.sbutton_p.s = SYMBOL_TABLE[7];
    p.sbutton_p.background_color = NULL;
    el = page_add_el(
	page,
	EL_SYMBOL_BUTTON,
	p,
	"peaknotch_btn",
	"peaknotch_btn");

    ((SymbolButton *)el->component)->stashed_val.int_v = IIR_PEAKNOTCH;

        
    button_lt = layout_add_child(button_container);
    layout_set_name(button_lt, "highshelf_btn");
    button_lt->x.type = STACK;
    button_lt->x.value = 20.0;
    button_lt->w.value = SYMBOL_STD_DIM * SYMBOL_FILTER_DIM_SCALAR_H;
    button_lt->h.value = SYMBOL_STD_DIM * SYMBOL_FILTER_DIM_SCALAR_V;
    layout_reset(button_lt);
    p.sbutton_p.action = filter_type_button_action;
    p.sbutton_p.target = eq;
    p.sbutton_p.s = SYMBOL_TABLE[6];
    p.sbutton_p.background_color = NULL;
    el = page_add_el(
	page,
	EL_SYMBOL_BUTTON,
	p,
	"highshelf_btn",
	"highshelf_btn");
    ((SymbolButton *)el->component)->stashed_val.int_v = IIR_HIGHSHELF;


    create_track_selection_area(page, eq->track);
    return page;
}

Page *add_fir_filter_page(FIRFilter *f, Track *track, TabView *tv)
{
    Page *page = tab_view_add_page(
	tv,
	f->effect->name,
	FIR_FILTER_LT_PATH,
	page_colors,
	&color_global_white,
	main_win);

    PageElParams p;
    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = LABEL_STD_FONT_SIZE;
    p.textbox_p.set_str = "Bandwidth:";
    p.textbox_p.win = page->win;
    PageEl *el = page_add_el(page, EL_TEXTBOX, p, "track_settings_filter_bandwidth_label", "bandwidth_label");

    Textbox *tb = el->component;
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

    p.toggle_p.value = &f->active;
    p.toggle_p.target = NULL;
    p.toggle_p.action = NULL;
    page_add_el(page, EL_TOGGLE, p, "track_settings_filter_toggle", "toggle_filter_on");
    
    f->cutoff_freq_unscaled = unscale_freq(f->cutoff_freq);
    p.slider_p.create_label_fn = label_freq_raw_to_hz;
    p.slider_p.style = SLIDER_TICK;
    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.min = (Value){.double_v = 0.0};
    p.slider_p.max = (Value){.double_v = 1.0};
    p.slider_p.ep = &f->cutoff_ep;
    el = page_add_el(page, EL_SLIDER, p, "track_settings_filter_cutoff_slider", "cutoff_slider");

    f->bandwidth_unscaled = unscale_freq(f->bandwidth);
    p.slider_p.ep = &f->bandwidth_ep;
    el = page_add_el(page, EL_SLIDER, p, "track_settings_filter_bandwidth_slider", "bandwidth_slider");

    p.slider_p.ep = &f->impulse_response_len_ep;

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
	/* track->buf_L_freq_mag, */
	/* track->buf_R_freq_mag, */
	f->output_freq_mag_L,
	f->output_freq_mag_R,
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
    return page;
}

Page *add_delay_page(DelayLine *d, Track *track, TabView *tv)
{
    Page *page = tab_view_add_page(
	tv,
        d->effect->name,
	DELAY_LINE_LT_PATH,
	page_colors + 1,
	&color_global_white,
	main_win);

    PageElParams p;
    p.toggle_p.value = &d->active;
    p.toggle_p.action = toggle_delay_line_target_action;
    p.toggle_p.target = (void *)(d);
    PageEl *el = page_add_el(page, EL_TOGGLE, p, "track_settings_delay_toggle", "toggle_delay");

    p.textbox_p.set_str = "Delay line on";
    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = LABEL_STD_FONT_SIZE;
    p.textbox_p.win = main_win;
    Textbox *tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "track_settings_delay_toggle_label", "toggle_label")->component);
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
    p.slider_p.ep = &d->len_ep;
    p.slider_p.min = (Value){.int32_v = 1};
    p.slider_p.max = (Value){.int32_v = 1000};
    p.slider_p.create_label_fn = label_msec;
    el = page_add_el(page, EL_SLIDER, p, "track_settings_delay_time_slider", "del_time_slider");

    Slider *sl = el->component;
    sl->disallow_unsafe_mode = true;
    slider_reset(sl);
    
    p.slider_p.create_label_fn = NULL;
    p.slider_p.ep = &d->amp_ep;
    p.slider_p.min = (Value){.double_v = 0.0};
    p.slider_p.max = (Value){.double_v = 0.99};

    el = page_add_el(page, EL_SLIDER, p, "track_settings_delay_amp_slider", "del_amp_slider");

    p.slider_p.ep = &d->stereo_offset_ep;
    p.slider_p.max = (Value){.double_v = 0.0};
    p.slider_p.max = (Value){.double_v = 1.0};

    el = page_add_el(page, EL_SLIDER, p, "track_settings_delay_stereo_offset_slider", "del_stereo_offset_slider");
    sl = el->component;
    slider_reset(sl);
    sl->disallow_unsafe_mode = true;
    
    create_track_selection_area(page, track);

    return page;
}

Page *add_saturation_page(Saturation *s, Track *track, TabView *tv)
{
    Page *page = tab_view_add_page(
	tv,
	s->effect->name,
	SATURATION_LT_PATH,
	page_colors + 3,
	&color_global_white,
	main_win);

    PageElParams p;
    p.toggle_p.value = &s->active;
    p.toggle_p.action = NULL;
    p.toggle_p.target = NULL;
    PageEl *el = page_add_el(page, EL_TOGGLE, p, "track_settings_saturation_toggle", "toggle_saturation");

    p.textbox_p.set_str = "Saturation on";
    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = LABEL_STD_FONT_SIZE;
    p.textbox_p.win = main_win;
    Textbox *tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "track_settings_saturation_toggle_label", "toggle_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);


    p.toggle_p.value = &s->do_gain_comp;
    p.toggle_p.action = toggle_saturation_gain_comp;
    p.toggle_p.target = s;
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

    
    p.slider_p.ep = &s->gain_ep;
    p.slider_p.min = (Value){.double_v = 1.0};
    p.slider_p.max = (Value){.double_v = SATURATION_MAX_GAIN};
    p.slider_p.create_label_fn = label_amp_to_dbstr;
    p.slider_p.style = SLIDER_FILL;
    p.slider_p.orientation = SLIDER_HORIZONTAL;
    el = page_add_el(page, EL_SLIDER, p, "track_settings_saturation_gain_slider", "amp_slider");
    Slider *sl = el->component;
    slider_reset(sl);

    static const char * saturation_type_names[] = {
	"Hyperbolic (tanh)",
	"Exponential"
    };
    
    p.radio_p.text_size = RADIO_STD_FONT_SIZE;
    p.radio_p.text_color = &color_global_white;
    p.radio_p.ep = &s->type_ep;
    p.radio_p.item_names = saturation_type_names;
    p.radio_p.num_items = 2;
    
    el = page_add_el(page, EL_RADIO, p, "track_settings_saturation_type", "type_radio");
    RadioButton *radio = el->component;
    radio_button_reset_from_endpoint(radio);
    /* radio->selected_item = (uint8_t)0; */


    create_track_selection_area(page, track);
    return page;
}

Page *add_compressor_page(Compressor *c, Track *track, TabView *tv)
{
    Page *page = tab_view_add_page(
	tv,
	c->effect->name,
	COMPRESSOR_LT_PATH,
	page_colors + 4,
	&color_global_white,
	main_win);

    PageElParams p;
    p.toggle_p.value = &c->active;
    p.toggle_p.action = NULL;
    p.toggle_p.target = NULL;
    PageEl *el = page_add_el(page, EL_TOGGLE, p, "track_settings_comp_toggle", "toggle_comp");

    p.textbox_p.set_str = "Compressor on";
    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = LABEL_STD_FONT_SIZE;
    p.textbox_p.win = main_win;
    Textbox *tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "", "toggle_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Attack time (ms)";
    tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "", "attack_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);
    
    p.textbox_p.set_str = "Release time (ms)";
    tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "", "release_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Threshold";
    tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "", "threshold_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Ratio";
    tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "", "ratio_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Makeup gain";
    tb = (Textbox *)(page_add_el(page, EL_TEXTBOX, p, "", "makeup_gain_label")->component);
    textbox_set_background_color(tb, NULL);
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    
    p.slider_p.create_label_fn = NULL;
    p.slider_p.style = SLIDER_TICK;
    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.ep = &c->attack_time_ep;
    p.slider_p.min = (Value){.float_v = 0.0};
    p.slider_p.max = (Value){.float_v = 200.0};
    p.slider_p.create_label_fn = label_msec;
    el = page_add_el(page, EL_SLIDER, p, "track_settings_comp_attack_slider", "attack_slider");

    Slider *sl = el->component;
    /* /\* sl->disallow_unsafe_mode = true; *\/ */
    slider_reset(sl);
    
    p.slider_p.create_label_fn = NULL;
    p.slider_p.ep = &c->release_time_ep;
    p.slider_p.min = (Value){.float_v = 0.0};
    p.slider_p.max = (Value){.float_v = 2000.0};
    el = page_add_el(page, EL_SLIDER, p, "track_settings_comp_release_slider", "release_slider");

    p.slider_p.ep = &c->threshold_ep;
    p.slider_p.min = (Value){.float_v = 0.0};
    p.slider_p.max = (Value){.float_v = 1.0};
    el = page_add_el(page, EL_SLIDER, p, "track_settings_comp_threshold_slider", "threshold_slider");
    sl = el->component;
    slider_reset(sl);

    p.slider_p.ep = &c->ratio_ep;
    p.slider_p.min = (Value){.float_v = 0.0};
    p.slider_p.max = (Value){.float_v = 1.0};
    el = page_add_el(page, EL_SLIDER, p, "track_settings_comp_ratio_slider", "ratio_slider");
    sl = el->component;
    slider_reset(sl);

    p.slider_p.ep = &c->makeup_gain_ep;
    p.slider_p.min = (Value){.float_v = 1.0};
    p.slider_p.max = (Value){.float_v = 10.0};
    p.slider_p.style = SLIDER_FILL;
    el = page_add_el(page, EL_SLIDER, p, "track_settings_makeup_gain_slider", "makeup_gain_slider");
    sl = el->component;
    slider_reset(sl);

    Layout *comp_draw_lt = layout_get_child_by_name_recursive(page->layout, "comp_display");
    p.canvas_p.draw_arg1 = c;
    p.canvas_p.draw_arg2 = comp_draw_lt;
    p.canvas_p.draw_fn = compressor_canvas_draw;

    page_add_el(page, EL_CANVAS, p, "", "comp_display");
    


    /* sl->disallow_unsafe_mode = true; */
    
    create_track_selection_area(page, track);
    return page;

}



/**************************************************************************/
/* utils */



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


void filter_tabs_canvas_draw(void *draw_arg1, void *draw_arg2)
{
    EQ *eq = draw_arg1;
    Layout *filter_tabs = draw_arg2;
    for (int i=0; i<filter_tabs->num_children; i++) {
	/* fprintf(stderr, "check %d\n", i); */
	if (i == eq->selected_ctrl) {
	    /* fprintf(stderr, "found !\n"); */
	    if (eq->ctrls[eq->selected_ctrl].filter_active) {
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(EQ_CTRL_COLORS_LIGHT[i]));
	    } else {
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(filter_selected_inactive));
	    }
	    SDL_RenderFillRect(main_win->rend, &filter_tabs->children[i]->rect);
	    break;
	}
    }    
}

static void compressor_canvas_draw(void *draw_arg1, void *draw_arg2)
{
    Compressor *c = draw_arg1;
    Layout *display_lt = draw_arg2;
    compressor_draw(c, &display_lt->rect);
}



bool filter_tabs_onclick(SDL_Point p, Canvas *self, void *draw_arg1, void *draw_arg2)
{
    EQ *eq = draw_arg1;
    Layout *filter_tabs = draw_arg2;
    for (int i=0; i<filter_tabs->num_children; i++) {
	if (SDL_PointInRect(&p, &filter_tabs->children[i]->rect)) {
	    eq_select_ctrl(eq, i);
	    /* eq->selected_ctrl = i; */
	    return true;
	    break;
	}
    }
    return false;    
}

int filter_active_toggle(void *self, void *target)
{
    EQ *eq = target;
    eq_toggle_selected_filter_active(eq);
    return 0;
}

void filter_type_selector_canvas_draw(void *draw_arg1, void *draw_arg2)
{
    EQ *eq = draw_arg1;
    Layout *selector_lt = draw_arg2;
    IIRFilterType t = eq->group.filters[eq->selected_ctrl].type;
    Layout *sbutton_lt = selector_lt->children[t];
    /* SDL_Rect r = sbutton_lt->rect; */
    /* fprintf(stderr, "Found layout to draw: %p, %d %d %d %d\n", sbutton_lt, r.x, r.y, r.w, r.h); */
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(filter_selected_clr));
    geom_fill_rounded_rect(main_win->rend, &sbutton_lt->rect, SYMBOL_TABLE[7]->corner_rad_pix);
}

int filter_type_button_action(void *self, void *target)
{
    SymbolButton *sb = self;
    EQ *eq = target;
    eq_set_filter_type(eq, sb->stashed_val.int_v);
    return 0;
}

void user_tl_track_selector_up(void *);
void user_tl_track_selector_down(void *);

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
