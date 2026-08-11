#include <stdio.h>
#include <stdlib.h>
#include "context_menu.h"
#include "log.h"
#include "menu.h"
#include "project.h"
#include "session.h"
#include "thread_safety.h"
#include "timeline.h"
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

int context_at_point(Ctx **list_dst, SDL_Point point)
{
    MAIN_THREAD_ONLY(context_at_mouse_cursor);
    Session *session = session_get();
    *list_dst = contexts;
    Timeline *tl = ACTIVE_TL;
    int num_ctxs = 0;
    if (!tl) {
        return 0;
    }
    contexts[num_ctxs] = (Ctx){CTX_PROJECT, tl->proj, tl->proj->name, NULL};
    num_ctxs++;
    contexts[num_ctxs] = (Ctx){CTX_TIMELINE_NAV, tl, tl->name, NULL};
    num_ctxs++;
    contexts[num_ctxs] = (Ctx){CTX_TIMELINE, tl, tl->name, NULL};
    num_ctxs++;


    Track *track = timeline_track_at_point(tl, point);
    ClickTrack *ct = NULL;
    if (track) {
        contexts[num_ctxs] = (Ctx){CTX_TRACK, track, track->name, track->layout};
        num_ctxs++;
        ClipRef *cr = track_clipref_at_point(track, point);
        if (cr) {
            CtxType cr_type = cr->type == CLIP_AUDIO ? CTX_CLIPREF_AUDIO : CLIP_MIDI ? CTX_CLIPREF_MIDI : CTX_CLIPREF_AUDIO;
            contexts[num_ctxs] = (Ctx){cr_type, cr, cr->name, cr->layout};
            num_ctxs++;
        }
        /* if (TRACK_AUTO_SELECTED(track)) { */
        /*     Automation *a = track->automations[track->selected_automation]; */
        /*     contexts[num_ctxs] = (Ctx){CTX_AUTOMATION, a, a->name}; */
        /*     num_ctxs++; */
        /* } */
    } else if ((ct = timeline_click_track_at_point(tl, point))) {
        contexts[num_ctxs] = (Ctx){CTX_CLICK_TRACK, ct, ct->name, ct->layout};
        num_ctxs++;
        ClickSegment *seg = click_track_get_segment_at_pos(ct, timeview_get_draw_x(&tl->timeview, tl->play_pos_sframes));
        if (seg) {
            contexts[num_ctxs] = (Ctx){CTX_CLICK_SEGMENT, seg, "", seg->track->layout};
            num_ctxs++;
        }
    }
    return num_ctxs;
}

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
    contexts[num_ctxs] = (Ctx){CTX_PROJECT, tl->proj, tl->proj->name, NULL};
    num_ctxs++;
    contexts[num_ctxs] = (Ctx){CTX_TIMELINE_NAV, tl, tl->name, NULL};
    num_ctxs++;
    contexts[num_ctxs] = (Ctx){CTX_TIMELINE, tl, tl->name, NULL};
    num_ctxs++;

    Track *track = timeline_selected_track(tl);
    ClickTrack *ct = NULL;
    if (track) {
        contexts[num_ctxs] = (Ctx){CTX_TRACK, track, track->name, track->layout};
        num_ctxs++;
        ClipRef *cr = clipref_at_cursor();
        if (cr) {
            CtxType cr_type = cr->type == CLIP_AUDIO ? CTX_CLIPREF_AUDIO : CLIP_MIDI ? CTX_CLIPREF_MIDI : CTX_CLIPREF_AUDIO;
            contexts[num_ctxs] = (Ctx){cr_type, cr, cr->name, cr->layout};
            num_ctxs++;
        }
        if (TRACK_AUTO_SELECTED(track)) {
            Automation *a = track->automations[track->selected_automation];
            contexts[num_ctxs] = (Ctx){CTX_AUTOMATION, a, a->name, a->layout};
            num_ctxs++;
        }
    } else if ((ct = timeline_selected_click_track(tl))) {
        contexts[num_ctxs] = (Ctx){CTX_CLICK_TRACK, ct, ct->name, ct->layout};
        num_ctxs++;
        ClickSegment *seg = click_track_get_segment_at_pos(ct, tl->play_pos_sframes);
        if (seg) {
            contexts[num_ctxs] = (Ctx){CTX_CLICK_SEGMENT, seg, "", seg->track->layout};
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
    case CTX_TIMELINE_NAV:
        return "Navigation";
    case CTX_PROJECT:
        return "Project";
    case NUM_CTX_TYPES:
        return NULL;
    }
}

static void ctx_track_minimize(void *track_v)
{
    Track *track = track_v;
    timeline_minimize_track_or_tracks(track->tl, track);
}

static void context_menu_add_fn(CtxType type, const char *name, void (*fn)(void *ctx_v), UserFn *related_userfn)
{
    struct ctxfn_list *l = context_fns + type;
    if (l->num >= MAX_CTX_FNS) {
        log_tmp(LOG_ERROR, "Reached max num context functions for type %s\n", context_type_name(type));
        return;
    }
    l->ctx_fns[l->num] = (CtxFn){
        name,
        fn,
        related_userfn
    };
    l->num++;
}

static void ctx_clipref_grab(void *cr_v)
{
    timeline_clipref_grab(cr_v, CLIPREF_EDGE_NONE);
}

static void ctx_click_track_delete(void *ct_v)
{
    click_track_delete(ct_v);
}

static void ctx_click_track_edit(void *ct_v)
{
    click_track_edit(ct_v);
}

static void ctx_automation_delete(void *a_v)
{
    automation_delete(a_v);
}

void context_menu_init()
{

    /* Project */

    context_menu_add_fn(
        CTX_PROJECT,
        "Save as",
        user_global_save_as,
        input_get_fn_by_fnptr(user_global_save_as));

    context_menu_add_fn(
        CTX_PROJECT,
        "Save",
        user_global_save_project,
        input_get_fn_by_fnptr(user_global_save_project));


    context_menu_add_fn(
        CTX_PROJECT,
        "Export audio",
        user_tl_write_mixdown_to_wav,
        input_get_fn_by_fnptr(user_tl_write_mixdown_to_wav));



    /* Timeline navigation */

    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Zoom in",
        user_tl_zoom_in,
        input_get_fn_by_fnptr(user_tl_zoom_in));
    
    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Zoom out",
        user_tl_zoom_out,
        input_get_fn_by_fnptr(user_tl_zoom_out));

    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Move view right",
        user_tl_move_right,
        input_get_fn_by_fnptr(user_tl_move_right));

    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Move view left",
        user_tl_move_left,
        input_get_fn_by_fnptr(user_tl_move_left));

    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Select previous track (cursor up)",
        user_tl_track_selector_up,
        input_get_fn_by_fnptr(user_tl_track_selector_up));

    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Select next track (cursor down)",
        user_tl_track_selector_down,
        input_get_fn_by_fnptr(user_tl_track_selector_down));

    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Go to mark in",
        user_tl_goto_mark_in,
        input_get_fn_by_fnptr(user_tl_goto_mark_in));
    
    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Go to mark out",
        user_tl_goto_mark_out,
        input_get_fn_by_fnptr(user_tl_goto_mark_out));
    
    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Go to t=0",
        user_tl_goto_zero,
        input_get_fn_by_fnptr(user_tl_goto_zero));

    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Go to next clip boundary on sel track",
        user_tl_goto_next_clip_boundary,
        input_get_fn_by_fnptr(user_tl_goto_next_clip_boundary));

    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Go to previous clip boundary on sel track",
        user_tl_goto_previous_clip_boundary,
        input_get_fn_by_fnptr(user_tl_goto_previous_clip_boundary));

    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Go to next measure",
        user_tl_goto_next_measure,
        input_get_fn_by_fnptr(user_tl_goto_next_measure));

    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Go to next beat",
        user_tl_goto_next_beat,
        input_get_fn_by_fnptr(user_tl_goto_next_beat));

    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Go to next beat subdivision",
        user_tl_goto_next_subdiv,
        input_get_fn_by_fnptr(user_tl_goto_next_subdiv));

    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Go to prev measure",
        user_tl_goto_prev_measure,
        input_get_fn_by_fnptr(user_tl_goto_prev_measure));

    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Go to prev beat",
        user_tl_goto_prev_beat,
        input_get_fn_by_fnptr(user_tl_goto_prev_beat));

    context_menu_add_fn(
        CTX_TIMELINE_NAV,
        "Go to prev beat subdivision",
        user_tl_goto_prev_subdiv,
        input_get_fn_by_fnptr(user_tl_goto_prev_subdiv));


    /* Timeline */
    
    context_menu_add_fn(
        CTX_TIMELINE,
        "Set mark in",
        user_tl_set_mark_in,
        input_get_fn_by_fnptr(user_tl_set_mark_in));

    context_menu_add_fn(
        CTX_TIMELINE,
        "Set mark out",
        user_tl_set_mark_out,
        input_get_fn_by_fnptr(user_tl_set_mark_out));
    
    context_menu_add_fn(
        CTX_TIMELINE,
        "Toggle loop playback",
        user_tl_toggle_loop_playback,
        input_get_fn_by_fnptr(user_tl_toggle_loop_playback));



    /* Track */
    
    context_menu_add_fn(
        CTX_TRACK,
        "Delete",
        user_tl_track_delete,
        input_get_fn_by_fnptr(user_tl_track_delete));

    context_menu_add_fn(
        CTX_TRACK,
        "Minimize / maximize",
        ctx_track_minimize,
        input_get_fn_by_fnptr(user_tl_tracks_minimize));

    context_menu_add_fn(
        CTX_TRACK,
        "Add effect",
        user_tl_track_add_effect,
        input_get_fn_by_fnptr(user_tl_track_add_effect));

    context_menu_add_fn(
        CTX_TRACK,
        "Open effects",
        user_tl_track_open_settings,
        input_get_fn_by_fnptr(user_tl_track_open_settings));

    context_menu_add_fn(
        CTX_TRACK,
        "Open synth",
        user_tl_track_open_synth,
        input_get_fn_by_fnptr(user_tl_track_open_synth));

    context_menu_add_fn(
        CTX_TRACK,
        "Add automation",
        user_tl_track_add_automation,
        input_get_fn_by_fnptr(user_tl_track_add_automation));

    context_menu_add_fn(
        CTX_TRACK,
        "Hide / show automations",
        user_tl_track_show_hide_automations,
        input_get_fn_by_fnptr(user_tl_track_show_hide_automations));

    context_menu_add_fn(
        CTX_TRACK,
        "Audio routes out",
        user_tl_audio_routes_out_open_page,
        input_get_fn_by_fnptr(user_tl_audio_routes_out_open_page));

    context_menu_add_fn(
        CTX_TRACK,
        "Audio routes in",
        user_tl_audio_routes_in_open_page,
        input_get_fn_by_fnptr(user_tl_audio_routes_in_open_page));

    context_menu_add_fn(
        CTX_TRACK,
        "Quick add audio route out",
        user_tl_audio_route_out_quick_add,
        input_get_fn_by_fnptr(user_tl_audio_route_out_quick_add));

    context_menu_add_fn(
        CTX_TRACK,
        "Quick add audio route in",
        user_tl_audio_route_in_quick_add,
        input_get_fn_by_fnptr(user_tl_audio_route_in_quick_add));
    






    /* Automation */
    
    context_menu_add_fn(
        CTX_AUTOMATION,
        "Delete",
        ctx_automation_delete,
        input_get_fn_by_fnptr(user_tl_track_delete));
    
    /* Click Track */
    
    context_menu_add_fn(
        CTX_CLICK_TRACK,
        "Delete",
        ctx_click_track_delete,
        input_get_fn_by_fnptr(user_tl_track_delete));

    context_menu_add_fn(
        CTX_CLICK_TRACK,
        "Edit",
        ctx_click_track_edit,
        input_get_fn_by_fnptr(user_tl_track_open_settings));


    context_menu_add_fn(
        CTX_CLIPREF_AUDIO,
        "Grab / ungrab",
        ctx_clipref_grab,
        NULL);

    

}

extern Window *main_win;

static SDL_Point saved_point = {0};

static void create_menu_from_isol_ctx(void *ctx_v)
{
    Ctx *c = ctx_v;
    Menu *menu = menu_create_at_point(saved_point.x + 16, saved_point.y + 8);
    MenuColumn *col = menu_column_add(menu, context_type_name(c->type));
    MenuSection *sctn = menu_section_add(col, c->name);

    for (int j=0; j<context_fns[c->type].num; j++) {
        CtxFn fn = context_fns[c->type].ctx_fns[j];
        const char *annot = fn.related_userfn ? fn.related_userfn->annotation : NULL;
        menu_item_add(
            sctn,
            fn.display_name,
            annot,
            fn.do_fn,
            c->obj);
    }
    session_get()->gui.focus_lt = c->layout;
    window_add_menu(main_win, menu);

}
Menu *context_menu_create(int num_ctxs, SDL_Point at)
{
    if (num_ctxs == 0) return NULL;
    at.y -= 32;
    at.x += 8;
    saved_point = at;
    /* Layout *layout = layout_add_child(main_win->layout); */
    /* layout_set_default_dims(layout); */
    /* layout_reset(layout); */
    window_clear_menus(main_win);
    Menu *menu = menu_create_at_point(at.x, at.y);
    MenuColumn *col = NULL;
    /* MenuColumn *addtl_ctxs_col = NULL; */
    MenuSection *sctn = NULL;
    MenuSection *addtl_ctxs_sctn = NULL;
    Layout *focus_lt = NULL;
    bool primary_col_created = false;
    for (int i=num_ctxs-1; i>=0; i--) {
        Ctx c = contexts[i];
        if (context_fns[c.type].num > 0) {
            if (!primary_col_created) {
                col = menu_column_add(menu, context_type_name(c.type));
                sctn = menu_section_add(col, c.name);
                for (int j=0; j<context_fns[c.type].num; j++) {
                    CtxFn fn = context_fns[c.type].ctx_fns[j];
                    const char *annot = fn.related_userfn ? fn.related_userfn->annotation : NULL;
                    menu_item_add(
                        sctn,
                        fn.display_name,
                        annot,
                        fn.do_fn,
                        c.obj);
                }
                primary_col_created = true;
                focus_lt = c.layout;
            } else {
                if (!addtl_ctxs_sctn) {
                    addtl_ctxs_sctn = menu_section_add(col, NULL);
                }
                menu_item_add(addtl_ctxs_sctn, context_type_name(c.type), ">", create_menu_from_isol_ctx, contexts + i);
                
            }
        }
    }
    /* menu_add_header(menu,  */
    menu_reset_layout(menu);
    session_get()->gui.focus_lt = focus_lt;
    window_add_menu(main_win, menu);
    menu_reset_layout(menu);
    return menu;
}


void context_at_cursor_create_menu()
{
    Ctx *arr = NULL;
    int num_ctxs = context_at_cursor(&arr);
    if (num_ctxs == 0) return;
    Session *session = session_get();
    SDL_Point p = timeline_cursor_point(ACTIVE_TL);
    context_menu_create(num_ctxs, p);
}

void context_at_point_create_menu(SDL_Point point)
{
    Ctx *arr = NULL;
    int num_ctxs = context_at_point(&arr, point);
    if (num_ctxs == 0) return;
    context_menu_create(num_ctxs, point);
}

