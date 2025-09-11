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
#include "midi_objs.h"
#include "textbox.h"

#define INIT_NUM_NOTES 16
/* #define MAX_MIDI_CLIP_REFS (INT16_MAX - 1) */
#define MAX_NUM_NOTES (INT32_MAX - 1)
#define MIDI_NUM_CONTROLLERS 120

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
    pthread_mutex_t notes_arr_lock;

    Controller controllers[MIDI_NUM_CONTROLLERS];
    PitchBend pitch_bend;
    /* MIDICC *ccs; */
    /* uint32_t num_ccs; */
    /* uint32_t ccs_alloc_len; */

    /* MIDIPitchBend *pbs; */
    /* uint32_t num_pbs; */
    /* uint32_t pbs_alloc_len; */
    
    ClipRef **refs;
    uint16_t num_refs;
    uint16_t refs_alloc_len;
    bool recording;

    char *midi_track_name;
    int primary_instrument;
    const char *primary_instrument_name;

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

void midi_clip_add_note(MIDIClip *mc, int channel, int note_val, int velocity, int32_t start_rel, int32_t end_rel);
int32_t midi_clipref_check_get_first_note(ClipRef *cr);
int32_t midi_clipref_check_get_last_note(ClipRef *cr);

void midi_clip_add_controller_change(MIDIClip *mclip, PmEvent e, int32_t pos);
void midi_clip_add_pitch_bend(MIDIClip *mclip, PmEvent e, int32_t pos);

/* void midi_clip_add_cc(MIDIClip *mc, MIDICC cc_in); */
/* int32_t midi_clipref_check_get_first_cc(ClipRef *cr); */
/* void midi_clip_add_pb(MIDIClip *mc, MIDIPitchBend pb_in); */
/* int32_t midi_clipref_check_get_first_pb(ClipRef *cr); */

int midi_clipref_output_chunk(ClipRef *cr, PmEvent *event_buf, int event_buf_max_len, int32_t chunk_tl_start, int32_t chunk_tl_end);

/* Destroys all refs */
void midi_clip_destroy(MIDIClip *mc);

/* Allocates an array at dst, return number of events */
uint32_t midi_clip_get_all_events(MIDIClip *mclip, PmEvent **dst);

/* Add notes etc. */
void midi_clip_read_events(MIDIClip *mclip, PmEvent *events, uint32_t num_events, enum midi_ts_type, int32_t ts_offset);

/* Returns the timeline position of the next note */
Note *midi_clipref_get_next_note(ClipRef *cr, int32_t from, int32_t *pos_dst);
Note *midi_clipref_get_prev_note(ClipRef *cr, int32_t from, int32_t *pos_dst);

#endif
