#ifndef JDAW_PANEL_H
#define JDAW_PANEL_H

#include "page.h"

#define MAX_PANELS 8;

typedef struct panel_area PanelArea;

typedef struct panel {
    PanelArea *area;
    Layout *layout;
    Layout *content_layout;
    uint8_t current_page;
    Textbox *selector;
} Panel;

typedef struct panel_area {
    Layout *layout;
    Panel *panels[8];
    uint8_t num_panels;
    Page *pages[MAX_TABS];
    uint8_t num_pages;
    Window *win;
} PanelArea;

PanelArea *panel_area_create(Layout *lt, Window *win);
Panel *panel_area_add_panel(PanelArea *pa);

Page *panel_area_add_page(
    PanelArea *pa,
    const char *page_title,
    const char *layout_filepath,
    SDL_Color *background_color,
    SDL_Color *text_color,
    Window *win);
Page *panel_select_page(PanelArea *pa, uint8_t panel, uint8_t new_selection);
void panel_insert_page(PanelArea *pa, uint8_t dst_panel_i, uint8_t page_i);
void panel_area_draw(PanelArea *panel);

bool panel_area_mouse_click(PanelArea *pa);
bool panel_area_mouse_motion(PanelArea *pa);

void panel_area_destroy(PanelArea *pa);

PageEl *panel_area_get_el_by_id(PanelArea *pa, const char *id);
void panel_page_refocus(PanelArea *pa, const char *page_title, uint8_t refocus_panel);

#endif

