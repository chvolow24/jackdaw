#ifndef JDAW_AUDIO_H
#define JDAW_AUDIO_H

#include "project.h"
#include "SDL2/SDL_audio.h"

typedef struct audiodevice{
    SDL_AudioDeviceID id;
    char *name;
    SDL_AudioSpec spec;
} AudioDevice;


void init_audio(void);
void start_recording(void);
void stop_recording(Clip *clip);
void start_playback(void);
void stop_playback(void);
int query_audio_devices(const char ***device_list, int iscapture);

#endif