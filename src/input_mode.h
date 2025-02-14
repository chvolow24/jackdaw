/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    input_mode.h

    * Define input mode enum, listing available input modes
 *****************************************************************************************************************/


#ifndef JDAW_GUI_INPUT_MODE_H
#define JDAW_GUI_INPUT_MODE_H

typedef enum input_mode {
/* typedef enum input_mode : uint8_t { */
    GLOBAL=0,
    MENU_NAV=1,
    TIMELINE=2,
    SOURCE=3,
    MODAL=4,
    TEXT_EDIT=5,
    TABVIEW=6
} InputMode;

#define TOP_MODE (main_win->modes[main_win->num_modes - 1])

#endif
