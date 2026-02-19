/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <stdlib.h>
#include "audio_clip.h"
#include "clipref.h"
#include "grab.h"
#include "project.h"
#include "session.h"
#include "status.h"
#include "timeline.h"
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

static NEW_EVENT_FN(undo_move_clips, "undo move clips / adj clip bounds")
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

static NEW_EVENT_FN(redo_move_clips, "redo move clips / adj clip bounds")
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
	status_set_errstr("Cannot grab more than %d clips", MAX_GRABBED_CLIPS);
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

/* HIGH-LEVEL INTERFACE
   - functions accessible more or less directly from userfn.c
   - grab/ungrab, grab edge, and delete grabbed clips
*/

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
    if (!cr) return;
    timeline_set_play_position(cr->track->tl, cr->tl_pos, false);
    timeline_clipref_grab(cr, CLIPREF_EDGE_LEFT);
    if (session_get()->dragging) {
	status_stat_drag();
    }    

    tl->needs_redraw = true;
}

/* Grab right edge of clip at cursor */
void timeline_grab_right_edge(Timeline *tl)
{
    ClipRef *cr = clipref_at_cursor();
    if (!cr) return;
    timeline_set_play_position(cr->track->tl, cr->tl_pos + clipref_len(cr), false);
    timeline_clipref_grab(cr, CLIPREF_EDGE_RIGHT);
    if (session_get()->dragging) {
	status_stat_drag();
    }    

    tl->needs_redraw = true;
}

void timeline_grab_no_edge(Timeline *tl)
{
    ClipRef *cr = clipref_at_cursor();
    if (!cr) return;
    if (!cr->grabbed) {
	timeline_clipref_grab(cr, CLIPREF_EDGE_NONE);
    }
    cr->grabbed_edge = CLIPREF_EDGE_NONE;
    Session *session = session_get();
    if (session_get()->dragging) {
	status_stat_drag();
    }    

    ACTIVE_TL->needs_redraw = true;

}


static NEW_EVENT_FN(undo_delete_clips, "undo delete clips")
    ClipRef **clips = (ClipRef **)obj1;
    uint8_t num = val1.uint8_v;
    for (uint8_t i=0; i<num; i++) {
	clipref_undelete(clips[i]);
    }
}
static NEW_EVENT_FN(redo_delete_clips, "redo delete clips")
    ClipRef **clips = (ClipRef **)obj1;
    uint8_t num = val1.uint8_v;
    for (uint8_t i=0; i<num; i++) {
	clipref_delete(clips[i]);
    }
}
static NEW_EVENT_FN(dispose_delete_clips, "")
    ClipRef **clips = (ClipRef **)obj1;
    uint8_t num = val1.uint8_v;
    for (uint8_t i=0; i<num; i++) {
	clipref_destroy(clips[i], true);
    }
}

void timeline_delete_grabbed_cliprefs(Timeline *tl)
{
    if (session_get()->dragging) {
	timeline_push_grabbed_clip_move_event(tl);
    }
    ClipRef **deleted_cliprefs = calloc(tl->num_grabbed_clips, sizeof(ClipRef *));
    for (uint8_t i=0; i<tl->num_grabbed_clips; i++) {
	ClipRef *cr = tl->grabbed_clips[i];
	loc_clipref_ungrab(cr);
	/* cr->grabbed = false; */
	clipref_delete(cr);
	deleted_cliprefs[i] = cr;
    }
    Value num = {.uint8_v = tl->num_grabbed_clips};
    user_event_push(
	undo_delete_clips,
	redo_delete_clips,
	dispose_delete_clips,
	NULL,
	(void *)deleted_cliprefs,
	NULL,
	num,
	num,
	num,
	num,
	0, 0, true, false);
    tl->num_grabbed_clips = 0;
}

void timeline_grab_marked_range(Timeline *tl, ClipRefEdge edge)
{
    bool had_active_track = false;
    for (int i=0; i<tl->num_tracks; i++) {
	Track *t = tl->tracks[i];
	if (t->active) {
	    had_active_track = true;
	    for (int i=0; i<t->num_clips; i++) {
		ClipRef *cr = t->clips[i];
		if (clipref_marked(tl, cr)) {
		    timeline_clipref_grab(cr, edge);
		}
	    }
	}
    }
    Track *t = timeline_selected_track(tl);
    if (!had_active_track && t) {
	/* Track *t = timeline_selected_track(tl); */
	for (int i=0; i<t->num_clips; i++) {
	    ClipRef *cr = t->clips[i];
	    if (clipref_marked(tl, cr)) {
		timeline_clipref_grab(cr, edge);
	    }
	}
    }
    if (session_get()->dragging) {
	status_stat_drag();
    }    

    tl->needs_redraw = true;
}


/* Use this function every time the playhead moves while clips are grabbed */
void timeline_grabbed_clips_move(Timeline *tl, int32_t move_by_sframes)
{
    Session *session = session_get();
    if (session->piano_roll) {
    /* 	piano_roll_grabbed_notes_move(move_by_sframes); */
	return;
    }
    for (int i=0; i<tl->num_grabbed_clips; i++) {
	ClipRef *cr = tl->grabbed_clips[i];
	int32_t clip_len = 0;
	switch (cr->type) {
	case CLIP_AUDIO:
	    clip_len = ((Clip *)cr->source_clip)->len_sframes;
	    break;
	case CLIP_MIDI:
	    clip_len = ((MIDIClip *)cr->source_clip)->len_sframes;
	    break;
	}
	switch(cr->grabbed_edge) {
	case CLIPREF_EDGE_LEFT: {
	    int32_t new_start = cr->start_in_clip + move_by_sframes;
	    if (new_start < 0) new_start = 0;
	    if (new_start >= cr->end_in_clip) new_start = cr->end_in_clip - 1;
	    cr->tl_pos += new_start - cr->start_in_clip;
	    cr->start_in_clip = new_start;
	}
	    break;
	case CLIPREF_EDGE_RIGHT: {
	    int32_t new_end = cr->end_in_clip + move_by_sframes;
	    if (new_end <= cr->start_in_clip) new_end = cr->start_in_clip + 1;
	    if (new_end > clip_len) new_end = clip_len;
	    cr->end_in_clip = new_end;
	}
	    break;
	case CLIPREF_EDGE_NONE:
	    cr->tl_pos += move_by_sframes;
	    break;
	}
	/* clipref_reset(cr, false); */
    }
}
