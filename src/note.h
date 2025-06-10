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

#ifndef JDAW_NOTE_H
#define JDAW_NOTE_H

#include <stdint.h>

typedef struct note {
    uint8_t note;
    uint8_t velocity;
    int32_t start_rel; /* sample frames from clip start */
    int32_t end_rel; /* sample frames from clip start */
} Note;

double mtof_calc(double m);

#endif
