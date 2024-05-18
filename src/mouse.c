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
    mouse.c

    * Functions related to mouse clicks
 *****************************************************************************************************************/


#include "project.h"

extern Window *main_win;
extern Project *proj;

//SDL_BUTTON_LEFT | SDL_BUTTON_RIGHT

static void mouse_triage_click_timeline(uint8_t button)
{

}

static void mouse_triage_click_control_bar(uint8_t button)
{


}

void mouse_triage_click_project(uint8_t button)
{
    Layout *tl_lt = layout_get_child_by_name_recursive(proj->layout, "timeline");
    
    SDL_Point mousep = main_win->mousep;
    if (SDL_PointInRect(&main_win->mousep, &tl_lt->rect)) {
	mouse_triage_click_timeline(button);
    } else if (SDL_PointInRect(&main_win->mousep, proj->control_bar_rect)) {
	mouse_triage_click_control_bar(button);
    }
    

}
