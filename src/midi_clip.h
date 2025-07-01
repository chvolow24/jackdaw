/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    midi_clip.h

    * collections of MIDI Notes (note.h)
    * referenced on timeline as ClipRef (see clipref.h)
    * can be recorded directly from a MIDI device (midi_io.h)
    * audio counterpart in audio_clip.h
*****************************************************************************************************************/


#ifndef JDAW_MIDI_NOTE
#define JDAW_MIDI_NOTE

#include <stdbool.h>
#include <stdint.h>
#include "midi_io.h"
#include "note.h"
#include "textbox.h"

#define INIT_NUM_NOTES 16
/* #define MAX_MIDI_CLIP_REFS (INT16_MAX - 1) */
#define MAX_NUM_NOTES (INT32_MAX - 1)

typedef struct track Track;
/* typedef struct midi_clip_ref MIDIClipRef; */

/* typedef struct midi_note { */
/*     uint8_t note; */
/*     uint8_t velocity; */
/*     int32_t start_rel; /\* sample frames from clip start *\/ */
/*     int32_t end_rel; /\* sample frames from clip start *\/ */
/* } MIDINote; */
typedef struct clip_ref ClipRef;

typedef struct midi_clip {
    char name[MAX_NAMELENGTH];
    uint32_t len_sframes;
    
    Note *notes;
    uint32_t num_notes;
    uint32_t notes_alloc_len;
    ClipRef **refs;
    uint16_t num_refs;
    uint16_t refs_alloc_len;
    bool recording;

    MIDIDevice *recorded_from;
} MIDIClip;

/* typedef struct midi_clip_ref { */
/*     char name[MAX_NAMELENGTH]; */
/*     bool grabbed; */
/*     bool deleted; */
    
/*     MIDIClip *clip; */
/*     int32_t tl_pos; */
/*     int32_t start_in_clip; */
/*     int32_t end_in_clip; */
/*     Track *track; */

/*     Layout *layout; */
/*     Textbox *label; */
/* } MIDIClipRef; */

/* Mirrors audio_clip_create; higher level than alloc->init */
MIDIClip *midi_clip_create(MIDIDevice *device, Track *target);


/* Mandatory initialization after allocating */
void midi_clip_init(MIDIClip *mclip);

void midi_clip_add_note(MIDIClip *mc, int note_val, int velocity, int32_t start_rel, int32_t end_rel);
int32_t midi_clipref_check_get_first_note(ClipRef *cr);
void midi_clip_add_note(MIDIClip *mc, int note_val, int velocity, int32_t start_rel, int32_t end_rel);

int midi_clipref_output_chunk(ClipRef *cr, PmEvent *event_buf, int event_buf_max_len, int32_t chunk_tl_start, int32_t chunk_tl_end);
#endif
