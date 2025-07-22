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

typedef struct note {
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
    int32_t start_rel; /* sample frames from clip start */
    int32_t end_rel; /* sample frames from clip start */
    bool unclosed; /* used in midi_device_record_chunk */
} Note;

typedef struct midi_cc {
    uint8_t channel;
    int32_t pos_rel;
    uint8_t type;
    uint8_t value;
    bool has_lsb;
    uint8_t value_LSB;
    uint16_t precise_value;
    bool is_switch;
    bool switch_state;
} MIDICC;

typedef struct midi_pitch_bend {
    uint8_t channel;
    int32_t pos_rel;
    uint16_t value;
    float floatval;

    uint8_t data1; /* Redundant, */
    uint8_t data2; /* but preferred to recalculating */

} MIDIPitchBend;


MIDICC midi_cc_from_event(PmEvent *e, int32_t pos_rel);

/* PmEvent note_create_event_no_ts(Note *note, bool is_note_off); */
PmEvent note_create_event_no_ts(Note *note, uint8_t channel, bool is_note_off);
PmEvent midi_cc_create_event_no_ts(MIDICC *cc);
PmEvent midi_pitch_bend_create_event_no_ts(MIDIPitchBend *pb);

MIDIPitchBend midi_pitch_bend_from_event(PmEvent *e, int32_t pos_rel);
float midi_pitch_bend_float_from_event(PmEvent *e);

double mtof_calc(double m);

#endif
