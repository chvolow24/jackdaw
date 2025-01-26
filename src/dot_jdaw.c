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
    dot_jdaw.c

    * Define the .jdaw file type and provide functions for saving and opening .jdaw files
 *****************************************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "project.h"
#include "tempo.h"
#include "transport.h"
#include "type_serialize.h"

/* #define BYTEORDER_FATAL fprintf(stderr, "Fatal error (TODO): big-endian byte order not supported\n"); exit(1); */
#define OLD_FLOAT_SER_W 16

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
const static char hdr_click[] = {'C', 'L', 'C', 'K'};
const static char hdr_click_segm[] = {'C', 'T', 'S', 'G'};

const static char current_file_spec_version[] = {'0','0','.','1','5'};
/* const static float current_file_spec_version_f = 0.11f; */

/* const static char nullterm = '\0'; */

static float read_file_spec_version;

static void debug_print_bytes_around(FILE *f)
{
    char errbuf[32];
    fseek(f, -4, SEEK_CUR);
    fread(errbuf, 1, 31, f);
    for (int i=0; i<32; i++) {
	if (errbuf[i] == '\0') errbuf[i] = '_';
    }
    errbuf[31] = '\0';
    fprintf(stderr, "BYTES AROUND: %s\n", errbuf);

}

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
    /* fwrite(&nullterm, 1, 1, f); */
    /* fwrite(&proj->channels, 1, 1, f); */
    uint8_ser(f, &proj->channels);

    uint32_ser_le(f, &proj->sample_rate);
    uint16_ser_le(f, &proj->chunk_size_sframes);
    uint16_ser_le(f, (uint16_t *)&proj->fmt);
    uint16_ser_le(f, &proj->num_clips);
    uint8_ser(f, &proj->num_timelines);
    /* fwrite(&proj->num_timelines, 1, 1, f); */
    
    for (uint16_t i=0; i<proj->num_clips; i++) {
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
    uint8_ser(f, &namelen);
    fwrite(&clip->name, 1, namelen, f);
    uint8_t index_8 = (uint8_t)index;
    uint8_ser(f, &index_8);
    uint8_ser(f, &clip->channels);
    uint32_ser_le(f, &clip->len_sframes);
    /* if (SYS_BYTEORDER_LE) { */
    /* 	fwrite(&clip->len_sframes, 4, 1, f); */
    /* } else { */
    /*     BYTEORDER_FATAL */
    /* } */

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
    for (uint32_t i=0; i<clip->len_sframes * clip->channels; i++) {
	int16_ser_le(f, clip_samples + i);
    }
    /* fwrite(clip_samples, 2, clip->len_sframes * clip->channels, f); */
    free(clip_samples);
}



static void jdaw_write_track(FILE *f, Track *track);
static void jdaw_write_click_track(FILE *f, ClickTrack *ct);

static void jdaw_write_timeline(FILE *f, Timeline *tl)
{
    fwrite(hdr_timeline, 1, 8, f);
    uint8_t timeline_namelen = strlen(tl->name);
    uint8_ser(f, &timeline_namelen);
    fwrite(tl->name, 1, timeline_namelen, f);

    /* Write number of tracks + click tracks */
    uint8_ser(f, &tl->track_area->num_children);

    for (uint8_t i=0; i<tl->track_area->num_children; i++) {
	tl->layout_selector = i;
	timeline_rectify_track_indices(tl);
	Track *t;
	if ((t = timeline_selected_track(tl))) {
	    jdaw_write_track(f, t);
	} else {
	    ClickTrack *ct = timeline_selected_click_track(tl);
	    jdaw_write_click_track(f, ct);
	}
    }
    /* uint8_ser(f, &tl->num_tracks); */
    /* for (uint8_t i=0; i<tl->num_tracks; i++) { */
    /* 	jdaw_write_track(f, tl->tracks[i]); */
    /* } */
    /* uint8_ser(f, &tl->num_click_tracks); */
    /* for (uint8_t i=0; i<tl->num_click_tracks; i++) { */
    /* 	jdaw_write_click_track(f, tl->click_tracks[i]); */
    /* } */
}

static void jdaw_write_clipref(FILE *f, ClipRef *cr);
static void jdaw_write_automation(FILE *f, Automation *a);

static void jdaw_write_track(FILE *f, Track *track)
{
    fwrite(hdr_track, 1, 4, f);
    uint8_t track_namelen = strlen(track->name);
    fwrite(&track_namelen, 1, 1, f);
    fwrite(track->name, 1, track_namelen, f);
    /* fwrite(&nullterm, 1, 1, f); */
    fwrite(&track->color, 1, 4, f);

    /* vol and pan */
    float_ser40_le(f, track->vol);
    float_ser40_le(f, track->pan);
    
    /* Write mute and solo values */
    fwrite(&track->muted, 1, 1, f);
    fwrite(&track->solo, 1, 1, f);

    fwrite(&track->solo_muted, 1, 1, f);
    uint16_ser_le(f, &track->num_clips);
    for (uint16_t i=0; i<track->num_clips; i++) {
	jdaw_write_clipref(f, track->clips[i]);
    }

    uint8_ser(f, &track->num_automations);
    for (uint8_t i=0; i<track->num_automations; i++) {
	jdaw_write_automation(f, track->automations[i]);
    }
    
    /* TRCK_FX */
    fwrite(&track->fir_filter_active, 1, 1, f);
    
    FIRFilter *filter = &track->fir_filter;
    if (track->fir_filter_active && (filter = &track->fir_filter)) {
	uint8_t type_byte = (uint8_t)(filter->type);
	fwrite(&type_byte, 1, 1, f);
	float_ser40_le(f, filter->cutoff_freq);
	float_ser40_le(f, filter->bandwidth);
        uint16_ser_le(f, &filter->impulse_response_len);
    } else {
	fseek(f, 13, SEEK_CUR);

    }
    fwrite(&track->delay_line_active, 1, 1, f);
    if (track->delay_line_active) {
	int32_ser_le(f, &track->delay_line.len);
	float_ser40_le(f, track->delay_line.stereo_offset);
	float_ser40_le(f, track->delay_line.amp);
    } else {
	fseek(f, 14, SEEK_CUR);
    }
	/* fwrite(&track->delay_line.len, 4, 1, f); */
	/* snprintf(float_value_buffer, STD_FLOAT_SER_W, "%f", track->delay_line.stereo_offset); */
	/* fwrite(float_value_buffer, STD_FLOAT_SER_W, 1, f); */
	/* snprintf(float_value_buffer, STD_FLOAT_SER_W, "%f", track->delay_line.amp); */
	/* fwrite(float_value_buffer, 1, STD_FLOAT_SER_W, f); */
    /* } else { */
	/* fseek(f, 36, SEEK_CUR); */
    /* } */
    
}

static void jdaw_write_click_segment(FILE *f, ClickSegment *s);
static void jdaw_write_click_track(FILE *f, ClickTrack *ct)
{
    fwrite(hdr_click, 1, 4, f);
    uint8_t namelen = strlen(ct->name);
    uint8_ser(f, &namelen);
    fwrite(ct->name, 1, namelen, f);
    float_ser40_le(f, ct->metronome_vol);
    uint8_ser(f, (uint8_t *)&ct->muted);
    ClickSegment *s = ct->segments;
    while (s) {
	jdaw_write_click_segment(f, s);
	s = s->next;
    }
    
}

static void jdaw_write_click_segment(FILE *f, ClickSegment *s)
{
    fwrite(hdr_click_segm, 1, 4, f);
    int32_ser_le(f, &s->start_pos);
    int32_ser_le(f, &s->end_pos);
    int16_ser_le(f, &s->first_measure_index);
    int32_ser_le(f, &s->num_measures);
    int16_t bpm = (int16_t)s->cfg.bpm;
    int16_ser_le(f, &bpm);
    uint8_ser(f, &s->cfg.num_beats);
    fwrite(&s->cfg.beat_subdiv_lens, 1, s->cfg.num_beats, f);
    uint8_t more_segments = s->next != NULL;
    uint8_ser(f, &more_segments);    
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
    uint8_ser(f, &src_clip_index);
    /* fwrite(&src_clip_index, 1, 1, f); */

    int32_ser_le(f, &cr->pos_sframes);
    uint32_ser_le(f, &cr->in_mark_sframes);
    uint32_ser_le(f, &cr->out_mark_sframes);
    uint32_ser_le(f, &cr->start_ramp_len);
    uint32_ser_le(f, &cr->end_ramp_len);
}

static void jdaw_write_auto_keyframe(FILE *f, Keyframe *k);
static void jdaw_write_automation(FILE *f, Automation *a)
{
    fwrite(hdr_auto, 1, 4, f);
    /* Automation type */
    uint8_t type_byte = (uint8_t)a->type;
    fwrite(&type_byte, 1, 1, f);
    /* Value type */
    type_byte = (uint8_t)a->val_type;
    fwrite(&type_byte, 1, 1, f);

    jdaw_val_serialize(f, a->min, a->val_type);
    jdaw_val_serialize(f, a->max, a->val_type);
    jdaw_val_serialize(f, a->range, a->val_type);
    fwrite(&a->read, 1, 1, f);
    fwrite(&a->shown, 1, 1, f);
    uint16_ser_le(f, &a->num_keyframes);
    for (uint16_t i=0; i<a->num_keyframes; i++) {
	jdaw_write_auto_keyframe(f, a->keyframes + i);
    }
    
}
static void jdaw_write_auto_keyframe(FILE *f, Keyframe *k)
{
    fwrite(hdr_keyf, 1, 4, f);
    int32_ser_le(f, &k->pos);
    jdaw_val_serialize(f, k->value, k->automation->val_type);
}




/**********************************************/
/******************** READ ********************/
/**********************************************/



static int jdaw_read_clip(FILE *f, Project *proj);
static int jdaw_read_timeline(FILE *f, Project *proj);

static Project *proj_reading;

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

    uint8_t proj_namelen;
    char project_name[MAX_NAMELENGTH];
    uint8_t channels;
    uint32_t sample_rate;
    SDL_AudioFormat fmt;
    uint16_t chunk_size;
    uint16_t num_clips;
    uint8_t num_timelines;

    proj_namelen = uint8_deser(f);
    fread(project_name, 1, proj_namelen, f);
    project_name[proj_namelen] = '\0';
    if (read_file_spec_version < 0.15f) {
	/* Extraneous nullterm in older versions */
	char c;
	fread(&c, 1, 1, f);
    }

    channels = uint8_deser(f);
    sample_rate = uint32_deser_le(f);
    chunk_size = uint16_deser_le(f);
    fmt = (SDL_AudioFormat)uint16_deser_le(f);
    
    if (read_file_spec_version < 0.14) {
	uint8_t byte_num_clips;
	fread(&byte_num_clips, 1, 1, f);
	num_clips = byte_num_clips;
    } else {
	num_clips = uint16_deser_le(f);
	/* fread(&num_clips, 2, 1, f); */
    }
    
    num_timelines = uint8_deser(f);

    proj_reading = project_create(
	project_name,
	channels,
	sample_rate,
	fmt,
	chunk_size,
	1024 // TODO: variable fourier len
	);

    
    proj_reading->num_timelines = 0;

    while (num_clips > 0) {
	if (jdaw_read_clip(f, proj_reading) != 0) {
	    goto jdaw_parse_error;
	}
	num_clips--;
    }

    while (num_timelines > 0) {
	if (jdaw_read_timeline(f, proj_reading) != 0) {
	    goto jdaw_parse_error;
	}
	num_timelines--;
    }

    /* timeline_reset(proj->timelines[0]); */
    fclose(f);
    return proj_reading;
    
jdaw_parse_error:
    debug_print_bytes_around(f);
    fclose(f);
    fprintf(stderr, "Error parsing .jdaw file at %s, filespec version %05.2f\n", path, read_file_spec_version);
    /* project_destroy(proj_loc); */
    return NULL;
    
}

static int jdaw_read_clip(FILE *f, Project *proj)
{
    char hdr_buffer[5];
    fread(hdr_buffer, 1, 4, f);
    if (strncmp(hdr_buffer, hdr_clip, 4) != 0) {
	fprintf(stderr, "Error: CLIP indicator missing.\n");
	return 1;
    }

    Clip *clip = calloc(1, sizeof(Clip));
    proj->clips[proj->num_clips] = clip;
    proj->num_clips++;
    proj->active_clip_index++;
    
    uint8_t name_length;
    
    fread(&name_length, 1, 1, f);
    if (read_file_spec_version < 0.15f) {
	fread(clip->name, 1, name_length + 1, f);
    } else {
	fread(clip->name, 1, name_length, f);
    }
    clip->name[name_length] = '\0';

    uint8_t clip_index;
    fread(&clip_index, 1, 1, f);
    
    fread(&clip->channels, 1, 1, f);
    clip->len_sframes = uint32_deser_le(f);    
    fread(hdr_buffer, 1, 4, f);
    if (strncmp(hdr_buffer, hdr_data, 4) != 0) {
	fprintf(stderr, "Error: clip 'data' indicator missing.\n");
	return 1;
    }

    /* Read clip data */
    create_clip_buffers(clip, clip->len_sframes);
    uint32_t clip_len_samples = clip->len_sframes * clip->channels;
    int16_t *interleaved_clip_samples = malloc(sizeof(int16_t) * clip_len_samples);

    /* Read data */
    for (uint32_t i=0; i<clip_len_samples; i++) {
	interleaved_clip_samples[i] = int16_deser_le(f);
    }
    
    /* write data to clip */
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
    return 0;
}


void user_tl_move_track_up(void *nullarg);
void user_tl_move_track_down(void *nullarg);
void user_tl_track_selector_up(void *nullarg);
void user_tl_track_selector_down(void *nullarg);
static void local_track_selector_down()
{
    Project *sg = proj;
    proj = proj_reading;
    user_tl_track_selector_down(NULL);
    proj = sg;

}
static void local_track_selector_up()
{
    Project *sg = proj;
    proj = proj_reading;
    user_tl_track_selector_up(NULL);
    proj = sg;
}
static void local_move_track_down()
{
    Project *sg = proj;
    proj = proj_reading;
    user_tl_move_track_down(NULL);
    proj = sg;
}
/* static void local_move_track_up() */
/* { */
/*     Project *sg = proj; */
/*     proj = proj_reading; */
/*     user_tl_move_track_up(NULL); */
/*     proj = sg; */
/* } */

static int jdaw_read_track(FILE *f, Timeline *tl);
static int jdaw_read_click_track(FILE *f, Timeline *tl);
static int jdaw_read_timeline(FILE *f, Project *proj_loc)
{
    char hdr_buffer[8];
    fread(hdr_buffer, 1, 8, f);
    if (strncmp(hdr_buffer, hdr_timeline, 8) != 0) {
	fprintf(stderr, "Error: \"TIMELINE\" indicator not found\n");
	return 1;
    }
    uint8_t tl_namelen = uint8_deser(f);
    
    char tl_name[tl_namelen + 1];
    fread(tl_name, 1, tl_namelen, f);
    if (read_file_spec_version < 0.15f) {
	char c;
	fread(&c, 1, 1, f); /* Extraneous nullterm in earlier versions */
    }
    tl_name[tl_namelen] = '\0';
    uint8_t index = project_add_timeline(proj_loc, tl_name);
    Timeline *tl = proj_loc->timelines[index];

    uint8_t num_tracks = uint8_deser(f);

    if (read_file_spec_version < 0.15) {
	while (num_tracks > 0) {
	    if (jdaw_read_track(f, tl) != 0) {
		return 1;
	    }
	    num_tracks--;
	}
    } else {
	bool more_tracks = true;
	while (more_tracks) {
	    char hdr_buf[4];
	    fread(hdr_buf, 1, 4, f);
	    fseek(f, -4, SEEK_CUR);
	    if (strncmp(hdr_buf, hdr_track, 4) == 0) {
		if (jdaw_read_track(f, tl) != 0)
		    return 1;
	    } else if (strncmp(hdr_buf, hdr_click, 4) == 0) {
		if (jdaw_read_click_track(f, tl) != 0)
		    return 1;
		local_move_track_down();
	    } else {
		more_tracks = false;
	    }
	    hdr_buf[0] = '\0';
	    local_track_selector_down();
	}
	tl->layout_selector = 0;
	timeline_rectify_track_indices(tl);
	while (num_tracks > 0) {
	    local_track_selector_up();
	    num_tracks--;
	}
    }

    /* if (read_file_spec_version >= 0.15) { */
    /* 	uint8_t num_click_tracks = uint8_deser(f); */
    /* 	while (num_click_tracks > 0) { */
    /* 	    if (jdaw_read_click_track(f, tl) != 0) { */
    /* 		return 1; */
    /* 	    } */
    /* 	    num_click_tracks--; */
    /* 	} */
    /* } */
    return 0;
}

static int jdaw_read_clipref(FILE *f, Track *track);
static int jdaw_read_automation(FILE *f, Track *track);
static int jdaw_read_track(FILE *f, Timeline *tl)
{
    char hdr_buffer[4];
    fread(hdr_buffer, 1, 4, f);
    if (strncmp(hdr_buffer, hdr_track, 4) != 0) {
	fprintf(stderr, "Error: .jdaw parsing error: \"TRCK\" indicator not found\n");
	debug_print_bytes_around(f);
	return 1;
    }
    
    /* Track *track = calloc(sizeof(Track), 1); */
    /* tl->tracks[tl->num_tracks] = track; */
    /* tl->num_tracks++; */

    timeline_add_track(tl);
    Track *track = tl->tracks[tl->num_tracks - 1];

    uint8_t track_namelen = uint8_deser(f);
    fread(track->name, 1, track_namelen, f);
    if (read_file_spec_version < 0.15f) {
	char c;
	fread(&c, 1, 1, f);
    }
    track->name[track_namelen] = '\0';
    fread(&track->color, 1, 4, f);

    char floatvals[17];
    if (read_file_spec_version < 0.15f) {
	fread(floatvals, 1, OLD_FLOAT_SER_W, f);
	floatvals[OLD_FLOAT_SER_W] = '\0';
	track->vol = atof(floatvals);
	fread(floatvals, 1, OLD_FLOAT_SER_W, f);
	track->pan = atof(floatvals);
    } else {
    	track->vol = float_deser40_le(f);
	track->pan = float_deser40_le(f);
    }
    
    bool muted = uint8_deser(f);
    bool solo = uint8_deser(f);
    bool solo_muted = uint8_deser(f);
    
    if (muted) {
	track_mute(track);
    }
    if (solo) {
	track_solo(track);
    }
    if (solo_muted) {
	track_solomute(track);
    }

    uint16_t num_cliprefs;
    if (read_file_spec_version < 0.14) {
	uint8_t byte_num_cliprefs;
	fread(&byte_num_cliprefs, 1, 1, f);
	num_cliprefs = byte_num_cliprefs;
    } else {
	num_cliprefs = uint16_deser_le(f);
    }

    while (num_cliprefs > 0) {
	if (jdaw_read_clipref(f, track) != 0) {
	    return 1;
	}
	num_cliprefs--;
    }
    if (read_file_spec_version >= 00.13f) {
	uint8_t num_automations = uint8_deser(f);
	while (num_automations > 0) {
	    if (jdaw_read_automation(f, track) != 0) {
		return 1;
	    }
	    num_automations--;
	}
    }
    if (read_file_spec_version >= 00.11f) {
	/* fread(&track->fir_filter_active, 1, 1, f); */
	track->fir_filter_active = (bool)uint8_deser(f);
	if (track->fir_filter_active) {
	    uint8_t type_byte;
	    FilterType type;
	    double cutoff_freq;
	    double bandwidth;
	    uint16_t impulse_response_len;
	
	    fread(&type_byte, 1, 1, f);
	    type = (FilterType)type_byte;

	    if (read_file_spec_version < 0.15f) {
		fread(floatvals, 1, OLD_FLOAT_SER_W, f);
		floatvals[OLD_FLOAT_SER_W] = '\0';
		cutoff_freq = atof(floatvals);
		fread(floatvals, 1, OLD_FLOAT_SER_W, f);
		bandwidth = atof(floatvals);
		fread(&impulse_response_len, 2, 1, f);
	    } else {
		cutoff_freq = float_deser40_le(f);
		bandwidth = float_deser40_le(f);
		impulse_response_len = uint16_deser_le(f);
	    }
	    filter_init(
		&track->fir_filter,
		track,
		type,
		impulse_response_len,
		tl->proj->fourier_len_sframes * 2);
	    filter_set_params(&track->fir_filter, type, cutoff_freq, bandwidth);
	} else {
	    if (read_file_spec_version < 0.15f) {
		fseek(f, 35, SEEK_CUR);
	    } else {
		fseek(f, 13, SEEK_CUR);
	    }
	}
	track->delay_line_active = (bool)uint8_deser(f);
	/* fread(&track->delay_line_active, 1, 1, f); */
	if (track->delay_line_active) {
	    int32_t len;
	    double amp;
	    double stereo_offset;
	    if (read_file_spec_version < 0.15f) {
		fread(&len, 4, 1, f);
		fread(&floatvals, 1, OLD_FLOAT_SER_W, f);
		floatvals[OLD_FLOAT_SER_W] = '\0';
		stereo_offset = atof(floatvals);
		fread(floatvals, 1, OLD_FLOAT_SER_W, f);
		floatvals[OLD_FLOAT_SER_W] = '\0';
		amp = atof(floatvals);
	    } else {
		len = int32_deser_le(f);
		amp = float_deser40_le(f);
		stereo_offset = float_deser40_le(f);
	    }
	    delay_line_init(&track->delay_line, track, tl->proj->sample_rate);
	    delay_line_set_params(&track->delay_line, amp, len);
	    track->delay_line.stereo_offset = stereo_offset;
	} else {
	    /* delay_line_init(&track->delay_line, track, tl->proj->sample_rate); */
	    if (read_file_spec_version < 0.15f) {
		fseek(f, 36, SEEK_CUR);
	    } else {
		fseek(f, 14, SEEK_CUR);
	    }
	}
    }
    return 0;
}

static int jdaw_read_clipref(FILE *f, Track *track)
{

    char hdr_buffer[7];
    fread(hdr_buffer, 1, 7, f);

    if (strncmp(hdr_buffer, hdr_clipref, 7) != 0) {
	fprintf(stderr, "Error: .jdaw parsing error: \"CLIPREF\" indicator missing\n");
	return 1;
    }

    /* ClipRef *cr = calloc(sizeof(ClipRef), 1); */
    /* track->clips[track->num_clips] = cr; */
    /* track->num_clips++; */

    uint8_t clipref_namelen = uint8_deser(f);
    char clipref_name[clipref_namelen + 1];
    fread(clipref_name, 1, clipref_namelen, f);
    clipref_name[clipref_namelen] = '\0';
    bool clipref_home = uint8_deser(f);
    uint8_t src_clip_index = uint8_deser(f);
    Clip *clip = track->tl->proj->clips[src_clip_index];
    ClipRef *cr = track_create_clip_ref(track, clip, 0, clipref_home);

    cr->pos_sframes = int32_deser_le(f);
    cr->in_mark_sframes = uint32_deser_le(f);
    cr->out_mark_sframes = uint32_deser_le(f);
    cr->start_ramp_len = uint32_deser_le(f);
    cr->end_ramp_len = uint32_deser_le(f);
    return 0;
}

static int jdaw_read_keyframe(FILE *f, Automation *a);

static int jdaw_read_automation(FILE *f, Track *track)
{

    char hdr_buffer[4];
    fread(hdr_buffer, 1, 4, f);

    if (strncmp(hdr_buffer, hdr_auto, 4) != 0) {
	fprintf(stderr, "Error: .jdaw parsing error: \"AUTO\" indicator missing\n");
	return 1;
    }

    uint8_t type_byte = uint8_deser(f);
    AutomationType t = (AutomationType)type_byte;
    Automation *a = track_add_automation(track, t);
    a->val_type = (ValType)uint8_deser(f);

    if (read_file_spec_version < 0.15f) {
	a->min = jdaw_val_deserialize_OLD(f, OLD_FLOAT_SER_W, a->val_type);
	a->max = jdaw_val_deserialize_OLD(f, OLD_FLOAT_SER_W, a->val_type);
	a->range = jdaw_val_deserialize_OLD(f, OLD_FLOAT_SER_W, a->val_type);
    } else {
	a->min = jdaw_val_deserialize(f);
	a->max = jdaw_val_deserialize(f);
	a->range = jdaw_val_deserialize(f);
    }
    bool read = (bool)uint8_deser(f);
    if (!read) {
	automation_toggle_read(a);
    }
    /* TODO: figure out how to initialize this */
    a->shown = uint8_deser(f);
    if (a->shown) {
	automation_show(a);
    }
    uint16_t num_keyframes = uint16_deser_le(f);
    
    a->num_keyframes = 0;
    
    while (num_keyframes > 0) {
	if (jdaw_read_keyframe(f, a) != 0) {
	    return 1;
	}
	num_keyframes--;
    }
    return 0;
}


static int jdaw_read_keyframe(FILE *f, Automation *a)
{
    char hdr_buffer[4];
    fread(hdr_buffer, 1, 4, f);
    if (strncmp(hdr_buffer, hdr_keyf, 4) != 0) {
	fprintf(stderr, "Error: .jdaw parsing error: \"KEYF\" indicator missing.\n");
	return 1;
    }
    int32_t pos = int32_deser_le(f);
    Value val;
    if (read_file_spec_version < 0.15) {
	val = jdaw_val_deserialize_OLD(f, OLD_FLOAT_SER_W, a->val_type);
    } else {
	val = jdaw_val_deserialize(f);
    }
    automation_insert_keyframe_at(a, pos, val);
    return 0;
}


static int jdaw_read_click_segment(FILE *f, ClickTrack *ct, bool *more_segments);
static int jdaw_read_click_track(FILE *f, Timeline *tl)
{
    char hdr_buf[4];
    fread(hdr_buf, 1, 4, f);
    if (strncmp(hdr_buf, hdr_click, 4) != 0) {
	fprintf(stderr, "Error: .jdaw parse error: \"CLICK\" indicator not found where expected\n");
	return 1;
    }
    ClickTrack *ct = timeline_add_click_track(tl);
    uint8_t namelen = uint8_deser(f);
    fread(ct->name, 1, namelen, f);
    ct->name[namelen] = '\0';

    float metronome_vol = float_deser40_le(f);
    bool muted = (bool)uint8_deser(f);

    if (muted) click_track_mute_unmute(ct);
    endpoint_write(&ct->metronome_vol_ep, (Value){.float_v = metronome_vol}, true, true, true, false);
    
    bool more_segments = true;
    while (more_segments) {
	if (jdaw_read_click_segment(f, ct, &more_segments) != 0) {
	    return 1;
	}
    }
    return 0;
    
}

static bool first_segment = true;

static int jdaw_read_click_segment(FILE *f, ClickTrack *ct, bool *more_segments)
{
    char hdr_buf[4];
    fread(hdr_buf, 1, 4, f);
    if (strncmp(hdr_buf, hdr_click_segm, 4) != 0) {
	fprintf(stderr, "Error: .jdaw parse error: \"CTSG\" indicator not found where expected.\n");
	return 1;
    }

    int32_t start_pos = int32_deser_le(f);
    int32_deser_le(f); /* End pos:  unusued */
    int16_deser_le(f); /* First measure index:  unused */
    int32_t num_measures = int32_deser_le(f);
    int16_t tempo = int16_deser_le(f);
    uint8_t num_beats = uint8_deser(f);
    uint8_t beat_subdivs[num_beats];
    fread(beat_subdivs, 1, num_beats, f);
    if (first_segment) {
	click_segment_set_config(ct->segments, num_measures, tempo, num_beats, beat_subdivs, 0);
	first_segment = false;
    } else {
	click_track_add_segment(ct, start_pos, num_measures, tempo, num_beats, beat_subdivs);
    }
    *more_segments = (bool)uint8_deser(f);
    return 0;
}
