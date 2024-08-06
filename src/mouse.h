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
    mouse.h

    * Functions related to mouse clicks
 *****************************************************************************************************************/

#ifndef JDAW_MOUSE_H
#define JDAW_MOUSE_H

#include <stdint.h>
#include <stdbool.h>

void mouse_triage_click_project(uint8_t button);
void mouse_triage_motion_menu();
void mouse_triage_click_menu(uint8_t button);
void mouse_triage_motion_timeline();
void mouse_triage_motion_modal();
void mouse_triage_click_modal(uint8_t button);
void mouse_triage_wheel(int x, int y);
void mouse_triage_click_text_edit(uint8_t button);
bool mouse_triage_motion_page();
bool mouse_triage_click_page();
bool mouse_triage_click_tabview();
bool mouse_triage_motion_tabview();


#endif
