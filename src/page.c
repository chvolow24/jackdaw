/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    page.c

    * Implementation of "Page" and "TabView" objects, for GUI unrelated to the timeline
 *****************************************************************************************************************/


#include "color.h"
#include "geometry.h"
#include "input.h"
#include "page.h"
#include "project.h"
#include "layout.h"
#include "layout_xml.h"
#include "textbox.h"
#include "value.h"
#include "waveform.h"


extern SDL_Color color_global_clear;
extern SDL_Color color_global_white;
extern SDL_Color color_global_black;

extern Window *main_win;
extern Project *proj;

TabView *tabview_create(const char *title, Layout *parent_lt, Window *win)
{
    TabView *tv = calloc(1, sizeof(Page));
    tv->title = title;
    Layout *tv_lt = layout_add_child(parent_lt);
    tv_lt->w.type = SCALE;
    tv_lt->h.type = SCALE;
    tv_lt->x.type = SCALE;
    tv_lt->y.type = SCALE;
    tv_lt->x.value = 0.1;
    tv_lt->y.value = 0.1;
    tv_lt->w.value = 0.8;
    tv_lt->h.value = 0.8;

    tv->layout = tv_lt;
    tv->win = win;
    Layout *tabs_container = layout_add_child(tv_lt);
    Layout *page_container = layout_add_child(tv_lt);
    tabs_container->w.type = SCALE;
    page_container->w.type = SCALE;
    tabs_container->w.value = 1.0f;
    page_container->w.value = 1.0f;
    tabs_container->h.value = TAB_H;
    page_container->y.value = TAB_H;
    page_container->h.type = COMPLEMENT;
    return tv;
}

Page *tab_view_add_page(
    TabView *tv,
    const char *page_title,
    const char *layout_filepath,
    SDL_Color *background_color,
    SDL_Color *text_color,
    Window *win)
{
    /* Second child of tab view layout is page container */
    Layout *page_lt = layout_add_child(tv->layout->children[1]);
    page_lt->w.type = SCALE;
    page_lt->h.type = SCALE;
    page_lt->w.value = 1.0f;
    page_lt->h.value = 1.0f;

    Page *page = page_create(
	page_title,
	layout_filepath,
	tv->layout->children[1],
	background_color,
	text_color,
	win);
    tv->tabs[tv->num_tabs] = page;
    Layout *tab_lt = layout_add_child(tv->layout->children[0]);
    tab_lt->x.type = STACK;
    tab_lt->y.value = 0;
    if (tv->num_tabs == 0) {
	tab_lt->x.value = TAB_H_SPACING + TAB_R * 2;
    } else {
	tab_lt->x.value = TAB_H_SPACING;
    }
    tab_lt->h.type = SCALE;
    tab_lt->h.value = 1.0f;

    /* ??? Problems */
    /* layout_force_reset(tv->layout); */

    layout_reset(tv->layout);


    
    Textbox *tab_tb = textbox_create_from_str((char *)page_title, tab_lt, tv->win->mono_bold_font, 14, tv->win);
    textbox_set_background_color(tab_tb, &color_global_clear);
    textbox_set_text_color(tab_tb, text_color);
    textbox_size_to_fit_h(tab_tb, 20);
    textbox_reset_full(tab_tb);
    tv->labels[tv->num_tabs] = tab_tb;
    tv->num_tabs++;
    return page;

}


void tabview_destroy(TabView *tv)
{
    for (uint8_t i=0; i<tv->num_tabs; i++) {
	page_destroy(tv->tabs[i]);
	textbox_destroy(tv->labels[i]);
    }
    free(tv);
}
/* void layout_write(FILE *f, Layout *lt); */

Page *page_create(
    const char *title,
    const char *layout_filepath,
    Layout *parent_lt,
    SDL_Color *background_color,
    SDL_Color *text_color,
    Window *win)
{
    Page *page = calloc(1, sizeof(Page));
    page->title = title;
    page->background_color = background_color;
    page->text_color = text_color;
    Layout *page_lt;
    if (layout_filepath) {
	page_lt = layout_read_from_xml(layout_filepath);
	if (!page_lt) {
	    fprintf(stderr, "Critical error: unable to read layout at %s\n", layout_filepath);
	}
    } else {
	page_lt = layout_create();
    }
    page->layout = page_lt;
    if (parent_lt) {
	layout_reparent(page_lt, parent_lt);
    }
    /* layout_write(stdout, page->layout, 0); */
    /* layout_reset(page->layout); */
    page->win = win;
    return page;
}

static void page_el_destroy(PageEl *el)
{
    switch (el->type) {
    case EL_TEXTAREA:
	txt_area_destroy((TextArea *)el->component);
	break;
    case EL_TEXTBOX:
	textbox_destroy((Textbox *)el->component);
	break;
    case EL_TEXTENTRY:
	textentry_destroy((TextEntry *)el->component);
	break;
    case EL_SLIDER:
	slider_destroy((Slider *)el->component);
	break;
    case EL_RADIO:
	radio_destroy((RadioButton *)el->component);
	break;
    case EL_TOGGLE:
	toggle_destroy((Toggle *)el->component);
	break;
    case EL_WAVEFORM:
	waveform_destroy((Waveform *)el->component);
	break;
    case EL_FREQ_PLOT:
	waveform_destroy_freq_plot((struct freq_plot *)el->component);
	break;
    case EL_BUTTON:
	button_destroy((Button *)el->component);
	break;
    case EL_CANVAS:
	canvas_destroy((Canvas *)el->component);
	break;
    default:
	break;
    }
    free(el);
}
void page_destroy(Page *page)
{
    if (page->win->txt_editing) {
	txt_stop_editing(page->win->txt_editing);
    }
    for (uint8_t i=0; i<page->num_elements; i++) {
	page_el_destroy(page->elements[i]);
    }
    layout_destroy(page->layout);
    free(page);
}

static void page_el_reset(PageEl *el)
{
    switch (el->type) {
    case EL_TEXTAREA:
	break;
    case EL_TEXTBOX:
	textbox_reset_full(el->component);
	break;
    case EL_TEXTENTRY:
	textentry_reset(el->component);
	break;
    case EL_SLIDER:
	slider_reset(el->component);
	break;
    case EL_RADIO:
	break;
    case EL_TOGGLE:
	break;
    case EL_WAVEFORM:
	break;
    case EL_FREQ_PLOT:
	waveform_reset_freq_plot((struct freq_plot *)el->component);
	break;
    case EL_BUTTON:
	textbox_reset_full(((Button *)el->component)->tb);
	break;
    default:
	break;
    }
}

static inline bool el_is_selectable(PageElType type)
{
    switch (type) {
    case EL_BUTTON:
    case EL_RADIO:
    case EL_SLIDER:
    case EL_TEXTENTRY:
    case EL_TOGGLE:
	return true;
    default:
	return false;
	
    }
}

void page_el_set_params(PageEl *el, PageElParams params, Page *page)
{
    if (el->component) {
	free(el->component);
    }
    switch (el->type) {
    case EL_TEXTAREA:
	el->component = (void *)txt_area_create(
	    params.textarea_p.value,
	    el->layout,
	    params.textarea_p.font,
	    params.textarea_p.text_size,
	    params.textarea_p.color,
	    params.textarea_p.win);
	break;
    case EL_TEXTBOX: {
	Textbox *tb = textbox_create_from_str(
	    params.textbox_p.set_str,
	    el->layout,
	    params.textbox_p.font,
	    params.textbox_p.text_size,
	    params.textbox_p.win);
	textbox_set_background_color(tb, NULL);
	textbox_set_text_color(tb, page->text_color);
	el->component = (void *)tb;
	    }
	break;
	
    case EL_TEXTENTRY:
	el->component = (void *)textentry_create(
	    el->layout,
	    params.textentry_p.value_handle,
	    params.textentry_p.buf_len,
	    params.textentry_p.font,
	    params.textentry_p.text_size,
	    params.textentry_p.validation,
	    params.textentry_p.completion,
	    params.textentry_p.completion_target,
	    page->win);
	break;
    case EL_SLIDER:
	el->component = (void *)slider_create(
	    el->layout,
	    params.slider_p.ep,
	    params.slider_p.min,
	    params.slider_p.max,
	    params.slider_p.orientation,
	    params.slider_p.style,
	    params.slider_p.create_label_fn,
	    /* el->layout, */
	    /* params.slider_p.value, */
	    /* params.slider_p.val_type, */
	    /* params.slider_p.orientation, */
	    /* params.slider_p.style, */
	    /* params.slider_p.create_label_fn, */
	    /* params.slider_p.action, */
	    /* params.slider_p.target, */
	    &proj->dragged_component);
	break;
    case EL_RADIO:
	el->component = (void *)radio_button_create(
	    el->layout,
	    params.radio_p.text_size,
	    params.radio_p.text_color,
	    params.radio_p.ep,
	    /* params.radio_p.target, */
	    /* params.radio_p.action, */
	    params.radio_p.item_names,
	    params.radio_p.num_items);
	break;
    case EL_TOGGLE:
	el->component = (void *)toggle_create(
	    el->layout,
	    params.toggle_p.value,
	    params.toggle_p.action,
	    params.toggle_p.target);
	break;
    case EL_WAVEFORM:
	el->component = (void *)waveform_create(
	    el->layout,
	    params.waveform_p.channels,
	    params.waveform_p.type,
	    params.waveform_p.num_channels,
	    params.waveform_p.len,
	    params.waveform_p.background_color,
	    params.waveform_p.plot_color);
	break;
    case EL_FREQ_PLOT:
	el->component = (void *)waveform_create_freq_plot(
	    params.freqplot_p.arrays,
	    params.freqplot_p.num_arrays,
	    params.freqplot_p.colors,
	    params.freqplot_p.steps,
	    params.freqplot_p.num_items,
	    el->layout);
	break;
    case EL_BUTTON:
	el->component = (void *)button_create(
	    el->layout,
	    params.button_p.set_str,
	    params.button_p.action,
	    params.button_p.target,
	    params.button_p.font,
	    params.button_p.text_size,
	    params.button_p.text_color,
	    params.button_p.background_color);
	break;
    case EL_CANVAS:
	el->component = (void *)canvas_create(
	    el->layout,
	    params.canvas_p.draw_fn,
	    params.canvas_p.draw_arg1,
	    params.canvas_p.draw_arg2);
	break;
    default:
	break;
    }
    page_el_reset(el);
}

PageEl *page_add_el(
    Page *page,
    PageElType type,
    PageElParams params,
    const char *id,
    const char *layout_name)
{
    PageEl *el = calloc(1, sizeof(PageEl));
    el->id = id;
    el->type = type;
    if (layout_name) {
	el->layout = layout_get_child_by_name_recursive(page->layout, layout_name);
	if (!el->layout) {
	    fprintf(stdout, "Error in layout at %s; unable to find child named %s\n", page->layout->name, layout_name);
	}
    } else {
	el->layout = layout_add_child(page->layout);
    }
    page_el_set_params(el, params, page);
    page->elements[page->num_elements] = el;
    page->num_elements++;
    if (el_is_selectable(type)) {
	page->selectable_els[page->num_selectable] = el;
	page->num_selectable++;
    }
    page_el_reset(el);
    return el;
}

PageEl *page_add_el_custom_layout(
    Page *page,
    PageElType type,
    PageElParams params,
    const char *id,
    Layout *parent,    
    const char *new_layout_name,
    Layout *(*create_layout_fn)(Layout *parent))
{
    Layout *lt = create_layout_fn(parent);
    layout_set_name(lt, new_layout_name);
    return page_add_el(
	page,
	type,
	params,
	id,
	new_layout_name);
}


void page_reset(Page *page)
{
    layout_force_reset(page->layout);
    for (uint8_t i=0; i<page->num_elements; i++) {
	page_el_reset(page->elements[i]);
    }
    /* if (page->selectable_els[page->selected_i]->type == EL_TEXTENTRY) { */
    /* 	textentry_edit(page->selectable_els[page->selected_i]); */
    /* } */
}

static bool page_element_mouse_motion(PageEl *el, Window *win)
{
    /* switch (el->type) { */
    /* case EL_TEXTAREA: */
    /* 	break; */
    /* case EL_TEXTBOX: */
    /* 	break; */
    /* case EL_TEXTENTRY: */
    /* 	break; */
    /* case EL_SLIDER: { */
    /* 	Slider *s = (Slider *)el->component; */
    /* 	/\* Value saved = jdaw_val_from_ptr(s->value, s->val_type); *\/ */
    /* 	if (slider_mouse_motion(s, win)) { */
    /* 	    /\* if (!proj->currently_editing_slider) { *\/ */
    /* 	    /\* 	proj->currently_editing_slider = s; *\/ */
    /* 	    /\* 	proj->cached_slider_val = saved; *\/ */
    /* 	    /\* 	proj->cached_slider_type = SLIDER_TARGET_TRACK_EFFECT_PARAM; *\/ */
    /* 	    /\* } *\/ */
    /* 	    return true; */
    /* 	} */
    /* } */
    /* 	break; */
    /* case EL_RADIO: */
    /* 	break; */
    /* case EL_TOGGLE: */
    /* 	break; */
    /* case EL_WAVEFORM: */
    /* 	break; */
    /* case EL_FREQ_PLOT: */
    /* 	break; */
    /* case EL_BUTTON: */
    /* 	break; */
    /* default: */
    /* 	break; */
    /* } */
    /* return false; */
    return true;
}

static bool page_element_mouse_click(PageEl *el, Window *win)
{
    if (!SDL_PointInRect(&win->mousep, &el->layout->rect)) {
	return false;
    }
    switch (el->type) {
    case EL_TEXTAREA:
	break;
    case EL_TEXTBOX:
	break;
    case EL_TEXTENTRY:
	if (win->txt_editing) {
	    textentry_complete_edit((TextEntry *)el->component);
	} else {
	    textentry_edit((TextEntry *)el->component);
	}
	break;
    case EL_SLIDER:
	return slider_mouse_click((Slider *)el->component, win);
    case EL_RADIO:
	return radio_click((RadioButton *)el->component, win);
    case EL_TOGGLE:
	return toggle_click((Toggle *)el->component, win);
    case EL_WAVEFORM:
	break;
    case EL_FREQ_PLOT:
	break;
    case EL_BUTTON:
	return button_click((Button *)el->component, win);
	break;
    default:
	break;
    }
    return false;
}

bool page_mouse_motion(Page *page, Window *win)
{
    if (SDL_PointInRect(&win->mousep, &page->layout->rect)) {
	for (uint8_t i=0; i<page->num_elements; i++) {
	    if (page_element_mouse_motion(page->elements[i], win)) {
		return true;
	    }
	}
	if (win->i_state & I_STATE_MOUSE_L) return true;
    }
    return false;
}

bool page_mouse_click(Page *page, Window *win)
{
    if (SDL_PointInRect(&win->mousep, &page->layout->rect)) {
	for (uint8_t i=0; i<page->num_elements; i++) {
	    if (page_element_mouse_click(page->elements[i], win)) {
		return true;
	    }
	}
    }
    return false;

}

void tab_view_reset(TabView *tv)
{
    layout_reset(tv->layout);
}

bool tabview_mouse_click(TabView *tv)
{
    if (SDL_PointInRect(&tv->win->mousep, &tv->layout->children[0]->rect)) {
	for (uint8_t i=0; i<tv->num_tabs; i++) {
	    Textbox *tb = tv->labels[i];
	    if (SDL_PointInRect(&tv->win->mousep, &tb->layout->rect)) {
		tv->current_tab = i;
		tab_view_reset(tv);
		return true;
	    }
	}
    } else if (SDL_PointInRect(&tv->win->mousep, &tv->layout->children[1]->rect)) {
	page_mouse_click(tv->tabs[tv->current_tab], tv->win);
	return true;
    }
    return false;	    
}

bool tabview_mouse_motion(TabView *tv)
{
    /* if (SDL_PointInRect(&tv->win->mousep, &tv->layout->children[1]->rect)) { */
    if (SDL_PointInRect(&tv->win->mousep, &tv->layout->children[1]->rect)) {
	return page_mouse_motion(tv->tabs[tv->current_tab], tv->win);
    }
    /* } */
    return false;    
}


static void page_el_draw(PageEl *el)
{
    switch (el->type) {
    case EL_TEXTAREA:
	txt_area_draw((TextArea *)el->component);
	break;
    case EL_TEXTBOX:
	textbox_draw((Textbox *)el->component);
	break;
    case EL_TEXTENTRY:
	textentry_draw((TextEntry *)el->component);
	break;
    case EL_SLIDER:
	slider_draw((Slider *)el->component);
	break;
    case EL_RADIO:
	radio_button_draw((RadioButton *)el->component);
	break;
    case EL_TOGGLE:
	toggle_draw((Toggle *)el->component);
	break;
    case EL_WAVEFORM:
	waveform_draw((Waveform *)el->component);
	break;
    case EL_FREQ_PLOT:
	waveform_draw_freq_plot((struct freq_plot *)el->component);
	break;
    case EL_BUTTON:
	button_draw((Button *)el->component);
	break;
    case EL_CANVAS:
	canvas_draw((Canvas *)el->component);
	break;
    default:
	break;
    }
}

void page_draw(Page *page)
{
    /* fprintf(stdout, "page lt rect %d %d %d %d\n", page->layout->rect.x, page->layout->rect.y, page->layout->rect.w, page->layout->rect.h); */
    /* fprintf(stdout, "Page dims: %d %d %f %f\n", page->layout->x.value.intval, page->layout->y.value.intval, page->layout->w.value, page->layout->h.value); */
    if (page->background_color) {
	SDL_SetRenderDrawColor(page->win->rend, sdl_colorp_expand(page->background_color));
	SDL_Rect temp = page->layout->rect;
	int r = PAGE_R * page->win->dpi_scale_factor;
	geom_fill_rounded_rect(page->win->rend, &temp, r);

	int brdr = 7 * page->win->dpi_scale_factor;
	int brdrtt = 2 * brdr;
	temp.x += brdr;
	temp.y += brdr;
	temp.w -= brdrtt;
	temp.h -= brdrtt;
	/* static SDL_Color brdrclr = {25, 25, 25, 255}; */

	SDL_SetRenderDrawColor(page->win->rend, sdl_color_expand(color_global_black));
	geom_draw_rounded_rect_thick(page->win->rend, &temp, 7, TAB_R * page->win->dpi_scale_factor, page->win->dpi_scale_factor);
    }
    for (uint8_t i=0; i<page->num_elements; i++) {
	page_el_draw(page->elements[i]);
	if (page->selected_i >= 0 && page->elements[i] == page->selectable_els[page->selected_i]) {
	    SDL_SetRenderDrawColor(page->win->rend, 255, 200, 10, 255);
	    SDL_Rect r = page->elements[i]->layout->rect;
	    geom_draw_rect_thick(page->win->rend, &r, 2, 2);
	}
    }
    /* static bool sf = false; */
    /* layout_draw(page->win, page->layout); */
    /* if (!sf) { */
    /* 	FILE *f = fopen("test.xml", "w"); */
    /* 	layout_write(f, page->layout, 0); */
    /* } */
}

static inline void tabview_draw_inner(TabView *tv, uint8_t i)
{
    Page *page = tv->tabs[i];
    Textbox *tb = tv->labels[i];
    SDL_SetRenderDrawColor(tv->win->rend, sdl_colorp_expand(page->background_color));
    geom_fill_tab(tv->win->rend, &tb->layout->rect, TAB_R, tv->win->dpi_scale_factor);
    textbox_draw(tb);
    SDL_SetRenderDrawColor(tv->win->rend, 160, 160, 160, 255);
    geom_draw_rounded_rect(tv->win->rend, &page->layout->rect, TAB_R * tv->win->dpi_scale_factor);
    geom_draw_tab(tv->win->rend, &tb->layout->rect, TAB_R, tv->win->dpi_scale_factor);
    SDL_SetRenderDrawColor(tv->win->rend, sdl_colorp_expand(page->background_color));
    int left_x = tb->layout->rect.x - TAB_R * tv->win->dpi_scale_factor;
    int right_x = left_x + tb->layout->rect.w + 2 * TAB_R * tv->win->dpi_scale_factor;
    int y = tb->layout->rect.y + tb->layout->rect.h;
    SDL_RenderDrawLine(tv->win->rend, left_x, y, right_x - 1, y);
}

void tabview_draw(TabView *tv)
{
    for (uint8_t i=tv->num_tabs - 1; i>tv->current_tab; i--) {
        tabview_draw_inner(tv, i);

    }
    for (uint8_t i=0; i<=tv->current_tab; i++) {
	if (i == tv->current_tab) {
	    page_draw(tv->tabs[i]);
	}
	tabview_draw_inner(tv, i);
    }
    /* layout_draw(main_win, tv->layout); */

    /* page = tv->tabs[tv->current_tab]; */
    /* tb = tv->labels[tv->current_tab]; */
    /* SDL_SetRenderDrawColor(tv->win->rend, sdl_colorp_expand(page->background_color)); */
    /* geom_fill_tab(tv->win->rend, &tb->layout->rect, TAB_R, tv->win->dpi_scale_factor); */
    /* textbox_draw(tb); */
    /* page_draw(page); */
}

/* page_create(const char *title, const char *layout_filepath, Layout *parent_lt, SDL_Color *background_color, Window *win) -> Page * */

static void page_el_select(PageEl *el)
{
    if (!el) return;
    if (el->type == EL_TEXTENTRY) {
	textentry_edit((TextEntry *)el->component);
    }
}
static void page_el_deselect(PageEl *el)
{
    if (!el) return;
    if (el->type == EL_TEXTENTRY) {
	textentry_complete_edit((TextEntry *)el->component);
    }
}

static void tabview_select_el(TabView *tv)
{
    Page *current = tv->tabs[tv->current_tab];
    if (current->num_selectable == 0) return;
    page_el_select(current->selectable_els[current->selected_i]);   
}
static void tabview_deselect_el(TabView *tv)
{
    Page *current = tv->tabs[tv->current_tab];
    if (current->num_selectable == 0) return;
    page_el_deselect(current->selectable_els[current->selected_i]);
}

void page_activate(Page *page)
{
    Window *win = page->win;
    if (win->active_page) {
	page_destroy(win->active_page);
    }
    win->active_page = page;
}

void tabview_activate(TabView *tv)
{
    Window *win = tv->win;
    if (win->num_modals  > 0) {
	window_pop_modal(win);
    }
    while (win->num_menus > 0) {
	window_pop_menu(win);
    }
    if (win->active_tabview) {
	tabview_destroy(win->active_tabview);
    }
    
    win->active_tabview = tv;
    window_push_mode(tv->win, TABVIEW);

    tabview_select_el(tv);
    /* Page *current = tv->tabs[tv->current_tab]; */
    /* page_el_select(current->selectable_els[current->selected_i]); */
}

void page_close(Page *page)
{
    page->win->active_page = NULL;
    page_destroy(page);
}
void tabview_close(TabView *tv)
{
    tabview_deselect_el(tv);
    while (tv->win->num_menus > 0) {
	window_pop_menu(tv->win);
    }
    window_pop_mode(tv->win);
    tv->win->active_tabview = NULL;
    tabview_destroy(tv);
}

void tabview_next_tab(TabView *tv)
{
    tabview_deselect_el(tv);
    if (tv->current_tab < tv->num_tabs - 1)
	tv->current_tab++;
    else tv->current_tab = 0;
    tabview_select_el(tv);
}

void tabview_previous_tab(TabView *tv)
{
    tabview_deselect_el(tv);
    if (tv->current_tab > 0)
	tv->current_tab--;
    else tv->current_tab = tv->num_tabs - 1;
    tabview_select_el(tv);
}

/* NAVIGATION FUNCTIONS */

/* static void check_move_off_textentry(Page *page) */
/* { */
/*     PageEl *from = page->selectable_els[page->selected_i]; */
/*     if (from->type == EL_TEXTENTRY) { */
/* 	textentry_complete_edit((TextEntry *)from->component); */
/*     } */
/* } */

/* static void check_move_to_textentry(Page *page) */
/* { */
/*     PageEl *to = page->selectable_els[page->selected_i]; */
/*     if (to->type == EL_TEXTENTRY) { */
/* 	textentry_edit((TextEntry *)to->component); */
/*     } */
/* } */
    
void page_next_escape(Page *page)
{
    page_el_deselect(page->selectable_els[page->selected_i]);
    /* check_move_off_textentry(page); */
    if (page->selected_i < page->num_selectable - 1)
	page->selected_i++;
    else page->selected_i = 0;
    page_el_select(page->selectable_els[page->selected_i]);
}

void page_previous_escape(Page *page)
{
    page_el_deselect(page->selectable_els[page->selected_i]);
    if (page->selected_i > 0)
	page->selected_i--;
    else page->selected_i = page->num_selectable - 1;
    page_el_select(page->selectable_els[page->selected_i]);
}

void page_enter(Page *page)
{
    PageEl *el = page->selectable_els[page->selected_i];
    if (!el) return;
    switch (el->type) {
    case EL_TOGGLE:
	toggle_toggle((Toggle *)el->component);
	break;
    case EL_RADIO:
	radio_cycle((RadioButton *)el->component);
	break;
    case EL_BUTTON: {
	Button *b = (Button *)el->component;
	b->action(b, b->target);
	break;
    }
    case EL_TEXTENTRY:
	break;
    default:
	break;
    }
}
void page_next(Page *page)
{

}
void page_previous(Page *page)
{

}

void page_right(Page *page)
{
    PageEl *el = page->selectable_els[page->selected_i];
    if (!el) return;
    switch (el->type) {
    case EL_SLIDER:
	slider_nudge_right((Slider *)el->component);
	break;
    default:
	break;
    }
}

void page_left(Page *page)
{
    PageEl *el = page->selectable_els[page->selected_i];
    if (!el) return;
    switch (el->type) {
    case EL_SLIDER:
	slider_nudge_left((Slider *)el->component);
	break;
    default:
	break;
    }
}

PageEl *page_get_el_by_id(Page *page, const char *id)
{
    PageEl *el = NULL;
    for (uint8_t i=0; i<page->num_elements; i++) {
	el = page->elements[i];
	if (strcmp(el->id, id) == 0) break;
	else if (i == page->num_elements - 1) el = NULL;
    }
    return el;
}

void page_select_el_by_id(Page *page, const char *id)
{
    if (page->num_selectable == 0) return;
    while (strcmp(page->selectable_els[page->selected_i]->id, id) != 0) {
	page->selected_i++;
	if (page->selected_i >= page->num_selectable) {
	    page->selected_i = 0;
	    break;
	}
    }
}


void tabview_clear_all_contents(TabView *tv)
{
    for (int i=0; i<tv->num_tabs; i++) {
	page_destroy(tv->tabs[i]);
	textbox_destroy(tv->labels[i]);
    }
    tv->num_tabs = 0;
    /* tv->current_tab = 0; */
}


    
