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
    audio_device.h

    * 
 *****************************************************************************************************************/


#ifndef JDAW_AUDIO_DEVICE_H
#define JDAW_AUDIO_DEVICE_H

#include <stdbool.h>
#include "SDL.h"
/* #include "project.h" */

typedef struct project Project;

/* Struct to contain information related to an audio device, including the SDL_AudioDeviceID. */
typedef struct audio_device{
    int index; /* index in the list created by query_audio_devices */
    const char *name;
    bool open; /* true if the device has been opened (SDL_OpenAudioDevice) */
    bool iscapture; /* true if device is a recording device, not a playback device */
    SDL_AudioDeviceID id;
    SDL_AudioSpec spec;
    int16_t *rec_buffer;
    uint32_t rec_buf_len_samples;
    int32_t write_buffpos_samples;
    bool active;
} AudioDevice;

int query_audio_devices(Project *proj, int iscapture);
int audio_device_open(Project *proj, AudioDevice *device);

#endif
