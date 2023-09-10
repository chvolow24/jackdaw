#ifndef JDAW_AUDIO_H
#define JDAW_AUDIO_H

#include <stdbool.h>
#include "project.h"
#include "SDL2/SDL_audio.h"

void init_audio(void);
void start_recording(void);
void stop_recording(Clip *clip);
void start_playback(void);
void stop_playback(void);
int query_audio_devices(AudioDevice ***device_list, int iscapture);
int open_audio_device(AudioDevice *device, uint8_t desired_channels, int desired_sample_rate, uint16_t chunk_size);
const char *get_audio_fmt_str(SDL_AudioFormat fmt);


#endif