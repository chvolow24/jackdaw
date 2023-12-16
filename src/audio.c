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

    * System audio controls; callbacks, etc.
    * Audio device handling
    * Signal processing is handled in dsp.c
 *****************************************************************************************************************/

#include <pthread.h>
#include <time.h>
#include <string.h>
#include "SDL.h"
#include "SDL_audio.h"

#include "audio.h"
#include "mixdown.h"
#include "project.h"
#include "timeline.h"
#include "wav.h"



/* The main playback buffer, which is an array of 16-bit unsigned integer samples */
// static Sint16 audio_buffer[BUFFLEN];

/* TODO: get rid of this nonsense */
// int write_buffpos = 0;
// static int read_buffpos = 0;

// SDL_AudioDeviceID playback_device;
// static SDL_AudioDeviceID recording_device;

extern Project *proj;


static void recording_callback(void* user_data, uint8_t *stream, int streamLength)
{
    AudioDevice *dev = (AudioDevice *)user_data;
    uint32_t stream_len_samples = streamLength / sizeof(int16_t);

    if (dev->write_buffpos_samples + stream_len_samples < dev->rec_buf_len_samples) {
        memcpy(dev->rec_buffer + dev->write_buffpos_samples, stream, streamLength);
    } else {
        dev->write_buffpos_samples = 0;
        stop_device_recording(dev);
        fprintf(stderr, "ERROR: overwriting audio buffer of device: %s\n", dev->name);
    }

    dev->write_buffpos_samples += stream_len_samples;

    for (uint8_t i=0; i<proj->num_active_clips; i++) {
        Clip *clip = proj->active_clips[i];
        if (clip->input == dev) {
            clip->len_sframes += stream_len_samples / clip->channels;
            reset_cliprect(clip);
        }
    }
}


static void play_callback(void* user_data, Uint8* stream, int streamLength)
{
    memset(stream, '\0', streamLength);
    // AudioDevice *pbdev = (AudioDevice *)user_data;
    // fprintf(stderr, "\n\nPlayback device name: %s\n", pbdev->name);
    // fprintf(stderr, "Playback channels: %d\n", pbdev->spec.channels);
    // fprintf(stderr, "Playback format: %s\n", get_audio_fmt_str(pbdev->spec.format));
    // fprintf(stderr, "Playback freq: %d\n", pbdev->spec.freq);
    uint32_t stream_len_samples = streamLength / sizeof(int16_t);

    float *chunk_L = get_mixdown_chunk(proj->tl, 0, stream_len_samples / proj->channels, false);
    float *chunk_R = get_mixdown_chunk(proj->tl, 1, stream_len_samples / proj->channels, false);
    // Printing sample values to confirm that every other sample has value 0
    // for (uint8_t i = 0; i<200; i++) {
    //     fprintf(stderr, "%hd ", (int16_t)(chunk[i]));
    // }

    int16_t *stream_fmt = (int16_t *)stream;
    for (uint32_t i=0; i<stream_len_samples; i+=2)
    {
        float val_L = chunk_L[i/2];
        float val_R = chunk_R[i/2];
        proj->output_L[i/2] = val_L;
        proj->output_R[i/2] = val_R;
        stream_fmt[i] = (int16_t) (val_L * INT16_MAX);
        stream_fmt[i+1] = (int16_t) (val_R * INT16_MAX);
    }
    // memcpy(stream, chunk, streamLength);
    // Printing sample values to confirm that every other sample has value 0
    free(chunk_L);
    free(chunk_R);
    // for (uint8_t i = 0; i<40; i+=2) {
    //     fprintf(stderr, "%hd ", (int16_t)(stream[i]));
    // }
    // if (proj->tl->play_offset == 0) {
    //     long            ms; // Milliseconds
    //     time_t          s;  // Seconds
    //     struct timespec spec;
    //     clock_gettime(CLOCK_REALTIME, &spec);
    //     s  = spec.tv_sec;
    //     ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
    //     if (ms > 999) {
    //         s++;
    //         ms = 0;
    //     }
    //     // proj->tl->play_offset = ms;
    // }
    move_play_position(proj->play_speed * stream_len_samples / proj->channels);
}

void init_audio()
{
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "\nError initializing audio: %s", SDL_GetError());
        exit(1);
    }
}

// void start_device_recording(AudioDevice *dev)
// {
//     fprintf(stderr, "START RECORDING dev: %s\n", dev->name);
//     if (open_audio_device(proj, dev, 2) == 0) {
//         fprintf(stderr, "Opened device\n");
//         SDL_PauseAudioDevice(dev->id, 0);
//     } else {
//         fprintf(stderr, "Failed to open device\n");
//     }
// }

void stop_device_recording(AudioDevice *dev)
{
    fprintf(stderr, "STOP RECORDING dev: %s\n", dev->name);
    close_audio_device(dev);
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
    // proj->tl->play_offset = 0;
}

void copy_device_buff_to_clip(Clip *clip)
{
    fprintf(stderr, "Enter copy_buff_to_clip\n");
    clip->len_sframes = clip->input->write_buffpos_samples / clip->channels;
    create_clip_buffers(clip, clip->len_sframes);
    // clip->pre_proc = malloc(sizeof(int16_t) * clip->len_sframes * clip->channels);
    // clip->post_proc = malloc(sizeof(int16_t) * clip->len_sframes * clip->channels);
    // if (!clip->post_proc || !clip->pre_proc) {
    //     fprintf(stderr, "Error: unable to allocate space for clip samples\n");
    //     return;
    // }
    // int16_t sample = clip->input->rec_buffer[0];
    for (int i=0; i<clip->input->write_buffpos_samples; i+= clip->channels) {
        float sample_L = (float) clip->input->rec_buffer[i] / INT16_MAX;
        float sample_R = (float) clip->input->rec_buffer[i+1] / INT16_MAX;
        // fprintf(stderr, "Copying samples to clip %d: %f, %d: %f\n", i, sample_L, i+1, sample_R);
        clip->L[i/clip->channels] = sample_L;
        clip->R[i/clip->channels] = sample_R;
        // (clip->pre_proc)[i] = sample;
        // (clip->post_proc)[i] = sample;
    }
    clip->done = true;
    fprintf(stderr, "\t->exit copy_buff_to_clip\n");
    return;
}


int query_audio_devices(AudioDevice ***device_list, int iscapture)
{
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
    (*device_list) = malloc(sizeof(AudioDevice *) * num_devices);
    if (!(*device_list)) {
        fprintf(stderr, "Error: unable to allocate memory for device list.\n");
        exit(1);
    }
    (*device_list)[0] = default_dev;
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

int open_audio_device(Project *proj, AudioDevice *device, uint8_t desired_channels)
{
    SDL_AudioSpec obtained;
    SDL_zero(obtained);
    SDL_zero(device->spec);

    /* Project determines high-level audio settings */
    device->spec.format = AUDIO_S16LSB;
    device->spec.samples = proj->chunk_size_sframes;
    device->spec.freq = proj->sample_rate;

    device->spec.channels = desired_channels;
    device->spec.callback = device->iscapture ? recording_callback : play_callback;
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
    device->rec_buf_len_samples = proj->sample_rate * DEVICE_BUFFLEN_SECONDS * device->spec.channels;
    uint32_t device_buf_len_bytes = device->rec_buf_len_samples * sizeof(int16_t);
    device->rec_buffer = malloc(device_buf_len_bytes);
    device->write_buffpos_samples = 0;
    // if (dev->write_buffpos_samples + stream_len_samples < dev->rec_buf_len_samples
    if (!(device->rec_buffer)) {
        fprintf(stderr, "Error: unable to allocate space for device buffer.\n");
    }
    return 0;
}

void close_audio_device(AudioDevice *device)
{
    SDL_CloseAudioDevice(device->id);
}

void destroy_audio_device(AudioDevice *device)
{
    if (device->rec_buffer) {
        free(device->rec_buffer);
        device->rec_buffer = NULL;
        free(device);
        device = NULL;
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