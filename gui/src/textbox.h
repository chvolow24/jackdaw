#ifndef JDAW_GUI_TEXTBOX_H
#define JDAW_GUI_TEXTBOX_H

#include "layout.h"
#include "text.h"

#define MAX_MENU_SCTN_LEN 64
#define MAX_MENU_SECTIONS 16
#define MAX_MENU_COLUMNS 8
#define MAX_ADDTL_TBS 16
#define BUTTON_CORNER_RADIUS 6;

#define MAX_NAMELENGTH 64

typedef int (*ComponentFn)(void *self, void *target);
/*
  Textbox layout must have "text" target
  and "box" target
*/

enum textbox_style {
    NONE=0,
    BUTTON_CLASSIC=1,
    BUTTON_DARK=2
};

typedef struct textbox {
    Text *text;
    Layout *layout;
    bool wrap;

    enum textbox_style style;
    SDL_Color *bckgrnd_clr;
    SDL_Color *border_clr;
    int border_thickness;
    int corner_radius;
    Window *window;

    /* 'live' textboxes check value at every draw frame
       to determine if redraw necessary */
    bool live;

    /* COLOR CHANGE CALLBACKS: DEPRECATED AND REPLACED WITH animation.c */
    
    /* int color_change_timer; */
    /* bool color_change_target_text; */
    /* SDL_Color *color_change_new_color; */
    /* ComponentFn color_change_callback; */
    /* void *color_change_callback_target; */
    
} Textbox;

typedef struct text_lines_item {
    Textbox *tb;
    void *obj;
    void *(*action)(void *self, void *target);
    Textbox *addtl_tbs[MAX_ADDTL_TBS];
    int num_addtl_tbs;
} TLinesItem;

typedef struct text_lines {
    TLinesItem *items;
    uint32_t num_items;
    /* uint32_t items_arrlen; */
    Layout *container;
} TextLines;

/* enum textbox_style { */
/*     BLANK, */
/*     NUMBOX */
/* }; */

Textbox *textbox_create();
void textbox_destroy(Textbox *);
Textbox *textbox_create_from_str(
    const char *set_str,
    Layout *lt,
    /* TTF_Font *font, */
    Font *font,
    uint8_t text_size,
    Window *win
    );

/* WARNING: deprecated. Use 'textbox_size_to_fit' instead */
void textbox_pad(Textbox *tb, int pad);
void textbox_draw(Textbox *tb);
void textbox_size_to_fit(Textbox *tb, int w_pad, int v_pad);
void textbox_size_to_fit_v(Textbox *tb, int v_pad);
void textbox_size_to_fit_h(Textbox *tb, int h_pad);
void textbox_set_fixed_w(Textbox *tb, int fixed_w);
void textbox_set_text_color(Textbox *tb, const SDL_Color *clr);
void textbox_set_trunc(Textbox *tb, bool trunc);
/* void textbox_set_text_color(Textbox *tb, SDL_Color *clr); */
void textbox_set_background_color(Textbox *tb, SDL_Color *clr);
void textbox_set_border_color(Textbox *tb, SDL_Color *clr);
void textbox_set_border(Textbox *tb, SDL_Color *color, int thickness);
void textbox_set_align(Textbox *tb, TextAlign align);

/* Reset drawable only. For full reset, use textbox_reset_full */
void textbox_reset(Textbox *tb);
/* Include resetting of display value */
void textbox_reset_full(Textbox *tb);

void textbox_set_pad(Textbox *tb, int h_pad, int v_pad);
void textbox_set_value_handle(Textbox *tb, const char *new_value);

void textbox_set_style(Textbox *tb, enum textbox_style style);
/* void textbox_schedule_color_change( */
/*     Textbox *tb, */
/*     int timer, */
/*     SDL_Color *new_color, */
/*     bool change_text_color, */
/*     ComponentFn color_change_callback, */
/*     void *color_change_callback_target); */

typedef void (*CreateTline)(TextLines *tlines, TLinesItem *current_line, const void *src_item, void *xarg);
typedef int (*TlinesFilter)(void *item, void *x_arg);

TextLines *textlines_create(
    void *src_items,
    size_t item_width,
    uint32_t num_items,
    CreateTline create_line,
    TlinesFilter filter,
    /* int (*filter)(void *item, void *x_arg), */
    /* TLinesItem *(*create_item)(void ***curent_item, Layout *container, void *x_arg, int (*filter)(void *item, void *x_arg)), */
    Layout *container,
    void *x_arg);
void textlines_draw(TextLines *tlines);
void textlines_destroy(TextLines *tlines);

Textbox *textline_add_addtl_tb(
    TLinesItem *line,
    const char *set_str,
    Layout *lt,
    Font *font,
    uint8_t text_size,
    Window *win);

#endif
