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

#include <stdbool.h>
#include <string.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "layout.h"
#include "lt_params.h"
#include "openfile.h"
#include "text.h"
#include "text.h"
#include "window.h"

#define CURSOR_WIDTH 4
#define DTTD_LN_LEN 20
#define MASK_CLR 20, 20, 20, 230

#define NAMERECT_H 24
#define NAMERECT_W 200
#define NAMERECT_PAD 4

extern Layout *main_lt;
extern Layout *clicked_lt;
extern Layout *param_lt;
extern Layout *openfile_lt;
extern Window *main_win;
extern bool show_layout_params;
bool show_openfile;
//extern TTF_Font *open_sans;
extern LTParams *lt_params;
extern OpenFile *openfile;

SDL_Color white = {255, 255, 255, 255};
SDL_Color clr_white = {255, 255, 255, 127};
SDL_Color highlight = {0, 0, 255, 255};
SDL_Color iter_clr = {0, 100, 100, 255};

SDL_Color rect_clrs[2] = {
    {255, 0, 0, 255},
    {0, 255, 0, 255}
};

SDL_Color rect_clrs_dttd[2] = {
    {255, 0, 0, 100},
    {0, 255, 0, 100}
};

//int SDL_RenderDrawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2)
void draw_dotted_horizontal(SDL_Renderer *rend, int x1, int x2, int y)
{
    while (x1 < x2) {
        SDL_RenderDrawLine(rend, x1, y, x1 + DTTD_LN_LEN, y);
        x1 += DTTD_LN_LEN * 2;
    }
}

void draw_dotted_vertical(SDL_Renderer *rend, int x, int y1, int y2)
{
    while (y1 < y2) {
        SDL_RenderDrawLine(rend, x, y1, x, y1 + DTTD_LN_LEN);
        y1 += DTTD_LN_LEN * 2;
    }
}

void txt_draw(Text *txt)
{
    if (txt->display_value[0] == '\0' || !txt->texture) {
	return;
    }
    if (txt->show_cursor) {
        if (txt->cursor_end_pos > txt->cursor_start_pos) {
            char leftstr[255];
            strncpy(leftstr, txt->display_value, txt->cursor_start_pos);
            leftstr[txt->cursor_start_pos] = '\0';
            char rightstr[255];
            strncpy(rightstr, txt->display_value, txt->cursor_end_pos);
            rightstr[txt->cursor_end_pos] = '\0';
            int wl, wr;
            TTF_SizeUTF8(txt->font, leftstr, &wl, NULL);
            TTF_SizeUTF8(txt->font, rightstr, &wr, NULL);
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
            TTF_SizeUTF8(txt->font, newstr, &w, NULL);
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
        SDL_RenderCopy(txt->rend, txt->texture, NULL, &(txt->text_rect));
    }

}

void draw_layout_params()
{
    SDL_SetRenderDrawColor(main_win->rend, MASK_CLR);
    SDL_RenderFillRect(main_win->rend, &(param_lt->rect));  

    txt_draw(lt_params->name_label);
    txt_draw(lt_params->x_type_label);
    txt_draw(lt_params->y_type_label);
    txt_draw(lt_params->w_type_label);
    txt_draw(lt_params->h_type_label);
    txt_draw(lt_params->name_value);
    txt_draw(lt_params->x_type_value);
    txt_draw(lt_params->y_type_value);
    txt_draw(lt_params->w_type_value);
    txt_draw(lt_params->h_type_value);
    txt_draw(lt_params->x_value);
    txt_draw(lt_params->y_value);
    txt_draw(lt_params->w_value);
    txt_draw(lt_params->h_value);
}

void draw_openfile_dialogue() 
{
    SDL_SetRenderDrawColor(main_win->rend, MASK_CLR);
    SDL_RenderFillRect(main_win->rend, &(layout_get_child_by_name(openfile_lt, "container")->rect));
    txt_draw(openfile->label);
    txt_draw(openfile->filepath);
}

void draw_layout(Window *win, Layout *lt)
{
    if (lt->type == PRGRM_INTERNAL) {
        return;
    }

    if (lt->iterator) {
        for (int i=0; i<lt->iterator->num_iterations; i++) {
            draw_layout(win, lt->iterator->iterations[i]);
        }
    }
    
    SDL_Color picked_clr;
    if (lt->type == ITERATION) {
        picked_clr = iter_clr;
    } else {
        picked_clr = lt->selected ? rect_clrs[1] : rect_clrs[0];
    }

    SDL_SetRenderDrawColor(win->rend, picked_clr.r, picked_clr.g, picked_clr.b, picked_clr.a);

    if (lt->selected) {
        txt_draw(lt->namelabel);
        SDL_RenderDrawRect(win->rend, &(lt->label_rect));
    }
    SDL_RenderDrawRect(win->rend, &(lt->rect));

    if (lt->type != ITERATION) {
	SDL_Color dotted_clr = lt->selected ? rect_clrs_dttd[1] : rect_clrs_dttd[0];
        SDL_SetRenderDrawColor(win->rend, dotted_clr.r, dotted_clr.g, dotted_clr.b, dotted_clr.a);
        draw_dotted_horizontal(win->rend, 0, win->w, lt->rect.y);
        draw_dotted_horizontal(win->rend, 0, win->w, lt->rect.y + lt->rect.h);
        draw_dotted_vertical(win->rend, lt->rect.x, 0, win->h);
        draw_dotted_vertical(win->rend, lt->rect.x + lt->rect.w, 0, win->h);
    }


    for (uint8_t i=0; i<lt->num_children; i++) {
        draw_layout(win, lt->children[i]);
    }

}

void layout_draw_main()
{
    SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 0);
    SDL_RenderClear(main_win->rend);
    draw_layout(main_win, main_lt);
    if (show_layout_params) {
        draw_layout_params();
    }
    if (show_openfile) {
        draw_openfile_dialogue();
    }
    SDL_RenderPresent(main_win->rend);
}
