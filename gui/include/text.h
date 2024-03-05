#ifndef JDAW_GUI_TEXT_H
#define JDAW_GUI_TEXT_H

#include <stdint.h>
#include <stdbool.h>
#include "SDL.h"
#include "SDL_ttf.h"
/* #include "window.h" */

#define CURSOR_WIDTH 4
#define CURSOR_COUNTDOWN_MAX 100
#define TEXT_BUFLEN 256

#define STD_FONT_SIZES {10, 12, 14, 16, 18, 24, 30, 36, 48, 60, 72}
#define STD_FONT_ARRLEN 11

typedef struct window Window;

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

typedef struct font {
    const char *path;
    TTF_Font *ttf_array[STD_FONT_ARRLEN];
} Font;

    

// Text *create_empty_text(SDL_Rect *container, TTF_Font *font, SDL_Color txt_clr, TextAlign align, bool truncate, SDL_Renderer *rend);

/* Create a Text from an existing string. If the string is a pointer to a const char *, Text cannot be edited. */
Text *create_text_from_str(char *set_str, int max_len, SDL_Rect *container, TTF_Font *font, SDL_Color txt_clr, TextAlign align, bool truncate, SDL_Renderer *rend);

void destroy_text(Text *txt);

/* Enter an event loop to edit a text. Once done, string pointed to by value_handle is modified */
void edit_text(Text *txt);


/* void print_text(Text *txt); */

/* Draw the text to the screen */
void draw_text(Text *txt);

/* Overwrite data the text display handle points to with a new string */
void set_text_value(Text *txt, char *new_value);

/* Change the value handle pointer, and reset the text display accordingly */
void set_text_value_handle(Text *txt, char *set_str);

/* Set the text display value from the value handle and truncate as needed */
void reset_text_display_value(Text *txt);

/* Open a TTF font at a specified size, accounting for dpi scaling */ 
TTF_Font *open_font(const char* path, int size, Window *win);

/* Initialze an array of TTF fonts at standard font sizes */
Font *init_font(const char *path, Window *win);

/* Given an existing Font object, get the actual TTF_Font at a given size */
TTF_Font *get_ttf_at_size(Font *font, int size);

#endif
