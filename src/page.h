/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/


/*****************************************************************************************************************
    page.h

    * Define Page and TabView structs
    * Interface for creation of dynamic GUIs with accompanying Layout files
 *****************************************************************************************************************/


#ifndef JDAW_PAGE_H
#define JDAW_PAGE_H

#include "components.h"
#include "layout.h"
#include "textbox.h"
#include "value.h"

#define MAX_ELEMENTS 64
#define MAX_TABS 16

#define TAB_H 32
#define PAGE_R 14
#define TAB_R PAGE_R
#define TAB_H_SPACING 0

typedef enum page_el_type {
    EL_TEXTAREA,
    EL_TEXTBOX,
    EL_TEXTENTRY,
    EL_SLIDER,
    EL_RADIO,
    EL_TOGGLE,
    EL_WAVEFORM,
    EL_FREQ_PLOT,
    EL_BUTTON,
    EL_CANVAS
} PageElType;

typedef struct page_element {
    const char *id;
    PageElType type;
    void *component;
    Layout *layout;
} PageEl;

typedef struct page {
    const char *title;
    PageEl *elements[MAX_ELEMENTS];
    PageEl *selectable_els[MAX_ELEMENTS];
    uint8_t num_elements;
    uint8_t num_selectable;
    int selected_i;
    Layout *layout;
    SDL_Color *background_color;
    SDL_Color *text_color;
    Window *win;
} Page;

typedef struct tab_view {
    const char *title;
    Page *tabs[MAX_TABS];
    Textbox *labels[MAX_TABS];
    uint8_t num_tabs;
    uint8_t current_tab;
    Layout *layout;
    Window *win;
} TabView;

struct slider_params {
    void *value;
    ValType val_type;
    enum slider_orientation orientation;
    enum slider_style style;
    SliderStrFn *create_label_fn;
    ComponentFn action;
    void *target;
};

struct textbox_params {
    char *set_str;
    Font *font;
    uint8_t text_size;
    Window *win;
};

struct textarea_params {
    const char *value;
    Font *font;
    uint8_t text_size;
    SDL_Color color;
    Window *win;   
};

struct textentry_params {
    char *value_handle;
    int buf_len;
    Font *font;
    uint8_t text_size;
    int (*validation)(Text *txt, char input);
    int (*completion)(Text *txt, void *target);
    void *completion_target;
};

struct freqplot_params {
    double **arrays;
    int num_arrays;
    SDL_Color **colors;
    int *steps;
    int num_items;
};

struct button_params {
    char *set_str;
    Font *font;
    uint8_t text_size;
    ComponentFn action;
    void *target;
    Window *win;
    SDL_Color *background_color;
    SDL_Color *text_color;
};

struct radio_params {
    int text_size;
    SDL_Color *text_color;
    void *target;
    ComponentFn action;
    const char **item_names;
    uint8_t num_items;
};

struct toggle_params {
    bool *value;
    ComponentFn action;
    void *target;
};

struct waveform_params {
    void **channels;
    ValType type;
    uint8_t num_channels;
    uint32_t len;
    SDL_Color *background_color;
    SDL_Color *plot_color;
};

struct canvas_params {
    void (*draw_fn)(void *arg1, void *arg2);
    void *draw_arg1;
    void *draw_arg2;
};

typedef union page_el_params {
    struct slider_params slider_p;
    struct textbox_params textbox_p;
    struct textarea_params textarea_p;
    struct textentry_params textentry_p;
    struct freqplot_params freqplot_p;
    struct button_params button_p;
    struct toggle_params toggle_p;
    struct radio_params radio_p;
    struct waveform_params waveform_p;
    struct canvas_params canvas_p;
} PageElParams;



/* TabView methods */

TabView *tabview_create(const char *title, Layout *parent_lt, Window *win);
void tabview_destroy(TabView *tv);
void tabview_activate(TabView *tv);
void tabview_close(TabView *tv);
Page *tab_view_add_page(
    TabView *tv,
    const char *page_title,
    const char *layout_filepath,
    SDL_Color *background_color,
    SDL_Color *text_color,
    Window *win);

bool tabview_mouse_click(TabView *tv);
bool tabview_mouse_motion(TabView *tv);
void tabview_next_tab(TabView *tv);
void tabview_previous_tab(TabView *tv);

/* Page methods */

Page *page_create(
    const char *title,
    const char *layout_filepath,
    Layout *parent_lt,
    SDL_Color *background_color,
    SDL_Color *text_color,
    Window *win);

void page_destroy(Page *page);
void page_el_set_params(PageEl *el, PageElParams params, Page *page);

/* Add an element at an already-existing named layout, which must be a child of page layout */
PageEl *page_add_el(
    Page *page,
    PageElType type,
    PageElParams params,
    const char *id,
    const char *layout_name);

/* Add an element with custom logic to create its layout from a specified parent */
PageEl *page_add_el_custom_layout(
    Page *page,
    PageElType type,
    PageElParams params,
    const char *id,
    Layout *parent,
    const char *new_layout_name,
    Layout *(*create_layout_fn)(Layout *parent));
bool page_mouse_motion(Page *page, Window *win);
bool page_mouse_click(Page *page, Window *win);

/* Reset functions */

void page_reset(Page *page);
void tab_view_reset(TabView *tv);

/* Draw functions */

void page_draw(Page *page);
void tabview_draw(TabView *tv);

void page_activate(Page *page);
void page_close(Page *page);

void page_next_escape(Page *page);
void page_previous_escape(Page *page);
void page_next(Page *page);
void page_previous(Page *page);
void page_right(Page *page);
void page_left(Page *page);
void page_enter(Page *page);

PageEl *page_get_el_by_id(Page *page, const char *id);
void page_select_el_by_id(Page *page, const char *id);

void tabview_clear_all_contents(TabView *tv);

#endif
