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
#include <string.h>
#include "SDL.h"
#include "SDL_audio.h"

#include "project.h"
#include "audio.h"
#include "wav.h"


/* The main playback buffer, which is an array of 16-bit unsigned integer samples */
static Sint16 audio_buffer[BUFFLEN];

/* TODO: get rid of this nonsense */
static int write_buffpos = 0;
static int read_buffpos = 0;

static SDL_AudioDeviceID playback_device;
static SDL_AudioDeviceID recording_device;

extern Project *proj;


static void recording_callback(void* user_data, uint8_t *stream, int streamLength)
{
    if (write_buffpos + (streamLength / 2) < BUFFLEN) {
        memcpy(audio_buffer + write_buffpos, stream, streamLength);
    } else {
        write_buffpos = 0;
    }
    write_buffpos += streamLength / 2;
    proj->active_clip->length += streamLength / 2;
    proj->tl->record_position += streamLength / 2;
    // proj->tl->play_position += streamLength / 2;
}

static void play_callback(void* user_data, uint8_t* stream, int streamLength)
{
    int16_t *chunk = get_mixdown_chunk(proj->tl, streamLength / 2, false);
    memcpy(stream, chunk, streamLength);
    free(chunk);
}

void init_audio()
{
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "\nError initializing audio: %s", SDL_GetError());
        exit(1);
    }

    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);
    SDL_zero(obtained);
    desired.freq = SAMPLE_RATE;
    desired.format = AUDIO_S16SYS;
    desired.samples = CHUNK_SIZE;
    desired.channels = 2;
    desired.callback = play_callback;
    playback_device = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, SDL_AUDIO_ALLOW_ANY_CHANGE);
    desired.callback = recording_callback;
    recording_device = SDL_OpenAudioDevice(NULL, 1, &desired, &obtained, SDL_AUDIO_ALLOW_ANY_CHANGE);

    if (!playback_device) {
        fprintf(stderr, "\nError opening audio device.");
        exit(1);
    }
    if (!recording_device) {
        fprintf(stderr, "\nError opening recording device.");
        exit(1);   
    }

    SDL_zero(audio_buffer);
}

void start_recording()
{
    proj->tl->record_position = proj->tl->play_position;
    SDL_PauseAudioDevice(recording_device, 0);
    proj->tl->record_position = proj->tl->play_position;
}


void start_playback()
{
    SDL_PauseAudioDevice(playback_device, 0);
}
void stop_playback()
{
    SDL_PauseAudioDevice(playback_device, 1);
}

static void *copy_buff_to_clip(void* arg)
{
    fprintf(stderr, "Enter copy_buff_to_clip\n");
    Clip *clip = (Clip *)arg;
    clip->length = write_buffpos;
    clip->samples = malloc(sizeof(int16_t) * write_buffpos);
    int sample = audio_buffer[0];
    int next_sample = 0;
    for (int i=0; i<write_buffpos - 1; i++) {
        next_sample = audio_buffer[i+1];
        /* high freq/amp filtering */
        if (abs(next_sample - sample) < 5000) {
            sample = next_sample;
        }
        (clip->samples)[i] = sample;
    }
    clip->done = true;
    memset(audio_buffer, '\0', BUFFLEN);
    write_buffpos = 0;
    fprintf(stderr, "\t->exit copy_buff_to_clip\n");
    return NULL;
}

void stop_recording(Clip *clip)
{
    fprintf(stderr, "Enter stop_recording\n");
    SDL_PauseAudioDevice(recording_device, 1);
    pthread_t t;
    pthread_create(&t, NULL, copy_buff_to_clip, clip);
    fprintf(stderr, "\t->exit stop_recording\n");

}


/* AUDIO DEVICE HANDLING */
int query_audio_devices(AudioDevice ***device_list, int iscapture)
{
    fprintf(stderr, "Querying audio devices. iscapture: %d\n", iscapture);
    int num_devices = SDL_GetNumAudioDevices(iscapture);
    (*device_list) = malloc(sizeof(AudioDevice *) * num_devices);
    if (!(*device_list)) {
        fprintf(stderr, "Error: unable to allocate memory for device list.\n");
        exit(1);
    }
    for (int i=0; i<num_devices; i++) {
        AudioDevice *dev = malloc(sizeof(AudioDevice));
        dev->open = false;
        dev->index = i;
        dev->name = SDL_GetAudioDeviceName(i, iscapture);
        SDL_AudioSpec spec;
        if (SDL_GetAudioDeviceSpec(i, iscapture, &spec) != 0) {
            fprintf(stderr, "Error getting device spec: %s\n", SDL_GetError());
        };
        dev->spec = spec;
        (*device_list)[i] = dev;
        fprintf(stderr, "\tFound device: %s, index: %d\n", dev->name, dev->index);

    }

        
        /*
        #define AUDIO_U8        0x0008  < Unsigned 8-bit samples
        #define AUDIO_S8        0x8008  < Signed 8-bit samples
        #define AUDIO_U16LSB    0x0010  < Unsigned 16-bit samples
        #define AUDIO_S16LSB    0x8010  < Signed 16-bit samples
        #define AUDIO_U16MSB    0x1010  < As above, but big-endian byte order
        #define AUDIO_S16MSB    0x9010  < As above, but big-endian byte order
        #define AUDIO_S32LSB    0x8020  < 32-bit integer samples
        #define AUDIO_S32MSB    0x9020  < As above, but big-endian byte order 
        #define AUDIO_F32LSB    0x8120  < 32-bit floating point samples
        #define AUDIO_F32MSB    0x9120  < As above, but big-endian byte order
        */

        // Format seems to be null until device is opened. Move this.
        // char format[16];

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
int open_audio_device(AudioDevice *device, uint8_t desired_channels, int desired_sample_rate, uint16_t chunk_size)
{
    // SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
    device->spec.format = AUDIO_S16SYS; // For now, only allow 16bit signed samples in native byte order.
    device->spec.samples = chunk_size;
    device->spec.channels = desired_channels;
    device->spec.callback = device->iscapture ? recording_callback : play_callback;
    if ((device->id = SDL_OpenAudioDevice(device->name, device->iscapture, &(device->spec), &obtained, SDL_AUDIO_ALLOW_ANY_CHANGE))) {
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
