/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    page.h

    * Page and TabView GUI objects
    * TabView is a collection of Pages accessible via tabs
    * Pages created from stored layout xml files (assets/layouts)
 *****************************************************************************************************************/


#ifndef JDAW_PAGE_H
#define JDAW_PAGE_H

#include "components.h"
#include "eq.h"
#include "layout.h"
#include "textbox.h"
#include "value.h"

#define MAX_ELEMENTS 256
#define MAX_SELECTABLE 64
#define MAX_TABS 16

#define TAB_H 32
#define PAGE_R 14
#define TAB_R PAGE_R
#define TAB_MARGIN_LEFT (TAB_R * 2)
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
    EL_CANVAS,
    EL_EQ_PLOT,
    EL_SYMBOL_BUTTON,
    EL_SYMBOL_RADIO,
    EL_DROPDOWN,
    EL_STATUS_LIGHT,
    EL_PIANO
    /* EL_TOGGLE_EP */
} PageElType;

enum linked_obj_type {
    PAGE_EFFECT=0
};

typedef struct page_element {
    char *id; /* strdup the ID */
    PageElType type;
    void *component;
    Layout *layout;
} PageEl;

typedef struct page {
    const char *title;
    PageEl *elements[MAX_ELEMENTS];
    PageEl *selectable_els[MAX_SELECTABLE];
    uint8_t num_elements;
    uint8_t num_selectable;
    int selected_i;
    Layout *layout;
    SDL_Color *background_color;
    SDL_Color *text_color;
    Window *win;

    enum linked_obj_type linked_obj_type;
    void *linked_obj;
    bool onscreen;
} Page;

typedef struct tab_view {
    const char *title;
    Page *tabs[MAX_TABS];
    Textbox *labels[MAX_TABS + 2]; /* add space for ellipsis tabs */
    Textbox *ellipsis_left;
    Textbox *ellipsis_right;
    bool ellipsis_left_inserted;
    bool ellipsis_right_inserted;
    
    uint8_t num_tabs;
    uint8_t current_tab;
    Layout *layout;
    Layout *tabs_container;
    Window *win;

    uint8_t leftmost_index; /* Set in reset function */
    uint8_t rightmost_index; /* Calculated in reset function */

    /* void *related_array; */
    void (*swap_fn)(void *array, int swap_i, int swap_j);
    void *swap_fn_target;
    /* size_t related_array_el_size; */

    char *label_str;
    Textbox *label; /* Upper-right corner of screen */
} TabView;

struct slider_params {
    
    /* void *value; */
    /* ValType val_type; */
    Endpoint *ep;
    Value min;
    Value max;
    enum slider_orientation orientation;
    enum slider_style style;
    LabelStrFn create_label_fn;
    /* ComponentFn action; */
    /* void *target; */
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
    Endpoint *ep;
    /* void *target; */
    /* ComponentFn action; */
    const char **item_names;
    uint8_t num_items;
};

struct toggle_params {
    Endpoint *ep;
    /* bool *value; */
    /* ComponentFn action; */
    /* void *target; */
};

struct toggle_ep_params {
    Endpoint *ep;
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

struct eq_plot_params {
    EQ *eq;
};

struct symbol_button_params {
    int symbol_index;
    /* Symbol *s; */
    ComponentFn action;
    void *target;
    SDL_Color *background_color;
};

struct symbol_radio_params {
    int *symbol_indices;
    /* Symbol **symbols; */
    uint8_t num_items;
    Endpoint *ep;
    bool align_horizontal;
    int padding;
    SDL_Color *sel_color;
    SDL_Color *unsel_color;
};

struct dropdown_params {
    const char *header;
    char **item_names;
    char **item_annotations;
    void **item_args;
    uint8_t num_items;
    int *reset_from;
    int (*selection_fn)(Dropdown *, void *);
};

struct status_light_params {
    void *value;
    size_t val_size;
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
    struct eq_plot_params eq_plot_p;
    struct symbol_button_params sbutton_p;
    struct toggle_ep_params toggle_ep_p;
    struct symbol_radio_params sradio_p;
    struct dropdown_params dropdown_p;
    struct status_light_params slight_p;
} PageElParams;



/* TabView methods */

TabView *tabview_create(const char *title, Layout *parent_lt, Window *win);
void tabview_destroy(TabView *tv);
void tabview_activate(TabView *tv);
void tabview_close(TabView *tv);
Page *tabview_add_page(
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


void tabview_swap_adjacent_tabs(TabView *tv, int current, int new, bool apply_swapfn);
void tabview_move_current_tab_left(TabView *tv);
void tabview_move_current_tab_right(TabView *tv);
    
void tabview_clear_all_contents(TabView *tv);
const char *tabview_active_tab_title(TabView *tv);

void tabview_tab_drag(TabView *tv);
Page *tabview_select_tab(TabView *tv, int i);


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
void tabview_reset(TabView *tv, uint8_t leftmost_index);

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
PageEl *tabview_get_el_by_id(TabView *tv, const char *page_title, const char *id);

void page_select_el_by_id(Page *page, const char *id);

void page_el_reset(PageEl *el);
void page_el_params_slider_from_ep(union page_el_params *p, Endpoint *ep);
void page_center_contents(Page *page);

#endif
