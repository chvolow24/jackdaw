/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/

/*****************************************************************************************************************
    wav.c

    * create and save wav files
 *****************************************************************************************************************/



/****************************** WAV File Specification ******************************

Positions	Sample Value	    Description
1 - 4	“RIFF”                  Marks the file as a riff file. Characters are each 1 byte long.
5 - 8	File size (integer)	Size of the overall file - 8 bytes, in bytes (32-bit integer). Typically, you’d fill this in after creation.
9 -12	“WAVE”	                File Type Header. For our purposes, it always equals “WAVE”.
13-16	“fmt "	                Format chunk marker. Includes trailing null
17-20	16	                Length of format data as listed above
21-22	1	                Type of format (1 is PCM) - 2 byte integer
23-24	2	                Number of Channels - 2 byte integer
25-28	44100	                Sample Rate - 32 byte integer. Common values are 44100 (CD), 48000 (DAT). Sample Rate = Number of Samples per second, or Hertz.
29-32	176400	                (Sample Rate * BitsPerSample * Channels) / 8.
33-34	4	                (BitsPerSample * Channels) / 8.1 - 8 bit mono2 - 8 bit stereo/16 bit mono4 - 16 bit stereo
35-36	16	                Bits per sample
37-40	“data”	                “data” chunk header. Marks the beginning of the data section.
41-44	File size (data)	Size of the data section.
*************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "SDL.h"
#include "project.h"
#include "mixdown.h"
#include "transport.h"
extern Project *proj;

//TODO: Endianness!
static void write_wav_header(FILE *f, uint32_t num_samples, uint16_t bits_per_sample, uint8_t channels)
{
    // int bps = 16;

    /* RIFF chunk descriptor */
    const static char spec[] = {'R','I','F','F'};
    uint32_t file_size = 44 + num_samples * (bits_per_sample / 8);
    const static char ftype[] = {'W','A','V','E'};

    /* fmt sub-chunk */
    const static char fmt_chunk_mk[] = {'f', 'm', 't', ' '};
    uint32_t fmt_data_len = 16;
    const static uint16_t fmt_type = 1;
    uint16_t num_channels = channels;
    uint32_t sample_rate = proj->sample_rate;
    uint32_t bytes_per_sec = (sample_rate * bits_per_sample * num_channels) / 8;
    uint16_t block_align = num_channels * fmt_data_len / 8;
    /* bits per sample goes here */

    /* data sub-chunk */
    char chunk_id[] = {'d','a','t','a'};
    uint32_t chunk_size = num_samples * (bits_per_sample / 8);

    fwrite(spec, 1, 4, f);
    fwrite(&file_size, 4, 1, f);
    fwrite(ftype, 1, 4, f);
    fwrite(fmt_chunk_mk, 1, 4, f);
    fwrite(&fmt_data_len, 4, 1, f);
    fwrite(&fmt_type, 2, 1, f);
    fwrite(&num_channels, 2, 1, f);
    fwrite(&sample_rate, 4, 1, f);
    fwrite(&bytes_per_sec, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&bits_per_sample, 2, 1, f);
    fwrite(chunk_id, 1, 4, f);
    fwrite(&chunk_size, 4, 1, f);
}

static void write_wav(const char *fname, int16_t *samples, uint32_t num_samples, uint16_t bits_per_sample, uint8_t channels)
{
    fprintf(stderr, "Write wav, num_samples: %d, chanels: %d, bitspsamle: %d\n", num_samples, channels, bits_per_sample);
    FILE* f = fopen(fname, "wb");
    if (!f) {
        fprintf(stderr, "Error: failed to open file at %s\n", fname);
    } else {
	fprintf(stderr, "Writing %lu samples (or %f minutes) to wav\n", (long unsigned) num_samples, (double)num_samples / 2.0f / 2.0f / proj->sample_rate);
        write_wav_header(f, num_samples, bits_per_sample, channels);
        fwrite(samples, bits_per_sample / 8, num_samples, f);
    }
    fclose(f);
    fprintf(stderr, "\t-> Done writing wav.\n");
}


void wav_write_mixdown(const char *filepath)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    /* reset_overlap_buffers(); */
    uint16_t chunk_len_sframes = proj->chunk_size_sframes;
    uint16_t chunk_len_samples = chunk_len_sframes * proj->channels;
    uint32_t len_sframes = tl->out_mark_sframes - tl->in_mark_sframes;
    uint32_t len_samples = proj->channels * len_sframes;
    uint16_t chunks = len_sframes / proj->chunk_size_sframes;
    int16_t *samples = malloc(sizeof(int16_t) * len_samples);

    for (int c=0; c<chunks; c++) {
        float *samples_L = get_mixdown_chunk(tl, 0, chunk_len_sframes, tl->in_mark_sframes + (c * chunk_len_sframes), 1);
        float *samples_R = get_mixdown_chunk(tl, 1, chunk_len_sframes, tl->in_mark_sframes + (c * chunk_len_sframes), 1);
        for (uint32_t i=0; i<chunk_len_samples; i+=2) {
            samples[c * chunk_len_samples + i] = samples_L[i/2] * INT16_MAX;
            samples[c * chunk_len_samples + i + 1] = samples_R[i/2] * INT16_MAX;
        }
        free(samples_L);
        free(samples_R);
    }
    write_wav(filepath, samples, len_samples, 16, proj->channels);
    free(samples);
}


void wav_load_to_track(Track *track, const char *filename, int32_t start_pos) {
    SDL_AudioSpec wav_spec;
    uint8_t* audio_buf = NULL;
    uint32_t audio_len_bytes = 0;

    // SDL_AudioSpec* SDL_LoadWAV(const char*    file,
    //                        SDL_AudioSpec* spec,
    //                        Uint8**        audio_buf,
    //                        Uint32*        audio_len)

    if (!(SDL_LoadWAV(filename, &wav_spec, &audio_buf, &audio_len_bytes))) {
        fprintf(stderr, "Error loading wav %s: %s", filename, SDL_GetError());
        return;
    }
    /* fprintf(stderr, "Success loading wav %s\n", filename); */
    /* fprintf(stderr, "Format = %s\n", get_audio_fmt_str(wav_spec.format)); */

    // int SDL_BuildAudioCVT(SDL_AudioCVT * cvt,
    //                   SDL_AudioFormat src_format,
    //                   Uint8 src_channels,
    //                   int src_rate,
    //                   SDL_AudioFormat dst_format,
    //                   Uint8 dst_channels,
    //                   int dst_rate);

    SDL_AudioCVT wav_cvt;
    int ret = SDL_BuildAudioCVT(&wav_cvt, wav_spec.format, wav_spec.channels, wav_spec.freq, proj->fmt, proj->channels, proj->sample_rate);
    if (ret < 0) {
        fprintf(stderr, "Error: unable to build SDL_AudioCVT. %s\n", SDL_GetError());
        return;
    } else if (ret == 1) { // Needs converstion
        fprintf(stderr, "Converting. Len mult: %d\n", wav_cvt.len_mult);
        wav_cvt.needed = 1;
        wav_cvt.len = audio_len_bytes;
        wav_cvt.buf = malloc(audio_len_bytes * wav_cvt.len_mult);
        memcpy(wav_cvt.buf, audio_buf, audio_len_bytes);
        if (SDL_ConvertAudio(&wav_cvt) < 0) {
            fprintf(stderr, "Error: Unable to convert audio. %s\n", SDL_GetError());
            return;
        }
        audio_len_bytes *= wav_cvt.len_ratio;
    } else if (ret == 0) { // No converstion needed
        fprintf(stderr, "No conversion needed. copying directly to track.\n");
        wav_cvt.buf = malloc(audio_len_bytes);
        memcpy(wav_cvt.buf, audio_buf, audio_len_bytes);
    } else {
        fprintf(stderr, "Error: unexpected return value for SDL_BuildAudioCVT.\n");
        return;
    }


    uint32_t buf_len_samples = audio_len_bytes / sizeof(int16_t);
    Clip *clip = project_add_clip(NULL, track);
    proj->active_clip_index++;
    
    clip->len_sframes = buf_len_samples / track->channels;
    create_clip_buffers(clip, clip->len_sframes);

    int16_t *src_buf = (int16_t *)wav_cvt.buf;

    for (uint32_t i=0; i<buf_len_samples - 2; i+=2) {
        clip->L[i/2] = (float) src_buf[i] / INT16_MAX;
        clip->R[i/2] = (float) src_buf[i+1] / INT16_MAX;
    }
    ClipRef *cr = track_create_clip_ref(track, clip, start_pos, true);
    timeline_reset(track->tl);
}
