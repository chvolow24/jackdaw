#ifndef JDAW_AUDIO_H
#define JDAW_AUDIO_H

#include "project.h"

void init_audio(void);
void start_recording(void);
void stop_recording(Clip *clip);
void start_playback(void);
void stop_playback(void);

#endif