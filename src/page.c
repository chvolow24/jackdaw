#include "color.h"
#include "page.h"
#include "layout.h"
#include "layout_xml.h"
#include "textbox.h"
#include "waveform.h"
/*

      switch (el->type) {
    case EL_TEXTAREA:
	break;
    case EL_TEXTBOX:
	break;
    case EL_TEXTENTRY:
	break;
    case EL_SLIDER:
	break;
    case EL_RADIO:
	break;
    case EL_TOGGLE:
	break;
    case EL_PLOT:
	break;
    case EL_FREQ_PLOT:
	break;
    case EL_BUTTON:
	break;
    default:
	break;
    }
*/
TabView *tab_view_create(const char *title, Layout *parent_lt, Window *win)
{
    TabView *tv = calloc(1, sizeof(Page));
    tv->title = title;
    Layout *tv_lt = layout_add_child(parent_lt);
    tv_lt->w.type = SCALE;
    tv_lt->h.type = SCALE;
    tv_lt->x.type = SCALE;
    tv_lt->y.type = SCALE;
    tv_lt->x.value.floatval = 0.05;
    tv_lt->y.value.floatval = 0.05;
    tv_lt->w.value.floatval = 0.9;
    tv_lt->h.value.floatval = 0.9;
    tv->layout = tv_lt;
    tv->win = win;
    return tv;
}

void tab_view_destroy(TabView *tv)
{
    for (uint8_t i=0; i<tv->num_tabs; i++) {
	page_destroy(tv->tabs[i]);
	textbox_destroy(tv->labels[i]);
    }
    free(tv);
}
/* void layout_write(FILE *f, Layout *lt); */

Page *page_create(const char *title, const char *layout_filepath, Layout *parent_lt, SDL_Color *background_color, Window *win)
{
    Page *page = calloc(1, sizeof(Page));
    page->title = title;
    page->background_color = background_color;
    Layout *page_lt = layout_read_from_xml(layout_filepath);
    page->layout = page_lt;
    layout_reparent(page_lt, parent_lt);
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
	break;
    case EL_SLIDER:
	slider_destroy((Slider *)el->component);
	break;
    case EL_RADIO:
	break;
    case EL_TOGGLE:
	break;
    case EL_PLOT:
	break;
    case EL_FREQ_PLOT:
	break;
    case EL_BUTTON:
	break;
    default:
	break;
    }
    free(el);
}
void page_destroy(Page *page)
{
    for (uint8_t i=0; i<page->num_elements; i++) {
	page_el_destroy(page->elements[i]);
    }
}

void page_el_set_params(PageEl *el, PageElParams params)
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
    case EL_TEXTBOX:
	el->component = (void *)textbox_create_from_str(
	    params.textbox_p.set_str,
	    el->layout,
	    params.textbox_p.font,
	    params.textbox_p.text_size,
	    params.textbox_p.win);
	break;
    case EL_TEXTENTRY:
	break;
    case EL_SLIDER:
	el->component = (void *)slider_create(
	    el->layout,
	    params.slider_p.value,
	    params.slider_p.val_type,
	    params.slider_p.orientation,
	    params.slider_p.style,
	    params.slider_p.fn);
	break;
    case EL_RADIO:
	break;
    case EL_TOGGLE:
	el->component = (void *)toggle_create(
	    el->layout,
	    params.toggle_p.value);
	break;
    case EL_PLOT:
	break;
    case EL_FREQ_PLOT:
	break;
    case EL_BUTTON:
	break;
    default:
	break;
    }
}

PageEl *page_add_el(
    Page *page,
    PageElType type,
    PageElParams params,
    const char *layout_name)
{
    PageEl *el = calloc(1, sizeof(PageEl));
    el->type = type;
    el->layout = layout_get_child_by_name_recursive(page->layout, layout_name);
    page_el_set_params(el, params);
    page->elements[page->num_elements] = el;
    page->num_elements++;
    return el;
}

static void page_el_reset(PageEl *el)
{
    switch (el->type) {
    case EL_TEXTAREA:
	break;
    case EL_TEXTBOX:
	break;
    case EL_TEXTENTRY:
	break;
    case EL_SLIDER:
	break;
    case EL_RADIO:
	break;
    case EL_TOGGLE:
	break;
    case EL_PLOT:
	break;
    case EL_FREQ_PLOT:
	break;
    case EL_BUTTON:
	break;
    default:
	break;
    }
}
void page_reset(Page *page)
{
    layout_reset(page->layout);
}

static bool page_element_mouse_motion(PageEl *el, Window *win)
{
    switch (el->type) {
    case EL_TEXTAREA:
	break;
    case EL_TEXTBOX:
	break;
    case EL_TEXTENTRY:
	break;
    case EL_SLIDER:
	return slider_mouse_motion((Slider *)el->component, win);
	break;
    case EL_RADIO:
	break;
    case EL_TOGGLE:
	break;
    case EL_PLOT:
	break;
    case EL_FREQ_PLOT:
	break;
    case EL_BUTTON:
	break;
    default:
	break;
    }
    return false;
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
	break;
    case EL_SLIDER:
	return slider_mouse_motion((Slider *)el->component, win);
	break;
    case EL_RADIO:
	break;
    case EL_TOGGLE:
	toggle_toggle((Toggle *)el->component);
	break;
    case EL_PLOT:
	break;
    case EL_FREQ_PLOT:
	break;
    case EL_BUTTON:
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
	break;
    case EL_SLIDER:
	slider_draw((Slider *)el->component);
	break;
    case EL_RADIO:
	break;
    case EL_TOGGLE:
	toggle_draw((Toggle *)el->component);
	break;
    case EL_PLOT:
	break;
    case EL_FREQ_PLOT:
	waveform_draw_freq_plot((struct freq_plot *)el->component);
	break;
    case EL_BUTTON:
	break;
    default:
	break;
    }
}

void page_draw(Page *page)
{
    /* fprintf(stdout, "page lt rect %d %d %d %d\n", page->layout->rect.x, page->layout->rect.y, page->layout->rect.w, page->layout->rect.h); */
    /* fprintf(stdout, "Page dims: %d %d %f %f\n", page->layout->x.value.intval, page->layout->y.value.intval, page->layout->w.value.floatval, page->layout->h.value.floatval); */
    SDL_SetRenderDrawColor(page->win->rend, sdl_colorp_expand(page->background_color));
    SDL_RenderFillRect(page->win->rend, &page->layout->rect);
    for (uint8_t i=0; i<page->num_elements; i++) {
	page_el_draw(page->elements[i]);
    }
    /* layout_draw(page->win, page->layout); */
}

void tab_view_draw(TabView *tv)
{

}



void page_activate(Page *page)
{
    Window *win = page->win;
    if (win->active_page) {
	page_destroy(win->active_page);
    }
    win->active_page = page;
}

void tab_view_activate(TabView *tv)
{
    Window *win = tv->win;
    if (win->active_tab_view) {
	tab_view_destroy(win->active_tab_view);
    }
    win->active_tab_view = tv;
}

void page_close(Page *page)
{
    page->win->active_page = NULL;
    page_destroy(page);
}
void tab_view_close(TabView *tv)
{
    tv->win->active_tab_view = NULL;
    tab_view_destroy(tv);
}

