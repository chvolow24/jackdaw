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
#include "transport.h"

#define BYTEORDER_FATAL fprintf(stderr, "Fatal error (TODO): big-endian byte order not supported\n"); exit(1);
#define STD_FLOAT_SER_W 16

extern Project *proj;
extern bool SYS_BYTEORDER_LE;
extern const char *JACKDAW_VERSION;
/* extern JDAW_Color black; */
/* extern JDAW_Color white; */

const static char hdr_jdaw[] = {'J','D','A','W'};
const static char hdr_version[] = {' ','V','E','R','S','I','O','N'};
const static char hdr_clip[] = {'C','L','I','P'};
const static char hdr_timeline[] = {'T','I','M','E','L','I','N','E'};
const static char hdr_track[] = {'T','R','C','K'};
const static char hdr_clipref[] = {'C','L','I','P','R','E','F'};
const static char hdr_data[] = {'d','a','t','a'};
const static char hdr_auto[] = {'A', 'U', 'T', 'O'};
const static char hdr_keyf[] = {'K', 'E', 'Y', 'F'};

const static char current_file_spec_version[] = {'0','0','.','1','3'};
/* const static float current_file_spec_version_f = 0.11f; */

const static char nullterm = '\0';

float read_file_spec_version;
/* static void write_clip_to_jdaw(FILE *f, Clip *clip); */
/* static void write_track_to_jdaw(FILE *f, Track *track); */
/* static void read_clip_from_jdaw(FILE *f, float file_spec_version, Clip *track); */
/* static void read_track_from_jdaw(FILE *f, float file_spec_version, Track *track); */

/* extern uint8_t scale_factor; */



/**********************************************/
/******************** WRITE *******************/
/**********************************************/



static void jdaw_write_clip(FILE *f, Clip *clip, int index);
static void jdaw_write_timeline(FILE *f, Timeline *tl);

void jdaw_write_project(const char *path) 
{
    if (!proj) {
        fprintf(stderr, "No project to save. Exiting.\n");
        return;
    }
    FILE* f = fopen(path, "wb");

    if (!f) {
	fprintf(stderr, "Error: unable to write to file at path %s: file could not be found\n", path);
    }
    fwrite(hdr_jdaw, 1, 4, f);
    fwrite(hdr_version, 1, 8, f);
    fwrite(current_file_spec_version, 1, 5, f);
    uint8_t namelen = strlen(proj->name);
    fwrite(&namelen, 1, 1, f);
    fwrite(proj->name, 1, namelen, f);
    fwrite(&nullterm, 1, 1, f);
    fwrite(&proj->channels, 1, 1, f);
    if (SYS_BYTEORDER_LE) {
        fwrite(&proj->sample_rate, 4, 1, f);
        fwrite(&proj->chunk_size_sframes, 2, 1, f);
    } else {
        BYTEORDER_FATAL
    }
    fwrite(&proj->fmt, 2, 1, f);
    fwrite(&proj->num_clips, 1, 1, f);
    fwrite(&proj->num_timelines, 1, 1, f);
    for (uint8_t i=0; i<proj->num_clips; i++) {
	jdaw_write_clip(f, proj->clips[i], i);
    }
    for (uint8_t i=0; i<proj->num_timelines; i++) {
	jdaw_write_timeline(f, proj->timelines[i]);
    }
    /* fwrite(&(proj->tl->num_tracks), 1, 1, f); */
    /* for (uint8_t i=0; i<proj->tl->num_tracks; i++) { */
        /* write_track_to_jdaw(f, proj->tl->tracks[i]); */
    /* } */
    fclose(f);
}

static void jdaw_write_clip(FILE *f, Clip *clip, int index)
{
    fwrite(hdr_clip, 1, 4, f);
    uint8_t namelen = strlen(clip->name);
    fwrite(&namelen, 1, 1, f);
    fwrite(&clip->name, 1, namelen, f);
    fwrite(&nullterm, 1, 1, f);
    fwrite(&index, 1, 1, f);
    fwrite(&clip->channels, 1, 1, f);
    if (SYS_BYTEORDER_LE) {
	fwrite(&clip->len_sframes, 4, 1, f);
    } else {
        BYTEORDER_FATAL
    }

    fwrite(hdr_data, 1, 4, f);

    /* Write clip sample data */
    uint32_t len_samples = clip->len_sframes * clip->channels;
    int16_t *clip_samples = malloc(sizeof(int16_t) * len_samples);
    if (!clip_samples) {
        fprintf(stderr, "Error: unable to allocate space for clip_samples\n");
        exit(1);
    }
    float clipped_sample;
    if (clip->channels == 2) {
        for (uint32_t i=0; i<len_samples; i+=2) {
	    clipped_sample = clip->L[i/2];
	    if (clipped_sample > 1.0f) clipped_sample = 1.0f;
	    else if (clipped_sample < -1.0f) clipped_sample = -1.0f;
            clip_samples[i] = clipped_sample * INT16_MAX;
	    clipped_sample = clip->R[i/2];
	    if (clipped_sample > 1.0f) clipped_sample = 1.0f;
	    else if (clipped_sample < -1.0f) clipped_sample = -1.0f;
            clip_samples[i+1] = clipped_sample * INT16_MAX;
        }
    } else if (clip->channels == 1) {

        for (uint32_t i=0; i<len_samples; i++) {
	    clipped_sample = clip->L[i];
	    if (clipped_sample > 1.0f) clipped_sample = 1.0f;
	    else if (clipped_sample < -1.0f) clipped_sample = -1.0f;
	    clip_samples[i] = (int16_t)(clip->L[i] * INT16_MAX);
        }
    }
    fwrite(clip_samples, 2, clip->len_sframes * clip->channels, f);
    free(clip_samples);
}



static void jdaw_write_track(FILE *f, Track *track);

static void jdaw_write_timeline(FILE *f, Timeline *tl)
{
    fwrite(hdr_timeline, 1, 8, f);
    uint8_t timeline_namelen = strlen(tl->name);
    fwrite(&timeline_namelen, 1, 1, f);
    fwrite(tl->name, 1, timeline_namelen, f);
    fwrite(&nullterm, 1, 1, f);
    fwrite(&tl->num_tracks, 1, 1, f);
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	jdaw_write_track(f, tl->tracks[i]);
    }
}

static void jdaw_write_clipref(FILE *f, ClipRef *cr);
static void jdaw_write_automation(FILE *f, Automation *a);

static void jdaw_write_track(FILE *f, Track *track)
{
    fwrite(hdr_track, 1, 4, f);
    uint8_t track_namelen = strlen(track->name);
    fwrite(&track_namelen, 1, 1, f);
    fwrite(track->name, 1, track_namelen, f);
    fwrite(&nullterm, 1, 1, f);
    fwrite(&track->color, 1, 4, f);

    /* vol and pan */
    char float_value_buffer[STD_FLOAT_SER_W];
    snprintf(float_value_buffer, STD_FLOAT_SER_W, "%f", track->vol);
    fwrite(float_value_buffer, 1, STD_FLOAT_SER_W, f);
    snprintf(float_value_buffer, STD_FLOAT_SER_W, "%f", track->pan);
    fwrite(float_value_buffer, 1, STD_FLOAT_SER_W, f);
    
    /* Write mute and solo values */
    fwrite(&track->muted, 1, 1, f);
    fwrite(&track->solo, 1, 1, f);

    fwrite(&track->solo_muted, 1, 1, f);
    fwrite(&track->num_clips, 1, 1, f);
    for (uint8_t i=0; i<track->num_clips; i++) {
	jdaw_write_clipref(f, track->clips[i]);
    }

    fwrite(&track->num_automations, 1, 1, f);
    for (uint8_t i=0; i<track->num_automations; i++) {
	jdaw_write_automation(f, track->automations[i]);
    }

    /* TRCK_FX */
    fwrite(&track->fir_filter_active, 1, 1, f);
    FIRFilter *filter;
    if (track->fir_filter_active && (filter = track->fir_filter)) {
	uint8_t type_byte = (uint8_t)filter->type;
	fwrite(&type_byte, 1, 1, f);
	snprintf(float_value_buffer, STD_FLOAT_SER_W, "%f", filter->cutoff_freq);
	fwrite(float_value_buffer, 1, STD_FLOAT_SER_W, f);
	snprintf(float_value_buffer, STD_FLOAT_SER_W, "%f", filter->bandwidth);
	fwrite(float_value_buffer, 1, STD_FLOAT_SER_W, f);
	fwrite(&filter->impulse_response_len, 2, 1, f);
    } else {
	fseek(f, 35, SEEK_CUR);
    }
    fwrite(&track->delay_line_active, 1, 1, f);
    if (track->delay_line_active) {
	fwrite(&track->delay_line.len, 4, 1, f);
	snprintf(float_value_buffer, STD_FLOAT_SER_W, "%f", track->delay_line.stereo_offset);
	fwrite(float_value_buffer, STD_FLOAT_SER_W, 1, f);
	snprintf(float_value_buffer, STD_FLOAT_SER_W, "%f", track->delay_line.amp);
	fwrite(float_value_buffer, 1, STD_FLOAT_SER_W, f);
    } else {
	fseek(f, 36, SEEK_CUR);
    }
    
}

static void jdaw_write_clipref(FILE *f, ClipRef *cr)
{
    fwrite(hdr_clipref, 1, 7, f);
    uint8_t clipref_namelen = strlen(cr->name);
    fwrite(&clipref_namelen, 1, 1, f);
    fwrite(cr->name, 1, clipref_namelen, f);
    fwrite(&cr->home, 1, 1, f);

    uint8_t src_clip_index = 0;
    while (proj->clips[src_clip_index] != cr->clip) {
	src_clip_index++;
    }
    fwrite(&src_clip_index, 1, 1, f);
    if (SYS_BYTEORDER_LE) {
	fwrite(&cr->pos_sframes, 4, 1, f);
	fwrite(&cr->in_mark_sframes, 4, 1, f);
	fwrite(&cr->out_mark_sframes, 4, 1, f);
	fwrite(&cr->start_ramp_len, 4, 1, f);
	fwrite(&cr->end_ramp_len, 4, 1, f);
    } else {
	BYTEORDER_FATAL
    }
}

static void jdaw_write_auto_keyframe(FILE *f, Keyframe *k);
static void jdaw_write_automation(FILE *f, Automation *a)
{
    fwrite(hdr_auto, 1, 4, f);
    uint8_t type_byte = (uint8_t)a->type;
    fwrite(&type_byte, 1, 1, f);
    type_byte = (uint8_t)a->val_type;
    fwrite(&type_byte, 1, 1, f);
    jdaw_val_serialize(a->min, a->val_type, f, STD_FLOAT_SER_W);
    jdaw_val_serialize(a->max, a->val_type, f, STD_FLOAT_SER_W);
    jdaw_val_serialize(a->range, a->val_type, f, STD_FLOAT_SER_W);
    fwrite(&a->read, 1, 1, f);
    fwrite(&a->shown, 1, 1, f);
    if (SYS_BYTEORDER_LE) {
	fwrite(&a->num_keyframes, 2, 1, f);
    } else {
	BYTEORDER_FATAL
    }
    for (uint16_t i=0; i<a->num_keyframes; i++) {
	jdaw_write_auto_keyframe(f, a->keyframes + i);
    }
    
}
static void jdaw_write_auto_keyframe(FILE *f, Keyframe *k)
{
    fwrite(hdr_keyf, 1, 4, f);
    if (SYS_BYTEORDER_LE) {
	fwrite(&k->pos, 4, 1, f);
    } else {
	BYTEORDER_FATAL
    }
    jdaw_val_serialize(k->value, k->automation->val_type, f, STD_FLOAT_SER_W);
}




/**********************************************/
/******************** READ ********************/
/**********************************************/



static void jdaw_read_clip(FILE *f, Project *proj);
static void jdaw_read_timeline(FILE *f, Project *proj);
Project *jdaw_read_file(const char *path)
{
    
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: could not find project file at path %s\n", path);
        return NULL;
    }
    char hdr_buffer[9];
    fread(hdr_buffer, 1, 4, f);
    hdr_buffer[4] = '\0';
    if (strncmp(hdr_buffer, hdr_jdaw, 4) != 0) {
        fprintf(stderr, "Error: unable to read file. 'JDAW' specifier missing %s\n", path);
        /* free(proj); */
        return NULL;
    }

    /* Check whether " VERSION" specifier exists to get the file spec version. If not, reset buffer position */
    fpos_t version_split_pos;
    fgetpos(f, &version_split_pos);
    fread(hdr_buffer, 1, 8, f);
    hdr_buffer[8] = '\0';
    if (strncmp(hdr_buffer, hdr_version, 8) != 0) {
	fprintf(stderr, "Error: \"VERSION\" specifier missing in .jdaw file\n");
	return NULL;
    }
    fread(hdr_buffer, 1, 5, f);
    hdr_buffer[5] = '\0';
    read_file_spec_version = atof(hdr_buffer);
    fprintf(stderr, "Reading JDAW file with version %2.2f\n", read_file_spec_version);
    if (read_file_spec_version < 00.10f) {
	fprintf(stderr, "Error: .jdaw file version %s is not compatible with the current jackdaw version (%s). You may need to downgrade to open this file.\n", hdr_buffer, JACKDAW_VERSION);
        /* free(proj); */
	return NULL;
    }

    /* fprintf(stdout, "JDAW FILE SPEC VERSION: %f\n", file_spec_version); */

    uint8_t proj_namelen;
    char project_name[MAX_NAMELENGTH];
    uint8_t channels;
    uint32_t sample_rate;
    SDL_AudioFormat fmt;
    uint16_t chunk_size;
    uint8_t num_clips;
    uint8_t num_timelines;

    fread(&proj_namelen, 1, 1, f);
    fread(project_name, 1, proj_namelen + 1, f);

    /* The file should contain a null terminator, but do this to be sure. */
    /* project_name[proj_namelen] = '\0'; */

    fread(&channels, 1, 1, f);
    if (SYS_BYTEORDER_LE) {
        fread(&sample_rate, 4, 1, f);
        fread(&chunk_size, 2, 1, f);
	/* fprintf(stdout, "CHUNK SIZE: %d\n", chunk_size); */
    } else {
	BYTEORDER_FATAL
    }
    fread(&fmt, 2, 1, f);
    fread(&num_clips, 1, 1, f);
    fread(&num_timelines, 1, 1, f);

    Project *proj_loc = project_create(
	project_name,
	channels,
	sample_rate,
	fmt,
	chunk_size,
	1024 // TODO: variable fourier len
	);

    
    proj_loc->num_timelines = 0;

    while (num_clips > 0) {
	jdaw_read_clip(f, proj_loc);
	num_clips--;
    }

    while (num_timelines > 0) {
	jdaw_read_timeline(f, proj_loc);
	num_timelines--;
    }

    /* timeline_reset(proj->timelines[0]); */

    
    fclose(f);
    return proj_loc;
}

static void jdaw_read_clip(FILE *f, Project *proj)
{
    char hdr_buffer[5];
    fread(hdr_buffer, 1, 4, f);
    if (strncmp(hdr_buffer, hdr_clip, 4) != 0) {
	fprintf(stderr, "Error: .jdaw parsing error: CLIP indicator missing.\n");
	return;
    }

    Clip *clip = calloc(1, sizeof(Clip));
    proj->clips[proj->num_clips] = clip;
    proj->num_clips++;
    proj->active_clip_index++;
    
    uint8_t name_length;
    
    fread(&name_length, 1, 1, f);
    fread(clip->name, 1, name_length + 1, f);
    clip->name[name_length] = '\0';

    uint8_t clip_index;
    fread(&clip_index, 1, 1, f);
    
    fread(&clip->channels, 1, 1, f);
    if (SYS_BYTEORDER_LE) {
	fread(&clip->len_sframes, 4, 1, f);
    } else {
	BYTEORDER_FATAL
    }
    fread(hdr_buffer, 1, 4, f);
    if (strncmp(hdr_buffer, hdr_data, 4) != 0) {
	fprintf(stderr, "Error: .jdaw parsing error: clip 'data' indicator missing.\n");
	return;
    }

    /* Read clip data */

    create_clip_buffers(clip, clip->len_sframes);
    uint32_t clip_len_samples = clip->len_sframes * clip->channels;

    int16_t *interleaved_clip_samples = malloc(sizeof(int16_t) * clip_len_samples);
    fread(interleaved_clip_samples, 2, clip_len_samples, f);
    /* fprintf(stderr, "Read into interleaved samples array\n"); */

    if (clip->channels == 2) {
        for (uint32_t i=0; i<clip_len_samples; i+=2) {
            clip->L[i/2] = (float)interleaved_clip_samples[i] / INT16_MAX;
            clip->R[i/2] = (float)interleaved_clip_samples[i+1] / INT16_MAX;
        }
    } else {
        for (uint32_t i=0; i<clip_len_samples; i++) {
            clip->L[i] = (float)interleaved_clip_samples[i] / INT16_MAX;
        }
    }
    free(interleaved_clip_samples);
    /* fprintf(stderr, "Finished creating samples array\n"); */

}


static void jdaw_read_track(FILE *f, Timeline *tl);
static void jdaw_read_timeline(FILE *f, Project *proj)
{
    char hdr_buffer[8];
    fread(hdr_buffer, 1, 8, f);
    if (strncmp(hdr_buffer, hdr_timeline, 8) != 0) {
	fprintf(stderr, "Error: .jdaw parsing error: \"TIMELINE\" indicator not found\n");
	return;
    }
    uint8_t tl_namelen;
    fread(&tl_namelen, 1, 1, f);
    /* Timeline *tl = calloc(sizeof(Timeline), 1); */
    /* proj->timelines[proj->num_timelines] = tl; */
    /* proj->num_timelines++; */
    char tl_name[MAX_NAMELENGTH + 1];
    fread(tl_name, 1, tl_namelen + 1, f);
    tl_name[tl_namelen] = '\0';
    uint8_t index = project_add_timeline(proj, tl_name);
    Timeline *tl = proj->timelines[index];
    uint8_t num_tracks;
    fread(&num_tracks, 1, 1, f);
    while (num_tracks > 0) {
	jdaw_read_track(f, tl);
	num_tracks--;
    }
}

static void jdaw_read_clipref(FILE *f, Track *track);
static void jdaw_read_automation(FILE *f, Track *track);
static void jdaw_read_track(FILE *f, Timeline *tl)
{
    char hdr_buffer[4];
    fread(hdr_buffer, 1, 4, f);
    if (strncmp(hdr_buffer, hdr_track, 4) != 0) {
	fprintf(stderr, "Error: .jdaw parsing error: \"TRCK\" indicator not found\n");
    }
    
    /* Track *track = calloc(sizeof(Track), 1); */
    /* tl->tracks[tl->num_tracks] = track; */
    /* tl->num_tracks++; */

    timeline_add_track(tl);
    Track *track = tl->tracks[tl->num_tracks - 1];

    uint8_t track_namelen;
    fread(&track_namelen, 1, 1, f);
    fread(track->name, 1, track_namelen + 1, f);
    track->name[track_namelen] = '\0';

    fread(&track->color, 1, 4, f);

    char floatvals[17];
    fread(floatvals, 1, STD_FLOAT_SER_W, f);
    floatvals[STD_FLOAT_SER_W] = '\0';
    track->vol = atof(floatvals);
    fread(floatvals, 1, STD_FLOAT_SER_W, f);
    track->pan = atof(floatvals);

    bool muted, solo, solo_muted;
    
    fread(&muted, 1, 1, f);
    fread(&solo, 1, 1, f);
    fread(&solo_muted, 1, 1, f);
    if (muted) {
	track_mute(track);
    }
    if (solo) {
	track_solo(track);
    }
    if (solo_muted) {
	track_solomute(track);
    }

    uint8_t num_cliprefs;
    fread(&num_cliprefs, 1, 1, f);

    while (num_cliprefs > 0) {
	jdaw_read_clipref(f, track);
	num_cliprefs--;
    }
    if (read_file_spec_version >= 00.13f) {
	uint8_t num_automations;
	fread(&num_automations, 1, 1, f);
	while (num_automations > 0) {
	    jdaw_read_automation(f, track);
	    num_automations--;
	}
    }
    if (read_file_spec_version >= 00.11f) {
	fread(&track->fir_filter_active, 1, 1, f);
	if (track->fir_filter_active) {
/* filter_create(FilterType type, uint16_t impulse_response_len, uint16_t frequency_response_len) -> FIRFilter * */
	    uint8_t type_byte;
	    FilterType type;
	    double cutoff_freq;
	    double bandwidth;
	    uint16_t impulse_response_len;
	    fread(&type_byte, 1, 1, f);
	    type = (FilterType)type_byte;
	    fread(floatvals, 1, STD_FLOAT_SER_W, f);
	    floatvals[STD_FLOAT_SER_W] = '\0';
	    cutoff_freq = atof(floatvals);
	    fread(floatvals, 1, STD_FLOAT_SER_W, f);
	    bandwidth = atof(floatvals);
	    fread(&impulse_response_len, 2, 1, f);
	    if (!track->fir_filter) track->fir_filter = filter_create(
		type,
		impulse_response_len,
		tl->proj->fourier_len_sframes * 2);
	    filter_set_params(track->fir_filter, type, cutoff_freq, bandwidth);
	} else {
	    fseek(f, 35, SEEK_CUR);
	}
	fread(&track->delay_line_active, 1, 1, f);
	if (track->delay_line_active) {
	    int32_t len;
	    double amp;
	    double stereo_offset;
	    fread(&len, 4, 1, f);
	    fread(&floatvals, 1, STD_FLOAT_SER_W, f);
	    floatvals[STD_FLOAT_SER_W] = '\0';
	    stereo_offset = atof(floatvals);
	    fread(floatvals, 1, STD_FLOAT_SER_W, f);
	    floatvals[STD_FLOAT_SER_W] = '\0';
	    amp = atof(floatvals);
	    delay_line_init(&track->delay_line);
	    delay_line_set_params(&track->delay_line, amp, len);
	    track->delay_line.stereo_offset = stereo_offset;
	} else {
	    fseek(f, 36, SEEK_CUR);
	}
    }
}

static void jdaw_read_clipref(FILE *f, Track *track)
{

    char hdr_buffer[7];
    fread(hdr_buffer, 1, 7, f);

    if (strncmp(hdr_buffer, hdr_clipref, 7) != 0) {
	fprintf(stderr, "Error: .jdaw parsing error: \"CLIPREF\" indicator missing\n");
	exit(1);
    }

    /* ClipRef *cr = calloc(sizeof(ClipRef), 1); */
    /* track->clips[track->num_clips] = cr; */
    /* track->num_clips++; */

    uint8_t clipref_namelen;
    fread(&clipref_namelen, 1, 1, f);
    char clipref_name[MAX_NAMELENGTH];
    fread(clipref_name, 1, clipref_namelen, f);
    clipref_name[clipref_namelen] = '\0';
    bool clipref_home;
    fread(&clipref_home, 1, 1, f);
    uint8_t src_clip_index;
    fread(&src_clip_index, 1, 1, f);
    Clip *clip = track->tl->proj->clips[src_clip_index];
    ClipRef *cr = track_create_clip_ref(track, clip, 0, clipref_home);
    if (SYS_BYTEORDER_LE) {
	fread(&cr->pos_sframes, 4, 1, f);
	fread(&cr->in_mark_sframes, 4, 1, f);
	fread(&cr->out_mark_sframes, 4, 1, f);
	fread(&cr->start_ramp_len, 4, 1, f);
	fread(&cr->end_ramp_len, 4, 1, f);
    } else {
	BYTEORDER_FATAL
    }
    
}

static void jdaw_read_keyframe(FILE *f, Automation *a);

static void jdaw_read_automation(FILE *f, Track *track)
{

    char hdr_buffer[4];
    fread(hdr_buffer, 1, 4, f);

    if (strncmp(hdr_buffer, hdr_auto, 4) != 0) {
	fprintf(stderr, "Error: .jdaw parsing error: \"AUTO\" indicator missing\n");
	exit(1);
    }

    uint8_t type_byte;
    fread(&type_byte, 1, 1, f);
    AutomationType t = (AutomationType)type_byte;
    Automation *a = track_add_automation(track, t);
    fread(&a->val_type, 1, 1, f);
    
    a->min = jdaw_val_deserialize(f, STD_FLOAT_SER_W, a->val_type);
    a->max = jdaw_val_deserialize(f, STD_FLOAT_SER_W, a->val_type);
    a->range = jdaw_val_deserialize(f, STD_FLOAT_SER_W, a->val_type);
    bool read;
    fread(&read, 1, 1, f);
    if (!read) {
	automation_toggle_read(a);
    }
    /* TODO: figure out how to initialize this */
    fread(&a->shown, 1, 1, f);
    if (a->shown) {
	automation_show(a);
    }
    uint16_t num_keyframes;
    if (SYS_BYTEORDER_LE) {
	fread(&num_keyframes, 2, 1, f);
    } else {
	BYTEORDER_FATAL
    }

    a->num_keyframes = 0;
    while (num_keyframes > 0) {
	jdaw_read_keyframe(f, a);
	num_keyframes--;
    }
}


static void jdaw_read_keyframe(FILE *f, Automation *a)
{
    char hdr_buffer[4];
    fread(hdr_buffer, 1, 4, f);
    if (strncmp(hdr_buffer, hdr_keyf, 4) != 0) {
	fprintf(stderr, "Error: .jdaw parsing error: \"KEYF\" indicator missing\n");
	exit(1);
    }
    int32_t pos;
    Value val;
    if (SYS_BYTEORDER_LE) {
	fread(&pos, 4, 1, f);
    } else {
	BYTEORDER_FATAL
    }
    val = jdaw_val_deserialize(f, STD_FLOAT_SER_W, a->val_type);
    automation_insert_keyframe_at(a, pos, val);
}


/* static void read_track_from_jdaw(FILE *f, float file_spec_version, Track *track) */
/* { */
/*     char hdr_buffer[5]; */
/*     fread(hdr_buffer, 1, 4, f); */
/*     hdr_buffer[4] = '\0'; */
/*     if (strcmp(hdr_buffer, "TRCK") != 0) { */
/*         fprintf(stderr, "Error: unable to read file. 'TRCK' specifier not found where expected. Found '%s'\n", hdr_buffer); */
/*         // free(proj); */
/*         // destroy_track(track); */
/*         return; */
/*     } */
/*     uint8_t trck_namelen = 0; */
/*     fread(&trck_namelen, 1, 1, f); */
/*     fread(track->name, 1, trck_namelen + 1, f); */
/*     reset_textbox_value(track->name_box, track->name); */


/*     if (file_spec_version > 0) { */
/*         char float_value_buffer[16]; */
/*         fread(float_value_buffer, 1, 16, f); */
/*         track->vol_ctrl->value = atof(float_value_buffer); */
/*         fread(float_value_buffer, 1, 16, f); */
/*         track->pan_ctrl->value = atof(float_value_buffer); */
/*         reset_fslider(track->vol_ctrl); */
/*         reset_fslider(track->pan_ctrl); */
/*         bool muted = false; */
/*         bool solo = false; */
/*         fread(&muted, 1, 1, f); */
/*         fread(&solo, 1, 1, f); */
/*         if (muted) { */
/*             mute_track(track); */
/*         } */
/*         if (solo) { */
/*             solo_track(track); */
/*         } */
/*         if (file_spec_version > 0.01) { */
/*             bool solo_muted = false; */
/*             fread(&solo_muted, 1, 1, f); */
/*             if (solo_muted) { */
/*                 solo_mute_track(track); */
/*             } */
/*         } */
/*     } */


/*     uint8_t num_clips; */
/*     fread(&num_clips, 1, 1, f); */
/*     while (num_clips > 0) { */
/*         Clip *clip = create_clip(track, 0, 0); */
/*         read_clip_from_jdaw(f, file_spec_version, clip); */
/*         num_clips--; */
/*     } */
/*     // process_track_vol_and_pan(track); */

/* } */

/* static void read_clip_from_jdaw(FILE *f, float file_spec_version, Clip *clip)  */
/* { */
/*     char hdr_buffer[5]; */
/*     fread(hdr_buffer, 1, 4, f); */
/*     hdr_buffer[4] = '\0'; */
/*     if (strcmp(hdr_buffer, "CLIP") != 0) { */
/*         fprintf(stderr, "Error: unable to read file. 'CLIP' specifier not found where expected.\n"); */
/*         // destroy_clip(clip); */
/*         return; */
/*     } */
/*     uint8_t clip_namelen = 0; */
/*     fread(&clip_namelen, 1, 1, f); */
/*     fread(clip->name, 1, clip_namelen + 1, f); */
/*     fprintf(stderr, "Clip named: '%s'\n", clip->name); */
/*     // clip->channels = 2; //TODO: make sure this is written in */


/*     if (sys_byteorder_le) { */
/*         fread(&(clip->abs_pos_sframes), 4, 1, f); */
/*         fread(&(clip->len_sframes), 4, 1, f); */
/*         if (file_spec_version > 0.03) { */
/*             fread(&(clip->channels), 1, 1, f); */
/*             fprintf(stderr, "Reading clip channels from file: %d\n", clip->channels); */
/*         } else { */
/*             clip->channels = 2; */
/*             fprintf(stderr, "Setting default value for clip channels: %d\n", clip->channels); */
/*         } */
/*     } else { */
/*         //TODO: handle big endian */
/*         exit(1); */
/*     } */

/*     if (file_spec_version > 0.02) { */
/*         if (sys_byteorder_le) { */
/*             fread(&(clip->start_ramp_len), 4, 1, f); */
/*             fread(&(clip->end_ramp_len), 4, 1, f); */
/*         } else { */
/*             //TODO: handle big endian */
/*             exit(1); */
/*         } */
/*     } */
/*     fread(hdr_buffer, 1, 4, f); */
/*     if (strcmp(hdr_buffer, "data") != 0) { */
/*         fprintf(stderr, "Error: unable to read file. 'data' specifier not found where expected.\n"); */
/*         // destroy_clip(clip); */
/*         return; */
/*     } */

/*     //TODO */



/*     /\* TODO: everything commented below. *\/ */

/*     create_clip_buffers(clip, clip->len_sframes); */
/*     fprintf(stderr, "Creeated clip buffers\n"); */
/*     uint32_t clip_len_samples = clip->len_sframes * clip->channels; */
/*     fprintf(stderr, "Clip len samples: %d\n", clip_len_samples); */

/*     int16_t *interleaved_clip_samples = malloc(sizeof(int16_t) * clip_len_samples); */
/*     fread(interleaved_clip_samples, 2, clip_len_samples, f); */
/*     fprintf(stderr, "Read into interleaved samples array\n"); */

/*     if (clip->channels == 2) { */
/*         for (uint32_t i=0; i<clip_len_samples; i+=2) { */
/*             clip->L[i/2] = (float)interleaved_clip_samples[i] / INT16_MAX; */
/*             clip->R[i/2] = (float)interleaved_clip_samples[i+1] / INT16_MAX; */
/*         } */
/*     } else { */
/*         for (uint32_t i=0; i<clip_len_samples; i++) { */
/*             clip->L[i] = (float)interleaved_clip_samples[i] / INT16_MAX; */
/*         }      */
/*     } */
/*     free(interleaved_clip_samples); */
/*     fprintf(stderr, "Finished creating samples array\n"); */


/*     // clip->pre_proc = malloc(sizeof(int16_t) * clip->len_sframes * clip->channels); */
/*     // clip->post_proc = malloc(sizeof(int16_t) * clip->len_sframes * clip->channels); */
/*     // if (!(clip->pre_proc)) { */
/*     //     fprintf(stderr, "Error allocating space for clip->samples\n"); */
/*     //     exit(1); */
/*     // } */
/*     // if (sys_byteorder_le) { */
/*     //     // size_t t= fread(NULL, 2, clip->length, f); */
/*     //     fread(clip->pre_proc, 2, clip->len_sframes * clip->channels, f); */
/*     // } else { */
/*     //     //TODO: handle big endian */
/*     //     exit(1); */
/*     // } */
    
/*     clip->done = true; */
/*     reset_cliprect(clip); */
/* } */

