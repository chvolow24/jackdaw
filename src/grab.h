/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    grab.h

    * interfaces for ClipRef grabbing/ungrabbing/moving
    * ~duplicated in piano roll for notes, unfortunately
*****************************************************************************************************************/

/* #include "clipref.h" */
/* #include "project.h" */

#ifndef JDAW_GRAB_H
#define JDAW_GRAB_H

#include <stdint.h>

typedef enum clipref_edge {
    CLIPREF_EDGE_NONE,
    CLIPREF_EDGE_RIGHT,
    CLIPREF_EDGE_LEFT
} ClipRefEdge;

typedef struct timeline Timeline;
typedef struct clip_ref ClipRef;


/* HIGH-LEVEL INTERFACE
   - accessible from userfn.c
   - defined grab-specific actions user can take
*/

/* Main entrypoint from userfn.c -- triggered by 'g' key */
/* void timeline_grab_ungrab(Timeline *tl); */
void timeline_grab_ungrab(Timeline *tl);

/* Grab left edge of clip at cursor */
void timeline_grab_left_edge(Timeline *tl);

/* Grab right edge of clip at cursor */
void timeline_grab_right_edge(Timeline *tl);

/* Ungrab the edge if grabbed, and grab the clip */
void timeline_grab_no_edge(Timeline *tl);

/* Triggered if a "delete" action occurs on the timeline, not on a click track */
void timeline_delete_grabbed_cliprefs(Timeline *tl);

void timeline_grab_marked_range(Timeline *tl, ClipRefEdge edge);

/****** END HIGH-LEVEL INTERFACE ******/

/* MID-LEVEL INTERFACE
 - clips may be individually ungrabbed incidentally, e.g. when deleted
 - timeline state must be updated accordingly
 - status bar should also be updated
*/

/* Ungrab all clips, update tl state, and update drag state on status bar */
void timeline_ungrab_all_cliprefs(Timeline *tl);

/* Grab an individual clip, update tl state, and update drag state on status bar */
void timeline_clipref_grab(ClipRef *cr, ClipRefEdge edge);

/* NOT an entrypoint for user.
   Ungrabs clipref AND manages timeline grab state. */
void timeline_clipref_ungrab(ClipRef *cr);

/* Use anytime the playhead moves while clips are grabbed and dragging */
void timeline_grabbed_clips_move(Timeline *tl, int32_t move_by_sframes);

/****** END MID-LEVEL INTERFACE ******/

/* POSITION CACHEING, UNDO/REDO:
   - CACHE: grabbed clipref positions, edges, and track offsets need to be cached
     before undoable alterations occur
   - PUSH: when an undoable event is *completed*, an event must be pushed
   - UNDO/REDO: standard undo redo functions use cached positions
*/

/* Cache offsets from selector for use in moving clips from track to track */
void timeline_cache_grabbed_clip_offsets(Timeline *tl);

/* Call anytime a drag event *begins*  */
void timeline_cache_grabbed_clip_positions(Timeline *tl);

/* Call anytime a drag event has been completed */
void timeline_push_grabbed_clip_move_event(Timeline *tl);

/****** END POSITION CACHEING, UNDO/REDO ******/


#endif
