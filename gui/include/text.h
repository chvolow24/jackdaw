#ifndef JDAW_GUI_TEXT_H
#define JDAW_GUI_TEXT_H

#include <stdint.h>
#include <stdbool.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "window.h"

#define CURSOR_COUNTDOWN_MAX 100
#define TEXT_BUFLEN 256

typedef enum textalign {
    CENTER,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    CENTER_LEFT
} TextAlign;

typedef struct text {
    char *value_handle;
    char display_value[TEXT_BUFLEN];
    int len;
    int max_len;
    int trunc_len;
    int cursor_start_pos;
    int cursor_end_pos;
    int cursor_countdown;
    bool show_cursor;
    SDL_Rect *container;
    SDL_Rect text_rect;
    SDL_Color color;
    TTF_Font *font;
    TextAlign align;
    bool truncate;

    SDL_Renderer *rend;
    SDL_Surface *surface;
    SDL_Texture *texture;
} Text;


// Text *create_empty_text(SDL_Rect *container, TTF_Font *font, SDL_Color txt_clr, TextAlign align, bool truncate, SDL_Renderer *rend);
Text *create_text_from_str(char *set_str, int max_len, SDL_Rect *container, TTF_Font *font, SDL_Color txt_clr, TextAlign align, bool truncate, SDL_Renderer *rend);
void destroy_text(Text *txt);
void edit_text(Text *txt);
void print_text(Text *txt);
void draw_text(Text *txt);
void set_text_value(Text *txt, char *new_value);
TTF_Font *open_font(const char* path, int size, Window *win);

#endif