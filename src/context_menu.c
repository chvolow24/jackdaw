#include <stdio.h>
#include <stdlib.h>
#include "context_menu.h"
#include "menu.h"
#include "project.h"
#include "session.h"
#include "thread_safety.h"
#include "userfn.h"
#include "window.h"

#define MAX_CONTEXTS 8
#define MAX_CTX_FNS 255

struct ctxfn_list {
    int num;
    CtxFn ctx_fns[MAX_CTX_FNS];
};

static struct ctxfn_list context_fns[NUM_CTX_TYPES] = {0};

static Ctx contexts[MAX_CONTEXTS] = {0};

int context_at_cursor(Ctx **list_dst)
{
    MAIN_THREAD_ONLY(context_at_cursor);
    Session *session = session_get();
    *list_dst = contexts;
    Timeline *tl = ACTIVE_TL;
    int num_ctxs = 0;
    if (!tl) {
        return 0;
    }
    contexts[num_ctxs] = (Ctx){CTX_TIMELINE, tl, tl->name};
    num_ctxs++;

    Track *track = timeline_selected_track(tl);
    ClickTrack *ct = NULL;
    if (track) {
        contexts[num_ctxs] = (Ctx){CTX_TRACK, track, track->name};
        num_ctxs++;
        ClipRef *cr = clipref_at_cursor();
        if (cr) {
            CtxType cr_type = cr->type == CLIP_AUDIO ? CTX_CLIPREF_AUDIO : CLIP_MIDI ? CTX_CLIPREF_MIDI : CTX_CLIPREF_AUDIO;
            contexts[num_ctxs] = (Ctx){cr_type, cr, cr->name};
            num_ctxs++;
        }
        if (TRACK_AUTO_SELECTED(track)) {
            Automation *a = track->automations[track->selected_automation];
            contexts[num_ctxs] = (Ctx){CTX_AUTOMATION, a, a->name};
            num_ctxs++;
        }
    } else if ((ct = timeline_selected_click_track(tl))) {
        contexts[num_ctxs] = (Ctx){CTX_CLICK_TRACK, ct, ct->name};
        num_ctxs++;
        ClickSegment *seg = click_track_get_segment_at_pos(ct, tl->play_pos_sframes);
        if (seg) {
            contexts[num_ctxs] = (Ctx){CTX_CLICK_SEGMENT, seg, ""};
            num_ctxs++;
        }
    }
    return num_ctxs;
    
}

const char *context_type_name(CtxType t)
{
    switch(t) {
    case CTX_CLIPREF_AUDIO:
        return "Audio clip";
    case CTX_CLIPREF_MIDI:
        return "MIDI clip";
    case CTX_TRACK:
        return "Track";
    case CTX_CLICK_SEGMENT:
        return "Click track segment";
    case CTX_CLICK_TRACK:
        return "Click track";
    case CTX_AUTOMATION:
        return "Automation";
    case CTX_TIMELINE:
        return "Timeline";
    case CTX_PROJECT:
        return "Project";
    case NUM_CTX_TYPES:
        return NULL;
    }
}

static void ctx_track_delete(void *track_v)
{
    /* track_delete(track_v); */
}

static void ctx_track_minimize(void *track_v)
{
    Track *track = track_v;
    timeline_minimize_track_or_tracks(track->tl, track);
}

static void ctx_clipref_delete(void *cr_v)
{
    clipref_delete(cr_v);
}

static void ctx_clipref_grab(void *cr_v)
{
    timeline_clipref_grab(cr_v, CLIPREF_EDGE_NONE);
}


void context_menu_init()
{
    context_fns[CTX_TIMELINE] = (struct ctxfn_list){
        2,
        {
            {"Set mark in", user_tl_set_mark_in, NULL},
            {"Set mark in", user_tl_set_mark_out, NULL},
        }
    };
    context_fns[CTX_TRACK] = (struct ctxfn_list){
        2,
        {
            {"Delete", ctx_track_delete, NULL},
            {"Minimize", ctx_track_minimize, NULL},
        }
    };
    context_fns[CTX_CLIPREF_AUDIO] = (struct ctxfn_list) {
        2,
        {
            {"Delete", ctx_clipref_delete, NULL},
            {"Grab", ctx_clipref_grab, NULL}
        }
            
    };
}

extern Window *main_win;

static void create_menu_from_isol_ctx(void *ctx_v)
{
    Ctx *c = ctx_v;
    Layout *layout = layout_add_child(main_win->layout);
    layout_set_default_dims(layout);
    layout_reset(layout);
    Menu *menu = menu_create(layout, main_win);
    MenuColumn *col = menu_column_add(menu, c->name);
    MenuSection *sctn = menu_section_add(col, c->name);

    for (int j=0; j<context_fns[c->type].num; j++) {
        CtxFn fn = context_fns[c->type].ctx_fns[j];
        menu_item_add(
            sctn,
            fn.display_name,
            NULL,
            fn.do_fn,
            c->obj);
    }
    window_add_menu(main_win, menu);

}
Menu *context_menu_create(int num_ctxs)
{
    Layout *layout = layout_add_child(main_win->layout);
    layout_set_default_dims(layout);
    layout_reset(layout);
    Menu *menu = menu_create(layout, main_win);
    MenuColumn *col = NULL;
    /* MenuColumn *addtl_ctxs_col = NULL; */
    MenuSection *sctn = NULL;
    MenuSection *addtl_ctxs_sctn = NULL;
    for (int i=num_ctxs-1; i>=0; i--) {
        Ctx c = contexts[i];
        if (context_fns[c.type].num > 0) {
            if (i == num_ctxs-1) {
                col = menu_column_add(menu, c.name);
                sctn = menu_section_add(col, c.name);
                for (int j=0; j<context_fns[c.type].num; j++) {
                    CtxFn fn = context_fns[c.type].ctx_fns[j];
                    menu_item_add(
                        sctn,
                        fn.display_name,
                        NULL,
                        fn.do_fn,
                        c.obj);
                }
            } else {
                if (!addtl_ctxs_sctn) {
                    addtl_ctxs_sctn = menu_section_add(col, "");
                }
                menu_item_add(addtl_ctxs_sctn, context_type_name(c.type), ">", create_menu_from_isol_ctx, contexts + i);
                
            }
        }
    }
    menu_reset_layout(menu);
    window_add_menu(main_win, menu);
    menu_reset_layout(menu);
    return menu;
}
