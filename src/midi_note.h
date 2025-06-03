#ifndef JDAW_MIDI_NOTE
#define JDAW_MIDI_NOTE

#include <stdbool.h>
#include <stdint.h>
#include "layout.h"
#include "textbox.h"

#define INIT_NUM_NOTES 16
#define MAX_MIDI_CLIP_REFS

typedef struct track Track;
typedef struct midi_clip_ref MIDIClipRef;

typedef struct midi_note {
    uint8_t note;
    uint8_t velocity;
    int32_t start_rel; /* sample frames from clip start */
    int32_t end_rel; /* sample frames from clip start */
} MIDINote;

typedef struct midi_clip {
    char name[MAX_NAMELENGTH];
    
    MIDINote *notes;
    uint32_t num_notes;
    uint32_t notes_alloc_len;
    MIDIClipRef **refs;
    uint16_t num_refs;
    uint16_t refs_alloc_len;

    bool recording;
} MIDIClip;

typedef struct midi_clip_ref {
    char name[MAX_NAMELENGTH];
    bool grabbed;
    bool deleted;
    
    MIDIClip *clip;
    int32_t tl_pos;
    int32_t start_in_clip;
    int32_t end_in_clip;
    Track *track;

    Layout *layout;
    Textbox *label;
} MIDIClipRef;

#endif
