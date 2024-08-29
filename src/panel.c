#include "color.h"
#include "panel.h"
#include "layout.h"

#define PANEL_H_SPACING 10


extern SDL_Color color_global_cerulean;

PanelArea *panel_area_create(Layout *lt, Window *win)
{
    PanelArea *pa = calloc(1, sizeof(PanelArea));
    pa->layout = lt;
    pa->win = win;
    return pa;
}

Panel *panel_area_add_panel(PanelArea *pa)
{
    Panel *p = calloc(1, sizeof(Panel));
    pa->panels[pa->num_panels] = p;
    pa->num_panels++;
    p->area = pa;
    Layout *panel_lt = layout_add_child(pa->layout);
    panel_lt->h.type = SCALE;
    panel_lt->h.value.floatval = 1.0f;
    panel_lt->x.type = STACK;
    panel_lt->x.value.intval = PANEL_H_SPACING;
    panel_lt->w.value.intval = 200;
    /* panel_lt->h.value.intval = 200; */
    p->layout = panel_lt;
    Layout *selector_lt = layout_add_child(panel_lt);
    layout_set_default_dims(selector_lt);
    layout_reset(selector_lt);
    p->selector = textbox_create_from_str(NULL, selector_lt, pa->win->mathematical_font, 12, pa->win);
    return p;
}

Page *panel_area_add_page(
    PanelArea *pa,
    const char *page_title,
    const char *layout_filepath,
    SDL_Color *background_color,
    SDL_Color *text_color,
    Window *win)
{
    Page *p = page_create(
	page_title,
	layout_filepath,
        NULL,
	background_color,
	text_color,
	pa->win);
    p->layout->w.type = SCALE;
    p->layout->h.type = SCALE;
    p->layout->w.value.floatval = 1.0f;
    p->layout->h.value.floatval = 1.0f;
    pa->pages[pa->num_pages] = p;
    pa->num_pages++;
    return p;
}

Page *panel_select_page(PanelArea *pa, uint8_t panel_i, uint8_t new_selection)
{
    Panel *panel = pa->panels[panel_i];
    panel->current_page = new_selection;
    Page *page = pa->pages[new_selection];
    if (page->layout->parent) {
	layout_remove_child(page->layout);
    }
    layout_reparent(page->layout, panel->layout);
    textbox_set_value_handle(panel->selector, page->title);
    textbox_size_to_fit(panel->selector, 0, 0);
    textbox_set_background_color(panel->selector, NULL);
    textbox_set_text_color(panel->selector, &color_global_cerulean);
    textbox_reset_full(panel->selector);
    return page;
}


static void panel_draw(Panel *p)
{
    PanelArea *pa = p->area;
    Page *page = pa->pages[p->current_page];
    page_draw(page);
    textbox_draw(p->selector);
}

void panel_area_draw(PanelArea *pa)
{
    for (uint8_t i=0; i<pa->num_panels; i++) {
	panel_draw(pa->panels[i]);
	if (i != pa->num_panels - 1) {
	    SDL_SetRenderDrawColor(pa->win->rend, sdl_color_expand(color_global_cerulean));
	    SDL_Rect rect = pa->panels[i]->layout->rect;
	    SDL_RenderDrawLine(pa->win->rend, rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h);
	}
    }
}


bool panel_area_mouse_click(PanelArea *pa)
{
    for (uint8_t i=0; i<pa->num_panels; i++) {
	Panel *p = pa->panels[i];
	if (SDL_PointInRect(&pa->win->mousep, &pa->layout->rect)) {
	    Page *page = pa->pages[p->current_page];
	    return page_mouse_click(page, pa->win);
	}
    }
    return false;
}

bool panel_area_mouse_motion(PanelArea *pa)
{
    for (uint8_t i=0; i<pa->num_panels; i++) {
	Panel *p = pa->panels[i];
	if (SDL_PointInRect(&pa->win->mousep, &pa->layout->rect)) {
	    Page *page = pa->pages[p->current_page];
	    return page_mouse_motion(page, pa->win);
	}
    }
    return false;
}


/* WHEN panel selecting,
 - remove page layout from parent;
 - reparent page layout -> panel layout
*/
