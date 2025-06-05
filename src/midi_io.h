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
#include "note.h"


#define MAX_UNCLOSED_NOTES 64
#define PM_EVENT_BUF_NUM_EVENTS 64
#define MAX_MIDI_DEVICES 6
#define MIDI_OUTPUT_LATENCY 0

#if defined(__APPLE__) && defined(__MACH__)
#define MIDI_INTERFACE_NAME "CoreMIDI"
#elif defined(__linux__)
#define MIDI_INTERFACE_NAME "ALSA"
#endif

int midi_io_init();
void midi_io_deinit(void);

typedef struct midi_clip MIDIClip;

typedef struct midi_device {
    int32_t latency; /* Applicable for output devices only */
    PmDeviceID id;
    PmStream *stream;
    PmEvent buffer[PM_EVENT_BUF_NUM_EVENTS];
    const PmDeviceInfo *info;
    MIDIClip *current_clip;

    PmTimestamp record_start;

    Note unclosed_notes[128]; /* lookup table by note value */
} MIDIDevice;

struct midi_io {
    MIDIDevice in;
    bool in_active;
    MIDIDevice out;
    bool out_active;
    
    MIDIDevice inputs[MAX_MIDI_DEVICES];
    uint8_t num_inputs;
    MIDIDevice outputs[MAX_MIDI_DEVICES];
    uint8_t num_outputs;

    MIDIDevice *primary_output;
};


/* One instance stored on Project */

/* returns number of devices available */
/* int midi_device_populate_list(MIDIDevice *devices); */
/* int midi_create_virtual_devices(struct midi_io *midi); */
/* void midi_close_virtual_devices(struct midi_io *midi); */

typedef struct session Session;
int session_init_midi(Session *session);
void session_deinit_midi(Session *session);

/* Called at init time, but can be used to refresh the list */
void session_populate_midi_device_lists(Session *session);

int midi_device_open(MIDIDevice *d);
int midi_device_close(MIDIDevice *d);
void midi_device_record_chunk(MIDIDevice *d);

#endif
