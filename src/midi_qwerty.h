#ifndef JDAW_MIDI_QWERTY_H
#define JDAW_MIDI_QWERTY_H

#include <stdbool.h>
#include "midi_io.h"

void mqwert_octave(int incr);
void mqwert_transpose(int incr);
void mqwert_handle_key(char key, bool is_keyup);
void mqwert_get_current_notes(MIDIDevice *dst_device);
void mqwert_activate();
void mqwert_deactivate();


#endif
