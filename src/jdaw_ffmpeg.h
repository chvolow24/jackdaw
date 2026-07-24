#ifndef JDAW_FFMPEG_H
#define JDAW_FFMPEG_H

#include <stdint.h>
#include <stdio.h>

enum ProjectAudioBitDepth {
    PROJ_AUDIO_16,
    PROJ_AUDIO_32    
};

/*
  Uses FFmpeg to open a file, find an appropriate codec, decode the
  audio, resample into Jackdaw's native sample rate, and place newly
  created buffers into L_dst and R_dst.
  
  Returns length of PCM buffer in sample frames 
*/
int32_t av_open_file(const char *filepath, float **L_dst, float **R_dst);


/* Encodes a mono audio buffer in native float format as a FLAC
   bitstream and writes it serially to 'f'.

   A gain value (used to accomodate overflow, which float allows
   but fixed point integer as required by FLAC does not) is serialized
   first and used in the paired 'decode_flac' function.

   Return 0 on success, <0 on error.

*/
int encode_flac(float *buf, int32_t len_sframes, enum ProjectAudioBitDepth bit_depth, uint8_t **encoded_dst, size_t *size_dst);

int decode_flac(void *data, size_t data_size, float *buf, int32_t len_sframes, enum ProjectAudioBitDepth bit_depth);
#endif
