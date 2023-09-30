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
    dot_jdaw.c

    * Define the .jdaw file type and provide functions for saving and opening .jdaw files
 *****************************************************************************************************************/


#include <stdio.h>
#include "project.h"
#include "gui.h"
#include "audio.h"

extern Project *proj;
extern bool sys_byteorder_le;

/**************************** .JDAW FILE SPEC ***********************************

===========================================================================================================
SCTN          POS IN SCTN       TYPE                      BYTE ORDER   FIELD NAME OR VALUE
===========================================================================================================
HDR           0-3               char[4]                                "JDAW"
HDR           4-259             char[255]                              project name
HDR           260               uint8_t                                channels
HDR           261-264           uint32_t                  LE           sample rate
HDR           264-265           uint16_t                  LE           chunk size (power of 2)
HDR           266-267           SDL_AudioFormat (16bit)                SDL Audio Format
HDR           268               uint8_t                                num tracks
TRK_HDR       0-3               char[4]                                "TRCK"
TRK_HDR       4-259             char[255]                              track name
TRK_HDR       260               uint8_t                                num_clips
CLP_HDR       0-3               char[4]                                "CLIP"
CLP_HDR       4-259             char[255]                              clip name
CLP_HDR       260-263           int32_t                   LE           absolute position (in timeline)
CLP_HDR       264-267           uint32_t                  LE           length (samples)
CLP_HDR       268-271           char[4]                                "data"
CLP_DATA      268-???           [int16_t arr]             [LE]         CLIP SAMPLE DATA     

*********************************************************************************/


const static char hdr_jdaw[] = {'J', 'D', 'A', 'W'};
const static char hdr_trk[] = {'T', 'R', 'C', 'K'};
const static char hdr_clp[] = {'C', 'L', 'I', 'P'};
const static char hdr_data[] = {'d', 'a', 't', 'a'};

void write_clip_to_jdaw(FILE *f, Clip *clip);
void write_track_to_jdaw(FILE *f, Track *track);
void read_clip_from_jdaw(FILE *f, Clip *track);
void read_track_from_jdaw(FILE *f, Track *track);

extern uint8_t scale_factor;

void write_jdaw_file(const char *path) 
{
    if (!proj) {
        fprintf(stderr, "No project to save. Exiting.\n");
        return;
    }
    FILE* f = fopen(path, "wb");

    fwrite(hdr_jdaw, 1, 4, f);
    fwrite(proj->name, 1, 255, f);
    fwrite(&(proj->channels), 1, 1, f);
    if (sys_byteorder_le) {
        fwrite(&(proj->sample_rate), 4, 1, f);
        fwrite(&(proj->chunk_size), 2, 1, f);
    } else {
        //TODO: handle big endian
    }
    fwrite(&(proj->fmt), 2, 1, f);
    fwrite(&(proj->tl->num_tracks), 1, 1, f);
    for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
        write_track_to_jdaw(f, proj->tl->tracks[i]);
    }
    fclose(f);
}

void write_track_to_jdaw(FILE *f, Track *track)
{
    fwrite(hdr_trk, 1, 4, f);
    fwrite(track->name, 1, 255, f);
    fwrite(&(track->num_clips), 1, 1, f);
    for (uint8_t i=0; i<track->num_clips; i++) {
        write_clip_to_jdaw(f, track->clips[i]);
    }
}

void write_clip_to_jdaw(FILE *f, Clip *clip) 
{
    fwrite(hdr_clp, 1, 4, f);
    fwrite(clip->name, 1, 255, f);
    if (sys_byteorder_le) {
        fwrite(&(clip->absolute_position), 4, 1, f);
        fwrite(&(clip->length), 4, 1, f);
    } else {
        //TODO: handle big endian
    }
    fwrite(hdr_data, 1, 4, f);
    fwrite(clip->samples, 2, clip->length, f);
}

Project *open_jdaw_file(const char *path)
{
    proj = create_empty_project();
    FILE *f = fopen(path, "r");
    char hdr_buffer[5];
    fread(hdr_buffer, 1, 4, f);
    hdr_buffer[4] = '\0';
    if (strcmp(hdr_buffer, "JDAW") != 0) {
        fprintf(stderr, "Error: unable to read file. 'JDAW' specifier missing %s\n", path);
        free(proj);
        return NULL;
    }
    fread(proj->name, 1, 255, f);
    fread(&(proj->channels), 1, 1, f);
    if (sys_byteorder_le) {
        fread(&(proj->sample_rate), 4, 1, f);
        fread(&(proj->chunk_size), 2, 1, f);
    } else {
        //TODO: handle big endian
    }
    fread(&(proj->fmt), 2, 1, f);
    fprintf(stderr, "Format : %s\n", get_audio_fmt_str(proj->fmt));
    uint8_t num_tracks = 0;
    fread(&num_tracks, 1, 1, f);
    fprintf(stderr, "num tracks? %d\n", num_tracks);

    char project_window_name[MAX_NAMELENGTH + 10];
    sprintf(project_window_name, "Jackdaw | %s", proj->name);

    proj->jwin = create_jwin(project_window_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED-20, 900, 650);
    proj->tl->console_width = TRACK_CONSOLE_WIDTH;
    proj->tl->rect = get_rect((SDL_Rect){0, 0, proj->jwin->w, proj->jwin->h,}, TL_RECT);
    proj->tl->audio_rect = (SDL_Rect) {proj->tl->rect.x + TRACK_CONSOLE_WIDTH + COLOR_BAR_W + PADDING, proj->tl->rect.y, proj->tl->rect.w, proj->tl->rect.h}; // SET x in track
    activate_audio_devices(proj);

    char logfilename[11];

    while (num_tracks > 0) {
        fprintf(stderr, "TRACKS TO DO: %d\n", num_tracks);
        sprintf(logfilename, "error%d.log", num_tracks);
        FILE *logf = fopen(logfilename, "w");
        log_project_state(logf);
        Track *track = create_track(proj->tl, true);
        read_track_from_jdaw(f, track);
        num_tracks--;
        fclose(logf);
    }
    fprintf(stderr, "DONE with file read. Creating log.\n");

    FILE *final_log = fopen("final.log", "w");
    log_project_state(final_log);
    fclose(final_log);
    fclose(f);
    return proj;
}

void read_track_from_jdaw(FILE *f, Track *track)
{
    char hdr_buffer[5];
    fread(hdr_buffer, 1, 4, f);
    hdr_buffer[4] = '\0';
    if (strcmp(hdr_buffer, "TRCK") != 0) {
        fprintf(stderr, "Error: unable to read file. 'TRCK' specifier not found where expected. Found '%s'\n", hdr_buffer);
        fprintf(stderr, "%c %c %c %c\n", hdr_buffer[0], hdr_buffer[1], hdr_buffer[2], hdr_buffer[3]);
        // free(proj);
        // destroy_track(track);
        return;
    }
    fread(track->name, 1, 255, f);
    fprintf(stderr, "Track named: '%s'\n", track->name);
    uint8_t num_clips;
    fread(&num_clips, 1, 1, f);
    while (num_clips > 0) {
        Clip *clip = create_clip(track, 0, 0);
        fprintf(stderr, "Read clip from jdaw\n");
        read_clip_from_jdaw(f, clip);
        num_clips--;
    }
    fprintf(stderr, "DONE with track\n");
}

void read_clip_from_jdaw(FILE *f, Clip *clip) 
{
    char hdr_buffer[5];
    fread(hdr_buffer, 1, 4, f);
    hdr_buffer[4] = '\0';
    if (strcmp(hdr_buffer, "CLIP") != 0) {
        fprintf(stderr, "Error: unable to read file. 'CLIP' specifier not found where expected.\n");
        // destroy_clip(clip);
        return;
    }
    fread(clip->name, 1, 255, f);
    fprintf(stderr, "Clip named: '%s'\n", clip->name);

    if (sys_byteorder_le) {
        fread(&(clip->absolute_position), 4, 1, f);
        fread(&(clip->length), 4, 1, f);
    } else {
        //TODO: handle big endian
    }
    fread(hdr_buffer, 1, 4, f);
    if (strcmp(hdr_buffer, "data") != 0) {
        fprintf(stderr, "Error: unable to read file. 'data' specifier not found where expected.\n");
        // destroy_clip(clip);
        return;
    }
    clip->samples = malloc(sizeof(int16_t) * clip->length);
    if (!(clip->samples)) {
        fprintf(stderr, "Error allocating space for clip->samples\n");
        exit(1);
    }
    if (sys_byteorder_le) {
        // size_t t= fread(NULL, 2, clip->length, f);
        size_t t = fread(clip->samples, 2, clip->length, f);
    } else {
        //TODO: handle big endian
    }
    clip->done = true;
    reset_cliprect(clip);
}

