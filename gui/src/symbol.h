#ifndef JDAW_SYMBOLS_H
#define JDAW_SYMBOLS_H

#include "window.h"

#define SYMBOL_STD_DIM 16

typedef struct symbol {
    Window *window;
    int x_dim_pix;
    int y_dim_pix;
    int corner_rad_pix;
    SDL_Texture *texture;
    void (*draw_fn)(void *self);
    bool redraw;
} Symbol;

enum symbol_id {
    SYMBOL_X=0,
    SYMBOL_MINIMIZE=1,
    SYMBOL_DROPDOWN=2,
    SYMBOL_DROPUP=3,
    SYMBOL_KEYFRAME=4
};

void init_symbol_table(Window *win);
void symbol_quit(Window *win);
Symbol *symbol_create(
    Window *win,
    int x_dim_pix,
    int y_dim_pix,
    int corner_rad_pix,
    void (*draw_fn)(void *));
void symbol_draw(Symbol *symbol, SDL_Rect *dst);
/* void symbol_draw_w_bckgrnd(Symbol *symbol, SDL_Rect *dst, SDL_Color *bckgrnd); */
void symbol_draw_w_bckgrnd(Symbol *s, SDL_Rect *dst, SDL_Color *bckgrnd);


#endif
