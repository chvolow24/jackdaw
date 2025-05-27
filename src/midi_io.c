/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    midi_io.c

    * MIDI devices, virtual and real
*****************************************************************************************************************/

#include "midi_io.h"
#include "portmidi.h"

int midi_device_populate_list(MIDIDevice *devices)
{
    int num_devices = Pm_CountDevices();

    for (int i=0; i<num_devices; i++) {
	MIDIDevice d = devices[i];
	d.info = Pm_GetDeviceInfo(i);
	d.stream = NULL;
	d.latency = 0;
    }
    return num_devices;
}

static int midi_device_create_jackdaw_out(MIDIDevice *dst)
{
    static const char *device_name = "Jackdaw MIDI out";
    PmError err = Pm_CreateVirtualOutput(
	device_name,
	MIDI_INTERFACE_NAME,
	NULL);
    if (err == pmInterfaceNotSupported) {
	fprintf(stderr, "Error: Interface \"%s\" not supported\n", MIDI_INTERFACE_NAME);
	return -1;
    } else if (err == pmInvalidDeviceId) {
	fprintf(stderr, "Error: device name \"%s\" invalid or already in use\n", device_name);
	return -1;
    }
    PmDeviceID id = err;
    dst->id = id;
    dst->info = Pm_GetDeviceInfo(id);
    return id;
}

static int midi_device_create_jackdaw_in(MIDIDevice *dst)
{
    static const char *device_name = "Jackdaw MIDI in";
    PmError err = Pm_CreateVirtualInput(
	device_name,
	MIDI_INTERFACE_NAME,
	NULL);
    if (err == pmInterfaceNotSupported) {
	fprintf(stderr, "Error: Interface \"%s\" not supported\n", MIDI_INTERFACE_NAME);
	return -1;
    } else if (err == pmInvalidDeviceId) {
	fprintf(stderr, "Error: device name \"%s\" invalid or already in use\n", device_name);
	return -1;
    }
    PmDeviceID id = err;
    dst->id = id;
    dst->info = Pm_GetDeviceInfo(id);
    return id;
}

int midi_io_init(void)
{

    PmError err = Pm_Initialize();
    if (err != pmNoError) {
	fprintf(stderr, "Error initializing PortMidi: %d\n", err);
    }
    return err;

    /* MIDIDevice jackdaw_midi_out, jackdaw_midi_in; */
}


void midi_io_deinit(void)
{
    PmError err = Pm_Terminate();
    if (err != pmNoError) {
	fprintf(stderr, "Error terminating PortMidi: %d\n", err);
    }
}

int midi_create_virtual_devices(struct midi *midi)
{
    int ret = midi_device_create_jackdaw_out(&midi->out);
    if (ret < 0) {
	fprintf(stderr, "Error creating jackdaw midi out\n");
	return ret;
    }
    ret = midi_device_create_jackdaw_in(&midi->in);
    if (ret < 0) {
	fprintf(stderr, "Error creating jackdaw midi in\n");
	return ret;
    }
    PmError err = Pm_OpenInput(&midi->in.stream, midi->in.id, NULL, PM_EVENT_BUF_NUM_EVENTS, NULL, NULL);
    if (err != pmNoError) {
	fprintf(stderr, "Error opening jackdaw midi in! %d\n", err);
    }
    err = Pm_OpenOutput(&midi->out.stream, midi->out.id, NULL, PM_EVENT_BUF_NUM_EVENTS, NULL, NULL, 0);
    if (err != pmNoError) {
	fprintf(stderr, "Error opening jackdaw midi out! %d\n", err);
    }
    return ret;
}

void midi_close_virtual_devices(struct midi *midi)
{
    PmError err = Pm_Close(midi->in.stream);
    if (err != pmNoError) {
	fprintf(stderr, "Error closing input device: %s\n", Pm_GetErrorText(err));
    }
    err = Pm_Close(midi->out.stream);
    if (err != pmNoError) {
	fprintf(stderr, "Error closing output device: %s\n", Pm_GetErrorText(err));
    }

}
