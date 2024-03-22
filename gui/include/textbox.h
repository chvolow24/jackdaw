#ifndef JDAW_GUI_TEXTBOX_H
#define JDAW_GUI_TEXTBOX_H

#include "layout.h"
#include "text.h"

#define MAX_MENU_SCTN_LEN 64
#define MAX_MENU_SECTIONS 16
#define MAX_MENU_COLUMNS 8

/*
  Textbox layout must have "text" target
  and "box" target
*/
typedef struct textbox {
    Text *text;
    Layout *layout;
    SDL_Color *bckgrnd_clr;
    SDL_Color *border_clr;
    int border_thickness;
    int corner_radius;
    Window *window;
} Textbox;

Textbox *textbox_create();
void textbox_destroy(Textbox *);
Textbox *textbox_create_from_str(
    char *set_str,
    Layout *lt,
    TTF_Font *font,
    Window *win
    );

/* WARNING: deprecated. Use 'textbox_size_to_fit' instead */
void textbox_pad(Textbox *tb, int pad);
void textbox_draw(Textbox *tb);
void textbox_size_to_fit(Textbox *tb, int w_pad, int v_pad);
void textbox_set_fixed_w(Textbox *tb, int fixed_w);
void textbox_set_text_color(Textbox *tb, SDL_Color *clr);
void textbox_set_trunc(Textbox *tb, bool trunc);
void textbox_set_text_color(Textbox *tb, SDL_Color *clr);
void textbox_set_background_color(Textbox *tb, SDL_Color *clr);
void textbox_set_border_color(Textbox *tb, SDL_Color *clr);
#endif
