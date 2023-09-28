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
    audio.c

    * High-level system audio controls; device settings, callbacks, etc.
    * Signal processing is handled in dsp.c
 *****************************************************************************************************************/

#include <pthread.h>
#include <time.h>
#include <string.h>
#include "SDL.h"
#include "SDL_audio.h"

#include "project.h"
#include "audio.h"
#include "wav.h"



/* The main playback buffer, which is an array of 16-bit unsigned integer samples */
// static Sint16 audio_buffer[BUFFLEN];

/* TODO: get rid of this nonsense */
int write_buffpos = 0;
// static int read_buffpos = 0;

// SDL_AudioDeviceID playback_device;
// static SDL_AudioDeviceID recording_device;

extern Project *proj;


static void recording_callback(void* user_data, uint8_t *stream, int streamLength)
{
    // fprintf(stderr, "RECORD: %d\n", proj->tl->play_position);
    AudioDevice *dev = (AudioDevice *)user_data;

    if (dev->write_buffpos + (streamLength / 2) < BUFFLEN) {
        if (proj->tl->record_offset == 0) {
            proj->tl->record_offset = proj->tl->play_position + proj->chunk_size * 4 - proj->active_clips[0]->absolute_position;
            long            ms; // Milliseconds
            time_t          s;  // Seconds
            struct timespec spec;
            clock_gettime(CLOCK_REALTIME, &spec);
            s  = spec.tv_sec;
            ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
            if (ms > 999) {
                s++;
                ms = 0;
            }
            fprintf(stderr, "RECORD time: %ld: %ld\n", s, ms);
            // fprintf(stderr, "First recording callback. Record offset: %d. Play position: %d, abspos: %d\n", proj->tl->record_offset, proj->tl->play_position, proj->active_clips[0]->absolute_position);
        }
        memcpy(dev->rec_buffer + dev->write_buffpos, stream, streamLength);
    } else {
        dev->write_buffpos = 0;
        stop_device_recording(dev);
        fprintf(stderr, "ERROR: overwriting audio buffer of device: %s\n", dev->name);
    }

    dev->write_buffpos += streamLength / 2;
    Clip *clip = NULL;
    for (uint8_t i=0; i<proj->num_active_clips; i++) {
        clip = proj->active_clips[i];
        if (clip->input == dev) {
            clip->length += streamLength / 2;
            reset_cliprect(clip);
        }
    }
}

static void play_callback(void* user_data, uint8_t* stream, int streamLength)
{
    // fprintf(stderr, "PLAY: %d\n", proj->tl->play_position);

    int16_t *chunk = get_mixdown_chunk(proj->tl, streamLength / 2, false);
    memcpy(stream, chunk, streamLength);
    free(chunk);
    if (proj->tl->play_offset == 0) {
        long            ms; // Milliseconds
        time_t          s;  // Seconds
        struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);
        s  = spec.tv_sec;
        ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
        if (ms > 999) {
            s++;
            ms = 0;
        }
        fprintf(stderr, "PLAY time: %ld: %ld\n", s, ms);
        proj->tl->play_offset = ms;
    }
}

void init_audio()
{
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "\nError initializing audio: %s", SDL_GetError());
        exit(1);
    }

    // SDL_AudioSpec desired, obtained;
    // SDL_zero(desired);
    // SDL_zero(obtained);
    // desired.freq = SAMPLE_RATE;
    // desired.format = AUDIO_S16SYS;
    // desired.samples = CHUNK_SIZE;
    // desired.channels = 2;
    // desired.callback = play_callback;
    // playback_device = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, SDL_AUDIO_ALLOW_ANY_CHANGE);
    // // desired.callback = recording_callback;
    // // recording_device = SDL_OpenAudioDevice("MacBook Air Microphone", 1, &desired, &obtained, SDL_AUDIO_ALLOW_ANY_CHANGE);

    // if (!playback_device) {
    //     fprintf(stderr, "Error opening audio device: %s", SDL_GetError());
    //     exit(1);
    // }
    // // if (!recording_device) {
    // //     fprintf(stderr, "Error opening recording device: %s\n", SDL_GetError());
    // //     exit(1);   
    // // }

    // SDL_zero(audio_buffer);
}

void start_device_recording(AudioDevice *dev)
{
    fprintf(stderr, "START RECORDING dev: %s\n", dev->name);
    // proj->tl->record_position = proj->tl->play_position;
    SDL_PauseAudioDevice(dev->id, 0);
    // proj->tl->record_position = proj->tl->play_position;
}

void stop_device_recording(AudioDevice *dev)
{
    fprintf(stderr, "STOP RECORDING dev: %s\n", dev->name);
    SDL_PauseAudioDevice(dev->id, 1);
    // dev->active = false;
}

void start_device_playback()
{
    SDL_PauseAudioDevice(proj->playback_device->id, 0);
    proj->playback_device->active = true;
}

void stop_device_playback()
{
    SDL_PauseAudioDevice(proj->playback_device->id, 1);
    proj->playback_device->active = false;
    proj->tl->play_offset = 0;
}

int j=0;
void *copy_buff_to_clip(void* arg)
{
    fprintf(stderr, "Enter copy_buff_to_clip\n");
    Clip *clip = (Clip *)arg;
    clip->length = clip->input->write_buffpos;
    if (j<20) {
        fprintf(stderr, "Clip length: %d\n", clip->length);
        j++;
    }
    clip->samples = malloc(sizeof(int16_t) * clip->input->write_buffpos);
    // clip->length = clip->input->write_buffpos;
    int16_t sample = clip->input->rec_buffer[0];
    // int sample = audio_buffer[0];
    int16_t next_sample = 0;
    for (int i=0; i<clip->input->write_buffpos - 1; i++) {
        next_sample = clip->input->rec_buffer[i+1];
        // /* high freq/amp filtering */
        // if (abs(next_sample - sample) < 5000) {
            sample = next_sample;
        // }
        (clip->samples)[i] = sample;
    }
    clip->done = true;
    fprintf(stderr, "\t->exit copy_buff_to_clip\n");
    return NULL;
}


/* AUDIO DEVICE HANDLING */
int query_audio_devices(AudioDevice ***device_list, int iscapture)
{
    fprintf(stderr, "Querying audio devices. iscapture: %d\n", iscapture);

    AudioDevice *default_dev = malloc(sizeof(AudioDevice));

    if (SDL_GetDefaultAudioInfo((char**) &((default_dev->name)), &(default_dev->spec), iscapture) != 0) {
        fprintf(stderr, "Error: unable to retrieve default %s device info. %s\n", iscapture ? "input" : "output", SDL_GetError());
    }
    default_dev->open = false;
    default_dev->active = false;
    default_dev->index = 0;
    default_dev->iscapture = iscapture;
    default_dev->write_buffpos = 0;
    memset(default_dev->rec_buffer, '\0', BUFFLEN / 2);


    int num_devices = SDL_GetNumAudioDevices(iscapture);
    (*device_list) = malloc(sizeof(AudioDevice *) * num_devices);
    if (!(*device_list)) {
        fprintf(stderr, "Error: unable to allocate memory for device list.\n");
        exit(1);
    }
    (*device_list)[0] = default_dev;
    fprintf(stderr, "Created default dev, device list, and set index 0\n");
    for (int i=0,j=1; i<num_devices; i++,j++) {
        AudioDevice *dev = malloc(sizeof(AudioDevice));
        dev->name = SDL_GetAudioDeviceName(i, iscapture);
        if (strcmp(dev->name, default_dev->name) == 0) {
            free(dev);
            j--;
            break;
        }
        dev->open = false;
        dev->active = false;
        dev->index = j;
        dev->iscapture = iscapture ;
        dev->write_buffpos = 0;
        memset(dev->rec_buffer, '\0', BUFFLEN / 2);
        SDL_AudioSpec spec;
        if (SDL_GetAudioDeviceSpec(i, iscapture, &spec) != 0) {
            fprintf(stderr, "Error getting device spec: %s\n", SDL_GetError());
        };
        dev->spec = spec;
        (*device_list)[j] = dev;
        fprintf(stderr, "\tFound device: %s, index: %d\n", dev->name, dev->index);

    }
    return num_devices;
}


/*==================== SDL_AudioSpec Data Fields =========================

int                 | freq      | DSP frequency (samples per second); see Remarks for details
SDL_AudioFormat     | format    | audio data format; see Remarks for details
Uint8               | channels  | number of separate sound channels: see Remarks for details
Uint8               | silence   | audio buffer silence value (calculated)
Uint16              | samples   | audio buffer size in samples (power of 2); see Remarks for details
Uint32              | size      | audio buffer size in bytes (calculated)
SDL_AudioCallback   | callback  | the function to call when the audio device needs more data; see Remarks for details
void*               | userdata  | a pointer that is passed to callback (otherwise ignored by SDL)

==========================================================================*/

/* Open an audio device and store info in the returned AudioDevice struct. */
int open_audio_device(AudioDevice *device, uint8_t desired_channels)
{
    if (!proj) {
        fprintf(stderr, "Error: opening audio device without an active Project is forbidden.\n");
        exit(1);
    }

    SDL_AudioSpec obtained;

    /* Project determines high-level audio settings */
    device->spec.format = proj->fmt;
    device->spec.samples = proj->chunk_size;
    device->spec.freq = proj->sample_rate;

    device->spec.channels = desired_channels;
    device->spec.callback = device->iscapture ? recording_callback : play_callback;
    device->spec.userdata = device;

    if ((device->id = SDL_OpenAudioDevice(device->name, device->iscapture, &(device->spec), &(obtained), 0)) > 0) {
        device->spec = obtained;
        device->open = true;
        return 0;
    } else {
        device->open = false;
        fprintf(stderr, "Error opening audio device %s : %s\n", device->name, SDL_GetError());
        return -1;
    }
}


const char *get_audio_fmt_str(SDL_AudioFormat fmt)
{
    const char* fmt_str = NULL;
    switch (fmt) {
        case AUDIO_U8:
            fmt_str = "AUDIO_U8";
            break;
        case AUDIO_S8:
            fmt_str = "AUDIO_S8";
            break;
        case AUDIO_U16LSB:
            fmt_str = "AUDIO_U16LSB";
            break;
        case AUDIO_S16LSB:
            fmt_str = "AUDIO_S16LSB";
            break;
        case AUDIO_U16MSB:
            fmt_str = "AUDIO_U16MSB";
            break;
        case AUDIO_S16MSB:
            fmt_str = "AUDIO_S16MSB";
            break;
        case AUDIO_S32LSB:
            fmt_str = "AUDIO_S32LSB";
            break;
        case AUDIO_S32MSB:
            fmt_str = "AUDIO_S32MSB";
            break;
        case AUDIO_F32LSB:
            fmt_str = "AUDIO_F32LSB";
            break;
        case AUDIO_F32MSB:
            fmt_str = "AUDIO_F32MSB";
            break;
        default:
            break;
    }
    return fmt_str;
}


void write_mixdown_to_wav()
{
    uint32_t num_samples = proj->tl->out_mark - proj->tl->in_mark;
    int16_t *samples = get_mixdown_chunk(proj->tl, num_samples, true);
    write_wav("wavs/testfile.wav", samples, num_samples, 16, 2);
    free(samples);
}
