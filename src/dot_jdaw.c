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
#include "dsp.h"
#include "theme.h"

extern Project *proj;
extern bool sys_byteorder_le;
extern JDAW_Color black;
extern JDAW_Color white;

/**************************** .JDAW FILE SPEC ***********************************

===========================================================================================================
SCTN          LEN IN BYTES      TYPE                      BYTE ORDER   FIELD NAME OR VALUE
===========================================================================================================
HDR           4                 char[4]                                "JDAW"
HDR           1                 uint8_t                                project name length
HDR           0-255             char[]                                 project name
HDR           1                 uint8_t                                channels
HDR           4                 uint32_t                  LE           sample rate
HDR           4                 uint16_t                  LE           chunk size (power of 2)
HDR           2                 SDL_AudioFormat (16bit)                SDL Audio Format
HDR           1                 uint8_t                                num tracks
TRK_HDR       4                 char[4]                                "TRCK"
TRK_HDR       1                 uint8_t                                track name length
TRK_HDR       0-255             char[]                                 track name
TRK_HDR       1                 uint8_t                                num_clips
CLP_HDR       4                 char[4]                                "CLIP"
CLP_HDR       1                 uint8_t                                clip name length
CLP_HDR       0-255             char[]                                 clip name
CLP_HDR       4                 int32_t                   LE           absolute position (in timeline)
CLP_HDR       4                 uint32_t                  LE           length (samples)
CLP_HDR       4                 char[4]                                "data"
CLP_DATA      0-?               [int16_t arr]             [LE]         CLIP SAMPLE DATA     

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
    uint8_t namelength = strlen(proj->name);
    fwrite(&namelength, 1, 1, f);
    fwrite(proj->name, 1, namelength + 1, f);
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
    uint8_t trck_namelength = strlen(track->name);
    fwrite(&trck_namelength, 1, 1, f);
    fwrite(track->name, 1, trck_namelength + 1, f);
    fwrite(&(track->num_clips), 1, 1, f);
    for (uint8_t i=0; i<track->num_clips; i++) {
        write_clip_to_jdaw(f, track->clips[i]);
    }
}

void write_clip_to_jdaw(FILE *f, Clip *clip) 
{
    fwrite(hdr_clp, 1, 4, f);
    uint8_t clip_namelength = strlen(clip->name);
    fwrite(&clip_namelength, 1, 1, f);
    fwrite(clip->name, 1, clip_namelength + 1, f);
    if (sys_byteorder_le) {
        fwrite(&(clip->abs_pos_sframes), 4, 1, f);
        fwrite(&(clip->len_sframes), 4, 1, f);
    } else {
        //TODO: handle big endian
    }
    fwrite(hdr_data, 1, 4, f);
    fwrite(clip->pre_proc, 2, clip->len_sframes * clip->channels, f);
}

Project *open_jdaw_file(const char *path)
{
    proj = create_empty_project();
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: could not find project file at path %s\n", path);
        return NULL;
    }
    char hdr_buffer[5];
    fread(hdr_buffer, 1, 4, f);
    hdr_buffer[4] = '\0';
    if (strcmp(hdr_buffer, "JDAW") != 0) {
        fprintf(stderr, "Error: unable to read file. 'JDAW' specifier missing %s\n", path);
        free(proj);
        return NULL;
    }
    uint8_t proj_namelength = 0;
    fread(&proj_namelength, 1, 1, f);
    fread(proj->name, 1, proj_namelength + 1, f);
    fread(&(proj->channels), 1, 1, f);
    if (sys_byteorder_le) {
        fread(&(proj->sample_rate), 4, 1, f);
        fread(&(proj->chunk_size), 2, 1, f);
    } else {
        //TODO: handle big endian
    }
    fread(&(proj->fmt), 2, 1, f);
    uint8_t num_tracks = 0;
    fread(&num_tracks, 1, 1, f);

    char project_window_name[MAX_NAMELENGTH + 10];
    sprintf(project_window_name, "Jackdaw | %s", proj->name);

    proj->jwin = create_jwin(project_window_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED-20, 900, 650);
        // tl->console_width = TRACK_CONSOLE_WIDTH;
    // proj->tl->rect = get_rect((SDL_Rect){0, 0, proj->jwin->w, proj->jwin->h,}, TL_RECT);
    // proj->tl->ruler_rect = get_rect(proj->tl->rect, RULER_RECT);
    // proj->tl->tc_rect = get_rect(proj->tl->rect, TC_RECT);
    // int audio_rect_x = proj->tl->rect.x + TRACK_CONSOLE_W + COLOR_BAR_W + PADDING;
    // proj->tl->audio_rect = (SDL_Rect) {audio_rect_x, proj->tl->rect.y, proj->jwin->w - audio_rect_x, proj->tl->rect.h}; // SET x in track
    proj->tl->timecode_tb = create_textbox(0, proj->tl->tc_rect.h, 0, proj->jwin->mono_fonts[3], "00:00:00:00000", &white, NULL, &black, NULL, NULL, NULL, NULL, NULL, true);
    reset_tl_rects(proj);
    // position_textbox(proj->tl->timecode_tb, proj->tl->tc_rect.x, proj->tl->tc_rect.y);
    //TODO: get this outta here
    // proj->tl->console_width = TRACK_CONSOLE_WIDTH;
    activate_audio_devices(proj);


    while (num_tracks > 0) {
        Track *track = create_track(proj->tl, true);
        read_track_from_jdaw(f, track);
        num_tracks--;
    }
    reset_tl_rects(proj);
    fprintf(stderr, "DONE with file read.\n");
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
    uint8_t trck_namelength = 0;
    fread(&trck_namelength, 1, 1, f);
    fread(track->name, 1, trck_namelength + 1, f);
    reset_textbox_value(track->name_box, track->name);
    uint8_t num_clips;
    fread(&num_clips, 1, 1, f);
    while (num_clips > 0) {
        Clip *clip = create_clip(track, 0, 0);
        read_clip_from_jdaw(f, clip);
        num_clips--;
    }
    process_track_vol_and_pan(track);

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
    uint8_t clip_namelength = 0;
    fread(&clip_namelength, 1, 1, f);
    fread(clip->name, 1, clip_namelength + 1, f);
    fprintf(stderr, "Clip named: '%s'\n", clip->name);
    clip->channels = 2; //TODO: make sure this is written in


    if (sys_byteorder_le) {
        fread(&(clip->abs_pos_sframes), 4, 1, f);
        fread(&(clip->len_sframes), 4, 1, f);
    } else {
        //TODO: handle big endian
    }
    fread(hdr_buffer, 1, 4, f);
    if (strcmp(hdr_buffer, "data") != 0) {
        fprintf(stderr, "Error: unable to read file. 'data' specifier not found where expected.\n");
        // destroy_clip(clip);
        return;
    }
    clip->pre_proc = malloc(sizeof(int16_t) * clip->len_sframes * clip->channels);
    clip->post_proc = malloc(sizeof(int16_t) * clip->len_sframes * clip->channels);
    if (!(clip->pre_proc)) {
        fprintf(stderr, "Error allocating space for clip->samples\n");
        exit(1);
    }
    if (sys_byteorder_le) {
        // size_t t= fread(NULL, 2, clip->length, f);
        fread(clip->pre_proc, 2, clip->len_sframes * clip->channels, f);
    } else {
        //TODO: handle big endian
    }
    clip->done = true;
    reset_cliprect(clip);
}

