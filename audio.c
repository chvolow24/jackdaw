/**************************************************************************************************
 * System audio controls; device settings, callbacks, etc.
 * Functions called primarily by project.c
 **************************************************************************************************/

#include <pthread.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_audio.h"

#include "project.h"

/* Define some low-level audio constants */
#define SAMPLE_RATE 44100
#define SAMPLES 512
#define BUFFLEN_SECONDS 60
#define BUFFLEN 44100 * BUFFLEN_SECONDS

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
    proj->tl->play_position += streamLength / 2;
}

static void play_callback(void* user_data, uint8_t* stream, int streamLength)
{
    int16_t *chunk = get_mixdown_chunk(proj->tl, streamLength / 2);
    memcpy(stream, chunk, streamLength);



    // if (read_buffpos + (streamLength / 2) < BUFFLEN) {
    //     memcpy(stream, audio_buffer + read_buffpos, streamLength);
    // } else {
    //     read_buffpos = 0;
    // }
    // read_buffpos += streamLength / 2;
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
    desired.samples = SAMPLES;
    desired.channels = 1;
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
    SDL_PauseAudioDevice(recording_device, 0);
}
// void stop_recording()
// {
//     SDL_PauseAudioDevice(recording_device, 1);
// }
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
    // memcpy(clip->samples, audio_buffer, write_buffpos);
    clip->done = true;
    memset(audio_buffer, '\0', BUFFLEN);
    write_buffpos = 0;
    // FILE *w = fopen("samples.txt", "w");
    // for (int i=0; i<clip->length; i++) {
    //     fprintf(w, "%d\n", (clip->samples)[i]);
    // }
}

void stop_recording(Clip *clip)
{
    SDL_PauseAudioDevice(recording_device, 1);
    pthread_t t;
    pthread_create(&t, NULL, copy_buff_to_clip, clip);
}