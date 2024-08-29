#ifndef JDAW_PANEL_H
#define JDAW_PANEL_H

#include "page.h"

#define MAX_PANELS 8;

typedef struct panel_area PanelArea;

typedef struct panel {
    PanelArea *area;
    Layout *layout;
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
bool panel_triage_click(Panel *panel);
void panel_destroy(Panel *panel);
void panel_area_draw(PanelArea *panel);
bool panel_triage_click(Panel *panel);
bool panel_triage_motion(Panel *panel);

#endif
