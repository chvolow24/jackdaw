#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_audio.h"

/* Define some low-level audio constants */
#define SAMPLE_RATE 44100
#define SAMPLES 512
#define BUFFLEN_SECONDS 5
#define BUFFLEN 44100 * BUFFLEN_SECONDS

/* Timeline- and clip-related constants */
#define MAX_TRACKS 25
#define MAX_NAMELENGTH 25

/* The main playback buffer, which is an array of 16-bit unsigned integer samples */
static Sint16 audio_buffer[BUFFLEN];

/* TODO: get rid of this nonsense */
static int write_buffpos = 0;
static int read_buffpos = 0;

static SDL_AudioDeviceID playback_device;
static SDL_AudioDeviceID recording_device;

typedef struct track {
    char name[MAX_NAMELENGTH];
    bool stereo;
    bool muted;
    bool solo;
    bool record;
    uint8_t tl_rank;
    Clip *clips;
    uint16_t num_clips;
    Timeline* timeline;
} Track;

typedef struct clip {
    char name[MAX_NAMELENGTH];
    int clip_gain;
    Track *track;
    uint32_t length; // samples
    uint32_t absolute_position; // samples
    Sint16 *samples;
} Clip;

struct time_sig {
    uint8_t num;
    uint8_t denom;
    uint8_t beats;
    uint8_t subidv;
};

typedef struct timeline {
    uint32_t play_position; // samples
    pthread_mutex_t play_position_lock;
    Track *tracks;
    uint8_t num_tracks;
    uint16_t tempo;
    bool click_on;
} Timeline;

bool in_clip(Track *track)
{

}

Sint16 get_track_sample(Track *track)
{
    if (track->muted) {
        return 0;
    }
    for (int i=0; track->num_clips; i++) {
        Clip *clip = track->clips + i;
        long int pos_in_clip = track->timeline->play_position - clip->absolute_position;
        if (pos_in_clip >= 0 && pos_in_clip < clip->length) {
            return *(clip->samples + pos_in_clip);
        } else {
            return 0;
        }
    }
}

Sint16 get_tl_sample(Timeline* tl) // Do I want to advance the play head in this function?
{
    Sint16 sample = 0;
    // pthread_mutex_lock(&(tl->play_position_lock));

    for (int i=0; i<tl->num_tracks; i++) {
        sample += get_track_sample(tl->tracks + i);
    }
    return sample;
    // pthread_mutex_unlock(&(tl->play_position_lock));
}

/*********************************************
 ******************* END TL ******************
**********************************************/


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