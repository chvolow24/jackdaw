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
#include <stdlib.h>
#include "project.h"
#include "gui.h"
#include "audio.h"
#include "dsp.h"
#include "theme.h"

extern Project *proj;
extern bool sys_byteorder_le;
extern JDAW_Color black;
extern JDAW_Color white;

/**************************** .JDAW VERSION 00.00 FILE SPEC ***********************************

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


/**************************** .JDAW VERSION 00.01 FILE SPEC ***********************************

===========================================================================================================
SCTN          LEN IN BYTES      TYPE                      BYTE ORDER   FIELD NAME OR VALUE
===========================================================================================================
HDR           4                 char[4]                                "JDAW"
HDR           8                 char[8]                                " VERSION"
HDR           5                 char[5]                                file spec version (e.g. "00.01")
HDR           1                 uint8_t                                project name length
HDR           0-255             char[]                                 project name
HDR           1                 uint8_t                                channels
HDR           4                 uint32_t                  LE           sample rate
HDR           2                 uint16_t                  LE           chunk size (power of 2)
HDR           2                 SDL_AudioFormat (16bit)                SDL Audio Format
HDR           1                 uint8_t                                num tracks
TRK_HDR       4                 char[4]                                "TRCK"
TRK_HDR       1                 uint8_t                                track name length
TRK_HDR       0-255             char[]                                 track name
TRK_HDR       10                char[16] (from float)                  vol ctrl value
TRK_HDR       10                char[16] (from float)                  pan ctrl value
TRK_HDR       1                 bool                                   muted
TRK_HDR       1                 bool                                   soloed
TRK_HDR       1                 uint8_t                                num_clips
CLP_HDR       4                 char[4]                                "CLIP"
CLP_HDR       1                 uint8_t                                clip name length
CLP_HDR       0-255             char[]                                 clip name
CLP_HDR       4                 int32_t                   LE           absolute position (in timeline)
CLP_HDR       4                 uint32_t                  LE           length (samples)
CLP_HDR       4                 char[4]                                "data"
CLP_DATA      0-?               [int16_t arr]             [LE]         CLIP SAMPLE DATA     

*********************************************************************************/

/**************************** .JDAW VERSION 00.02 FILE SPEC ***********************************

===========================================================================================================
SCTN          LEN IN BYTES      TYPE                      BYTE ORDER   FIELD NAME OR VALUE
===========================================================================================================
HDR           4                 char[4]                                "JDAW"
HDR           8                 char[8]                                " VERSION"
HDR           5                 char[5]                                file spec version (e.g. "00.01")
HDR           1                 uint8_t                                project name length
HDR           0-255             char[]                                 project name
HDR           1                 uint8_t                                channels
HDR           4                 uint32_t                  LE           sample rate
HDR           2                 uint16_t                  LE           chunk size (power of 2)
HDR           2                 SDL_AudioFormat (16bit)                SDL Audio Format
HDR           1                 uint8_t                                num tracks
TRK_HDR       4                 char[4]                                "TRCK"
TRK_HDR       1                 uint8_t                                track name length
TRK_HDR       0-255             char[]                                 track name
TRK_HDR       10                char[16] (from float)                  vol ctrl value
TRK_HDR       10                char[16] (from float)                  pan ctrl value
TRK_HDR       1                 bool                                   muted
TRK_HDR       1                 bool                                   soloed
TRK_HDR       1                 bool                                   solo muted
TRK_HDR       1                 uint8_t                                num_clips
CLP_HDR       4                 char[4]                                "CLIP"
CLP_HDR       1                 uint8_t                                clip name length
CLP_HDR       0-255             char[]                                 clip name
CLP_HDR       4                 int32_t                   LE           absolute position (in timeline)
CLP_HDR       4                 uint32_t                  LE           length (samples)
CLP_HDR       4                 char[4]                                "data"
CLP_DATA      0-?               [int16_t arr]             [LE]         CLIP SAMPLE DATA     

*********************************************************************************/

/**************************** .JDAW VERSION 00.03 FILE SPEC ***********************************

===========================================================================================================
SCTN          LEN IN BYTES      TYPE                      BYTE ORDER   FIELD NAME OR VALUE
===========================================================================================================
HDR           4                 char[4]                                "JDAW"
HDR           8                 char[8]                                " VERSION"
HDR           5                 char[5]                                file spec version (e.g. "00.01")
HDR           1                 uint8_t                                project name length
HDR           0-255             char[]                                 project name
HDR           1                 uint8_t                                channels
HDR           4                 uint32_t                  LE           sample rate
HDR           2                 uint16_t                  LE           chunk size (power of 2)
HDR           2                 SDL_AudioFormat (16bit)                SDL Audio Format
HDR           1                 uint8_t                                num tracks
TRK_HDR       4                 char[4]                                "TRCK"
TRK_HDR       1                 uint8_t                                track name length
TRK_HDR       0-255             char[]                                 track name
TRK_HDR       10                char[16] (from float)                  vol ctrl value
TRK_HDR       10                char[16] (from float)                  pan ctrl value
TRK_HDR       1                 bool                                   muted
TRK_HDR       1                 bool                                   soloed
TRK_HDR       1                 bool                                   solo muted
TRK_HDR       1                 uint8_t                                num_clips
CLP_HDR       4                 char[4]                                "CLIP"
CLP_HDR       1                 uint8_t                                clip name length
CLP_HDR       0-255             char[]                                 clip name
CLP_HDR       4                 int32_t                   LE           absolute position (in timeline)
CLP_HDR       4                 uint32_t                  LE           length (sframes)
CLP_HDR      4                 uint32_t                  LE           clip start ramp len (sframes)
CLP_HDR      4                 uint32_t                  LE           clip end ramp len (sframes)
CLP_HDR       4                 char[4]                                "data"
CLP_DATA      0-?               [int16_t arr]             [LE]         CLIP SAMPLE DATA     

*********************************************************************************/

const static char hdr_jdaw[] = {'J', 'D', 'A', 'W'};
const static char hdr_version[] = {' ', 'V', 'E', 'R', 'S', 'I', 'O', 'N'};
const static char hdr_trk[] = {'T', 'R', 'C', 'K'};
const static char hdr_clp[] = {'C', 'L', 'I', 'P'};
const static char hdr_data[] = {'d', 'a', 't', 'a'};

const static char current_file_spec_version[] = {'0', '0', '.', '0', '3'};

static void write_clip_to_jdaw(FILE *f, Clip *clip);
static void write_track_to_jdaw(FILE *f, Track *track);
static void read_clip_from_jdaw(FILE *f, float file_spec_version, Clip *track);
static void read_track_from_jdaw(FILE *f, float file_spec_version, Track *track);

extern uint8_t scale_factor;

void write_jdaw_file(const char *path) 
{
    if (!proj) {
        fprintf(stderr, "No project to save. Exiting.\n");
        return;
    }
    FILE* f = fopen(path, "wb");

    fwrite(hdr_jdaw, 1, 4, f);
    fwrite(hdr_version, 1, 8, f);
    fwrite(current_file_spec_version, 1, 5, f);
    uint8_t namelength = strlen(proj->name);
    fwrite(&namelength, 1, 1, f);
    fwrite(proj->name, 1, namelength, f);
    char null = '\0';
    fwrite(&null, 1, 1, f);
    fwrite(&(proj->channels), 1, 1, f);
    if (sys_byteorder_le) {
        fwrite(&(proj->sample_rate), 4, 1, f);
        fwrite(&(proj->chunk_size), 2, 1, f);
    } else {
        //TODO: handle big endian
        exit(1);
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

    /* Write vol and pan values */
    char float_value_buffer[16];
    snprintf(float_value_buffer, 16, "%f", track->vol_ctrl->value);
    fwrite(float_value_buffer, 1, 16, f);
    snprintf(float_value_buffer, 16, "%f", track->pan_ctrl->value);
    fwrite(float_value_buffer, 1, 16, f);

    /* Write mute and solo values */
    fwrite(&(track->muted), 1, 1, f);
    fwrite(&(track->solo), 1, 1, f);
    fwrite(&(track->solo_muted), 1, 1, f);


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
        fwrite(&(clip->start_ramp_len), 4, 1, f);
        fwrite(&(clip->end_ramp_len), 4, 1, f);
    } else {
        //TODO: handle big endian
        exit(1);
    }
    fwrite(hdr_data, 1, 4, f);
    fwrite(clip->pre_proc, 2, clip->len_sframes * clip->channels, f);
}

Project *open_jdaw_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: could not find project file at path %s\n", path);
        return NULL;
    }
    char hdr_buffer[9];
    fread(hdr_buffer, 1, 4, f);
    hdr_buffer[4] = '\0';
    if (strcmp(hdr_buffer, "JDAW") != 0) {
        fprintf(stderr, "Error: unable to read file. 'JDAW' specifier missing %s\n", path);
        free(proj);
        return NULL;
    }

    /* Check whether " VERSION" specifier exists to get the file spec version. If not, reset buffer position */
    fpos_t version_split_pos;
    fgetpos(f, &version_split_pos);
    fread(hdr_buffer, 1, 8, f);
    hdr_buffer[8] = '\0';
    float file_spec_version = 0.0;
    if (strcmp(hdr_buffer, " VERSION") == 0) {
        fread(hdr_buffer, 1, 5, f);
        hdr_buffer[5] = '\0';
        file_spec_version = atof(hdr_buffer);
    } else {
        fsetpos(f, &version_split_pos);
    }

    fprintf(stderr, "JDAW FILE SPEC VERSION: %f\n", file_spec_version);

    uint8_t proj_namelength;
    char project_name[MAX_NAMELENGTH];
    uint8_t channels;
    int sample_rate;
    SDL_AudioFormat fmt;
    uint16_t chunk_size;

    fread(&proj_namelength, 1, 1, f);
    fread((void *)project_name, 1, proj_namelength + 1, f);

    // The file should contain a null terminator, but do this to be sure.
    project_name[proj_namelength] = '\0';

    fread(&(channels), 1, 1, f);
    if (sys_byteorder_le) {
        fread(&(sample_rate), 4, 1, f);
        fread(&(chunk_size), 2, 1, f);
    } else {
        //TODO: handle big endian
        exit(1);
    }
    fread(&(fmt), 2, 1, f);

    proj = create_project(project_name, channels, sample_rate, fmt, chunk_size);
    uint8_t num_tracks = 0;
    fread(&num_tracks, 1, 1, f);

    char project_window_name[MAX_NAMELENGTH + 10];
    sprintf(project_window_name, "Jackdaw | %s", proj->name);
    reset_tl_rects(proj);
    activate_audio_devices(proj);

    fprintf(stderr, "Sample rate: %d, num_tracks: %d, fmt: %s, chunk size: %d\n", sample_rate, num_tracks, get_audio_fmt_str(fmt), chunk_size);
    while (num_tracks > 0) {
        Track *track = create_track(proj->tl, true);
        read_track_from_jdaw(f, file_spec_version, track);
        num_tracks--;
    }
    reset_tl_rects(proj);
    fprintf(stderr, "DONE with file read.\n");
    fclose(f);
    return proj;
}



// TRK_HDR       4                 char[4]                                "TRCK"
// TRK_HDR       1                 uint8_t                                track name length
// TRK_HDR       0-255             char[]                                 track name
// TRK_HDR       10                char[16] (from float)                  vol ctrl value
// TRK_HDR       10                char[16] (from float)                  pan ctrl value
// TRK_HDR       1                 bool                                   muted
static void read_track_from_jdaw(FILE *f, float file_spec_version, Track *track)
{
    char hdr_buffer[5];
    fread(hdr_buffer, 1, 4, f);
    hdr_buffer[4] = '\0';
    if (strcmp(hdr_buffer, "TRCK") != 0) {
        fprintf(stderr, "Error: unable to read file. 'TRCK' specifier not found where expected. Found '%s'\n", hdr_buffer);
        // free(proj);
        // destroy_track(track);
        return;
    }
    uint8_t trck_namelength = 0;
    fread(&trck_namelength, 1, 1, f);
    fread(track->name, 1, trck_namelength + 1, f);
    reset_textbox_value(track->name_box, track->name);


    if (file_spec_version > 0) {
        char float_value_buffer[16];
        fread(float_value_buffer, 1, 16, f);
        track->vol_ctrl->value = atof(float_value_buffer);
        fread(float_value_buffer, 1, 16, f);
        track->pan_ctrl->value = atof(float_value_buffer);
        reset_fslider(track->vol_ctrl);
        reset_fslider(track->pan_ctrl);
        bool muted = false;
        bool solo = false;
        fread(&muted, 1, 1, f);
        fread(&solo, 1, 1, f);
        if (muted) {
            mute_track(track);
        }
        if (solo) {
            solo_track(track);
        }
        if (file_spec_version > 0.01) {
            bool solo_muted = false;
            fread(&solo_muted, 1, 1, f);
            if (solo_muted) {
                solo_mute_track(track);
            }
        }
    }


    uint8_t num_clips;
    fread(&num_clips, 1, 1, f);
    while (num_clips > 0) {
        Clip *clip = create_clip(track, 0, 0);
        read_clip_from_jdaw(f, file_spec_version, clip);
        num_clips--;
    }
    process_track_vol_and_pan(track);

}

static void read_clip_from_jdaw(FILE *f, float file_spec_version, Clip *clip) 
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
        exit(1);
    }

    if (file_spec_version > 0.02) {
        if (sys_byteorder_le) {
            fread(&(clip->start_ramp_len), 4, 1, f);
            fread(&(clip->end_ramp_len), 4, 1, f);
        } else {
            //TODO: handle big endian
            exit(1);
        }
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
        exit(1);
    }
    clip->done = true;
    reset_cliprect(clip);
}

