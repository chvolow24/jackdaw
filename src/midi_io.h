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
#include "midi_objs.h"
/* #include "synth.h" */


#define MAX_UNCLOSED_NOTES 64
#define PM_EVENT_BUF_NUM_EVENTS 64
#define MAX_MIDI_DEVICES 16
#define MIDI_OUTPUT_LATENCY 0

#define MAX_SYNTHS 16

#if defined(__APPLE__) && defined(__MACH__)
#define MIDI_INTERFACE_NAME "CoreMIDI"
#elif defined(__linux__)
#define MIDI_INTERFACE_NAME "ALSA"
#endif

int midi_io_init();
void midi_io_deinit(void);

typedef struct midi_clip MIDIClip;
typedef struct synth Synth;

enum midi_ts_type {
    MIDI_TS_SFRAMES,
    MIDI_TS_MSEC
};

enum midi_device_type {
    MIDI_DEVICE_PM, /* A device managed by PortMidi */
    MIDI_DEVICE_QWERTY, /* The computer keyboard */
    MIDI_DEVICE_OTHER 
};

typedef struct midi_device {
    enum midi_device_type type;
    
    int32_t latency; /* Applicable for output devices only */
    PmDeviceID id;
    PmStream *stream;
    PmEvent buffer[PM_EVENT_BUF_NUM_EVENTS];
    uint8_t num_unconsumed_events;
    
    const PmDeviceInfo *info;
    char *name; /* Alias for info->name if PortMidi device */
    int input; /* Alias for info->input if PortMidi device */
    int output; /* Alias for info->output if PortMidi device */
    int opened; /* Alias for info->opened if PortMidi device */

    MIDIClip *current_clip;

    PmTimestamp record_start;
    bool recording;

    Note unclosed_notes[128]; /* lookup table by note value */
} MIDIDevice;

struct midi_io {
    /* MIDIDevice in; */
    /* bool in_active; */
    /* MIDIDevice out; */
    /* bool out_active; */
    
    MIDIDevice inputs[MAX_MIDI_DEVICES];
    uint8_t num_inputs;
    MIDIDevice outputs[MAX_MIDI_DEVICES];
    uint8_t num_outputs;

    MIDIDevice *primary_output;

    /* Synth *synths[MAX_SYNTHS]; */
    /* uint8_t num_synths; */

    Synth *monitor_synth;
    MIDIDevice *monitor_device;
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
void midi_device_read(MIDIDevice *d);
int midi_device_add_event(MIDIDevice *d, PmEvent e);
void midi_device_close_all_notes(MIDIDevice *d);
typedef struct clip_ref ClipRef;
/* ts_fmt: 0 = sample frames, 1 = msec */
void midi_device_output_chunk_to_clip(MIDIDevice *d, enum midi_ts_type);
int midi_clipref_output_chunk(ClipRef *cr, PmEvent *event_buf, int event_buf_max_len, int32_t chunk_tl_start, int32_t chunk_tl_end);
void timeline_flush_unclosed_midi_notes();
/* void midi_device_record_chunk(MIDIDevice *d); */

#endif
