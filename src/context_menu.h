#ifndef JDAW_CONTEXT_MENU_H
#define JDAW_CONTEXT_MENU_H

#include "layout.h"

typedef enum ctx_type {
    CTX_CLIPREF,
    CTX_AUDIO,
    CTX_MIDI,
    CTX_TRACK,
    CTX_CLICK_SEGMENT,
    CTX_CLICK_TRACK,
    CTX_AUTOMATION,
    CTX_TIMELINE,
    CTX_TIMELINE_NAV,
    CTX_PROJECT,
    NUM_CTX_TYPES
} CtxType;


typedef struct ctx {
    CtxType type;
    void *obj;
    const char *name;
    Layout *layout;
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

void context_at_point_create_menu(SDL_Point point);
void context_at_cursor_create_menu();


#endif
