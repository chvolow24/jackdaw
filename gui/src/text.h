#ifndef JDAW_GUI_TEXT_H
#define JDAW_GUI_TEXT_H

#include <stdint.h>
#include <stdbool.h>
#include "assets.h"
#include "SDL.h"
#include "SDL_ttf.h"
/* #include "window.h" */

/* #define OPEN_SANS_PATH INSTALL_DIR "/assets/ttf/NotoSansSymbols2-Regular.ttf" */
/* #define OPEN_SANS_BOLD_PATH OPEN_SANS_PATH */
/* #define LTSUPERIOR_PATH OPEN_SANS_PATH */
/* #define LTSUPERIOR_BOLD_PATH OPEN_SANS_PATH */

/* #define OPEN_SANS_PATH LTSUPERIOR_BOLD_PATH */
/* #define OPEN_SANS_BOLD_PATH LTSUPERIOR_BOLD_PATH */
/* #define OPEN_SANS_BOLD_PATH INSTALL_DIR "/assets/ttf/Iosevka-Bold.ttf" */
#define TTF_SPEC_ADJUST 1

#define CURSOR_WIDTH 4
#define CURSOR_COUNTDOWN_MAX 100
#define TEXT_BUFLEN 255

#define STD_FONT_SIZES {10, 12, 14, 16, 18, 24, 30, 36, 48, 60, 72}
#define STD_FONT_ARRLEN 11

#define TXTAREA_MAX_LINES 255


#define txt_int_range_completion(minval, maxval)			\
	int txt_int_range_completion_##minval##_##maxval(Text *txt, void *arg) { \
	    int value = atoi(txt->value_handle);			\
	    bool err = false;						\
	    if (value < minval) {						\
		snprintf(txt->value_handle, txt->max_len, "%d", minval);	\
		txt_reset_display_value(txt);					\
		err = true;						\
	    } else if (value > maxval) {					\
		snprintf(txt->value_handle, txt->max_len, "%d", maxval);	\
		txt_reset_display_value(txt);					\
		err = true;						\
	    }								\
	    if (err) {							\
	        char buf[255];						\
		snprintf(buf, 255, "Allowed range: %d - %d", minval, maxval); \
		status_set_errstr(buf);					\
		return 1;						\
	    }							        \
	     return 0;							\
	}								\


/* #define txt_int_validation_range(txt, input, minval, maxval) \ */
/* 	int txt_int_validation_range_##minval##_##maxval(Text *txt, char input) { \ */
/* 	    char buf[255];						\ */
/* 	    if (input < '0' || input > '9') {				\ */
/* 		status_set_errstr("Only integer values allowed");	\ */
/* 		return 1;						\ */
/* 	    }								\ */
/* 	    if (strlen(txt->display_value) - (txt->cursor_end_pos - txt->cursor_start_pos) >= txt->max_len - 1) { \ */
/* 		snprintf(buf, 255, "Field cannot exceed %d characters", txt->max_len - 1); \ */
/* 		return 1;						\ */
/* 	    }								\ */
/* 	    snprintf(buf, 255, "%s", txt->display_value);\ */
/* 	    snprintf(buf + txt->cursor_start_pos, 255, "%s%c", txt->display_value + txt->cursor_end_pos, input);	\ */
/* 	    int val = atoi(buf);					\ */
/* 	    fprintf(stderr, "BUf: %s: val: %d\n", buf, val);		\ */
/* 	    if (val < minval || val > maxval) {				\ */
/* 		status_set_errstr("Values must fall in range " #minval " - " #maxval); \ */
/* 		return 1;						\ */
/* 	    }								\ */
/* 	    return 0;							\ */
/* 	}								\ */


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

typedef enum font_type {
    REG,
    BOLD,
    MONO,
    MONO_BOLD,
    SYMBOLIC,
    MATHEMATICAL
} FontType;


typedef struct layout Layout;
typedef struct font Font;

/* Cannot be modified. Includes line wrapping. */ 
typedef struct text_area {
    const char *value_handle;
    int num_lines;
    int text_h;
    int line_spacing;
    Layout *layout;
    /* unsigned *line_break_indices; */
    SDL_Texture *line_textures[TXTAREA_MAX_LINES];
    int line_widths[TXTAREA_MAX_LINES];
    int line_heights[TXTAREA_MAX_LINES];
    Font *font;
    uint8_t text_size;
    /* TTF_Font *font; */
    SDL_Color color;
    Window *win;
} TextArea;

typedef struct text Text;
typedef struct text {
    char *value_handle;
    char display_value[TEXT_BUFLEN];
    char *cached_value;
    int len;
    int max_len;
    int trunc_len;
    int cursor_start_pos;
    int cursor_end_pos;
    int cursor_countdown;
    int h_pad;
    int v_pad;
    bool show_cursor;
    /* SDL_Rect *container; */
    Layout *container;
    /* SDL_Rect text_rect; */
    Layout *text_lt;
    SDL_Color color;
    /* TTF_Font *font; */
    Font *font;
    uint8_t text_size;
    TextAlign align;
    bool truncate;

    Window *win;
    SDL_Texture *texture;

    int (*validation)(Text *self, char input);
    int (*after_edit)(Text *self, void *obj);
    int (*completion)(Text *self, void *obj);
    void *after_edit_target;
    void *completion_target;
    /* int (*submit_validation)(Text *self); */
    /* pthread_mutex_t draw_lock; */
} Text;


typedef struct font {
    const char *path;
    TTF_Font *ttf_array[STD_FONT_ARRLEN];
} Font;

    

// Text *create_empty_text(SDL_Rect *container, TTF_Font *font, SDL_Color txt_clr, TextAlign align, bool truncate, SDL_Renderer *rend);

/* Create a Text from an existing string. If the string is a pointer to a const char *, Text cannot be edited. */
Text *txt_create_from_str(
    char *set_str,
    int max_len,
    Layout *container,
    /* SDL_Rect *container, */
    /* TTF_Font *font, */
    Font *font,
    uint8_t text_size,
    SDL_Color txt_clr,
    TextAlign align,
    bool truncate,
    Window *win
    );


/* Initialize an existing text from an existing string. Use instead of create_text_from_str for pre-allocated Text */
void txt_init_from_str(
    Text *txt,
    char *set_str,
    int max_len,
    Layout *container,
    /* SDL_Rect *container, */
    Font *font,
    uint8_t text_size,
    /* TTF_Font *font, */
    SDL_Color txt_clr,
    TextAlign align,
    bool truncate,
    Window *win
    );

void txt_destroy(Text *txt);

/* Enter an event loop to edit a text. Once done, string pointed to by value_handle is modified */
void txt_edit(Text *txt, void (*draw_fn)(void));

void txt_input_event_handler(Text *txt, SDL_Event *e);
void txt_stop_editing(Text *txt);

void txt_edit_backspace(Text *txt);
void txt_edit_move_cursor(Text *txt, bool left);
void txt_edit_select_all(Text *txt);

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
/* Font *ttf_init_font(const char *path, Window *win); */
Font *ttf_init_font(const char *path, Window *win, int style);

/* Re-initialize a font (if the DPI scale factor has changed) */
void ttf_reset_dpi_scale_factor(Font *font);

/* Given an existing Font object, get the actual TTF_Font at a given size */
TTF_Font *ttf_get_font_at_size(Font *font, int size);

/* Set a text color and refresh the drawable elements */
void txt_set_color(Text *txt, SDL_Color *clr);

/* Set text pad values and refresh drawable elements */
void txt_set_pad(Text *txt, int h_pad, int v_pad);


TextArea *txt_area_create(
    const char *value,
    Layout *layout,
    Font *font,
    uint8_t text_size,
    SDL_Color color,
    Window *win);

void txt_area_draw(TextArea *txtarea);

void txt_area_create_lines(TextArea *txtarea);

/* Free all line textures and the textarea itself. Destroy its layout. */
void txt_area_destroy(TextArea *ta);

/* Destroy the font array and free the Font */
void ttf_destroy_font(Font *font);

int txt_name_validation(Text *txt, char input);
int txt_integer_validation(Text *txt, char input);

#endif
