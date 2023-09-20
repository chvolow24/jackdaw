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

/*****************************************************************************************************************
    gui.c

    * Creation of GUI primitives, like Textbox
    * Relative rect sizing
 *****************************************************************************************************************/

#include <float.h>
#include <math.h>
#include <string.h>
#include "SDL.h"
#include "theme.h"
#include "project.h"
#include "text.h"
#include "theme.h"
#include "audio.h"
#include "gui.h"
#include "draw.h"

extern JDAW_Color default_textbox_text_color;
extern JDAW_Color default_textbox_border_color;
extern JDAW_Color default_textbox_fill_color;


//TODO: Replace project arguments with reference to global var;
extern Project *proj;

/* Get SDL_Rect with size and position relative to parent window */
SDL_Rect relative_rect(SDL_Rect *win_rect, float x_rel, float y_rel, float w_rel, float h_rel)
{
    SDL_Rect ret;
    ret.x = x_rel * win_rect->w;
    ret.y = y_rel * win_rect->h;
    ret.w = w_rel * win_rect->w;
    ret.h = h_rel * win_rect->h;
    return ret;
}

/* Get an SDL_Rect from four dimensions, which can be relative or absolute */
SDL_Rect get_rect(SDL_Rect parent_rect, Dim x, Dim y, Dim w, Dim h) {
    if (!proj || !(proj->scale_factor)) {
        return (SDL_Rect) {0,0,0,0};
    }

    SDL_Rect ret;
    switch (x.dimType) {
        case ABS:
            ret.x = parent_rect.x + x.value * proj->scale_factor;
            break;
        case REL:
            ret.x = parent_rect.x + x.value *  parent_rect.w / 100;
            break;
    }
    switch (y.dimType) {
        case ABS:
            ret.y = parent_rect.y + y.value * proj->scale_factor;
            break;
        case REL:
            ret.y = parent_rect.y + y.value * parent_rect.h / 100;
            break;
    }
    switch (w.dimType) {
        case ABS:
            ret.w = w.value * proj->scale_factor;
            break;
        case REL:
            ret.w = w.value * parent_rect.w / 100;
            break;
    }
    switch (h.dimType) {
        case ABS:
            ret.h = h.value * proj->scale_factor;
            break;
        case REL:
            ret.h = h.value * parent_rect.h / 100;
            break;
    }
    return ret;
}

/* Create a new textbox struct, default position at the origin */
Textbox *create_textbox(
    int fixed_w, 
    int fixed_h, 
    int padding, 
    TTF_Font *font, 
    char *value,
    JDAW_Color *txt_color,
    JDAW_Color *border_color,
    JDAW_Color *bckgrnd_clr,
    void (*onclick)(void *self),
    void (*onhover)(void *self),
    char *tooltip,
    int radius
)
{
    Textbox *tb = malloc(sizeof(Textbox));
    tb->container.x = 0;
    tb->container.y = 0;
    tb->padding = padding;
    tb->font = font;
    tb->value = value;
    tb->radius = radius;
    if (txt_color) {
        tb->txt_color = txt_color;
    } else {
        tb->txt_color = &default_textbox_text_color;
    }
    if (border_color) {
        tb->border_color = border_color;
    } else {
        tb->border_color = &default_textbox_border_color;
    }
    if (bckgrnd_clr) {
        tb->bckgrnd_color = bckgrnd_clr;
    } else {
        tb->bckgrnd_color = &default_textbox_fill_color;
    }
    tb->onclick = onclick;
    tb->onhover = onhover;

    int txtw, txth;
    TTF_SizeText(font, value, &txtw, &txth);

    if (fixed_w) {
        tb->container.w = fixed_w;
        /* Truncate text if it doesn't fit in fixed width container (handle draw space overflow) */
        if (txtw > fixed_w) {
            int len = strlen(tb->value);
            char truncated[len];
            for (int i=0; i<strlen(tb->value); i++) {
                sprintf(&(truncated[i]), "%c...", tb->value[i]);
                TTF_SizeText(font, truncated, &txtw, NULL);
                if (txtw > fixed_w - 20) {
                    strcpy(tb->value, truncated);
                    break;
                }
            }
        }
    } else {
        tb->container.w = txtw + padding * 2;
    }
    if (fixed_h) {
        tb->container.h = fixed_h;
    } else {
        tb->container.h = txth + padding * 2;
    }
    tb->txt_container.w = txtw;
    tb->txt_container.h = txth;
    return tb;

}

/* Reposition a textbox */
void position_textbox(Textbox *tb, int x, int y)
{
    int scale_factor = proj && proj->scale_factor ? proj->scale_factor : 1;
    tb->container.x = x;
    tb->container.y = y;
    tb->txt_container.x = x + tb->padding * scale_factor;
    tb->txt_container.y = y + tb->padding * scale_factor;
}

/* Opens a new event loop to handle textual input */
void edit_textbox(Textbox *tb)
{
    bool done = false;
    bool shift_down = false;
    while (!done) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_MOUSEBUTTONDOWN) {
                done = true;
                /* Push the event back to the main event stack, so it can be handled in main.c */
                SDL_PushEvent(&e);
                break;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_RETURN:
                    case SDL_SCANCODE_KP_ENTER:
                        done = true;
                        break;
                    case SDL_SCANCODE_LSHIFT:
                    case SDL_SCANCODE_RSHIFT:
                        shift_down = true;
                        break;
                    case SDL_SCANCODE_LEFT:
                        if (tb->cursor_pos > 0) {
                            tb->cursor_pos--;
                        }
                        break;
                    case SDL_SCANCODE_RIGHT:
                        if (tb->cursor_pos < strlen(tb->value)) {
                            tb->cursor_pos++;
                        }
                        break;
                    case SDL_SCANCODE_SPACE:
                        tb->value[tb->cursor_pos] = ' ';
                        tb->value[tb->cursor_pos + 1] = '\0';
                        tb->cursor_pos++;
                        tb->cursor_countdown = CURSOR_COUNTDOWN;
                        break;
                    case SDL_SCANCODE_BACKSPACE: {
                        int len;
                        if (tb->cursor_pos > 0) {
                            tb->value[tb->cursor_pos - 1] = '\0';
                            tb->cursor_pos--;
                            tb->cursor_countdown = CURSOR_COUNTDOWN;
                        }
                    }
                    default: {
                        const char *key = SDL_GetKeyName(e.key.keysym.sym);
                        if (strlen(key) > 1) {
                            break;
                        }
                        int cmp = strcmp(key, "A");
                        if (cmp < 26 && cmp >= 0) {
                            tb->value[tb->cursor_pos] = shift_down ? 'A' + cmp : 'a' + cmp;
                            tb->value[tb->cursor_pos + 1] = '\0';
                            tb->cursor_pos++;
                            tb->cursor_countdown = CURSOR_COUNTDOWN;

                        } else if ((cmp = strcmp(key, "0")) < 10  && cmp >= 0) {
                            tb->value[tb->cursor_pos] = '0' + cmp;
                            tb->value[tb->cursor_pos + 1] = '\0';
                            tb->cursor_pos++;
                            tb->cursor_countdown = CURSOR_COUNTDOWN;

                        }
                    }
                }
                // sprintf(track->name, new_name);
            } else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_LSHIFT:
                    case SDL_SCANCODE_RSHIFT:
                        shift_down = false;
                        break;
                    default:
                        break;
                }
            }
        } // exit event loop

        int txtw;
        TTF_SizeText(tb->font, tb->value, &txtw, NULL);

        /* Truncate text if it doesn't fit in fixed width container (handle draw space overflow) */
        if (txtw > tb->container.w) {
            int len = strlen(tb->value);
            char truncated[len];
            for (int i=0; i<strlen(tb->value); i++) {
                sprintf(&(truncated[i]), "%c...", tb->value[i]);
                TTF_SizeText(tb->font, truncated, &txtw, NULL);
                if (txtw > tb->container.w - 20) {
                    strcpy(tb->value, truncated);
                    break;
                }
            }
        }
        draw_project(proj);
        SDL_Delay(1);

    }
}