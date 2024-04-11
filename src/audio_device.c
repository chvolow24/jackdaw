/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/

/*****************************************************************************************************************
    audio_device.c

    * Query for available audio devices
    * Open and close individual audio devices
 *****************************************************************************************************************/


#include "audio_device.h"
#include "project.h"

#define DEVICE_BUFLEN_SECONDS 60 /* TODO: reduce, and write to clip during recording */

int query_audio_devices(Project *proj, int iscapture)
{
    AudioDevice **device_list;
    if (iscapture) {
	device_list = proj->record_devices;
    } else {
	device_list = proj->playback_devices;
    }
    
    AudioDevice *default_dev = malloc(sizeof(AudioDevice));

    if (SDL_GetDefaultAudioInfo((char**) &((default_dev->name)), &(default_dev->spec), iscapture) != 0) {
        default_dev->iscapture = iscapture;
        default_dev->name = "(no device found)";
        fprintf(stderr, "Error: unable to retrieve default %s device info. %s\n", iscapture ? "input" : "output", SDL_GetError());
    }
    default_dev->open = false;
    default_dev->active = false;
    default_dev->index = 0;
    default_dev->iscapture = iscapture;
    default_dev->write_buffpos_samples = 0;
    default_dev->rec_buffer = NULL;
    // memset(default_dev->rec_buffer, '\0', BUFFLEN / 2);


    int num_devices = SDL_GetNumAudioDevices(iscapture);

    device_list[0] = default_dev;
    for (int i=0,j=1; i<num_devices; i++,j++) {
        AudioDevice *dev = malloc(sizeof(AudioDevice));
        dev->name = SDL_GetAudioDeviceName(i, iscapture);
        if (strcmp(dev->name, default_dev->name) == 0) {
            free(dev);
            j--;
            continue;
        }
        dev->open = false;
        dev->active = false;
        dev->index = j;
        dev->iscapture = iscapture;
        dev->write_buffpos_samples = 0;
        // memset(dev->rec_buffer, '\0', BUFFLEN / 2);
        SDL_AudioSpec spec;
        if (SDL_GetAudioDeviceSpec(i, iscapture, &spec) != 0) {
            fprintf(stderr, "Error getting device spec: %s\n", SDL_GetError());
        };
        dev->spec = spec;
        dev->rec_buffer = NULL;
        device_list[j] = dev;
        /* fprintf(stderr, "\tFound device: %s, index: %d\n", dev->name, dev->index); */

    }
    return num_devices;
}


void play_callback(void *user_data, uint8_t *buf, int buflen) {}
void record_callback(void *user_data, uint8_t *buf, int buflen) {}
int audio_device_open(Project *proj, AudioDevice *device)
{
    SDL_AudioSpec obtained;
    SDL_zero(obtained);
    SDL_zero(device->spec);

    /* Project determines high-level audio settings */
    device->spec.format = AUDIO_S16LSB;
    device->spec.samples = proj->chunk_size_sframes;
    device->spec.freq = proj->sample_rate;

    device->spec.channels = proj->channels;
    device->spec.callback = device->iscapture ? record_callback : play_callback;
    device->spec.userdata = device;

    if ((device->id = SDL_OpenAudioDevice(device->name, device->iscapture, &(device->spec), &(obtained), 0)) > 0) {
        device->spec = obtained;
        device->open = true;
        fprintf(stderr, "Successfully opened device %s, with id: %d\n", device->name, device->id);
    } else {
        device->open = false;
        fprintf(stderr, "Error opening audio device %s : %s\n", device->name, SDL_GetError());
        return -1;
    }
    if (device->iscapture) {
	device->rec_buf_len_samples = proj->sample_rate * DEVICE_BUFLEN_SECONDS * device->spec.channels;
	uint32_t device_buf_len_bytes = device->rec_buf_len_samples * sizeof(int16_t);
	device->rec_buffer = malloc(device_buf_len_bytes);
	device->write_buffpos_samples = 0;
	if (!(device->rec_buffer)) {
	    fprintf(stderr, "Error: unable to allocate space for device buffer.\n");
	}
    }
    return 0;
}
