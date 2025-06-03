/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    track_clip.h

    * abstraction over audio clip refs (type ClipRef) and MIDI clip refs (MIDIClipRef)
    * most timeline transformations (grabbing, moving, deleting, reinserting) handled abstractly
    * interface function names start with "tclip_"
    * audio- and MIDI-specific interfaces in audio_clip.h and MIDI_clip.h
*****************************************************************************************************************/

#ifndef JDAW_TRACK_CLIP_H
#define JDAW_TRACK_CLIP_H

#include "textbox.h"

enum track_clip_type {
    TCLIP_AUDIO,
    TCLIP_MIDI
};

typedef struct track Track;

typedef struct track_clip {
    char name[MAX_NAMELENGTH];
    enum track_clip_type type;
    void *obj;
    void *source_clip;
    bool deleted;
    bool grabbed;
    int32_t tl_pos;
    int32_t start_in_clip;
    int32_t end_in_clip;

    Track *track;

    Layout *layout;
    Textbox *label;
} TrackClip;

void tclip_reset(TrackClip *tc, bool rescaled);
void tclip_ungrab(TrackClip *tc);

#endif
