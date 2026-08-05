#ifndef JDAW_CONTEXT_MENU_H
#define JDAW_CONTEXT_MENU_H

#include "input.h"

typedef enum ctx_type {
    CTX_CLIPREF_AUDIO,
    CTX_CLIPREF_MIDI,
    CTX_TRACK,
    CTX_CLICK_SEGMENT,
    CTX_CLICK_TRACK,
    CTX_AUTOMATION,
    CTX_TIMELINE,
    CTX_PROJECT,
    NUM_CTX_TYPES
} CtxType;


typedef struct ctx {
    CtxType type;
    void *obj;
    const char *name;
} Ctx;

typedef struct CtxFn {
    const char *display_name;
    void (*do_fn)(void *ctx); /* Compatible with menu item fn */
    UserFn *related_userfn;
} CtxFn;

int context_at_cursor(Ctx **list_dst);

const char *context_type_name(CtxType t);

/* Getting context from cursor:
   - if there's a clipref at cursor, CT
   - automatically, parent TRACK
   - automatically, parent TL
 */


void context_menu_init();
Menu *context_menu_create(int num_ctxs);


#endif
