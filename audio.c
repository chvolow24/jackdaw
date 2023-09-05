/**************************************************************************************************
 * System audio controls; device settings, callbacks, etc.
 * Functions called primarily by project.c
 **************************************************************************************************/

#include <pthread.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_audio.h"

/* Define some low-level audio constants */
#define SAMPLE_RATE 44100
#define SAMPLES 512
#define BUFFLEN_SECONDS 5
#define BUFFLEN 44100 * BUFFLEN_SECONDS

/* The main playback buffer, which is an array of 16-bit unsigned integer samples */
static Sint16 audio_buffer[BUFFLEN];

/* TODO: get rid of this nonsense */
static int write_buffpos = 0;
static int read_buffpos = 0;

static SDL_AudioDeviceID playback_device;
static SDL_AudioDeviceID recording_device;


static void recording_callback(void* user_data, Uint8* stream, int streamLength)
{
    if (write_buffpos + (streamLength / 2) < BUFFLEN) {
        memcpy(audio_buffer + write_buffpos, stream, streamLength);
    } else {
        write_buffpos = 0;
    }
    write_buffpos += streamLength / 2;

}

static void play_callback(void* user_data, Uint8* stream, int streamLength)
{

    if (read_buffpos + (streamLength / 2) < BUFFLEN) {
        memcpy(stream, audio_buffer + read_buffpos, streamLength);
    } else {
        read_buffpos = 0;
    }
    read_buffpos += streamLength / 2;
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
void stop_recording()
{
    SDL_PauseAudioDevice(recording_device, 1);
}
void start_playback()
{
    SDL_PauseAudioDevice(playback_device, 0);
}
void stop_playback()
{
    SDL_PauseAudioDevice(playback_device, 1);
}