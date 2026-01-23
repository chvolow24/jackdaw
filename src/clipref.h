/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    clipref.h

    * a ClipRef is a reference to an audio clip (Clip) or MIDI clip (MIDIClip) on the timeline
    * audio- and MIDI-specific interfaces in audio_clip.h and MIDI_clip.h
*****************************************************************************************************************/

#ifndef JDAW_CLIPREF_H
#define JDAW_CLIPREF_H

#include "endpoint.h"
#include "grab.h"
#include "textbox.h"

typedef enum clip_type {
    CLIP_AUDIO,
    CLIP_MIDI
} ClipType;


typedef struct timeline Timeline;
typedef struct track Track;

typedef struct clip_ref {
    char name[MAX_NAMELENGTH];
    ClipType type;
    void *source_clip;
    bool deleted;
    bool grabbed;
    ClipRefEdge grabbed_edge;
    bool home;
    
    int32_t tl_pos;
    int32_t start_in_clip;
    int32_t end_in_clip;

    Track *track;

    Layout *layout;
    Textbox *label;

    /* Audio only */
    SDL_Texture *waveform_texture;
    SDL_Texture *old_texture;
    pthread_mutex_t waveform_texture_lock;
    pthread_mutex_t lock;
    bool waveform_redraw;

    /* MIDI only */
    int32_t first_note; /* index of the first note in clipref, or -1 if invalid */
    int32_t last_note; /* index of the last note in the clipref, of -1 if invalid */
    int32_t first_cc; /* index of first control change */
    int32_t first_pb; /* index of first pitch bend */

    /* Gain */
    Label *gain_label;
    float gain;
    float gain_ctrl; /* before exponent applied */
    Endpoint gain_ep;
} ClipRef;


ClipRef *clipref_create(
    Track *track,
    int32_t tl_pos,
    ClipType type,
    void *source_clip /* an audio or MIDI clip */
    );
void clipref_reset(ClipRef *tc, bool rescaled);
/* void clipref_grab(ClipRef *cr); */
/* void clipref_ungrab(ClipRef *tc); */
/* void clipref_grab_left(ClipRef *cr); */
/* void clipref_grab_right(ClipRef *cr); */
void clipref_delete(ClipRef *cr);
void clipref_undelete(ClipRef *cr);
void clipref_destroy(ClipRef *cr, bool displace_in_clip);
int32_t clipref_len(ClipRef *cr);
void clipref_move_to_track(ClipRef *cr, Track *target);
void clipref_destroy_no_displace(ClipRef *cr);
ClipRef *clipref_at_cursor();
ClipRef *clipref_before_cursor(int32_t *pos_dst);
ClipRef *clipref_after_cursor(int32_t *pos_dst);
ClipRef *clipref_at_cursor_in_track(Track *track);
void clipref_bring_to_front();
void clipref_rename(ClipRef *cr);
bool clipref_marked(Timeline *tl, ClipRef *cr);
int clipref_split_stereo_to_mono(ClipRef *cr, ClipRef **new_L_dst, ClipRef **new_R_dst);

void timeline_cut_at_cursor(Timeline *tl);
void timeline_cut_at_cursor_and_grab_edges(Timeline *tl);

#endif
