/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
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
#include "dir.h"
#include "project.h"
#include "mixdown.h"
#include "transport.h"
#include "status.h"
extern Project *proj;

#define WAV_READ_CK_LEN_BYTES 1000000

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
	fprintf(stderr, "Writing %lu samples (or %f minutes) to wav\n", (long unsigned) num_samples, (double)num_samples / 2.0f / 2.0f / proj->sample_rate / 60.0f);
        write_wav_header(f, num_samples, bits_per_sample, channels);
        fwrite(samples, bits_per_sample / 8, num_samples, f);
    }
    fclose(f);
    fprintf(stderr, "\t-> Done writing wav.\n");
}

const char *get_fmt_str(SDL_AudioFormat f)
{
    switch(f) {
    case AUDIO_U8:
	return "AUDIO_U8";
    case AUDIO_S8:
	return "AUDIO_S8";
    case AUDIO_U16LSB:
	return "AUDIO_U16LSB";
    case AUDIO_S16LSB:
	return "AUDIO_S16LSB";
    case AUDIO_U16MSB:
	return "AUDIO_U16MSB";
    case AUDIO_S16MSB:
	return "AUDIO_S16MSB";
    case AUDIO_S32LSB:
	return "AUDIO_S32LSB";
    case AUDIO_S32MSB:
	return "AUDIO_S32MSB";
    case AUDIO_F32LSB:
	return "AUDIO_F32LSB";
    case AUDIO_F32MSB:
	return "AUDIO_F32MSB";
    }
    return "unknown format";
}

void wav_write_mixdown(const char *filepath)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    /* reset_overlap_buffers(); */
    fprintf(stdout, "Chunk size sframes: %d, chan: %d, sr: %d\n", proj->chunk_size_sframes, proj->channels, proj->sample_rate);
    uint16_t chunk_len_sframes = proj->fourier_len_sframes;
    uint16_t chunk_len_samples = chunk_len_sframes * proj->channels;
    uint32_t len_sframes = tl->out_mark_sframes - tl->in_mark_sframes;
    uint32_t len_samples = proj->channels * len_sframes;
    fprintf(stderr, "len_sframes: %d, chunk len sframes: %d, chunks d: %f\n", len_sframes, chunk_len_sframes, (double)len_sframes / chunk_len_sframes);
    uint32_t chunks = len_sframes / chunk_len_sframes;
    uint32_t remainder_sframes = len_sframes - chunks * chunk_len_sframes;
    int16_t *samples = malloc(sizeof(int16_t) * len_samples);


    float *samples_L = malloc(sizeof(float) * chunk_len_sframes);
    float *samples_R = malloc(sizeof(float) * chunk_len_sframes);
    for (uint32_t c=0; c<chunks; c++) {
	get_mixdown_chunk(tl, samples_L, 0, chunk_len_sframes, tl->in_mark_sframes + (c * chunk_len_sframes), 1);
        get_mixdown_chunk(tl, samples_R, 1, chunk_len_sframes, tl->in_mark_sframes + (c * chunk_len_sframes), 1);
        for (uint32_t i=0; i<chunk_len_samples; i+=2) {
            samples[c * chunk_len_samples + i] = samples_L[i/2] * INT16_MAX;
            samples[c * chunk_len_samples + i + 1] = samples_R[i/2] * INT16_MAX;
        }
    }

    if (remainder_sframes > 0) {
	uint32_t done_len_sframes = chunk_len_sframes * chunks;
	uint32_t done_len_samples = chunk_len_samples * chunks;
	get_mixdown_chunk(tl, samples_L, 0, remainder_sframes, tl->in_mark_sframes + done_len_sframes, 1);
	get_mixdown_chunk(tl, samples_R, 1, remainder_sframes, tl->in_mark_sframes + done_len_sframes, 1);
	for (uint32_t i=0; i<remainder_sframes * proj->channels; i+=2) {
	    samples[done_len_samples + i] = samples_L[i/2] * INT16_MAX;
	    samples[done_len_samples + i + 1] = samples_R[i/2] * INT16_MAX;
	}
    }
    
    free(samples_L);
    free(samples_R);

	
    write_wav(filepath, samples, len_samples, 16, proj->channels);
    free(samples);
}


int32_t wav_load(Project *proj, const char *filename, float **L, float **R)
{
    SDL_AudioSpec wav_spec;
    uint8_t *audio_buf = NULL;
    uint32_t audio_len_bytes = 0;
    if (!(SDL_LoadWAV(filename, &wav_spec, &audio_buf, &audio_len_bytes))) {
	fprintf(stderr, "Error loading wav %s: %s\n", filename, SDL_GetError());
	return 0;
    }
    SDL_AudioCVT wav_cvt;
    int ret = SDL_BuildAudioCVT(&wav_cvt, wav_spec.format, wav_spec.channels, wav_spec.freq, proj->fmt, proj->channels, proj->sample_rate);
    uint8_t *final_buffer = NULL;
    int final_buffer_len;

    if (ret < 0) {
        fprintf(stderr, "Error: unable to build SDL_AudioCVT. %s\n", SDL_GetError());
        return 0;
    } else if (ret == 1) { // Needs conversion
        fprintf(stderr, "Converting. Len mult: %d\n", wav_cvt.len_mult);
	fprintf(stderr, "WAV specs: freq: %d, format: %s, channels: %d\n", wav_spec.freq, get_fmt_str(wav_spec.format), wav_spec.channels);
	fprintf(stderr, "dst specs: freq: %d, format %s, channels: %d\n", proj->sample_rate, get_fmt_str(proj->fmt), proj->channels);
	fprintf(stderr, "Len ratio: %f\n", wav_cvt.len_ratio);
	
	int read_pos = 0;
	int write_pos = 0;
        wav_cvt.needed = 1;

	while (read_pos < audio_len_bytes) {
	    int len = WAV_READ_CK_LEN_BYTES;
	    int remainder;
	    if ((remainder = audio_len_bytes - read_pos) < len) {
		len = remainder;
	    }
	    wav_cvt.len = len;
	
	    /* fprintf(stdout, "Audio len bytes: %"PRIu32" wav cvt len: %d\n", audio_len_bytes, wav_cvt.len); */
	    size_t alloc_len = (size_t)len * wav_cvt.len_mult;
	    /* fprintf(stdout, "Alloc len: %lu pos: %d/%d\n", alloc_len, read_pos, audio_len_bytes); */
	    wav_cvt.buf = malloc(alloc_len);
	    if (!wav_cvt.buf) {
		fprintf(stderr, "ERROR: unable to allocate space for conversion buffer\n");
		exit(1);
	    }
	    memcpy(wav_cvt.buf, audio_buf + read_pos, len);
	    if (SDL_ConvertAudio(&wav_cvt) < 0) {
		fprintf(stderr, "Error: Unable to convert audio. %s\n", SDL_GetError());
		return 1;
	    }
	    if (!final_buffer) {
		size_t final_buffer_alloc_len = audio_len_bytes * wav_cvt.len_ratio;
		final_buffer = malloc(final_buffer_alloc_len);
		final_buffer_len = audio_len_bytes * wav_cvt.len_ratio;
	    }
	    memcpy(final_buffer + write_pos, wav_cvt.buf, wav_cvt.len_cvt);
	    read_pos += len;
	    /* write_pos +=  wav_cvt.len; */
	    write_pos += wav_cvt.len_cvt;
	    free(wav_cvt.buf);
	}
    } else if (ret == 0) { // No conversion needed
        fprintf(stderr, "No conversion needed. copying directly to track.\n");
        /* wav_cvt.buf = malloc(audio_len_bytes); */
        /* memcpy(wav_cvt.buf, audio_buf, audio_len_bytes); */
	final_buffer = malloc(audio_len_bytes);
	memcpy(final_buffer, audio_buf, audio_len_bytes);
	final_buffer_len = audio_len_bytes;
	/* free(wav_cvt.buf); */
	/* SDL_FreeWAV(audio_buf); */
    } else {
        fprintf(stderr, "Error: unexpected return value for SDL_BuildAudioCVT.\n");
        return 0;
    }

    SDL_FreeWAV(audio_buf);

    uint32_t buf_len_samples = final_buffer_len / sizeof(int16_t);
    uint32_t buf_len_sframes = buf_len_samples / proj->channels;
    if (buf_len_samples < 3) {
	fprintf(stderr, "Error: cannot read wav file.\n");
	status_set_errstr("Error reading wav file.");
	return 0;
    }

    *L = malloc(buf_len_sframes * sizeof(float));
    *R = malloc(buf_len_sframes * sizeof(float));
    int16_t *src_buf = (int16_t *)final_buffer;
    for (uint32_t i=0; i<buf_len_sframes; i++) {
	(*L)[i] = (float)src_buf[i*2] / INT16_MAX;
	(*R)[i] = (float)src_buf[i*2+1] / INT16_MAX;
    }

    return buf_len_sframes;
}

ClipRef *wav_load_to_track(Track *track, const char *filename, int32_t start_pos) {
    if (track->num_clips == MAX_TRACK_CLIPS) return NULL;
    SDL_AudioSpec wav_spec;
    uint8_t* audio_buf = NULL;
    uint32_t audio_len_bytes = 0;
    if (!(SDL_LoadWAV(filename, &wav_spec, &audio_buf, &audio_len_bytes))) {
        fprintf(stderr, "Error loading wav %s: %s", filename, SDL_GetError());
        return NULL;
    }
    /* write_wav("TEST_WAV.wav", audio_buf, audio_len_bytes / 2, 16, 2); */
    /* exit(0); */

    SDL_AudioCVT wav_cvt;
    int ret = SDL_BuildAudioCVT(&wav_cvt, wav_spec.format, wav_spec.channels, wav_spec.freq, proj->fmt, proj->channels, proj->sample_rate);
    uint8_t *final_buffer = NULL;
    int final_buffer_len;
    if (ret < 0) {
        fprintf(stderr, "Error: unable to build SDL_AudioCVT. %s\n", SDL_GetError());
        return NULL;
    } else if (ret == 1) { // Needs conversion
        fprintf(stderr, "Converting. Len mult: %d\n", wav_cvt.len_mult);
	fprintf(stderr, "WAV specs: freq: %d, format: %s, channels: %d\n", wav_spec.freq, get_fmt_str(wav_spec.format), wav_spec.channels);
	fprintf(stderr, "dst specs: freq: %d, format %s, channels: %d\n", proj->sample_rate, get_fmt_str(proj->fmt), proj->channels);
	fprintf(stderr, "Len ratio: %f\n", wav_cvt.len_ratio);
	
	int read_pos = 0;
	int write_pos = 0;
        wav_cvt.needed = 1;

	while (read_pos < audio_len_bytes) {
	    int len = WAV_READ_CK_LEN_BYTES;
	    int remainder;
	    if ((remainder = audio_len_bytes - read_pos) < len) {
		len = remainder;
	    }
	    wav_cvt.len = len;
	
	    /* fprintf(stdout, "Audio len bytes: %"PRIu32" wav cvt len: %d\n", audio_len_bytes, wav_cvt.len); */
	    size_t alloc_len = (size_t)len * wav_cvt.len_mult;
	    /* fprintf(stdout, "Alloc len: %lu pos: %d/%d\n", alloc_len, read_pos, audio_len_bytes); */
	    wav_cvt.buf = malloc(alloc_len);
	    if (!wav_cvt.buf) {
		fprintf(stderr, "ERROR: unable to allocate space for conversion buffer\n");
		exit(1);
	    }
	    memcpy(wav_cvt.buf, audio_buf + read_pos, len);
	    if (SDL_ConvertAudio(&wav_cvt) < 0) {
		fprintf(stderr, "Error: Unable to convert audio. %s\n", SDL_GetError());
		return NULL;
	    }
	    if (!final_buffer) {
		size_t final_buffer_alloc_len = audio_len_bytes * wav_cvt.len_ratio;
		final_buffer = malloc(final_buffer_alloc_len);
		final_buffer_len = audio_len_bytes * wav_cvt.len_ratio;
	    }
	    memcpy(final_buffer + write_pos, wav_cvt.buf, wav_cvt.len_cvt);
	    read_pos += len;
	    /* write_pos +=  wav_cvt.len; */
	    write_pos += wav_cvt.len_cvt;
	    free(wav_cvt.buf);
	}
    } else if (ret == 0) { // No conversion needed
        fprintf(stderr, "No conversion needed. copying directly to track.\n");
        /* wav_cvt.buf = malloc(audio_len_bytes); */
        /* memcpy(wav_cvt.buf, audio_buf, audio_len_bytes); */
	final_buffer = malloc(audio_len_bytes);
	memcpy(final_buffer, audio_buf, audio_len_bytes);
	final_buffer_len = audio_len_bytes;
	/* free(wav_cvt.buf); */
	/* SDL_FreeWAV(audio_buf); */
    } else {
        fprintf(stderr, "Error: unexpected return value for SDL_BuildAudioCVT.\n");
        return NULL;
    }

    SDL_FreeWAV(audio_buf);

    uint32_t buf_len_samples = final_buffer_len / sizeof(int16_t);
    if (buf_len_samples < 3) {
	fprintf(stderr, "Error: cannot read wav file.\n");
	status_set_errstr("Error reading wav file.");
	return NULL;
    }
    Clip *clip = project_add_clip(NULL, track);
    proj->active_clip_index++;
    
    clip->len_sframes = buf_len_samples / track->channels;
    create_clip_buffers(clip, clip->len_sframes);

    int16_t *src_buf = (int16_t *)final_buffer;

    for (uint32_t i=0; i<buf_len_samples - 2; i+=2) {
        clip->L[i/2] = (float) src_buf[i] / INT16_MAX;
        clip->R[i/2] = (float) src_buf[i+1] / INT16_MAX;
    }
    free(final_buffer);
    final_buffer = NULL;
    src_buf = NULL;
    /* free(wav_cvt.buf); */
    ClipRef *cr = track_create_clip_ref(track, clip, start_pos, true);
    if (!cr) return NULL;
    char *filename_modifiable = strdup(filename);
    strncpy(clip->name, path_get_tail(filename_modifiable), MAX_NAMELENGTH);
    strncpy(cr->name, path_get_tail(filename_modifiable), MAX_NAMELENGTH);
    free(filename_modifiable);
    timeline_reset(track->tl, false);
    return cr;
}
