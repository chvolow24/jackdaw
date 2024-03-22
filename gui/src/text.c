/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/


#include "draw.h"
#include "text.h"
#include "layout.h"
#include "window.h"

extern Layout *main_layout;
extern Window *main_win;


static void init_empty_text(
    Text *txt,
    SDL_Rect *container, 
    TTF_Font *font, 
    SDL_Color txt_clr, 
    TextAlign align,
    bool truncate,
    Window *win
)
{
    txt->align = align;
    txt->value_handle = NULL;
    txt->len = 0;
    txt->max_len = 0;
    txt->container = container;

    txt->font = font;
    txt->color = txt_clr;
    txt->truncate = truncate;

    txt->cursor_countdown = 0;
    txt->cursor_end_pos = 0;
    txt->cursor_start_pos = 0;
    txt->show_cursor = false;

    txt->h_pad = 0;
    txt->v_pad = 0;
    
    txt->win = win;
    txt->surface = NULL;
    txt->texture = NULL;
}

static Text *create_empty_text(
    SDL_Rect *container, 
    TTF_Font *font, 
    SDL_Color txt_clr, 
    TextAlign align,
    bool truncate,
    Window *win
)
{

    Text *txt = malloc(sizeof(Text));
    init_empty_text(txt, container, font, txt_clr, align, truncate, win);
    return txt;
}


void txt_reset_drawable(Text *txt) 
{
   
    if (txt->len == 0 || txt->display_value[0] == '\0') { 
        return;
    }
    if (txt->surface) {
        SDL_FreeSurface(txt->surface);
	txt->surface = NULL;
    }
    if (txt->texture) {
        SDL_DestroyTexture(txt->texture);
	txt->texture = NULL;
    }

    txt->surface = TTF_RenderUTF8_Blended(txt->font, txt->display_value, txt->color);
    if (!txt->surface) {
        fprintf(stderr, "Error: TTF_RenderText_Blended failed: %s\n", TTF_GetError());
        return;
    }
    txt->texture = SDL_CreateTextureFromSurface(txt->win->rend, txt->surface);
    if (!txt->texture) {
        fprintf(stderr, "Error: SDL_CreateTextureFromSurface failed: %s\n", TTF_GetError());
        return;
    }
    SDL_QueryTexture(txt->texture, NULL, NULL, &(txt->text_rect.w), &(txt->text_rect.h));
    switch (txt->align) {
    case CENTER:
	txt->text_rect.x = txt->container->x + (int) round((float)txt->container->w / 2.0 - (float) txt->text_rect.w / 2.0);
	txt->text_rect.y = txt->container->y + (int) round((float)txt->container->h / 2.0 - (float) txt->text_rect.h / 2.0);
	break;
    case TOP_LEFT:
	txt->text_rect.x = txt->container->x + txt->h_pad * txt->win->dpi_scale_factor;
	txt->text_rect.y = txt->container->y;
	break;
    case TOP_RIGHT:
	txt->text_rect.x = txt->container->x + txt->container->w - txt->text_rect.w;
	txt->text_rect.y = txt->container->y + txt->v_pad * txt->win->dpi_scale_factor;
	break;
    case BOTTOM_LEFT:
	txt->text_rect.x = txt->container->x + txt->h_pad * txt->win->dpi_scale_factor;
	txt->text_rect.y = txt->container->y + txt->container->h - txt->text_rect.h - txt->v_pad * txt->win->dpi_scale_factor;
	break;
    case BOTTOM_RIGHT:
	txt->text_rect.x = txt->container->x + txt->container->w - txt->text_rect.w - txt->h_pad * txt->win->dpi_scale_factor;
	txt->text_rect.y = txt->container->y + txt->container->h - txt->text_rect.h - txt->v_pad * txt->win->dpi_scale_factor;
	break;
    case CENTER_LEFT:
	txt->text_rect.x = txt->container->x + txt->h_pad * txt->win->dpi_scale_factor;
	txt->text_rect.y = txt->container->y + (int) round((float)txt->container->h / 2.0 - (float) txt->text_rect.h / 2.0);
	break;
    case CENTER_RIGHT:
	txt->text_rect.x = txt->container->x + txt->container->w - txt->text_rect.w - txt->h_pad * txt->win->dpi_scale_factor;
	txt->text_rect.y = txt->container->y + (int) round((float)txt->container->h / 2.0 - (float) txt->text_rect.h / 2.0);

    }

}

void txt_reset_display_value(Text *txt) 
{
    int txtw, txth;

    TTF_SizeUTF8(txt->font, txt->value_handle, &txtw, &txth);

    /* fprintf(stdout, "Checked size of text: %s\n", txt->value_handle); */
    /* fprintf(stdout, "Container w: %d\n", txt->container->w); */
    if (txt->truncate && txtw > txt->container->w) {
        int approx_allowable_chars = (int) ((float) txt->len * txt->container->w / txtw);
        for (int i=0; i<approx_allowable_chars - 3; i++) {
            txt->display_value[i] = txt->value_handle[i];
        }

        for (int i=approx_allowable_chars - 3; i<approx_allowable_chars && i > 0; i++) {
            txt->display_value[i] = '.';
	   
        }
	txt->display_value[approx_allowable_chars] = '\0';
    } else {
        strcpy(txt->display_value, txt->value_handle);
    }
    txt_reset_drawable(txt);


}
void txt_set_value_handle(Text *txt, char *set_str)
{
    /* fprintf(stderr, "setting text value handle to %s\n", set_str); */
    txt->value_handle = set_str;
    txt->len = strlen(set_str);
    txt_reset_display_value(txt);

}
void txt_set_value(Text *txt, char *set_str)
{
    strcpy(txt->value_handle, set_str);
    txt->len = strlen(set_str);
    txt_reset_display_value(txt);
}

Text *txt_create_from_str(
    char *set_str,
    int max_len,
    SDL_Rect *container,
    TTF_Font *font,
    SDL_Color txt_clr,
    TextAlign align,
    bool truncate,
    Window *win
)
{
    Text *txt = create_empty_text(container, font, txt_clr, align, truncate, win);

    txt->max_len = max_len;
    if (set_str) {
        txt_set_value_handle(txt, set_str);
    }

    return txt;
}


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
    )
{
    init_empty_text(txt, container, font, txt_clr, align, truncate, win);
    txt->max_len = max_len;
    if (set_str) {
        txt_set_value_handle(txt, set_str);
    }
}

static void cursor_left(Text *txt)
{
    if (txt->cursor_start_pos > 0) {
        txt->cursor_start_pos--;
    }
    txt->cursor_end_pos = txt->cursor_start_pos;  
    txt->cursor_countdown = CURSOR_COUNTDOWN_MAX;

}

static void cursor_right(Text *txt)
{
    if (txt->cursor_end_pos < txt->len) {
        txt->cursor_end_pos++;
    }
    txt->cursor_start_pos = txt->cursor_end_pos;  
    txt->cursor_countdown = CURSOR_COUNTDOWN_MAX;


}
static void handle_backspace(Text *txt);
static int handle_char(Text *txt, char input)
{
    if (txt->len == txt->max_len) {
        return 0;
    }
    if (txt->cursor_start_pos != txt->cursor_end_pos) {
        handle_backspace(txt);
    }
    for (int i=txt->len; i>txt->cursor_end_pos - 1; i--) {
        txt->display_value[i+1] = txt->display_value[i];
    }
    txt->display_value[txt->cursor_end_pos] = input;
    txt->cursor_end_pos++;
    txt->cursor_start_pos = txt->cursor_end_pos;
    txt->len++;
    txt->cursor_countdown = CURSOR_COUNTDOWN_MAX;
    return 1;
}

static void handle_backspace(Text *txt) 
{
    // char *val = txt->display_value;
    int displace = txt->cursor_end_pos - txt->cursor_start_pos;
    int i = txt->cursor_start_pos;
    if (displace > 0) {
        while (i + displace <= txt->len) {
            txt->display_value[i] = txt->display_value[i + displace];
            char c = txt->display_value[i+displace];
            i++;
        }
        txt->len -= displace;
    } else {
        if (txt->cursor_end_pos == 0) {
            return;
        }
        while (i <= txt->len) {
            txt->display_value[i-1] = txt->display_value[i];
            i++;
        }
        txt->len--;
        txt->cursor_end_pos--;
        txt->cursor_start_pos = txt->cursor_end_pos;
    }
    txt->cursor_end_pos = txt->cursor_start_pos;
    txt->display_value[txt->len] = '\0';
    txt->cursor_countdown = CURSOR_COUNTDOWN_MAX;
}

void txt_edit(Text *txt, void (draw_fn) (void))
{
    bool save_truncate = txt->truncate;
    txt->truncate = false;
    txt_reset_display_value(txt);
    txt->show_cursor = true;
    txt->cursor_start_pos = 0;
    txt->cursor_end_pos = txt->len;
    bool done = false;
    // bool mousedown = false;
    bool cmdctrldown = false;
    bool shiftdown = false;
    SDL_StartTextInput();
    while (!done) {
        // get_mousep(main_win, &mousep);

        SDL_Event e;

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT 
                || (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) 
                || e.type == SDL_MOUSEBUTTONDOWN) {
                done = true;
                /* Push the event back to the main event stack, so it can be handled in main.c */
                SDL_PushEvent(&e);
            } else if (e.type == SDL_TEXTINPUT) {
                handle_char(txt, e.text.text[0]);
                txt_reset_drawable(txt);
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_RETURN:
                    case SDL_SCANCODE_KP_ENTER:
                        done = true;
                        break;
                    case SDL_SCANCODE_LSHIFT:
                    case SDL_SCANCODE_RSHIFT:
                        shiftdown = true;
                        break;
                    case SDL_SCANCODE_LGUI:
                    case SDL_SCANCODE_RGUI:
                    case SDL_SCANCODE_LCTRL:
                    case SDL_SCANCODE_RCTRL:
                        cmdctrldown = true;
                        break;
                    case SDL_SCANCODE_LEFT:
                        cursor_left(txt);
                        break;
                    case SDL_SCANCODE_RIGHT:
                        cursor_right(txt);
                        break;
                    case SDL_SCANCODE_BACKSPACE:
                        handle_backspace(txt);
                        // set_text_value(txt, txt->display_value);
                        txt_reset_drawable(txt);
                        // set_text_value(txt, txt->display_value);
                        break;
                    case SDL_SCANCODE_SPACE:
                        handle_char(txt, ' ');
                        txt_reset_drawable(txt);
                        // set_text_value(txt, txt->display_value);
                        break;
                    case SDL_SCANCODE_A:
                        if (cmdctrldown) {
                            txt->cursor_start_pos = 0;
                            txt->cursor_end_pos = strlen(txt->display_value);
                        }
                        break;
                    default: {
                        break;
                    }

                }
            } else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_LSHIFT:
                    case SDL_SCANCODE_RSHIFT:
                        shiftdown = false;
                        break;
                    case SDL_SCANCODE_LGUI:
                    case SDL_SCANCODE_RGUI:
                    case SDL_SCANCODE_LCTRL:
                    case SDL_SCANCODE_RCTRL:
                        cmdctrldown = false;
                        break;
                    default: 
                        break;
                }
            }
        }
        // fprintf(stderr, "About to call draw %p %p\n", main_win, txt);
        /* layout_draw_main(); */
	draw_fn();
        if (txt->cursor_countdown == 0) {
            txt->cursor_countdown = CURSOR_COUNTDOWN_MAX;
        } else {
            txt->cursor_countdown--;
        }
        // draw_textxtox(main_win, txt);
        SDL_Delay(1);

    }
    SDL_StopTextInput();
    txt->show_cursor = false;
    txt->truncate = save_truncate;
    txt_set_value(txt, txt->display_value);

    // draw_main();
}

void txt_destroy(Text *txt)
{
    if (txt->surface) {
        free(txt->surface);
    }
    if (txt->texture) {
        free(txt->texture);
    }
    free(txt);
}

TTF_Font *ttf_open_font(const char* path, int size, Window *win)
{
    size *= win->dpi_scale_factor;
    TTF_Font *font = TTF_OpenFont(path, size);
    if (!font) {
        fprintf(stderr, "\nError: failed to open font: %s", TTF_GetError());
        exit(1);
    }
    return font;
}

void destroy_font(TTF_Font *font)
{
    free(font);
}

Font *ttf_init_font(const char *path, Window *win)
{
    Font *font = malloc(sizeof(Font));
    int sizes[] = STD_FONT_SIZES;
    font->path = path;
    if (!font) {
	fprintf(stderr, "Error: unable to allocate space for Font. Exiting.\n");
	exit(1);
    }
    for (int i=0; i<STD_FONT_ARRLEN; i++) {;
	font->ttf_array[i] = ttf_open_font(path, sizes[i], win);
    }
    return font;
}

TTF_Font *ttf_get_font_at_size(Font *font, int size)
{
    int i = 0;
    int sizes[] = STD_FONT_SIZES;
    while (i<STD_FONT_ARRLEN) {
	if (size == sizes[i]) {
	    if (font->ttf_array[i]) {
		return font->ttf_array[i];
	    } else {
		fprintf(stderr, "Error: ttf_array at index %d (size %d) is null.\n", i, size);
		exit(1);
	    }
	}
	i++;
    }
    return NULL;
}

void txt_set_color(Text *txt, SDL_Color *clr)
{
    txt->color = *clr;
    txt_reset_drawable(txt);
}

void txt_set_pad(Text *txt, int h_pad, int v_pad)
{
    txt->h_pad = h_pad;
    txt->v_pad = v_pad;
}
