#include "color.h"
#include "consts.h"
#include "panel.h"
#include "session.h"
#include "waveform.h"
#include "userfn.h"

extern Window *main_win;
extern struct colors colors;

static int quickref_add_track(void *self_v, void *target)
{
    user_tl_add_track(NULL);
    return 0;
}

static int quickref_record(void *self_v, void *target)
{
    user_tl_record(NULL);
    return 0;
}

static int quickref_left(void *self_v, void *target)
{
    user_tl_move_left(NULL);
    return 0;
}

static int quickref_right(void *self_v, void *target)
{
    user_tl_move_right(NULL);
    return 0;
}

static int quickref_rewind(void *self_v, void *target)
{
    user_tl_rewind(NULL);
    return 0;
}


static int quickref_pause(void *self_v, void *target)
{
    user_tl_pause(NULL);
    return 0;
}

static int quickref_play(void *self_v, void *target)
{
    user_tl_play(NULL);
    return 0;
}

static int quickref_next(void *self_v, void *target)
{
    user_tl_track_selector_down(NULL);
    return 0;
}

static int quickref_previous(void *self_v, void *target)
{
    user_tl_track_selector_up(NULL);
    return 0;
}

static int quickref_zoom_in(void *self_v, void *target)
{
    user_tl_zoom_in(NULL);
    return 0;
}

static int quickref_zoom_out(void *self_v, void *target)
{
    user_tl_zoom_out(NULL);
    return 0;
}

static int quickref_open_file(void *self_v, void *target)
{
    user_global_open_file(NULL);
    return 0;
}

static int quickref_save(void *self_v, void *target)
{
    user_global_save_project(NULL);
    return 0;
}

static int quickref_export_wav(void *self_v, void *target)
{
    user_tl_write_mixdown_to_wav(NULL);
    return 0;
}

static int quickref_track_settings(void *self_v, void *target)
{
    user_tl_track_open_settings(NULL);
    return 0;
}

static Layout *create_quickref_button_lt(Layout *row)
{
    Layout *ret = layout_add_child(row);
    ret->h.type = SCALE;
    ret->h.value = 0.8f;
    ret->x.type = STACK;
    ret->x.value = 10.0f;
    return ret;
}

typedef int (*ComponentFn)(void *self, void *target);


int set_output_compfn(void *self, void *target)
{
    session_set_default_out(NULL);
    return 0;
}
static inline void session_init_output_panel(Page *output, Session *session)
{

    PageElParams p;
    p.textbox_p.win = output->win;
    p.textbox_p.font = main_win->bold_font;
    p.textbox_p.text_size = 12;
    p.textbox_p.set_str = "Default Out:";

    PageEl *el = page_add_el(
	output,
	EL_TEXTBOX,
	p,
	"panel_out_label",
	"default_out_label");
    /* textbox_set_align(proj->tb_out_label, CENTER_LEFT); */
    /* textbox_set_background_color(proj->tb_out_label, &colors.clear); */
    /* textbox_set_text_color(proj->tb_out_label, &colors.white); */

    textbox_set_align((Textbox *)el->component, CENTER_LEFT);
    p.button_p.text_color = &colors.white;
    p.button_p.background_color = &colors.quickref_button_blue;
    p.button_p.text_size = 12;
    p.button_p.font = main_win->std_font;
    p.button_p.set_str = (char *)session->audio_io.playback_conn->name;
    p.button_p.win = output->win;
    p.button_p.target = NULL;
    p.button_p.action = set_output_compfn;
    el = page_add_el(
	output,
	EL_BUTTON,
	p,
	"panel_out_value",
	"default_out_value");
    textbox_set_trunc(((Button *)el->component)->tb, true);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    
    void **output_L, **output_R;
    output_L = (void *)&(session->proj.output_L);
    output_R = (void *)&(session->proj.output_R);
    p.waveform_p.background_color = &colors.black;
    p.waveform_p.plot_color = &colors.white;
    p.waveform_p.num_channels = 1;
    p.waveform_p.len = DEFAULT_FOURIER_LEN_SFRAMES;
    p.waveform_p.type = JDAW_FLOAT;
    p.waveform_p.channels = output_L;
    page_add_el(
	output,
	EL_WAVEFORM,
	p,
	"panel_out_wav_left",
	"out_waveform_left");

    p.waveform_p.channels = output_R;
    page_add_el(
	output,
	EL_WAVEFORM,
	p,
	"panel_out_wav_right",
	"out_waveform_right");
    
    
        /* textbox_size_to_fit(proj->tb_out_value, 2, 1); */
    /* textbox_set_fixed_w(proj->tb_out_value, saved_w - 10); */
    /* textbox_set_border(proj->tb_out_value, &colors.black, 1); */

    
    /* proj->tb_out_value = textbox_create_from_str( */
    /* 	(char *)session->audio_io.playback_conn->name, */
    /* 	out_value_lt, */
    /* 	main_win->std_font, */
    /* 	12, */
    /* 	main_win); */
    /* textbox_set_align(proj->tb_out_value, CENTER_LEFT); */
    /* proj->tb_out_value->corner_radius = 6; */
    /* /\* textbox_set_align(proj->tb_out_value, CENTER); *\/ */
    /* int saved_w = proj->tb_out_value->layout->rect.w / main_win->dpi_scale_factor; */
    /* textbox_size_to_fit(proj->tb_out_value, 2, 1); */
    /* textbox_set_fixed_w(proj->tb_out_value, saved_w - 10); */
    /* textbox_set_border(proj->tb_out_value, &colors.black, 1); */

    /* Layout *output_l_lt, *output_r_lt; */
    /* output_l_lt = layout_get_child_by_name_recursive(session->gui.layout, "out_waveform_left"); */
    /* output_r_lt = layout_get_child_by_name_recursive(session->gui.layout, "out_waveform_right"); */

    /* proj->outwav_l_rect = &output_l_lt->rect; */
    /* proj->outwav_r_rect = &output_r_lt->rect; */

}

static inline void session_init_quickref_panels(Page *quickref1, Page *quickref2)
{
    PageElParams p;
    /* Layout *quickref_lt = layout_read_from_xml(QUICKREF_LT_PATH); */
    /* layout_reparent(quickref_lt, quickref1->layout); */
    Layout *quickref_lt = quickref1->layout;
    Layout *row1 = quickref_lt->children[0];
    Layout *row2 = quickref_lt->children[1];
    Layout *row3 = quickref_lt->children[2];
    
    p.button_p.font = main_win->symbolic_font;
    p.button_p.background_color = &colors.quickref_button_blue;
    p.button_p.text_color = &colors.white;
    p.button_p.set_str = "C-t (add track)";
    p.button_p.action = quickref_add_track;
    p.button_p.target = NULL;
    p.button_p.text_size = 14;
    /* q->add_track = create_button_from_params(button_lt, b); */
    
    PageEl *el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_add_track",
	row1,
	"add_track",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */

    p.button_p.action = quickref_record;
    p.button_p.set_str = "r ⏺";
    /* q->record = create_button_from_params(button_lt, b); */
    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_record",
	row1,
	"record",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */

    /* /\* ROW 2 *\/ */
    /* button_lt = create_quickref_button_lt(row2); */
    p.button_p.font = main_win->mono_font;
    p.button_p.action = quickref_left;
    p.button_p.set_str = "h ←";
    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_left",
	row2,
	"left",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
    /* q->left = create_button_from_params(button_lt, b); */

    /* button_lt = create_quickref_button_lt(row2); */
    p.button_p.font = main_win->symbolic_font;
    p.button_p.action = quickref_rewind;
    p.button_p.set_str = "j ⏴";
    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_rewind",
	row2,
	"rewind",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
    /* q->rewind = create_button_from_params(button_lt, b); */

    /* button_lt = create_quickref_button_lt(row2); */
    p.button_p.action = quickref_pause;
    p.button_p.set_str = "k ⏸";
    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_pause",
	row2,
	"pause",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
    /* q->pause = create_button_from_params(button_lt, b); */

    /* button_lt = create_quickref_button_lt(row2); */
    p.button_p.action = quickref_play;
    p.button_p.set_str = "l ⏵";

    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_play",
	row2,
	"play",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
    /* Q->play = create_button_from_params(button_lt, b); */

    /* button_lt = create_quickref_button_lt(row2); */
    p.button_p.font = main_win->mono_font;
    p.button_p.action = quickref_right;
    p.button_p.set_str = "; →";

    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_right",
	row2,
	"right",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
    /* q->right = create_button_from_params(button_lt, b); */


    /* /\* ROW 3 *\/ */

    /* button_lt = create_quickref_button_lt(row3); */
    p.button_p.action = quickref_next;
    p.button_p.set_str = "n ↓";

    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_next",
	row3,
	"next",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
    /* q->next = create_button_from_params(button_lt, b); */

    /* button_lt = create_quickref_button_lt(row3); */
    p.button_p.action = quickref_previous;
    p.button_p.set_str = "p ↑";

    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_previous",
	row3,
	"previous",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
    /* q->previous = create_button_from_params(button_lt, b); */


    /* button_lt = create_quickref_button_lt(row3); */
    p.button_p.font = main_win->symbolic_font;
    p.button_p.action = quickref_zoom_out;
    p.button_p.set_str = ", 🔍-";
    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_zoom_out",
	row3,
	"zoom_out",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
    /* q->zoom_out = create_button_from_params(button_lt, b); */

    /* button_lt = create_quickref_button_lt(row3); */
    p.button_p.action = quickref_zoom_in;
    p.button_p.set_str = ". 🔍+";
    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_zoom_in",
	row3,
	"zoom_in",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */

    
    /* q->zoom_in = create_button_from_params(button_lt, b); */

    page_reset(quickref1);
    layout_size_to_fit_children_h(row1, true, 0);
    layout_size_to_fit_children_h(row2, true, 0);
    layout_size_to_fit_children_h(row3, true, 0);

    /* COL2 */

    /* quickref_lt = layout_read_from_xml(QUICKREF_LT_PATH); */
    /* layout_reparent(quickref_lt, quickref2->layout); */
    quickref_lt = quickref2->layout;
    row1 = quickref_lt->children[0];
    row2 = quickref_lt->children[1];
    row3 = quickref_lt->children[2];
    
    /* p.button_p.font = main_win->symbolic_font; */
    /* p.button_p.background_color = &colors.quickref_button_blue; */
    /* p.button_p.text_color = &colors.white; */
    /* p.button_p.set_str = "C-t (add track)"; */
    /* p.button_p.action = quickref_add_track; */
    /* p.button_p.target = NULL; */
    /* p.button_p.text_size = 14; */


    /* button_lt = create_quickref_button_lt(row1); */
    /* /\* p.button_p.font = main_win->mono_bold_font; *\/ */
    p.button_p.action = quickref_open_file;
    p.button_p.text_size = 12;
    p.button_p.set_str = "Open file (C-o)";
    el = page_add_el_custom_layout(
	quickref2,
	EL_BUTTON,
	p,
	"panel_quickref_open_file",
	row1,
	"open_file",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */


    p.button_p.action = quickref_save;
    p.button_p.set_str = "Save (C-s)";
    el = page_add_el_custom_layout(
	quickref2,
	EL_BUTTON,
	p,
	"panel_quickref_save_file",
	row1,
	"save",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */


    p.button_p.action = quickref_export_wav;
    p.button_p.set_str = "Export wav (S-w)";
    
    el = page_add_el_custom_layout(
	quickref2,
	EL_BUTTON,
	p,
	"panel_quickref_export_wav",
	row2,
	"export_wav",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */

    p.button_p.action = quickref_track_settings;
    p.button_p.set_str = "Track settings (S-t)";

    el = page_add_el_custom_layout(
	quickref2,
	EL_BUTTON,
	p,
	"panel_quickref_track_settings",
	row3,
	"track_settings",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
	/* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
}


struct source_area_draw_arg {
    SDL_Rect *source_area_rect;
    SDL_Rect *source_clip_rect;
};

extern SDL_Color clip_ref_home_bckgrnd;
extern SDL_Color timeline_marked_bckgrnd;

static void source_area_draw(void *arg1, void *arg2)
{
    struct source_area_draw_arg *arg = (struct source_area_draw_arg *)arg1;
    SDL_Rect *source_clip_rect = arg->source_clip_rect;
    /* SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(source_mode_bckgrnd)); */
    /* SDL_RenderFillRect(main_win->rend, source_rect); */
    /* SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.white)); */
    /* SDL_RenderDrawRect(main_win->rend, source_rect); */
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(clip_ref_home_bckgrnd));
    SDL_RenderDrawRect(main_win->rend, source_clip_rect);
    Session *session = session_get();
    if (session->source_mode.src_clip) {
	SDL_RenderFillRect(main_win->rend, source_clip_rect);

	/* Draw src clip waveform */
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.black));
	uint8_t num_channels = session->source_mode.src_clip->channels;
	float *channels[num_channels];
	channels[0] = session->source_mode.src_clip->L;
	if (num_channels > 1) {
	    channels[1] = session->source_mode.src_clip->R;
	}
	waveform_draw_all_channels(channels, num_channels, session->source_mode.src_clip->len_sframes, source_clip_rect);
	/* draw_waveform_to_rect(session->src_clip->L, session->src_clip->len_sframes, session->source_clip_rect); */
	
	/* Draw play head line */
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.white));
	int ph_y = source_clip_rect->y;
	/* int tri_y = tl->session->ruler_rect->y; */
	int play_head_x = source_clip_rect->x + source_clip_rect->w * session->source_mode.src_play_pos_sframes / session->source_mode.src_clip->len_sframes;
	SDL_RenderDrawLine(main_win->rend, play_head_x, ph_y, play_head_x, ph_y + source_clip_rect->h);

	/* /\* Draw play head triangle *\/ */
	int tri_x1 = play_head_x;
	int tri_x2 = play_head_x;
	/* int ph_y = tl->rect.y; */
	for (int i=0; i<PLAYHEAD_TRI_H; i++) {
	    SDL_RenderDrawLine(main_win->rend, tri_x1, ph_y, tri_x2, ph_y);
	    ph_y -= 1;
	    tri_x2 += 1;
	    tri_x1 -= 1;
	}

	/* draw mark in */
	int in_x, out_x = -1;

	in_x = source_clip_rect->x + source_clip_rect->w * session->source_mode.src_in_sframes / session->source_mode.src_clip->len_sframes;
	int i_tri_x2 = in_x;
	ph_y = source_clip_rect->y;
	for (int i=0; i<PLAYHEAD_TRI_H; i++) {
	    SDL_RenderDrawLine(main_win->rend, in_x, ph_y, i_tri_x2, ph_y);
	    ph_y -= 1;
	    i_tri_x2 += 1;
	}

	/* draw mark out */

	out_x = source_clip_rect->x + source_clip_rect->w * session->source_mode.src_out_sframes / session->source_mode.src_clip->len_sframes;
	int o_tri_x2 = out_x;
	ph_y = source_clip_rect->y;
	for (int i=0; i<PLAYHEAD_TRI_H; i++) {
	    SDL_RenderDrawLine(main_win->rend, out_x, ph_y, o_tri_x2, ph_y);
	    ph_y -= 1;
	    o_tri_x2 -= 1;
	}
	if (in_x < out_x && out_x != 0) {
	    SDL_Rect in_out = (SDL_Rect) {in_x, source_clip_rect->y, out_x - in_x, source_clip_rect->h};
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(timeline_marked_bckgrnd));
	    SDL_RenderFillRect(main_win->rend, &(in_out));
	}
    }
}


static inline void session_init_source_area(Page *source_area, Session *session)
{
    PageElParams p;

    // source_clip_name
    p.textbox_p.font = main_win->std_font;
    p.textbox_p.text_size = 14;
    p.textbox_p.win = main_win;
    p.textbox_p.set_str = "(no source clip)";

    PageEl *el = page_add_el(
	source_area,
	EL_TEXTBOX,
	p,
	"panel_source_clip_name_tb",
	"source_clip_name");
    Textbox *tb = (Textbox *)el->component;
    textbox_set_align(tb, CENTER_LEFT);
    textbox_set_background_color(tb, NULL);
    /* textbox_set_text_color(session->source_name_tb,  */

    static struct source_area_draw_arg draw_arg;
    p.canvas_p.draw_fn = source_area_draw;
    p.canvas_p.draw_arg1 = &draw_arg;
    el = page_add_el(
	source_area,
	EL_CANVAS,
	p,
	"panel_source_canvas",
	"source_area");
    Layout *source_area_lt = el->layout;
    Layout *clip_lt = layout_get_child_by_name_recursive(source_area_lt, "source_clip");
    draw_arg.source_area_rect = &source_area_lt->rect;
    draw_arg.source_clip_rect = &clip_lt->rect;

}

static inline void session_init_output_spectrum(Page *output_spectrum, Session *session)
{
    
    double *arrays[] = {
	session->proj.output_L_freq,
	session->proj.output_R_freq
    };
    int steps[] = {1, 1};
    SDL_Color *plot_colors[] = {&colors.freq_L, &colors.freq_R, &colors.white};
    PageElParams p;
    p.freqplot_p.arrays = arrays;
    p.freqplot_p.steps = steps;
    p.freqplot_p.num_arrays = 2;
    p.freqplot_p.num_items = session->proj.fourier_len_sframes / 2;
    p.freqplot_p.colors = plot_colors;
    /* Project *saved_glob_proj = proj; */
    /* proj = proj_loc; /\* Not great *\/ */
    page_add_el(
	output_spectrum,
	EL_FREQ_PLOT,
	p,
	"panel_output_freqplot",
	"freqplot");
    /* proj=saved_glob_proj; /\* Not great *\/ */
}


void session_init_panels(Session *session)
{
    Layout *lt = layout_get_child_by_name_recursive(session->gui.layout, "panel_area");  
    PanelArea *pa = panel_area_create(lt, main_win);
    session->gui.panels = pa;
    panel_area_add_panel(pa);
    panel_area_add_panel(pa);
    panel_area_add_panel(pa);
    panel_area_add_panel(pa);

    Page *output_panel = panel_area_add_page(
	pa,
	"Output",
	OUTPUT_PANEL_LT_PATH,
	NULL,
	&colors.white,
	main_win);
    
    Page *quickref1 = panel_area_add_page(
	pa,
	"Quickref 1",
	QUICKREF_LT_PATH,
	NULL,
	&colors.white,
	main_win);
    Page *quickref2 = panel_area_add_page(
	pa,
	"Quickref 2",
	QUICKREF_LT_PATH,
	NULL,
	&colors.white,
	main_win);

    Page *source_area = panel_area_add_page(
	pa,
	"Sample source",
	SOURCE_AREA_LT_PATH,
	NULL,
	&colors.white,
	main_win);

    Page *output_spectrum = panel_area_add_page(
	pa,
	"Output spectrum",
	OUTPUT_SPECTRUM_LT_PATH,
	NULL,
	&colors.white,
	main_win);


    session_init_quickref_panels(quickref1, quickref2);
    session_init_output_panel(output_panel, session);
    session_init_source_area(source_area, session);
    session_init_output_spectrum(output_spectrum, session);
    panel_select_page(pa, 0, 0);
    panel_select_page(pa, 1, 1);
    panel_select_page(pa, 2, 2);
    panel_select_page(pa, 3, 3);
}

