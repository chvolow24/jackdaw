/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "clipref.h"
#include "grab.h"
#include "project.h"
#include "session.h"
#include "status.h"
#include "user_event.h"


/* LOW-LEVEL INTERFACE:
   - only touches clipref internal state
*/
static void loc_clipref_grab(ClipRef *cr, ClipRefEdge edge)
{
    cr->grabbed = true;
    cr->grabbed_edge = edge;
}

static void loc_clipref_ungrab(ClipRef *cr)
{
    cr->grabbed = false;
    cr->grabbed_edge = CLIPREF_EDGE_NONE;
}

/* CACHEING & UNDO/REDO:
   - CACHE: grabbed clipref positions, edges, and track offsets need to be cached
     before undoable alterations occur
   - PUSH: when an undoable event is *completed*, an event must be pushed
   - UNDO/REDO: standard undo redo functions use cached positions
*/


/* Offset cacheing:
   - cache a clips track position relative to the track selector
   - this is necessary for dragging clips across tracks in a way that preserves
     the dragged clips' positions relative to one another
   - separate from normal undo/redo cacheing
*/
void timeline_cache_grabbed_clip_offsets(Timeline *tl)
{
    for (int i=0; i<tl->num_grabbed_clips; i++) {
	tl->grabbed_clip_info_cache[i].track_offset = tl->grabbed_clips[i]->track->tl_rank - tl->track_selector;
    }
}

static NEW_EVENT_FN(undo_move_clips, "undo move clips")
    ClipRef **cliprefs = (ClipRef **)obj1;
    struct grabbed_clip_info *positions = (struct grabbed_clip_info *)obj2;
    uint8_t num = val1.uint8_v;
    if (num == 0) return;
    for (int i = 0; i<num; i++) {
	if (!positions[i].track) break;
	clipref_move_to_track(cliprefs[i], positions[i].track);
	cliprefs[i]->tl_pos = positions[i].pos;
	/* New: set bounds, always */
	cliprefs[i]->start_in_clip = positions[i].start_in_clip;
	cliprefs[i]->end_in_clip = positions[i].end_in_clip;
	/* End new */
	clipref_reset(cliprefs[i], false);
    }
    Timeline *tl = cliprefs[0]->track->tl;
    tl->needs_redraw = true;
}

static NEW_EVENT_FN(redo_move_clips, "redo move clips")
    ClipRef **cliprefs = (ClipRef **)obj1;
    struct grabbed_clip_info *positions = (struct grabbed_clip_info *)obj2;
    uint8_t num = val1.uint8_v;
    if (num == 0) return;
    for (int i=0; i<num; i++) {
	if (!positions[i].track) break;
	clipref_move_to_track(cliprefs[i], positions[i + num].track);
	cliprefs[i]->tl_pos = positions[i + num].pos;
	cliprefs[i]->start_in_clip = positions[i + num].start_in_clip;
	cliprefs[i]->end_in_clip = positions[i + num].end_in_clip;
	fprintf(stderr, "SETTING CR \"%s\" bounds: %d-%d\n", cliprefs[i]->name, cliprefs[i]->start_in_clip, cliprefs[i]->end_in_clip);
	clipref_reset(cliprefs[i], false);
    }
    Timeline *tl = cliprefs[0]->track->tl;
    tl->needs_redraw = true;
}

void timeline_cache_grabbed_clip_positions(Timeline *tl)
{
    if (!tl->grabbed_clip_cache_initialized) {
	tl->grabbed_clip_cache_initialized = true;
    } else if (!tl->grabbed_clip_cache_pushed) {
	timeline_push_grabbed_clip_move_event(tl);
    }
    for (int i=0; i<tl->num_grabbed_clips; i++) {
	tl->grabbed_clip_info_cache[i].track = tl->grabbed_clips[i]->track;
	tl->grabbed_clip_info_cache[i].pos = tl->grabbed_clips[i]->tl_pos;
	tl->grabbed_clip_info_cache[i].grabbed_edge = tl->grabbed_clips[i]->grabbed_edge;
	tl->grabbed_clip_info_cache[i].start_in_clip = tl->grabbed_clips[i]->start_in_clip;
	tl->grabbed_clip_info_cache[i].end_in_clip = tl->grabbed_clips[i]->end_in_clip;
    }
    tl->grabbed_clip_cache_pushed = false;
}

void timeline_push_grabbed_clip_move_event(Timeline *tl)
{
    ClipRef **cliprefs = calloc(tl->num_grabbed_clips, sizeof(ClipRef *));

    struct grabbed_clip_info *positions = calloc(tl->num_grabbed_clips * 2, sizeof(struct grabbed_clip_info));

    /* Undo positions in first half of array; redo in second half */
    for (int i=0; i<tl->num_grabbed_clips; i++) {
	clipref_reset(tl->grabbed_clips[i], false);
	cliprefs[i] = tl->grabbed_clips[i];
	/* undo pos */
	positions[i].track = tl->grabbed_clip_info_cache[i].track;
	positions[i].pos = tl->grabbed_clip_info_cache[i].pos;
	positions[i].grabbed_edge = tl->grabbed_clip_info_cache[i].grabbed_edge;
	positions[i].start_in_clip = tl->grabbed_clip_info_cache[i].start_in_clip;
	positions[i].end_in_clip = tl->grabbed_clip_info_cache[i].end_in_clip;
	/* positions[i].track_offset = positions[i]. */
	/* redo pos */
	positions[i + tl->num_grabbed_clips].track = cliprefs[i]->track;
	positions[i + tl->num_grabbed_clips].pos = cliprefs[i]->tl_pos;
	positions[i + tl->num_grabbed_clips].start_in_clip = cliprefs[i]->start_in_clip;
	positions[i + tl->num_grabbed_clips].end_in_clip = cliprefs[i]->end_in_clip;
    }
    Value num = {.uint8_v = tl->num_grabbed_clips};

    user_event_push(
	undo_move_clips,
	redo_move_clips,
	NULL,
	NULL,
	(void *)cliprefs,
	(void *)positions,
	num,num,num,num,
	0,0,true,true);
    tl->grabbed_clip_cache_pushed = true;
}



/* MID-LEVEL INTERFACE:
 - clips may be individually ungrabbed, e.g. when deleted
 - also grabbed, e.g. when a copied clip is pasted
 - timeline state must be updated accordingly
 - status bar should also be updated
*/

void timeline_clipref_grab(ClipRef *cr, enum clipref_edge edge)
{
    if (cr->grabbed) {
	cr->grabbed_edge = edge;
	return;
    }
    if (cr->track->tl->num_grabbed_clips == MAX_GRABBED_CLIPS) {
	char buf[128];
	snprintf(buf, 128, "Cannot grab more than %d clips", MAX_GRABBED_CLIPS);
	status_set_errstr(buf);
	return;
    }
    Timeline *tl = cr->track->tl;
    tl->grabbed_clips[tl->num_grabbed_clips] = cr;
    tl->num_grabbed_clips++;
    loc_clipref_grab(cr, edge);
}

void timeline_clipref_ungrab(ClipRef *cr)
{
    Timeline *tl = cr->track->tl;
    bool displace = false;
    for (int i=0; i<tl->num_grabbed_clips; i++) {
	if (tl->grabbed_clips[i] == cr) displace = true;
	else if (displace) tl->grabbed_clips[i-1] = tl->grabbed_clips[i];
    }
    tl->num_grabbed_clips--;
    loc_clipref_ungrab(cr);
}

/* Cleanup function, used to rectify state in mouse code and elsewhere */
void timeline_ungrab_all_cliprefs(Timeline *tl)
{
    for (int i=0; i<tl->num_grabbed_clips; i++) {
	loc_clipref_ungrab(tl->grabbed_clips[i]);
    }
    tl->num_grabbed_clips = 0;
}

/* END MID-LEVEL INTERFACE */


/* Main entrypoint from userfn.c -- triggered by 'g' key */
void timeline_grab_ungrab(Timeline *tl)
{
    if (tl->num_tracks == 0) return;
    
    Session *session = session_get();
    Track *track = NULL;
    ClipRef *cr =  NULL;
    
    ClipRef *clips_to_grab[MAX_GRABBED_CLIPS];
    uint8_t num_clips = 0;
    
    bool clip_grabbed = false;
    bool had_active_track = false;
    
    for (int i=0; i<tl->num_tracks; i++) {
	track = tl->tracks[i];
	if (track->active) {
	    had_active_track = true;
	    cr = clipref_at_cursor_in_track(track);
	    if (cr && !cr->grabbed) {
		clips_to_grab[num_clips] = cr;
		num_clips++;
		clip_grabbed = true;
	    }
	}
    }
    track = timeline_selected_track(tl);
    if (!had_active_track && track) {
	track = timeline_selected_track(tl);
	cr = clipref_at_cursor_in_track(track);
	if (cr && !cr->grabbed) {
	    clips_to_grab[num_clips] = cr;
	    num_clips++;
	    clip_grabbed = true;
	}
    }

    if (clip_grabbed) {
	for (int i=0; i<num_clips; i++) {
	    timeline_clipref_grab(clips_to_grab[i], CLIPREF_EDGE_NONE);
	}
	if (session->dragging && session->playback.playing) {
	    timeline_cache_grabbed_clip_positions(tl);
	}
    } else {
	if (session->dragging && session->playback.playing) {
	    timeline_push_grabbed_clip_move_event(tl);
	}
	for (int i=0; i<tl->num_grabbed_clips; i++) {
	    cr = tl->grabbed_clips[i];
	    cr->grabbed = false;
	    cr->grabbed_edge = CLIPREF_EDGE_NONE;
	}
	tl->num_grabbed_clips = 0;
    }
    
    if (session->dragging) {
	status_stat_drag();
    }    
    
    tl->needs_redraw = true;
}

/* Grab left edge of clip at cursor */
void timeline_grab_left_edge(Timeline *tl)
{
    ClipRef *cr = clipref_at_cursor();
    timeline_clipref_grab(cr, CLIPREF_EDGE_LEFT);
    tl->needs_redraw = true;
}

/* Grab right edge of clip at cursor */
void timeline_grab_right_edge(Timeline *tl)
{
    ClipRef *cr = clipref_at_cursor();
    timeline_clipref_grab(cr, CLIPREF_EDGE_RIGHT);
    tl->needs_redraw = true;
}

void timeline_delete_grabbed_cliprefs(Timeline *tl);

