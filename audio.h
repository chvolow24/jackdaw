/**************************************************************************************************
 * Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation. Built on SDL.
***************************************************************************************************

  Copyright (C) 2023 Charlie Volow

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

**************************************************************************************************/

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