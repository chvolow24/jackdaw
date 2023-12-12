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

    * Create and save wav files
 *****************************************************************************************************************/



/****************************** WAV File Specification ******************************

Positions	Sample Value	    Description
1 - 4	“RIFF”                  Marks the file as a riff file. Characters are each 1 byte long.
5 - 8	File size (integer)	    Size of the overall file - 8 bytes, in bytes (32-bit integer). Typically, you’d fill this in after creation.
9 -12	“WAVE”	                File Type Header. For our purposes, it always equals “WAVE”.
13-16	“fmt "	                Format chunk marker. Includes trailing null
17-20	16	                    Length of format data as listed above
21-22	1	                    Type of format (1 is PCM) - 2 byte integer
23-24	2	                    Number of Channels - 2 byte integer
25-28	44100	                Sample Rate - 32 byte integer. Common values are 44100 (CD), 48000 (DAT). Sample Rate = Number of Samples per second, or Hertz.
29-32	176400	                (Sample Rate * BitsPerSample * Channels) / 8.
33-34	4	                    (BitsPerSample * Channels) / 8.1 - 8 bit mono2 - 8 bit stereo/16 bit mono4 - 16 bit stereo
35-36	16	                    Bits per sample
37-40	“data”	                “data” chunk header. Marks the beginning of the data section.
41-44	File size (data)	    Size of the data section.
*************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "SDL.h"
#include "audio.h"
#include "project.h"

extern Project *proj;

//TODO: Endianness!
void write_wav_header(FILE *f, uint32_t num_samples, uint16_t bits_per_sample, uint8_t channels)
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
        write_wav_header(f, num_samples, bits_per_sample, channels);
        fwrite(samples, bits_per_sample / 8, num_samples, f);
    }
    fprintf(stderr, "/t-> Done writing wav.\n");
}

void write_mixdown_to_wav(char *filepath)
{
    uint32_t num_samples = (proj->tl->out_mark - proj->tl->in_mark) * proj->channels;
    int16_t *samples = get_mixdown_chunk(proj->tl, num_samples, true);
    write_wav(filepath, samples, num_samples, 16, 2);
    free(samples);
}


void load_wav_to_track(Track *track, const char *filename) {
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
    fprintf(stderr, "Success loading wav %s\n", filename);
    fprintf(stderr, "Format = %s\n", get_audio_fmt_str(wav_spec.format));

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

    Clip *clip = create_clip(track, audio_len_bytes / 2 / track->channels, proj->tl->play_position);
    // clip->len_sframes = audio_len_bytes / 2 / track->channels;

    clip->pre_proc = malloc(sizeof(int16_t) * clip->len_sframes * clip->channels);
    clip->post_proc = malloc(sizeof(int16_t) * clip->len_sframes * clip->channels);
    if (!clip->post_proc || !clip->pre_proc) {
        fprintf(stderr, "Error: unable to allocate space for clip samples\n");
        return;
    }

    fprintf(stderr, "memcpying %d bytes to clip.\n", audio_len_bytes);
    memcpy(clip->pre_proc, wav_cvt.buf, audio_len_bytes);
    memcpy(clip->post_proc, wav_cvt.buf, audio_len_bytes);
    clip->done = true;
}