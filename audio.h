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
    int index;
    const char *name;
    bool open;
    bool iscapture;
    SDL_AudioDeviceID id;
    SDL_AudioSpec spec;
    void *rec_buffer;
} AudioDevice;


void init_audio(void);
void start_recording(void);
void stop_recording(Clip *clip);
void start_playback(void);
void stop_playback(void);
int query_audio_devices(AudioDevice ***device_list, int iscapture);
int open_audio_device(AudioDevice *device, uint8_t desired_channels, int desired_sample_rate, uint16_t chunk_size);
const char *get_audio_fmt_str(SDL_AudioFormat fmt);
void write_mixdown_to_wav();


#endif