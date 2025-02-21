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

#define AUTOCOMPLETE_ENTRY_BUFLEN 64

struct autocompletion_item {
    const char *str;
    void *obj;
};

typedef struct autocompletion AutoCompletion;
typedef struct autocompletion {
    Layout *layout;
    char entry_buf[AUTOCOMPLETE_ENTRY_BUFLEN];
    TextEntry *entry;
    TextLines *lines;
    int (*update_records)(AutoCompletion *self, struct autocompletion_item **items_arr_p);
    CreateTline create_tline;
    TlinesFilter tline_filter;

    /* TLinesItem *(*create_tline)(void ***, Layout *, void *, int (*filter)(void *item, void *arg)); */
    /* int (*tline_filter)(void *item, void *xarg); */
    
    int selection; /* -1 if in textentry, otherwise index of selected line */
} AutoCompletion;

/* void autocompletion_init(AutoCompletion *ac, Layout *layout, int update_records(AutoCompletion *self, struct autocompletion_item **items_arr_p)); */
void autocompletion_init(
    AutoCompletion *ac,
    Layout *layout,
    int update_records(AutoCompletion *self, struct autocompletion_item **dst_loc),
    CreateTline create_tline,
    TlinesFilter filter);
    /* TLinesItem *(create_tline)(void ***, Layout *, void *, int (*filter)(void *item, void *arg)), */
    /* int (*filter)(void *item, void *xarg)); */

void autocompletion_draw(AutoCompletion *ac);

#endif
