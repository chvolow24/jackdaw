/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "color.h"
#include "menu.h"
#include "panel.h"
#include "layout.h"

#define PANEL_H_SPACING 10
#define PANEL_LABEL_PAD 6
/* #define PANEL_LABEL_DIVIDER_PAD (8 * pa->win->dpi_scale_factor) */

extern struct colors colors;

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
    char panel_lt_name[255];
    snprintf(panel_lt_name, 255, "panel_%d", pa->num_panels - 1);
    layout_set_name(panel_lt, panel_lt_name);
    panel_lt->h.type = SCALE;
    panel_lt->h.value = 1.0f;
    panel_lt->x.type = STACK;
    panel_lt->x.value = PANEL_H_SPACING;
    panel_lt->w.value = 200;

    /* panel_lt->h.value = 200; */
    p->layout = panel_lt;
    Layout *selector_lt = layout_add_child(panel_lt);
    /* selector_lt->w.value = 200; */
    /* selector_lt->h.value = 200; */
    layout_set_default_dims(selector_lt);
    layout_reset(selector_lt);
    p->selector = textbox_create_from_str(NULL, selector_lt, pa->win->mathematical_font, 12, pa->win);
    
    p->content_layout = layout_add_child(panel_lt);
    layout_set_name(p->content_layout, "content");
    p->content_layout->y.type = STACK;
    p->content_layout->y.value = 0;
    p->content_layout->h.type = COMPLEMENT;
    
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
    p->selected_i = -1;
    /* p->layout->w.type = SCALE; */
    /* p->layout->h.type = SCALE; */
    /* p->layout->w.value.floatval = 1.0f; */
    /* p->layout->h.value.floatval = 1.0f; */
    /* char layout_name[255]; */
    /* snprintf(layout_name, 255, "panel_page_%d", pa->num_pages); */
    /* layout_set_name(p->layout, layout_name); */
    pa->pages[pa->num_pages] = p;
    pa->num_pages++;
    return p;
}

Page *panel_select_page(PanelArea *pa, uint8_t panel_i, uint8_t new_selection)
{
    Panel *panel = pa->panels[panel_i];
    Layout *old_page_layout = pa->pages[panel->current_page]->layout;
    if (old_page_layout->parent) {
	layout_remove_child(old_page_layout);
    }
    panel->current_page = new_selection;
    Page *page = pa->pages[new_selection];
    if (page->layout->parent) {
	layout_remove_child(page->layout);
    }
    layout_reparent(page->layout, panel->content_layout);
    static char name[MAX_NAMELENGTH];
    snprintf(name, MAX_NAMELENGTH, "%s    ∨", page->title);
    textbox_set_value_handle(panel->selector, name);
    textbox_size_to_fit(panel->selector, 0, PANEL_LABEL_PAD);
    textbox_set_background_color(panel->selector, NULL);
    textbox_set_text_color(panel->selector, &colors.cerulean);
    textbox_reset_full(panel->selector);

    layout_force_reset(pa->layout);
    layout_size_to_fit_children_h(panel->content_layout->children[0], true, 0);
    layout_size_to_fit_children_h(panel->content_layout, true, 0);
    layout_size_to_fit_children_h(panel->layout, true, PANEL_H_SPACING);
    layout_force_reset(pa->layout);
    for (uint8_t i=0; i<pa->num_panels; i++) {
	page_reset(pa->pages[pa->panels[i]->current_page]);
    }

    layout_force_reset(pa->layout);

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
	    SDL_SetRenderDrawColor(pa->win->rend, sdl_color_expand(colors.cerulean_pale));
	    SDL_Rect rect = pa->panels[i]->layout->rect;
	    SDL_RenderDrawLine(pa->win->rend, rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h);
	}
	/* int y = pa->panels[i]->content_layout->rect.y - PANEL_LABEL_DIVIDER_PAD; */
	/* SDL_RenderDrawLine(pa->win->rend, rect.x, y, rect.x + rect.w, y); */
    }
}

void panel_insert_page(PanelArea *pa, uint8_t dst_panel_i, uint8_t page_i)
{
    if (pa->panels[dst_panel_i]->current_page == page_i) return;
    int current_pages[pa->num_panels];
    int destinations[pa->num_panels];
    for (uint8_t i=0; i<pa->num_panels; i++) {
	current_pages[i] = pa->panels[i]->current_page;
    }
    int offset = 0;
    for (uint8_t i=0; i<pa->num_panels; i++) {
	/* current_pages[i] = pa->panels[i]->current_page; */
	if (i == dst_panel_i) {
	    destinations[i] = page_i;
	    offset++;
	} else if (current_pages[i] == page_i) {
	    if (offset == 0) {
		offset--;
		destinations[i] = current_pages[i - offset];
	    } else {
		destinations[i] = current_pages[i - offset];
		offset--;
	    }
	    /* if (i - offset < pa->num_panels) { */
	    /* 	destinations[i] = current_pages[i - offset]; */
	    /* } */
	} else {
	    if (i - offset < pa->num_panels) {
		destinations[i] = current_pages[i - offset];
	    }

	    /* destinations[i] = current_pages[i - offset]; */
	}
	
    }
    for (uint8_t i=0; i<pa->num_panels; i++) {
	panel_select_page(pa, i, destinations[i]);
    }
}

struct pa_click_target {
    int panel_i;
    int page_i;
    PanelArea *pa;
};
void panel_selector_onclick(void *arg_v)
{
    struct pa_click_target *targ = (struct pa_click_target *)arg_v;
    /* Panel *swap_panel = NULL; */
    /* int swap_panel_i = -1; */
    /* for (uint8_t i=0; i<targ->pa->num_panels; i++) { */
    /* 	if (targ->pa->panels[i]->current_page == targ->page_i) { */
    /* 	    swap_panel_i = i; */
    /* 	} */
    /* } */
    /* if (swap_panel_i >= 0) { */
    /* 	panel_select_page(targ->pa, swap_panel_i, targ->pa->panels[targ->panel_i]->current_page); */
    /* } */
    /* panel_select_page(targ->pa, targ->panel_i, targ->page_i); */
    panel_insert_page(targ->pa, targ->panel_i, targ->page_i);
    window_pop_menu(targ->pa->win);
}

bool panel_area_mouse_click(PanelArea *pa)
{
    for (uint8_t i=0; i<pa->num_panels; i++) {
	Panel *p = pa->panels[i];
	/* fprintf(stdout, "TEST panel %d\n", i); */
	if (SDL_PointInRect(&pa->win->mousep, &p->layout->rect)) {
	    if (SDL_PointInRect(&pa->win->mousep, &p->selector->layout->rect)) {
		Menu *menu = menu_create_at_point(pa->win->mousep.x, pa->win->mousep.y);
		MenuColumn *c = menu_column_add(menu, "");
		MenuSection *sc = menu_section_add(c, "");
		for (int j=0; j<pa->num_pages; j++) {
		    Page *page = pa->pages[j];
		    struct pa_click_target *target = malloc(sizeof(struct pa_click_target));
		    target->panel_i = i;
		    target->page_i = j;
		    target->pa = pa;
		    /* fprintf(stdout, "Conn index: %d\n", conn->index); */
		    MenuItem *item = menu_item_add(
			sc,
			page->title,
			" ",
			panel_selector_onclick,
			target);
		    item->free_target_on_destroy = true;
		}
		menu_add_header(menu,"", "\n\n'n' to select next item; 'p' to select previous item.");
		/* menu_reset_layout(menu); */
		window_add_menu(pa->win, menu);
	    } else {
		Page *page = pa->pages[p->current_page];
		return page_mouse_click(page, pa->win);
	    }
	}
    }
    return false;
}

bool panel_area_mouse_motion(PanelArea *pa)
{
    for (uint8_t i=0; i<pa->num_panels; i++) {
	Panel *p = pa->panels[i];
	if (SDL_PointInRect(&pa->win->mousep, &p->layout->rect)) {
	    Page *page = pa->pages[p->current_page];
	    return page_mouse_motion(page, pa->win);
	}
    }
    return false;
}

static void panel_destroy(Panel *p)
{
    textbox_destroy(p->selector);
    free(p);
}

void panel_area_destroy(PanelArea *pa)
{
    for (uint8_t i=0; i<pa->num_panels; i++) {
	panel_destroy(pa->panels[i]);
    }
    for (uint8_t i=0; i<pa->num_pages; i++) {
	page_destroy(pa->pages[i]);
    }
    free(pa);
}

PageEl *panel_area_get_el_by_id(PanelArea *pa, const char *id)
{
    PageEl *el = NULL;
    for (uint8_t i=0; i<pa->num_pages; i++) {
	Page *p = pa->pages[i];
	el = page_get_el_by_id(p, id);
	if (el) break;
    }
    return el;
}

void panel_page_refocus(PanelArea *pa, const char *page_title, uint8_t refocus_panel)
{
    Page *p = NULL;
    uint8_t index;
    for (uint8_t i=0; i<pa->num_panels; i++) {
	Page *test = pa->pages[pa->panels[i]->current_page];
	if (strcmp(test->title, page_title) == 0) {
	    p = test;
	    index = pa->panels[i]->current_page;
	    break;
	}
    }
    if (!p) {
	for (uint8_t i=0; i<pa->num_pages; i++) {
	    Page *test = pa->pages[i];
	    if (strcmp(test->title, page_title) == 0) {
		index = i;
		break;
	    }
	}
    }
    if (!p || p->layout->rect.x + p->layout->rect.w > pa->win->w_pix) {
	panel_insert_page(pa, refocus_panel, index);
    }
}


/* WHEN panel selecting,
 - remove page layout from parent;
 - reparent page layout -> panel layout
*/
