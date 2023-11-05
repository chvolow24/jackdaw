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

#ifndef JDAW_AUDIO_H
#define JDAW_AUDIO_H

#include <stdbool.h>
#include "project.h"
#include "SDL_audio.h"

/* Define some low-level audio constants */
#define SAMPLE_RATE 96000
#define CHUNK_SIZE 512
#define BUFFLEN_SECONDS 3600
#define BUFFLEN SAMPLE_RATE * BUFFLEN_SECONDS

typedef struct clip Clip;

typedef struct audiodevice{
    int index; // index in the list created by query_audio_devices
    const char *name;
    bool open; // true if the device has been opened (SDL_OpenAudioDevice)
    bool iscapture; // true if device is a recording device, not a playback device
    SDL_AudioDeviceID id;
    SDL_AudioSpec spec;
    int16_t *rec_buffer;
    int32_t write_buffpos_sframes;
    bool active;
} AudioDevice;


void init_audio(void);
void start_device_recording(AudioDevice *dev);
void stop_device_recording(AudioDevice *dev);
void start_device_playback();
void stop_device_playback();
int query_audio_devices(AudioDevice ***device_list, int iscapture);
int open_audio_device(Project *proj, AudioDevice *device, uint8_t desired_channels);
void close_audio_device(AudioDevice *device);
void destroy_audio_device(AudioDevice *device);
const char *get_audio_fmt_str(SDL_AudioFormat fmt);
void write_mixdown_to_wav();

void *copy_device_buff_to_clip(void* arg);


#endif