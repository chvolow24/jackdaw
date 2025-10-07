/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    midi_objs.h

    * notes as represented on a piano roll
    * can be translated to MIDI messages
    * internal representaions of other MIDI objects
*****************************************************************************************************************/

#ifndef JDAW_NOTE_H
#define JDAW_NOTE_H

#include <stdbool.h>
#include <stdint.h>
#include "portmidi.h"

typedef enum note_edge {
    NOTE_EDGE_NONE,
    NOTE_EDGE_LEFT,
    NOTE_EDGE_RIGHT
} NoteEdge;

typedef struct note {
    uint8_t channel;
    uint8_t key;
    uint8_t velocity;
    int32_t start_rel; /* sample frames from clip start */
    int32_t end_rel; /* sample frames from clip start */
    bool unclosed; /* used in midi_device_record_chunk */

    /* Piano roll */
    bool grabbed;
    NoteEdge grabbed_edge;
    int32_t cached_start_rel;
    int32_t cached_end_rel;
} Note;

typedef struct {
    int32_t pos_rel;
    uint16_t value;
    float floatval;
} MEvent16bit;

typedef struct {
    int32_t pos_rel;
    uint8_t value;
    float floatval;
} MEvent8bit;


typedef struct controller {
    bool in_use;
    uint8_t type;
    uint8_t channel;
    bool has_lsb;
    MEvent8bit *changes;
    MEvent16bit *changes_precise;
    uint16_t num_changes;
    uint16_t changes_alloc_len;
} Controller;

typedef struct {
    uint8_t channel;
    MEvent16bit *changes;
    uint16_t num_changes;
    uint16_t changes_alloc_len;

} PitchBend;

typedef struct midi_event_ring_buf {
    int size;
    int read_i;
    int num_queued;
    PmEvent *buf;
} MIDIEventRingBuf;

void midi_event_ring_buf_init(MIDIEventRingBuf *rb);
void midi_event_ring_buf_deinit(MIDIEventRingBuf *rb);
PmEvent *midi_event_ring_buf_pop(MIDIEventRingBuf *rb, int32_t pop_if_before_or_at);

/* Events stored in ascending timestamp order
 Return 0 on success, -1 if ring buffer is full */
int midi_event_ring_buf_insert(MIDIEventRingBuf *rb, PmEvent e);


/* typedef struct midi_cc { */
/*     uint8_t channel; */
/*     int32_t pos_rel; */
/*     uint8_t type; */
/*     uint8_t value; */
/*     bool has_lsb; */
/*     uint8_t value_LSB; */
/*     uint16_t precise_value; */
/*     bool is_switch; */
/*     bool switch_state; */
/* } MIDICC; */

/* typedef struct midi_pitch_bend { */
/*     uint8_t channel; */
/*     int32_t pos_rel; */
/*     uint16_t value; */
/*     float floatval; */

/*     uint8_t data1; /\* Redundant, *\/ */
/*     uint8_t data2; /\* but preferred to recalculating *\/ */

/* } MIDIPitchBend; */


/* MIDICC midi_cc_from_event(PmEvent *e, int32_t pos_rel); */

/* PmEvent note_create_event_no_ts(Note *note, bool is_note_off); */
PmEvent note_create_event_no_ts(Note *note, uint8_t channel, bool is_note_off);
/* PmEvent midi_cc_create_event_no_ts(MIDICC *cc); */
/* PmEvent midi_pitch_bend_create_event_no_ts(MIDIPitchBend *pb); */
/* MIDIPitchBend midi_pitch_bend_from_event(PmEvent *e, int32_t pos_rel); */
float midi_pitch_bend_float_from_event(PmEvent *e);

void midi_controller_insert_change(Controller *c, int32_t pos, uint8_t data);
void midi_pitch_bend_insert_change(PitchBend *pb, int32_t pos, uint8_t data1, uint8_t data2);
PmEvent midi_controller_make_event(Controller *c, uint16_t index);
PmEvent pitch_bend_make_event(PitchBend *pb, uint16_t index);


void midi_event_ring_buf_init(struct midi_event_ring_buf *rb);


double mtof_calc(double m);
double ftom_calc(double f);

#endif
