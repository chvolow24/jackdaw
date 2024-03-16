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
    audio.h

    * System audio controls; callbacks, etc.
    * Audio device handling
    * Signal processing is handled in dsp.c
 *****************************************************************************************************************/


#ifndef JDAW_AUDIO_H
#define JDAW_AUDIO_H

#include <stdbool.h>
/* #include "project.h" */
#include "SDL_audio.h"

/* Define some low-level audio constants */
#define SAMPLE_RATE 48000
#define CHUNK_SIZE 512
#define DEVICE_BUFFLEN_SECONDS 3600
// #define DEVICE_BUFFLEN_SFRAMES SAMPLE_RATE * BUFFLEN_SECONDS

typedef struct clip Clip;
typedef struct project Project;

/* Struct to contain information related to an audio device, including the SDL_AudioDeviceID. */
typedef struct audiodevice{
    int index; // index in the list created by query_audio_devices
    const char *name;
    bool open; // true if the device has been opened (SDL_OpenAudioDevice)
    bool iscapture; // true if device is a recording device, not a playback device
    SDL_AudioDeviceID id;
    SDL_AudioSpec spec;
    int16_t *rec_buffer;
    uint32_t rec_buf_len_samples;
    int32_t write_buffpos_samples;
    bool active;
} AudioDevice;

/* Initialize SDL audio subsystem */
void init_audio(void);

/* Opens an audio device and, if successful, unpauses the device to begin recording */
void start_device_recording(AudioDevice *dev);

/* Pauses a device that is actively recording, then closes it */
void stop_device_recording(AudioDevice *dev);

/* Unpause the designated project playback device. */
void start_device_playback();

/* Pause the designated project playback device. */
void stop_device_playback();

/* Get the list of available devices for either value (0, 1) of iscapture. 
The default device, if found will be at index 0.
Returns the number of audio devices found. */
int query_audio_devices(AudioDevice ***device_list, int iscapture);

/* Open an audio device and store info in the returned AudioDevice struct.
Returns 0 on success, -1 on failure. */
int open_audio_device(Project *proj, AudioDevice *device, uint8_t desired_channels);

/* Close an audio device that has previously been opened. */
void close_audio_device(AudioDevice *device);

/* Destroy an audio device and free all related data */
void destroy_audio_device(AudioDevice *device);

/* Get a string describing an SDL_AudioFormat (equivalent to the macro name of the format) */
const char *get_audio_fmt_str(SDL_AudioFormat fmt);

/* Copy an audio device's record buffer to a clip. */
void copy_device_buff_to_clip(Clip *clip);


#endif
