/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    mouse.h

    * Functions related to mouse clicks
 *****************************************************************************************************************/

#ifndef JDAW_MOUSE_H
#define JDAW_MOUSE_H

#include <stdint.h>
#include <stdbool.h>
#include "layout.h"

int mouse_triage_click(SDL_Event e);

void mouse_triage_click_project(uint8_t button);
void mouse_triage_motion_menu();
void mouse_triage_click_menu(uint8_t button);
void mouse_triage_motion_timeline(int xrel, int yrel);
void mouse_triage_motion_modal();
void mouse_triage_click_modal(uint8_t button);
Layout *mouse_triage_wheel(int x, int y, bool dynamic);
bool mouse_triage_click_text_edit(uint8_t button);
bool mouse_triage_motion_page();
bool mouse_triage_click_page();
bool mouse_triage_click_tabview();
bool mouse_triage_motion_tabview();
void mouse_triage_motion_autocompletion();
void mouse_triage_click_autocompletion();


#endif
