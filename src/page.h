#ifndef JDAW_PAGE_H
#define JDAW_PAGE_H

#include "components.h"
#include "layout.h"
#include "textbox.h"
#include "value.h"

#define MAX_ELEMENTS 255
#define MAX_TABS 16

typedef enum page_el_type {
    EL_TEXTAREA,
    EL_TEXTBOX,
    EL_TEXTENTRY,
    EL_SLIDER,
    EL_RADIO,
    EL_TOGGLE,
    EL_PLOT,
    EL_FREQ_PLOT,
    EL_BUTTON
} PageElType;

typedef struct page_element {
    PageElType type;
    void *component;
    Layout *layout;
} PageEl;

typedef struct page {
    const char *title;
    PageEl *elements[MAX_ELEMENTS];
    uint8_t num_elements;
    Layout *layout;
    SDL_Color *background_color;
} Page;

typedef struct tab_view {
    const char *title;
    Page *tabs[MAX_TABS];
    Textbox *labels[MAX_TABS];
    uint8_t num_tabs;
    uint8_t current_tab;
    Layout *layout;
} TabView;

struct slider_params {
    Layout *layout;
    void *value;
    ValType val_type;
    enum slider_orientation orientation;
    enum slider_style style;
    SliderStrFn *fn;
};

struct textbox_params {
    Layout *layout;
    char *set_str;
    Font *font;
    uint8_t text_size;
    Window *win;
};

struct textarea_params {
    Layout *layout;
    const char *value;
    Font *font;
    uint8_t text_size;
    SDL_Color color;
    Window *win;
};

struct textentry_params {
    Layout *lt;
    char *init_val;
    Font *font;
    uint8_t text_size;
    int (*validation)(Text *txt, char input);
    int (*completion)(Text *txt);
};

struct freqplot_params {
    Layout *layout;
    double **arrays;
    int num_arrays;
    SDL_Color **colors;
    int *steps;
    int num_items;
};

struct button_params {
    Layout *layout;
    char *set_str;
    Font *font;
    uint8_t text_size;
    Window *win;
    void *(*action)(void *arg);
};

typedef union page_el_params {
    struct slider_params slider_p;
    struct textbox_params textbox_p;
    struct textarea_params textarea_p;
    struct textentry_params textentry_p;
    struct freqplot_params freqplot_p;
    struct button_params button_p;
} PageElParams;



/* TabView methods */

TabView *tab_view_create(const char *title, Layout *parent_lt);
void tab_view_destroy(TabView *tv);


/* Page methods */

Page *page_create(
    const char *title,
    const char *layout_filepath,
    SDL_Color *background_color);

void page_destroy(Page *page);
void page_el_set_params(PageEl *el, PageElParams params);
PageEl *page_add_el(
    Page *page,
    PageElType type,
    PageElParams params,
    const char *layout_name);


/* Reset functions */

void page_el_reset(PageEl *el);
void page_reset(Page *page);
void tab_view_reset(TabView *tv);

/* Draw functions */

void page_el_draw(PageEl *el);
void page_draw(Page *page);
void tab_view_draw(TabView *tv);

#endif
