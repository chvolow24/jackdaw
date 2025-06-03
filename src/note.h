/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    note.h

    * Notes as represented on a piano roll
    * Can be translated to MIDI messages
 *****************************************************************************************************************/


#ifdef THIS_IS_NOT_DEFINED
#ifndef JDAW_NOTE_H

#include <stdint.h>
#include "project.h"

typedef struct note {
    uint8_t note;
    uint8_t velocity;
    int32_t start_pos_rel;
    int32_t end_pos_rel;    
} Note;

typedef struct midi_clip_ref MIDIClipRef;

typedef struct midi_clip {
    Note **notes; /* dynamically-sized array of notes */
    uint32_t num_notes;
    uint32_t notes_arrlen; /* available space in array */
    MIDIClipRef **refs; /* dynamically-sized array of refs */
    uint16_t num_refs;
    uint16_t refs_arrlen; /* available space in refs array */
} MIDIClip;

typedef struct midi_clip_ref {
    MIDIClip *clip;
    int32_t tl_pos;
    int32_t start_in_clip;
    int32_t end_in_clip;
} MIDIClipRef;



#endif
#endif
