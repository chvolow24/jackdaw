/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    midi_qwerty.h

    * use the computer keyboard as a MIDI controller
    * used by MIDI_QWERTY input mode (see input_mode.c)

*****************************************************************************************************************/

#ifndef JDAW_MIDI_QWERTY_H
#define JDAW_MIDI_QWERTY_H

#include <stdbool.h>
#include "midi_io.h"

typedef struct endpoint Endpoint;

void mqwert_init(void);
void mqwert_octave(int incr);
void mqwert_transpose(int incr);
void mqwert_velocity(int incr);
void mqwert_handle_key(char key, bool is_keyup);
void mqwert_get_current_notes(MIDIDevice *dst_device);
void mqwert_activate();

void mqwert_deactivate();

char *mqwert_get_octave_str();
char *mqwert_get_transpose_str();
char *mqwert_get_velocity_str();
char *mqwert_get_monitor_device_str();
Endpoint *mqwert_get_active_ep();

uint8_t mqwert_get_key_state(char key);

void mqwert_set_monitor_device_name(const char *device_name);



#endif
