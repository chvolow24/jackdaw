#ifndef JDAW_SYMBOLS_H
#define JDAW_SYMBOLS_H

#include "window.h"

#define SYMBOL_STD_DIM 16

#define SYMBOL_DEFAULT_PAD 4
#define SYMBOL_DEFAULT_CORNER_R 4
#define SYMBOL_DEFAULT_THICKNESS 3
#define SYMBOL_FILTER_DIM_SCALAR_H 3
#define SYMBOL_FILTER_DIM_SCALAR_V 2.5
#define SYMBOL_FILTER_PAD 6

#define SYMBOL_WAV_W 24
#define SYMBOL_WAV_H 18
#define SYMBOL_WAV_PAD 4



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
    SYMBOL_X,
    SYMBOL_MINIMIZE,
    SYMBOL_DROPDOWN,
    SYMBOL_DROPUP,
    SYMBOL_KEYFRAME,
    SYMBOL_LOWSHELF,
    SYMBOL_HIGHSHELF,
    SYMBOL_PEAKNOTCH,
    SYMBOL_SINE,
    SYMBOL_SQUARE,
    SYMBOL_TRI,
    SYMBOL_SAW,
    NUM_SYMBOLS
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
