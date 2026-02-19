/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    autocompletion.h

    * typedef structs related to autocompletion display & navigation
 *****************************************************************************************************************/

#ifndef JDAW_AUTOCOMPLETION_H
#define JDAW_AUTOCOMPLETION_H

#include "components.h"
#include "textbox.h"
#include "autocompletion_struct.h" /* annoying */




/* typedef struct autocompletion AutoCompxletion; */

/* void autocompletion_init(AutoCompletion *ac, Layout *layout, int update_records(AutoCompletion *self, struct autocompletion_item **items_arr_p)); */
void autocompletion_init(
    AutoCompletion *ac,
    Layout *layout,
    const char *label,
    int update_records(AutoCompletion *self, struct autocompletion_item **dst_loc),
    TlinesFilter filter);
    /* TLinesItem *(create_tline)(void ***, Layout *, void *, int (*filter)(void *item, void *arg)), */
    /* int (*filter)(void *item, void *xarg)); */

void autocompletion_deinit(AutoCompletion *ac);

void autocompletion_draw(AutoCompletion *ac);

void autocompletion_reset_selection(AutoCompletion *ac, int new_sel);
void autocompletion_select(AutoCompletion *ac);
void autocompletion_escape();
bool autocompletion_triage_mouse_motion(AutoCompletion *ac);
bool autocompletion_triage_mouse_click(AutoCompletion *ac);
Layout *autocompletion_scroll(int y, bool dynamic);
#endif
