/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    input.h

    * Define types related to user input, keybindings, and input modes
 *****************************************************************************************************************/


#ifndef JDAW_GUI_INPUT_STATE_H
#define JDAW_GUI_INPUT_STATE_H

#include <stdint.h>
#include "SDL.h"

#include "menu.h"
#include "input_mode.h"

#define INPUT_HASH_SIZE 1024
#define INPUT_KEYUP_HASH_SIZE 128
#define MAX_ANNOT_STRLEN 255

#define NUM_INPUT_MODES 7
#define MAX_MODE_SUBCATS 16
#define MAX_MODE_SUBCAT_FNS 64

#define MAX_FN_KEYBS 8

/* Input state bitmasks */
#define I_STATE_QUIT 0x01
#define I_STATE_SHIFT 0x02
#define I_STATE_CMDCTRL 0x04
#define I_STATE_META 0x08
#define I_STATE_MOUSE_L 0x10
#define I_STATE_MOUSE_R 0x20
#define I_STATE_MOUSE_M 0x40
#define I_STATE_C_X 0x80
#define I_STATE_K 0x100

typedef struct mode Mode;

typedef struct user_fn {
    const char *fn_id;
    const char *fn_display_name;
    char annotation[MAX_ANNOT_STRLEN];
    bool is_toggle;
    void (*do_fn)(void *arg);
    Mode *mode;
    int hashes[MAX_FN_KEYBS];
    uint16_t i_states[MAX_FN_KEYBS];
    SDL_Keycode keycodes[MAX_FN_KEYBS];
    int num_hashes;
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


typedef struct keybinding {
    /* Key binding information */
    InputMode mode;
    uint16_t i_state;
    SDL_Keycode keycode;
    char *keycmd_str; /* E.g. "C-t", "C-S-3" */

    /* Modifiable pointer to UserFn struct */
    UserFn *fn;
} Keybinding;

typedef struct keybinding_node KeybNode;
typedef struct keybinding_node {
    Keybinding *kb;
    KeybNode *next;
} KeybNode;
/* Get an input mode str from enum value */
const char *input_mode_str(InputMode mode);

/* Must be called when program starts. Sets all vals in input_hash_table to NULL. */
void input_init_hash_table();

/* Get the UserInput struct for the given key inputs */
UserFn *input_get(uint16_t i_state, SDL_Keycode keycode);

/* Bind a function. This does NOT check if function is bound to another key cmd */
void input_bind_fn(UserFn *fn, uint16_t i_state, SDL_Keycode keycode, InputMode mode);

/* Retrieve the UserFn struct from its parent mode */
UserFn *input_get_fn_by_id(char *id, InputMode im);

/* Load a keybinding config file and assign keybindings accordingly */
void input_load_keybinding_config(const char *filepath);

/* Must be run at start time to initialize all modes and user fns */
void input_init_mode_load_all();

/* Create a GUI menu from a mode */
void input_create_menu_from_mode(InputMode im);

Menu *input_create_master_menu();

/* Public for debugging only */
char *input_get_keycmd_str(uint16_t i_state, SDL_Keycode keycode);

/* Free memory related to input subsystem */
void input_quit();

/* Write a markdown function describing available modes and functions */
void input_create_function_reference();

bool input_function_is_accessible(UserFn *fn, Window *win);

#endif

