/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#ifndef JDAW_WAV_H
#define JDAW_WAV_H

#include <stdint.h>
#include "project.h"


/* Gets a mixdown chunk and calls functions in wav.c to create a wav file */
void wav_write_mixdown(const char *filepath);

// void write_wav(const char *fname, int16_t *samples, uint32_t num_samples, uint16_t bits_per_sample, uint8_t channels);
int32_t wav_load(Project *proj, const char *filename, float **L, float **R);
ClipRef *wav_load_to_track(Track *track, const char *filename, int32_t startpos);

#endif
