/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    midi_io.h

    * MIDI devices, virtual and real
*****************************************************************************************************************/

#ifndef JDAW_MIDI_IO_H
#define JDAW_MIDI_IO_H

#include <stdio.h>
#include <stdbool.h>
#include "portmidi.h"

#define PM_EVENT_BUF_NUM_EVENTS 32
#define MAX_MIDI_DEVICES 6

#if defined(__APPLE__) && defined(__MACH__)
#define MIDI_INTERFACE_NAME "CoreMIDI"
#elif defined(__linux__)
#define MIDI_INTERFACE_NAME "ALSA"
#endif

typedef struct midi_device {
    int32_t latency; /* Applicable for output devices only */
    PmDeviceID id;
    PmStream *stream;
    PmEvent buffer[PM_EVENT_BUF_NUM_EVENTS];
    const PmDeviceInfo *info;
} MIDIDevice;

/* returns number of devices available */
int midi_device_populate_list(MIDIDevice *devices);

/* return 0 on success */
int midi_device_create_jackdaw_out(MIDIDevice *dst);



#endif
