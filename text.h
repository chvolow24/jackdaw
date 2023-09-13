#ifndef JDAW_TEXT_H
#define JDAW_TEXT_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "theme.h"

#define FREE_SANS "assets/ttf/free_sans.ttf"
#define OPEN_SANS "assets/ttf/OpenSans-Regular.ttf"
#define OPEN_SANS_VAR "assets/ttf/OpenSans-Variable.ttf"
#define COURIER_NEW "assets/ttf/CourierNew.ttf"
#define STD_FONT_SIZES {10, 12, 14, 16, 18, 24, 30, 36, 48, 60, 72}
#define STD_FONT_ARRLEN 11

typedef struct textbox {
    SDL_Rect *container;
    char *text;
    TTF_Font *font;
} Textbox;

void init_SDL_ttf();
TTF_Font* open_font(const char* path, int size);
void init_fonts(TTF_Font **font_array, const char *path, int arrlen);
void close_font(TTF_Font* font);
int write_text(
    SDL_Renderer *rend, 
    SDL_Rect *rect, 
    TTF_Font* font, 
    JDAW_Color* color, 
    const char *text, 
    bool allow_resize
);

#endif