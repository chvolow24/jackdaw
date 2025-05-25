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

int midi_device_create_jackdaw_out(MIDIDevice *dst)
{
    Pm_CreateVirtualOutput(
	"Jackdaw MIDI out",
	MIDI_INTERFACE_NAME,
	NULL);
}
