#ifndef JDAW_TEXT_H
#define JDAW_TEXT_H

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#define FREE_SANS "assets/ttf/free_sans.ttf"
#define OPEN_SANS "assets/ttf/OpenSans-Regular.ttf"

typedef struct textbox {
    SDL_Rect *container;
    char *text;
    TTF_Font *font;
} Textbox;

void init_SDL_ttf();
TTF_Font* open_font(const char* path, int size);
void close_font(TTF_Font* font);
void write_text(SDL_Renderer *rend, SDL_Rect *rect, TTF_Font* font, SDL_Color* color, const char *text, bool allow_resize);


#endif