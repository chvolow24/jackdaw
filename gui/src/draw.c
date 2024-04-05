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



extern SDL_Color color_global_clear;



//int SDL_RenderDrawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2)

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


void layout_draw_main()
{
    window_start_draw(main_win, &color_global_clear);
    /* SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 0); */
    /* SDL_RenderClear(main_win->rend); */
    layout_draw(main_win, main_lt);

    if (clicked_lt) {
	SDL_SetRenderDrawColor(main_win->rend, 0, 255, 0, 255);
	SDL_RenderDrawRect(main_win->rend, &clicked_lt->rect);
    }
    if (show_layout_params) {
        draw_layout_params();
    }
    if (show_openfile) {
        draw_openfile_dialogue();
    }
    window_end_draw(main_win);
    /* SDL_RenderPresent(main_win->rend); */
}
