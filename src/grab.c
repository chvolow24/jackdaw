/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "clipref.h"
#include "grab.h"
#include "project.h"
#include "status.h"

/* Only manages clipref state */
static void loc_clipref_grab(ClipRef *cr)
{
    cr->grabbed = true;
    cr->grabbed_edge = CLIPREF_EDGE_NONE;
}

static void loc_clipref_grab_left(ClipRef *cr)
{
    cr->grabbed = true;
    cr->grabbed_edge = CLIPREF_EDGE_LEFT;
}

static void loc_clipref_grab_right(ClipRef *cr)
{
    cr->grabbed = true;
    cr->grabbed_edge = CLIPREF_EDGE_RIGHT;
}

static void loc_clipref_ungrab(ClipRef *cr)
{
    cr->grabbed = false;
    cr->grabbed_edge = CLIPREF_EDGE_NONE;
}


/* MID-LEVEL INTERFACE:
 - clips may be individually ungrabbed, e.g. when deleted
 - also grabbed, e.g. when a copied clip is pasted
 - timeline state must be updated accordingly
 - status bar should also be updated
*/

void timeline_clipref_grab(ClipRef *cr, enum clipref_edge edge)
{
    if (cr->track->tl->num_grabbed_clips == MAX_GRABBED_CLIPS) {
	char buf[128];
	snprintf(buf, 128, "Cannot grab more than %d clips", MAX_GRABBED_CLIPS);
	status_set_errstr(buf);
	return;
    }
    Timeline *tl = cr->track->tl;
    tl->grabbed_clips[tl->num_grabbed_clips] = cr;
    loc_clipref_grab(cr);
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
void timeline_ungrab_all(Timeline *tl)
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
    
    
    tl->needs_redraw = true;
}

/* Grab left edge of clip at cursor */
void timeline_grab_left_edge(Timeline *tl);

/* Grab right edge of clip at cursor */
void timeline_grab_right_edge(Timeline *tl);

void timeline_delete_grabbed_cliprefs(Timeline *tl);

