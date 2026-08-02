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


/* 

   Return 0 on success, <0 on error.
*/
int encode_proj_audio_flac(float *buf, int32_t len_sframes, enum ProjectAudioBitDepth bit_depth, uint8_t **encoded_dst, size_t *size_dst, float *gain_dst);

int decode_proj_audio_flac(void *data, size_t data_size, float *buf, int32_t *len_sframes_dst, enum ProjectAudioBitDepth bit_depth, float regain);

void *resample_96_to_48_create_ctx();
void *resample_48_to_96_create_ctx();
void resample_destroy_ctx(void *swr_v);
/* int32_t resample(void *swr_v, float *in, int32_t len_samples, float **out_dst); */
int32_t resample(void *swr_v, float *in, int32_t len_samples, float **out_dst);

#endif
