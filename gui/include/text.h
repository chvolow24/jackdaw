#ifndef JDAW_GUI_TEXT_H
#define JDAW_GUI_TEXT_H

#include <stdint.h>
#include <stdbool.h>
#include "SDL.h"
#include "SDL_ttf.h"
/* #include "window.h" */

#define CURSOR_WIDTH 4
#define CURSOR_COUNTDOWN_MAX 100
#define TEXT_BUFLEN 255

#define STD_FONT_SIZES {10, 12, 14, 16, 18, 24, 30, 36, 48, 60, 72}
#define STD_FONT_ARRLEN 11

#define TXTAREA_MAX_LINES 255

typedef struct window Window;

typedef enum textalign {
    CENTER,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    CENTER_LEFT,
    CENTER_RIGHT
} TextAlign;


typedef struct layout Layout;

/* Cannot be modified. Includes line wrapping. */ 
typedef struct text_area {
    const char *value_handle;
    int num_lines;
    int text_h;
    int line_spacing;
    Layout *layout;
    unsigned *line_break_indices;
    SDL_Texture *line_textures[TXTAREA_MAX_LINES];
    int line_widths[TXTAREA_MAX_LINES];
    int line_heights[TXTAREA_MAX_LINES];
    TTF_Font *font;
    SDL_Color color;
    Window *win;
} TextArea;
    
    
typedef struct text {
    char *value_handle;
    char display_value[TEXT_BUFLEN];
    int len;
    int max_len;
    int trunc_len;
    int cursor_start_pos;
    int cursor_end_pos;
    int cursor_countdown;
    int h_pad;
    int v_pad;
    bool show_cursor;
    SDL_Rect *container;
    SDL_Rect text_rect;
    SDL_Color color;
    TTF_Font *font;
    TextAlign align;
    bool truncate;

    Window *win;
    SDL_Texture *texture;
} Text;


typedef struct font {
    const char *path;
    TTF_Font *ttf_array[STD_FONT_ARRLEN];
} Font;

    

// Text *create_empty_text(SDL_Rect *container, TTF_Font *font, SDL_Color txt_clr, TextAlign align, bool truncate, SDL_Renderer *rend);

/* Create a Text from an existing string. If the string is a pointer to a const char *, Text cannot be edited. */
Text *txt_create_from_str(char *set_str, int max_len, SDL_Rect *container, TTF_Font *font, SDL_Color txt_clr, TextAlign align, bool truncate, Window *win);


/* Initialize an existing text from an existing string. Use instead of create_text_from_str for pre-allocated Text */
void txt_init_from_str(
    Text *txt,
    char *set_str,
    int max_len,
    SDL_Rect *container,
    TTF_Font *font,
    SDL_Color txt_clr,
    TextAlign align,
    bool truncate,
    Window *win
    );

void txt_destroy(Text *txt);

/* Enter an event loop to edit a text. Once done, string pointed to by value_handle is modified */
void txt_edit(Text *txt, void (draw_fn)(void));


/* void print_text(Text *txt); */

/* Draw the text to the screen */
void txt_draw(Text *txt);

/* Overwrite data the text display handle points to with a new string */
void txt_set_value(Text *txt, char *new_value);

/* Change the value handle pointer, and reset the text display accordingly */
void txt_set_value_handle(Text *txt, char *set_str);

/* Recreate the text surface and texture */
void txt_reset_drawable(Text *txt);

/* Set the text display value from the value handle and truncate as needed */
void txt_reset_display_value(Text *txt);

/* Open a TTF font at a specified size, accounting for dpi scaling */ 
TTF_Font *ttf_open_font(const char* path, int size, Window *win);

/* Initialze an array of TTF fonts at standard font sizes */
Font *ttf_init_font(const char *path, Window *win);

/* Given an existing Font object, get the actual TTF_Font at a given size */
TTF_Font *ttf_get_font_at_size(Font *font, int size);


/* Set a text color and refresh the drawable elements */
void txt_set_color(Text *txt, SDL_Color *clr);

/* Set text pad values and refresh drawable elements */
void txt_set_pad(Text *txt, int h_pad, int v_pad);

#endif


TextArea *txt_area_create(const char *value, Layout *layout, TTF_Font *font, SDL_Color color, Window *win);

void txt_area_draw(TextArea *txtarea);

void txt_area_create_lines(TextArea *txtarea);
