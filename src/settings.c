/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <pthread.h>
#include "assets.h"
#include "color.h"
#include "dir.h"
#include "eq.h"
#include "layout.h"
#include "menu.h"
#include "modal.h"
#include "page.h"
#include "project.h"
#include "session.h"
#include "textbox.h"
#include "timeline.h"
#include "userfn.h"
#include "window.h"

#ifndef INSTALL_DIR
#define INSTALL_DIR "."
#endif

#define LABEL_STD_FONT_SIZE 12
#define RADIO_STD_FONT_SIZE 14

extern Window *main_win;

extern Symbol *SYMBOL_TABLE[];

extern struct colors colors;

extern SDL_Color EQ_CTRL_COLORS[];
extern SDL_Color EQ_CTRL_COLORS_LIGHT[];

/* SDL_Color filter_selected_clr = {50, 50, 200, 255}; */
/* SDL_Color filter_selected_inactive = {100, 100, 100, 100}; */

/* static struct freq_plot *current_fp; */
Waveform *current_waveform;

void settings_track_tabview_set_track(TabView *tv, Track *track)
{
    user_tl_track_open_settings(tv);
    user_tl_track_open_settings(tv);
    /* tabview_deactivate(tv); */
}
TabView *track_effects_tabview_create(Track *track);
/* TabView *settings_track_tabview_create(Track *track) */
/* { */
/*     return track_effects_tabview_create(track); */
/*     /\* TabView *tv = tabview_create("Track Settings", session->gui.layout, main_win); *\/ */
/*     /\* settings_track_tabview_set_track(tv, track); *\/ */
/*     /\* layout_force_reset(tv->layout); *\/ */
/*     /\* return tv; *\/ */
 
/* } */


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
    click_segment_set_config(s, -1, cpy->cfg.bpm, cpy->cfg.num_beats, cpy->cfg.beat_len_atoms, ebb);
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

struct metronome_buf_menu_item_data {
    MetronomeBuffer **buf_dst;
    int sel_i;
    Button *button_to_reset;
};

void metronome_buf_menu_item_onclick(void *target)
{
    Session *session = session_get();
    struct metronome_buf_menu_item_data *data = target;
    if (data->sel_i < 0) {
	*(data->buf_dst) = NULL;
	textbox_set_value_handle(data->button_to_reset->tb, "(none)");
	textbox_size_to_fit(data->button_to_reset->tb, 6, 2);
	textbox_reset_full(data->button_to_reset->tb);
    } else {
	*(data->buf_dst) = session->metronome_buffers + data->sel_i;
	textbox_set_value_handle(data->button_to_reset->tb, (*(data->buf_dst))->name);
	textbox_size_to_fit(data->button_to_reset->tb, 6, 2);
	textbox_reset_full(data->button_to_reset->tb);
    }
    window_pop_menu(main_win);
}

static int metronome_buf_button_action(void *self_v, void *target_v)
{
    Button *b = self_v;
    Menu *m = menu_create_at_point(b->tb->layout->rect.x + b->tb->layout->rect.w, b->tb->layout->rect.y + b->tb->layout->rect.h);
    MenuColumn *mc = menu_column_add(m, NULL);
    MenuSection *ms = menu_section_add(mc, NULL);

    MetronomeBuffer **dst = target_v;
    
    Session *session = session_get();
    for (int i=0; i<session->num_metronome_buffers; i++) {
	struct metronome_buf_menu_item_data *data = malloc(sizeof(struct metronome_buf_menu_item_data));
	data->buf_dst = dst;
	data->sel_i = i;
	data->button_to_reset = b;
	MenuItem *item = menu_item_add(ms, session->metronome_buffers[i].name, NULL, metronome_buf_menu_item_onclick, data);
	item->free_target_on_destroy = true;
    }

    struct metronome_buf_menu_item_data *data = malloc(sizeof(struct metronome_buf_menu_item_data));
    data->buf_dst = dst;
    data->sel_i = -1;
    data->button_to_reset = b;
    MenuItem *item = menu_item_add(ms, "(none)", NULL, metronome_buf_menu_item_onclick, data);
    item->free_target_on_destroy = true;
    
    window_add_menu(main_win, m);
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
    
    Page *page = tabview_add_page(
	tv,
	"Click track config",
	CLICK_TRACK_SETTINGS_LT_PATH,
	page_colors,
	&colors.white,
	main_win);
    if (tv->current_tab >= tv->num_tabs) {
	tv->current_tab = 0;
    }
    
    layout_force_reset(page->layout);

    PageElParams p;
    PageEl *el;
    Textbox *tb;
    p.textbox_p.font = main_win->mono_font;
    p.textbox_p.text_size = 16;
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
	    subdivs = s->cfg.beat_len_atoms[i];
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
    snprintf(tt->tempo_str, BPM_STRLEN, "%f", s->cfg.bpm);
    p.textentry_p.font = main_win->mono_bold_font;
    p.textentry_p.text_size = 14;
    p.textentry_p.value_handle = tt->tempo_str;
    p.textentry_p.buf_len = BPM_STRLEN;
    p.textentry_p.validation = txt_float_validation;
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
	p.radio_p.text_color = &colors.white;
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
    p.button_p.text_color = &colors.black;
    p.button_p.text_size = 14;
    p.button_p.background_color = &colors.light_grey;
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
	p.button_p.text_color = &colors.white;
	p.button_p.text_size = 18;
	p.button_p.background_color = &colors.quickref_button_blue;
	p.button_p.target = s;
    }
    if (s->prev) {
	p.button_p.set_str = "←";
	p.button_p.action = segment_prev_action;
	el = page_add_el(page, EL_BUTTON, p, "segment_left", "segment_left");
	Textbox *tb = ((Button *)el->component)->tb;
	textbox_set_border(tb, &colors.grey, 1, 4);

    }
    if (s->next) {
	p.button_p.set_str = "→";
	p.button_p.action = segment_next_action;
	el = page_add_el(page, EL_BUTTON, p, "segment_right", "segment_right");
	Textbox *tb = ((Button *)el->component)->tb;
	textbox_set_border(tb, &colors.grey, 1, 4);

    }
    page_reset(page);

/*------ metronome page ----------------------------------------------*/
    
    page = tabview_add_page(
	tv,
	"Metronome",
	METRONOME_PAGE_LT_PATH,
	page_colors,
	&colors.white,
	main_win);

    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = 16;
    p.textbox_p.win = main_win;
    p.textbox_p.set_str = "Measure:";
    page_add_el(page, EL_TEXTBOX, p, "", "measure_label");

    p.textbox_p.set_str = "Beat:";
    page_add_el(page, EL_TEXTBOX, p, "", "beat_label");

    p.textbox_p.set_str = "Offbeat (2 & 4):";
    page_add_el(page, EL_TEXTBOX, p, "", "offbeat_label");

    p.textbox_p.set_str = "Subdiv (8ths):";
    page_add_el(page, EL_TEXTBOX, p, "", "subdiv_label");

    p.button_p.action = metronome_buf_button_action;
    p.button_p.background_color = &colors.light_grey;
    p.button_p.text_color = &colors.black;
    p.button_p.text_size = 16;

    MetronomeBuffer **mb = &tt->metronome.bp_measure_buf;
    p.button_p.target = mb;
    p.button_p.set_str = *mb ? (char *)(*mb)->name : "(none)";
    page_add_el(page, EL_BUTTON, p, "", "measure_value");

    mb = &tt->metronome.bp_beat_buf;
    p.button_p.target = mb;
    p.button_p.set_str = *mb ? (char *)(*mb)->name : "(none)";
    page_add_el(page, EL_BUTTON, p, "", "beat_value");

    mb = &tt->metronome.bp_offbeat_buf;
    p.button_p.target = mb;
    p.button_p.set_str = *mb ? (char *)(*mb)->name : "(none)";
    page_add_el(page, EL_BUTTON, p, "", "offbeat_value");

    mb = &tt->metronome.bp_subdiv_buf;
    p.button_p.target = mb;
    p.button_p.set_str = *mb ? (char *)(*mb)->name : "(none)";    
    page_add_el(page, EL_BUTTON, p, "", "subdiv_value");

    page_reset(page);

}

void click_track_populate_settings_tabview(ClickTrack *tt, TabView *tv)
{
    ClickSegment *s = click_track_get_segment_at_pos(tt, tt->tl->play_pos_sframes);
    if (!s) s = tt->segments;
    click_track_populate_settings_internal(s, tv, true);
}

void timeline_click_track_edit(Timeline *tl)
{
    Session *session = session_get();
    ClickTrack *tt = timeline_selected_click_track(tl);
    if (!tt) return;

    TabView *tv = tabview_create("Click track settings", session->gui.layout, main_win);
    click_track_populate_settings_tabview(tt, tv);

    tabview_activate(tv);
    tl->needs_redraw = true;    
}
