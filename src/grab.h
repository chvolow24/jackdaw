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

#include "clipref.h"
#include "project.h"

/* HIGH-LEVEL INTERFACE
   - accessible from userfn.c
   - defined grab-specific actions user can take
*/

/* Main entrypoint from userfn.c -- triggered by 'g' key */
void timeline_grab_ungrab(Timeline *tl);

/* Grab left edge of clip at cursor */
void timeline_grab_left_edge(Timeline *tl);

/* Grab right edge of clip at cursor */
void timeline_grab_right_edge(Timeline *tl);

/* Triggered if a "delete" action occurs on the timeline, not on a click track */
void timeline_delete_grabbed_cliprefs(Timeline *tl);

/****** END HIGH_LEVEL INTERFACE ******/

/* MID-LEVEL INTERFACE
 - clips may be individually ungrabbed incidentally, e.g. when deleted
 - timeline state must be updated accordingly
 - status bar should also be updated
*/

/* Ungrab all clips, update tl state, and update drag state on status bar */
void timeline_ungrab_all(Timeline *tl);

/* Grab an individual clip, update tl state, and update drag state on status bar */
void timeline_clipref_grab(ClipRef *cr, enum clipref_edge edge);

/* NOT an entrypoint for user.
   Ungrabs clipref AND manages timeline grab state. */
void timeline_ungrab_clipref(ClipRef *cr);

/****** END MID-LEVEL INTERFACE ******/


