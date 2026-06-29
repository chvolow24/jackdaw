#ifndef JDAW_FFMPEG_H
#define JDAW_FFMPEG_H

#include <stdint.h>

/*
  Uses FFmpeg to open a file, find an appropriate codec, decode the
  audio, resample into Jackdaw's native sample rate, and place newly
  created buffers into L_dst and R_dst.
  
  Returns length of PCM buffer in sample frames 
*/
int32_t av_open_file(const char *filepath, float **L_dst, float **R_dst);


#endif
