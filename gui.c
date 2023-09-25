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

extern Project *proj;
extern uint8_t scale_factor;
// extern TTF_Font *fonts[11];
// extern TTF_Font *bold_fonts[11];
extern bool dark_mode;

extern JDAW_Color default_textbox_text_color;
extern JDAW_Color default_textbox_border_color;
extern JDAW_Color default_textbox_fill_color;
extern JDAW_Color clear;

JDAWWindow *create_jwin(const char *title, int x, int y, int w, int h)
{
    JDAWWindow *jwin = malloc(sizeof(JDAWWindow));
    jwin->win = SDL_CreateWindow(title, x, y, w, h, (Uint32)DEFAULT_WINDOW_FLAGS);
    if (!jwin->win) {
        fprintf(stderr, "Error creating window: %s", SDL_GetError());
        exit(1);
    }
    jwin->rend = SDL_CreateRenderer(jwin->win, -1, (Uint32)DEFAULT_RENDER_FLAGS);
    if (!jwin->rend) {
        fprintf(stderr, "Error creating renderer: %s", SDL_GetError());
    }

    SDL_SetRenderDrawBlendMode(jwin->rend, SDL_BLENDMODE_BLEND);

    int rw = 0, rh = 0, ww = 0, wh = 0;
    SDL_GetWindowSize(jwin->win, &ww, &wh);
    SDL_GetRendererOutputSize(jwin->rend, &rw, &rh);
    jwin->scale_factor = (float)rw / (float)ww;
    scale_factor = jwin->scale_factor;
    if (jwin->scale_factor != (float)rh / (float)wh) {
        fprintf(stderr, "Error: scale factor w != h.\n");
    }

    init_fonts(jwin->fonts, OPEN_SANS);
    init_fonts(jwin->bold_fonts, OPEN_SANS_BOLD);
    SDL_GL_GetDrawableSize(jwin->win, &(jwin->w), &(jwin->h));

    return jwin;
}


void destroy_jwin(JDAWWindow *jwin)
{
    SDL_DestroyRenderer(jwin->rend);
    SDL_DestroyWindow(jwin->win);
    close_fonts(jwin->fonts);
    close_fonts(jwin->bold_fonts);
    free(jwin);
    jwin = NULL;
}

void reset_dims(JDAWWindow *jwin)
{
    SDL_GL_GetDrawableSize(jwin->win, &(jwin->w), &(jwin->h));
}

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
    if (!scale_factor) {
        return (SDL_Rect) {0,0,0,0};
    }

    SDL_Rect ret;
    switch (x.dimType) {
        case ABS:
            ret.x = parent_rect.x + x.value * scale_factor;
            break;
        case REL:
            ret.x = parent_rect.x + x.value *  parent_rect.w / 100;
            break;
    }
    switch (y.dimType) {
        case ABS:
            ret.y = parent_rect.y + y.value * scale_factor;
            break;
        case REL:
            ret.y = parent_rect.y + y.value * parent_rect.h / 100;
            break;
    }
    switch (w.dimType) {
        case ABS:
            ret.w = w.value * scale_factor;
            break;
        case REL:
            ret.w = w.value * parent_rect.w / 100;
            break;
    }
    switch (h.dimType) {
        case ABS:
            ret.h = h.value * scale_factor;
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
    void (*onclick)(Textbox *self, void *object),
    void *target,
    void (*onhover)(void *self),
    char *tooltip,
    int radius
)
{
    if (font == NULL) {
        fprintf(stderr, "Error: font is null: %p", font);
        exit(1);
    }
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
    if (TTF_SizeUTF8(font, value, &txtw, &txth) == -1) {
        fprintf(stderr, "Error: could not size text. %s\n", TTF_GetError());
    };
    tb->txt_container.w = txtw;
    tb->txt_container.h = txth;
    bool test = (fixed_w);
    strcpy(tb->display_value, tb->value);
    if (fixed_w) {
        tb->container.w = fixed_w;
        /* Truncate text if it doesn't fit in fixed width container (handle draw space overflow) */
        if (txtw > fixed_w) {
            int len = strlen(tb->value);
            char truncated[len];
            for (int i=0; i<strlen(tb->value); i++) {
                sprintf(&(truncated[i]), "%c...", tb->value[i]);
                TTF_SizeUTF8(font, truncated, &txtw, NULL);
                if (txtw > fixed_w - 30) {
                    strcpy(tb->display_value, truncated);
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
    tb->mouse_hover = false;
    tb->target = target;
    fprintf(stderr, "created tb with val: %s, dis val: %s\n", tb->value, tb->display_value);
    return tb;

}

/* Reposition a textbox */
void position_textbox(Textbox *tb, int x, int y)
{
    tb->container.x = x;
    tb->container.y = y;
    tb->txt_container.x = x + tb->padding * scale_factor;
    tb->txt_container.y = y + tb->padding * scale_factor;
}

void destroy_textbox(Textbox *tb)
{
    free(tb);
    tb = NULL;
}

void reset_textbox_value(Textbox *tb, char *new_value)
{
    tb->value = new_value;
    int txtw;
    strcpy(tb->display_value, tb->value);
    TTF_SizeUTF8(tb->font, tb->value, &txtw, NULL);
    /* Truncate text if it doesn't fit in fixed width container (handle draw space overflow) */
    if (txtw > tb->container.w) {
        int len = strlen(tb->value);
        char truncated[len];
        for (int i=0; i<strlen(tb->value); i++) {
            sprintf(&(truncated[i]), "%c...", tb->value[i]);
            TTF_SizeUTF8(tb->font, truncated, &txtw, NULL);
            if (txtw > tb->container.w - 20) {
                strcpy(tb->display_value, truncated);
                break;
            }
        }
    }
}

/* Opens a new event loop to handle textual input. Returns new value */
char *edit_textbox(Textbox *tb)
{
    fprintf(stderr, "Edit textbox\n");
    bool done = false;
    bool shift_down = false;
    while (!done) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT 
                || (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) 
                || e.type == SDL_MOUSEBUTTONDOWN) {
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

        reset_textbox_value(tb, tb->value);
        // int txtw;
        // TTF_SizeUTF8(tb->font, tb->value, &txtw, NULL);

        // /* Truncate text if it doesn't fit in fixed width container (handle draw space overflow) */
        // if (txtw > tb->container.w) {
        //     int len = strlen(tb->value);
        //     char truncated[len];
        //     for (int i=0; i<strlen(tb->value); i++) {
        //         sprintf(&(truncated[i]), "%c...", tb->value[i]);
        //         TTF_SizeUTF8(tb->font, truncated, &txtw, NULL);
        //         if (txtw > tb->container.w - 20) {
        //             strcpy(tb->value, truncated);
        //             break;
        //         }
        //     }
        // }
        draw_project(proj);
        // draw_jwin_menus(proj->jwin);
        SDL_RenderPresent(proj->jwin->rend);
        SDL_Delay(1);
    }
    return tb->value;
}

TextboxList *create_textbox_list_from_strings(
    int fixed_w,
    int padding,
    TTF_Font *font,
    char **values,
    uint8_t num_values,
    JDAW_Color *txt_color,
    JDAW_Color *border_color,
    JDAW_Color *bckgrnd_clr,
    void (*onclick)(Textbox *self, void *object),
    void *target,
    void (*onhover)(void *object),
    char *tooltip,
    int radius
)
{
    TextboxList *list = malloc(sizeof(TextboxList));
    int max_text_w = 0;
    for (uint8_t i=0; i<num_values; i++) {
        list->textboxes[i] = create_textbox(
            fixed_w, 0, padding, font, values[i], txt_color, &clear, &clear, onclick, target, onhover, tooltip, radius
        );
        if (list->textboxes[i]->txt_container.w > max_text_w) {
            max_text_w = list->textboxes[i]->txt_container.w;
        }
    }
    for (uint8_t i=0; i<num_values; i++) {
        list->textboxes[i]->container.w = max_text_w + 4 * padding;
    }
    list->num_textboxes = num_values;
    list->container = (SDL_Rect) {0, 0, max_text_w + 8 * padding, (list->textboxes[0]->container.h + padding) * num_values};
    list->txt_color = txt_color;
    list->border_color = border_color;
    list->bckgrnd_color = bckgrnd_clr;
    list->padding = padding;
    return list;
}

void destroy_textbox_list(TextboxList *tbl)
{
    for (uint8_t i=0; i<tbl->num_textboxes; i++) {
        destroy_textbox(tbl->textboxes[i]);
    }
    free(tbl);
    tbl = NULL;
}

TextboxList *create_menulist(
    JDAWWindow *jwin,
    int fixed_w,
    int padding,
    TTF_Font *font,
    char **values,
    uint8_t num_values,
    void (*onclick)(Textbox *self, void *object),
    void *target
)
{
    TextboxList *tbl = create_textbox_list_from_strings(
        fixed_w,
        padding,
        font,
        values,
        num_values,
        NULL,
        NULL,
        NULL,
        onclick,
        target,
        NULL,
        NULL,
        0
    );

    jwin->active_menus[jwin->num_active_menus] = tbl;
    jwin->num_active_menus += 1;

    return tbl;
}


void destroy_pop_menulist(JDAWWindow *jwin)
{
    if (jwin->num_active_menus == 0) {
        return;
    }
    destroy_textbox_list(jwin->active_menus[jwin->num_active_menus - 1]);
    jwin->active_menus[jwin->num_active_menus - 1] = NULL;
    jwin->num_active_menus -= 1;
}

/* Reposition a textbox list */
void position_textbox_list(TextboxList *tbl, int x, int y)
{
    tbl->container.x = x;
    tbl->container.y = y;
    y += tbl->padding;
    for (uint8_t i=0; i<tbl->num_textboxes; i++) {
        position_textbox(tbl->textboxes[i], x + tbl->padding, y);
        y += tbl->padding + tbl->textboxes[0]->container.h;
    }
}

/* Run this in every JDAWWindow animation loop to ensure active menus are highlighted appropriately */
void menulist_hover(JDAWWindow *jwin, SDL_Point *mouse_p)
{   
    for (uint8_t i=0; i<jwin->num_active_menus; i++) {
        TextboxList *menu = jwin->active_menus[i];
        if (SDL_PointInRect(mouse_p, &(menu->container))) {
            for (uint8_t j=0; j<menu->num_textboxes; j++) {
                Textbox *tb = menu->textboxes[j];
                if (SDL_PointInRect(mouse_p, &(tb->container))) {
                    tb->mouse_hover = true;
                } else {
                    tb->mouse_hover = false;
                }
            }
        }
    }
}

/* Run this in every JDAWWindow animation loop to ensure active menu click events are run */
bool menulist_triage_click(JDAWWindow *jwin, SDL_Point *mouse_p)
{
    fprintf(stderr, "Mouse click. Active menues: %d\n", jwin->num_active_menus);
    for (uint8_t i=0; i<jwin->num_active_menus; i++) {
        fprintf(stderr, "\tmenu %d\n", i);

        TextboxList *menu = jwin->active_menus[i];
        if (SDL_PointInRect(mouse_p, &(menu->container))) {
            for (uint8_t j=0; j<menu->num_textboxes; j++) {
                fprintf(stderr, "\titem %d\n", j);

                Textbox *tb = menu->textboxes[j];
                if (SDL_PointInRect(mouse_p, &(tb->container))) {
                    tb->onclick(tb, tb->target);
                    destroy_pop_menulist(jwin);
                    return true;
                    /* Need to have target and some value to pass to the target*/
                }
            }
        }
    }
    if (jwin->num_active_menus != 0) {
        destroy_pop_menulist(jwin);
    }
    return false;
}