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

#include <stdint.h>
#include <stdbool.h>
#include "SDL.h"

#define MAX_ANNOT_STRLEN 255
#define MAX_MODE_SUBCATS 16
#define MAX_MODE_SUBCAT_FNS 64
#define MAX_FN_KEYBS 8


typedef enum input_mode {
/* typedef enum input_mode : uint8_t { */
    GLOBAL=0,
    MENU_NAV=1,
    TIMELINE=2,
    SOURCE=3,
    MODAL=4,
    TEXT_EDIT=5,
    TABVIEW=6,
    AUTOCOMPLETE_LIST=7
} InputMode;

#define NUM_INPUT_MODES 8

#define TOP_MODE (main_win->modes[main_win->num_modes - 1])

typedef struct mode Mode;
typedef struct keybinding Keybinding;
typedef struct user_fn {
    const char *fn_id;
    const char *fn_display_name;
    char annotation[MAX_ANNOT_STRLEN];
    bool is_toggle;
    void (*do_fn)(void *arg);
    Mode *mode;

    Keybinding *key_bindings[MAX_FN_KEYBS];
    int num_keybindings;
    /* int hashes[MAX_FN_KEYBS]; */
    
    /* uint16_t i_states[MAX_FN_KEYBS]; */
    /* SDL_Keycode keycodes[MAX_FN_KEYBS]; */
    /* int num_hashes; */
} UserFn;

typedef struct mode_subcat {
    const char *name;
    UserFn *fns[MAX_MODE_SUBCAT_FNS];
    uint8_t num_fns;
    Mode *mode;
} ModeSubcat;

typedef struct mode {
    const char *name;
    ModeSubcat *subcats[MAX_MODE_SUBCATS];
    uint8_t num_subcats;
    InputMode im;
} Mode;

const char *input_mode_str(InputMode im);
InputMode input_mode_from_str(char *str);


void mode_load_all();
/* void mode_load_global(); */
/* void mode_load_menu_nav(); */
/* void mode_load_timeline(); */
/* void mode_load_source(); */
/* void mode_load_modal(); */
/* void mode_load_text_edit(); */
/* void mode_load_tabview(); */
/* void mode_load_autocomplete_list(); */


#endif
