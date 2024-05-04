/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
3  
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

#include <string.h>

#include "draw.h"
#include "text.h"
#include "layout.h"
#include "window.h"

#define TXT_AREA_W_MIN 10
#define TXT_AREA_DEFAULT_LINE_SPACING 2

#define GLOBAL_TEXT_SCALE 1

extern Layout *main_layout;
extern Window *main_win;

SDL_Color highlight = {0, 0, 255, 255};


static void init_empty_text(
    Text *txt,
    SDL_Rect *container, 
    /* TTF_Font *font,  */
    Font *font,
    uint8_t text_size,
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

    /* txt->font = font; */
    txt->font = font;
    txt->text_size = text_size;
    txt->color = txt_clr;
    txt->truncate = truncate;

    txt->cursor_countdown = 0;
    txt->cursor_end_pos = 0;
    txt->cursor_start_pos = 0;
    txt->show_cursor = false;

    txt->h_pad = 0;
    txt->v_pad = 0;
    
    txt->win = win;
    txt->texture = NULL;
}

static Text *create_empty_text(
    SDL_Rect *container,
    Font *font,
    uint8_t text_size,
    /* TTF_Font *font,  */
    SDL_Color txt_clr, 
    TextAlign align,
    bool truncate,
    Window *win
)
{

    Text *txt = malloc(sizeof(Text));
    init_empty_text(txt, container, font, text_size, txt_clr, align, truncate, win);
    return txt;
}


void txt_reset_drawable(Text *txt) 
{
    TTF_Font *font = ttf_get_font_at_size(txt->font, txt->text_size);
    /* fprintf(stdout, "TEXT: %s\n", txt->value_handle); */
    /* fprintf(stdout, "Got ttf font %p from font %p, at size %d\n", font, txt->font, txt->text_size); */
   
    if (txt->len == 0 || txt->display_value[0] == '\0') { 
        return;
    }

    if (txt->texture) {
        SDL_DestroyTexture(txt->texture);
	txt->texture = NULL;
    }

    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, txt->display_value, txt->color);
    if (!surface) {
        fprintf(stderr, "Error: TTF_RenderText_Blended failed: %s\n", TTF_GetError());
        return;
    }
    txt->texture = SDL_CreateTextureFromSurface(txt->win->rend, surface);
    if (!txt->texture) {
        fprintf(stderr, "Error: SDL_CreateTextureFromSurface failed: %s\n", TTF_GetError());
        return;
    }
    SDL_FreeSurface(surface);
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

    TTF_Font *font = ttf_get_font_at_size(txt->font, txt->text_size);
    TTF_SizeUTF8(font, txt->value_handle, &txtw, &txth);

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
    /* TTF_Font *font, */
    Font *font,
    uint8_t text_size,
    SDL_Color txt_clr,
    TextAlign align,
    bool truncate,
    Window *win
)
{
    Text *txt = create_empty_text(container, font, text_size, txt_clr, align, truncate, win);

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
    /* TTF_Font *font, */
    Font *font,
    uint8_t text_size,
    SDL_Color txt_clr,
    TextAlign align,
    bool truncate,
    Window *win
    )
{
    init_empty_text(txt, container, font, text_size, txt_clr, align, truncate, win);
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

void txt_edit(Text *txt, void (*draw_fn) (void))
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
	/* window_start_draw(main_win, &color_global_black); */
	draw_fn();
	/* window_end_draw(main_win); */
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
    if (txt->texture) {
        SDL_DestroyTexture(txt->texture);
    }
    free(txt);
}

TTF_Font *ttf_open_font(const char* path, int size, Window *win)
{
    size *= win->dpi_scale_factor;
    TTF_Font *font = TTF_OpenFont(path, size * GLOBAL_TEXT_SCALE);
    if (!font) {
        fprintf(stderr, "\nError: failed to open font: %s", TTF_GetError());
        exit(1);
    }
    return font;
}

/* void destroy_font(TTF_Font *font) */
/* { */
/*     TTF_CloseFont(font); */
/* } */

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

void ttf_destroy_font(Font *font)
{
    for (int i=0; i<STD_FONT_ARRLEN; i++) {
	if (font->ttf_array[i]) {
	    TTF_CloseFont(font->ttf_array[i]);
	}
    }
    free(font);
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

void ttf_reset_dpi_scale_factor(Font *font)
{
    int sizes[] = STD_FONT_SIZES;
    for (int i=0; i<STD_FONT_ARRLEN; i++) {;
	font->ttf_array[i] = ttf_open_font(font->path, sizes[i], main_win); //TODO: Replace "main_win"
    }	
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

static void txt_area_make_line(TextArea *txtarea, char *line_start)
{
    TTF_Font *font = ttf_get_font_at_size(txtarea->font, txtarea->text_size);
    if (strlen(line_start) == 0) {
	return;
    }
    SDL_Surface *surface = TTF_RenderText_Blended(font, line_start, txtarea->color);
    if (!surface) {
	fprintf(stderr, "Error creating line surface in text area. %s\n", SDL_GetError());
	exit(1);
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(txtarea->win->rend, surface);
    if (!texture) {
	fprintf(stderr, "Error creating line texture from surface. %s\n", SDL_GetError());
    }
    SDL_FreeSurface(surface);
    int line_w, line_h;

    TTF_SizeUTF8(font, line_start, &line_w, &line_h);
    if (txtarea->text_h == 0) {
	txtarea->text_h = line_h;
    }
    txtarea->line_widths[txtarea->num_lines] = line_w;
    txtarea->line_heights[txtarea->num_lines] = line_h;
    txtarea->line_textures[txtarea->num_lines] = texture;
    txtarea->num_lines++;
}


/* Return 0 for completed last line; 1 for completed line; -1 for error */
static int txt_area_create_line(TextArea *txtarea, char **line_start, int w)
{
    char *cursor = *line_start;
    char *last_word_boundary = *line_start;
    char save_word_end;
    char save_last_word_end;
    int line_w = 0;
    int line_h = 0;
    /* int ret = 0; */
    /* SDL_Surface *surface; */
    /* SDL_Texture *texture; */

    TTF_Font *font = ttf_get_font_at_size(txtarea->font, txtarea->text_size);

    /* Run loop until line is done */
    while (1) {
	/* Iterate cursor through the end of the a word */
	while (*cursor != ' ' && *cursor != '\n' && *cursor != '-' && *cursor != '\0') {
	    cursor++;
	}
        save_word_end = *cursor;
	
	switch (save_word_end) {
	case '\0':
	case '\n':
	    *cursor = '\0';
	    if (cursor - *line_start != 0)  {
		txt_area_make_line(txtarea, *line_start);
		/* surface = TTF_RenderText_Blended(font,  *line_start, txtarea->color); */
		/* if (!surface) { */
		/*     fprintf(stderr, "Error creating line surface in text area. %s\n", SDL_GetError()); */
		/*     exit(1); */
		/* } */
		/* texture = SDL_CreateTextureFromSurface(txtarea->win->rend, surface); */
		/* if (!texture) { */
		/*     fprintf(stderr, "Error creating line texture from surface. %s\n", SDL_GetError()); */
		/* } */
		/* txtarea->line_widths[txtarea->num_lines] = line_w; */
		/* txtarea->line_textures[txtarea->num_lines] = texture; */
		/* txtarea->num_lines++; */
	    }
	    *cursor = save_word_end;
	    *line_start = cursor + 1;
	    if (save_word_end == '\n') {
		return 1;
	    } else {
		return 0;
	    }

        default:
	    *cursor = '\0';
	    TTF_SizeUTF8(font, *line_start, &line_w, &line_h);
	    if (line_w > w) {
		save_last_word_end = *last_word_boundary;
		*last_word_boundary = '\0';
		if (cursor - *line_start != 0)  {
		    txt_area_make_line(txtarea, *line_start);
		    /* surface = TTF_RenderText_Blended(font,  *line_start, txtarea->color); */
		    /* if (!surface) { */
		    /* 	fprintf(stderr, "Error creating line surface in text area. %s\n", SDL_GetError()); */
		    /* 	exit(1); */
		    /* } */
		    /* texture = SDL_CreateTextureFromSurface(txtarea->win->rend, surface); */
		    /* if (!texture) { */
		    /* 	fprintf(stderr, "Error creating line texture from surface. %s\n", SDL_GetError()); */
		    /* } */
		    /* txtarea->line_widths[txtarea->num_lines] = line_w; */
		    /* txtarea->line_textures[txtarea->num_lines] = texture; */
		    /* txtarea->num_lines++; */
		}
		*last_word_boundary = save_last_word_end;
		*line_start = last_word_boundary + 1;
		last_word_boundary = cursor;
		*cursor = save_word_end;
		return 1;
	    }
	    last_word_boundary = cursor;
	    *cursor = save_word_end;
	    
	    cursor++;
	    break;
	    
	}
    }  
    return 0;
}

void txt_area_create_lines(TextArea *txtarea)
{


    TTF_Font *font = ttf_get_font_at_size(txtarea->font, txtarea->text_size);

/* Clear old values if present */

    
    for (int i=0; i<txtarea->num_lines; i++) {
	SDL_DestroyTexture(txtarea->line_textures[i]);
    }
    txtarea->num_lines = 0;
    if (txtarea->layout->num_children == 1) {
	layout_destroy(txtarea->layout->children[0]);
    } else if (txtarea->layout->num_children > 1) {
	fprintf(stderr, "Error: potential memory leak. txtarea layout has more than one child\n");
    }

    
    char *value_copy = strdup(txtarea->value_handle);
    int w = txtarea->layout->rect.w;

    w = w < TXT_AREA_W_MIN ? TXT_AREA_W_MIN : w;
    fprintf(stdout, "CREATING HEADER w fixed w %d\n", w);
    char *line_start = value_copy;
    while (txt_area_create_line(txtarea, &line_start, w) > 0) {
    }
    free(value_copy);

    if (txtarea->text_h == 0) {
	int line_w, line_h;
	TTF_SizeUTF8(font, txtarea->value_handle, &line_w, &line_h);
	txtarea->text_h = line_h;
    }
    Layout *line_template = layout_add_child(txtarea->layout);
    line_template->y.value.intval = txtarea->line_spacing;
    line_template->h.value.intval = txtarea->text_h / txtarea->win->dpi_scale_factor;
    
    for (int i=0; i<txtarea->num_lines; i++) {
	txtarea->layout->h.value.intval += (txtarea->text_h / txtarea->win->dpi_scale_factor) + txtarea->line_spacing;
	layout_add_iter(line_template, VERTICAL, false);
    }

 
}


TextArea *txt_area_create(const char *value, Layout *layout, Font *font, uint8_t text_size, SDL_Color color, Window *win)
{
    TextArea *txtarea = malloc(sizeof(TextArea));
    txtarea->layout = layout;
    txtarea->font = font;
    txtarea->text_size = text_size;
    txtarea->value_handle = value;
    txtarea->num_lines = 0;
    txtarea->color = color;
    txtarea->win = win;
    txtarea->text_h = 0;

    txtarea->line_spacing = TXT_AREA_DEFAULT_LINE_SPACING;
    txt_area_create_lines(txtarea);
    
    return txtarea;
}

void txt_draw(Text *txt)
{
    TTF_Font *font = ttf_get_font_at_size(txt->font, txt->text_size);
    /* fprintf(stderr, "DRAW txt %p, disp: %s\n", txt, txt->display_value); */
    if (txt->display_value[0] == '\0' || !txt->texture) {
	return;
    }
    if (txt->show_cursor) {
	/* fprintf(stderr, "Showing cursor\n"); */
        if (txt->cursor_end_pos > txt->cursor_start_pos) {
            char leftstr[255];
            strncpy(leftstr, txt->display_value, txt->cursor_start_pos);
            leftstr[txt->cursor_start_pos] = '\0';
            char rightstr[255];
            strncpy(rightstr, txt->display_value, txt->cursor_end_pos);
            rightstr[txt->cursor_end_pos] = '\0';
            int wl, wr;
            TTF_SizeUTF8(font, leftstr, &wl, NULL);
            TTF_SizeUTF8(font, rightstr, &wr, NULL);
            SDL_SetRenderDrawColor(main_win->rend, highlight.r, highlight.g, highlight.b, highlight.a);
            SDL_Rect highlight = (SDL_Rect) {
                txt->text_rect.x + wl,
                txt->text_rect.y,
                wr - wl,
                txt->text_rect.h

            };
            SDL_RenderFillRect(main_win->rend, &highlight);
        } else if (txt->cursor_countdown > CURSOR_COUNTDOWN_MAX / 2) {
            char newstr[255];
            strncpy(newstr, txt->display_value, txt->cursor_start_pos);
            newstr[txt->cursor_start_pos] = '\0';
            int w;
            TTF_SizeUTF8(font, newstr, &w, NULL);
            SDL_SetRenderDrawColor(main_win->rend, txt->color.r, txt->color.g, txt->color.b, txt->color.a);
            // set_rend_color(main_win->rend, txt->txt_color);
            int x = txt->text_rect.x + w;
            for (int i=0; i<CURSOR_WIDTH; i++) {

                SDL_RenderDrawLine(main_win->rend, x, txt->text_rect.y, x, txt->text_rect.y + txt->text_rect.h);
                x++;
            }
	    
        }
    }
    if (txt->len > 0) {
        if (SDL_RenderCopy(txt->win->rend, txt->texture, NULL, &(txt->text_rect)) != 0) {
	    fprintf(stderr, "Error: Render Copy failed in txt_draw, on text: \"%s\". %s\n", txt->display_value, SDL_GetError());
	    /* exit(1); */
	}
	/* fprintf(stderr, "Copied rend over to %p!\n", txt->rend); */
    }

}
    


void txt_area_draw(TextArea *txtarea)
{
    for (int i=0; i<txtarea->num_lines; i++) {
	Layout *line_lt = txtarea->layout->children[0]->iterator->iterations[i];
	line_lt->rect.w = txtarea->line_widths[i];
	line_lt->rect.h = txtarea->line_heights[i];
	if (SDL_RenderCopy(txtarea->win->rend, txtarea->line_textures[i], NULL, &(line_lt->rect)) != 0) {
	    fprintf(stderr, "Error: Render Copy failed in txt_draw. %s\n", SDL_GetError());
	    exit(1);
	}
    }
}
