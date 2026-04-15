#include "color.h"
#include "layout.h"
#include "log.h"
#include "project.h"
#include "session.h"
#include "status.h"
#include "textbox.h"
#include "user_event.h"
#include "window.h"

extern Window *main_win;
extern struct colors colors;
/* static bool audio_route_has_cycle(Track *src, Track *dst) */
/* { */
/*     for (int i=0; i<dst->num_routes; i++) { */
/* 	if (dst->routes[i].dst == src) { */
/* 	    return true; */
/* 	} */
/* 	if (audio_route_has_cycle(dst, dst->routes[i].dst)) { */
/* 	    return true; */
/* 	} */
/*     } */
/*     return false; */
/* } */

static int track_proc_order_cmp(const void *a, const void *b)
{
    const Track *track_a = *((Track **)a);
    const Track *track_b = *((Track **)b);
    return track_a->proc_order < track_b->proc_order;
}

void timeline_resort_tracks_proc_order(Timeline *tl)
{
    qsort(tl->tracks_proc_order, tl->num_tracks, sizeof(Track *), track_proc_order_cmp);

    fprintf(stderr, "\n");
    for (int i=0; i<tl->num_tracks; i++) {
	fprintf(stderr, "%d) %s (%d)\n", i, tl->tracks_proc_order[i]->name, tl->tracks_proc_order[i]->proc_order);
    }
}

/* Return -1 if cycle detected */
/* static int longest_distance_to_out(Track *og_src, Track *src, Track *dst, int running) */
/* { */
/*     if (dst == og_src) return -1; */
    
/*     /\* Check for cycles and compute distance to out *\/ */
/*     int max_child_dist = running + 1; */
/*     for (int i=0; i<dst->num_routes; i++) { */
/* 	if (dst->routes[i]->dst == dst) { */
/* 	    return -2; */
/* 	} */
/* 	int child_dist = longest_distance_to_out(og_src, dst, dst->routes[i]->dst, running + 1); */
/* 	if (child_dist == -1) return -1; */
/* 	if (child_dist > max_child_dist) { */
/* 	    max_child_dist = child_dist; */
/* 	} */
/*     } */
/*     return max_child_dist; */
/* } */

static void track_reset_proc_order(Track *track)
{
    if (track->num_routes == 0) track->proc_order = 0;
    for (int i=0; i<track->num_routes; i++) {
	int order = track->routes[i]->dst->proc_order + 1;
	if (order > track->proc_order) {
	    track->proc_order = order;
	}
    }
    for (int i=0; i<track->num_route_ins; i++) {
	track_reset_proc_order(track->route_ins[i]->src);
    }
}

static bool proposed_route_has_feedback(Track *og_src, Track *src, Track *dst)
{
    if (dst == og_src) return true;
    for (int i=0; i<dst->num_routes; i++) {
	bool childret = proposed_route_has_feedback(og_src, dst, dst->routes[i]->dst);
	if (childret) return true;
    }
    return false;
}

void audio_route_destroy(AudioRoute *rt)
{
    free(rt);
}

static void audio_route_reinsert(AudioRoute *rt)
{
    /* int dist = longest_distance_to_out(rt->src, rt->src, rt->dst, 0); */
    /* Reset track processing order */
    rt->src->routes[rt->src->num_routes] = rt;
    rt->src->num_routes++;

    rt->dst->route_ins[rt->dst->num_route_ins] = rt;
    rt->dst->num_route_ins++;
    /* fprintf(stderr, "Reinserting %s in %s\n", rt->tl_gui.out_tb->layout->name, rt->tl_gui.out_tb->layout->parent->name); */
    layout_insert_child_at(rt->tl_gui.out_tb->layout, rt->tl_gui.out_tb->layout->cached_parent, rt->tl_gui.out_tb->layout->index);

    track_reset_proc_order(rt->src);
    timeline_resort_tracks_proc_order(rt->src->tl);
    /* timeline_update_track_proc_order(t */
    /* rt->dst-> */
}

static void audio_route_remove(AudioRoute *rt)
{
    /* Remove route out */
    bool displace = false;
    for (int i=0; i<rt->src->num_routes - 1; i++) {
	if (rt->src->routes[i] == rt) {	    
	    displace = true;
	}
	if (displace) {
	    rt->src->routes[i] = rt->src->routes[i + 1];
	}
    }
    rt->src->num_routes--;

    /* Remove route in */
    displace = false;
    for (int i=0; i<rt->dst->num_route_ins - 1; i++) {
	if (rt->dst->route_ins[i] == rt) {
	    displace = true;
	}
	if (displace) {
	    rt->dst->route_ins[i] = rt->dst->route_ins[i + 1];
	}
    }
    layout_remove_child(rt->tl_gui.out_tb->layout);
    track_reset_proc_order(rt->src);
    timeline_resort_tracks_proc_order(rt->src->tl);
}

NEW_EVENT_FN(undo_add_audio_route, "undo add audio route")
    audio_route_remove(obj1);
}

NEW_EVENT_FN(redo_add_audio_route, "redo add audio route")
    audio_route_reinsert(obj1);
}




void track_add_audio_route(Track *track, Track *dst, float init_amp)
{
    if (!dst || !track) {
	log_tmp(LOG_ERROR, "Call to track_add_audio_route with one or more null arg: src %p, dst %p\n", track, dst);
	return;
    }
    if (track->num_routes == TRACK_MAX_AUDIO_ROUTES) {
	status_set_errstr("No more than %d audio routes allowed on track\n", TRACK_MAX_AUDIO_ROUTES);
	return;
    }

    for (int i=0; i<track->num_routes; i++) {
	if (track->routes[i]->dst == dst) {
	    status_set_errstr("Track \"%s\" already has route to \"%s\"\n", track->name, dst->name);
	    return;
	}
    }

    if (proposed_route_has_feedback(track, track, dst)) {
	status_set_errstr("No feedback audio routes\n");
	return;

    }
    /* int dist = longest_distance_to_out(track, track, dst, 0); */
    
    /* if (dist == -1) { */
    /* } */
    AudioRoute *r = calloc(1, sizeof(AudioRoute));
    r->dst = dst;
    r->src = track;
    r->amp = init_amp;
    status_set_alertstr("Established route: %s => %s (%d steps to out)\n", track->name, dst->name, track->proc_order);
    Layout *parent = layout_get_child_by_name_recursive(track->inner_layout, "outs");
    Layout *out_tb_lt = layout_add_child(parent);
    out_tb_lt->w.type = SCALE;
    out_tb_lt->w.value = 1.0;
    /* out_tb_lt->y.type = STACK; */
    out_tb_lt->y.type = STACK;
    /* out_tb_lt->y.value = 4 + 20 * (track->num_routes - 1); */
    out_tb_lt->h.type = SCALE;
    out_tb_lt->h.value = 0.25;
    layout_reset(out_tb_lt);
    r->tl_gui.out_tb = textbox_create_from_str(
	dst->name,
	out_tb_lt,
	main_win->mono_bold_font,
	12,
	main_win);
    textbox_style(
	r->tl_gui.out_tb,
	CENTER,
	true,
	&dst->color,
	&colors.white);
    textbox_set_pad(r->tl_gui.out_tb, 4, 2);

    /* Reset track processing order */
    /* if (dist > track->proc_order) { */
    /* 	track->proc_order = dist; */
    /* 	timeline_update_track_proc_order(track->tl, track);	 */
    /* } */

    /* Add the route to both tracks */
    track->routes[track->num_routes] = r;
    track->num_routes++;

    dst->route_ins[dst->num_route_ins] = r;
    dst->num_route_ins++;

    /* Resort tracks */
    track_reset_proc_order(track);
    timeline_resort_tracks_proc_order(track->tl);

    track->tl->needs_redraw = true;

    user_event_push(
	undo_add_audio_route,
	redo_add_audio_route,
	NULL, NULL,
	r, NULL,
	(Value){0}, (Value){0},
	(Value){0}, (Value){0},
	0, 0, false, false);
    /* textbox_reset_full(r->tl_gui.out_tb); */
}




void audio_route_delete(AudioRoute *rt)
{
    
}

