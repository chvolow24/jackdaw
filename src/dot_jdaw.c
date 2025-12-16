/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "audio_clip.h"
#include "clipref.h"
#include "compressor.h"
#include "consts.h"
#include "delay_line.h"
#include "effect.h"
#include "eq.h"
#include "file_backup.h"
#include "fir_filter.h"
#include "midi_clip.h"
#include "midi_file.h"
#include "midi_io.h"
#include "project.h"
#include "session.h"
#include "tempo.h"
#include "transport.h"
#include "type_serialize.h"
#include "loading.h"

#define OLD_FLOAT_SER_W 16

extern bool SYS_BYTEORDER_LE;
extern const char *JACKDAW_VERSION;
/* extern JDAW_Color black; */
/* extern JDAW_Color white; */

const static char hdr_jdaw[] = {'J','D','A','W'};
const static char hdr_version[] = {' ','V','E','R','S','I','O','N'};
const static char hdr_clip[] = {'C','L','I','P'};
const static char hdr_mclip[] = {'M','C','L','I','P'};
const static char hdr_timeline[] = {'T','I','M','E','L','I','N','E'};
const static char hdr_track[] = {'T','R','C','K'};
const static char hdr_clipref[] = {'C','L','I','P','R','E','F'};
const static char hdr_data[] = {'d','a','t','a'};
const static char hdr_auto[] = {'A', 'U', 'T', 'O'};
const static char hdr_keyf[] = {'K', 'E', 'Y', 'F'};
const static char hdr_click[] = {'C', 'L', 'C', 'K'};
const static char hdr_click_segm[] = {'C', 'T', 'S', 'G'};
const static char hdr_trck_efct[] = {'E', 'F', 'C', 'T'};
const static char hdr_trck_synth[] = {'S','Y','N','T','H'};

const static char current_file_spec_version[] = {'0','0','.','2','1'};

static char read_file_spec_version[6];
bool read_file_version_older_than(const char *cmp_version)
{
    return strncmp(read_file_spec_version, cmp_version, 5) < 0;       
}

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
static void jdaw_write_midi_clip(FILE *f, MIDIClip *clip);
static void jdaw_write_timeline(FILE *f, Timeline *tl);

static Project *proj;
void jdaw_write_project(const char *path) 
{
    Session *session = session_get();
    proj = &session->proj;
    if (!proj) {
        fprintf(stderr, "No project to save. Exiting.\n");
        return;
    }


    if (file_exists(path)) {
	/* session_loading_screen_update("Backing up existing file...", 0.1); */
	file_backup(path);
    }

    session_set_loading_screen("Saving project", "Writing project settings...", true);
    
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
    uint16_ser_le(f, &proj->num_midi_clips);
    uint8_ser(f, &proj->num_timelines);
    /* fwrite(&proj->num_timelines, 1, 1, f); */

    session_loading_screen_update("Writing clip data...", 0.2);
    fprintf(stderr, "Serializing %d audio clips...\n", proj->num_clips);
    for (uint16_t i=0; i<proj->num_clips; i++) {
	session_loading_screen_update(NULL, 0.1 + 0.8 * (float)i/proj->num_clips);
	jdaw_write_clip(f, proj->clips[i], i);
    }
    session_loading_screen_update("Writing MIDI clips...", 0.2);
    for (uint16_t i=0; i<proj->num_midi_clips; i++) {
	session_loading_screen_update(NULL, 0.1 + 0.8 * (float)i/proj->num_midi_clips);
	jdaw_write_midi_clip(f, proj->midi_clips[i]);
    }
    session_loading_screen_update("Writing timelines...", 0.9);
    fprintf(stderr, "\t...done.\nSerializing %d timelines...\n", proj->num_timelines);
    for (uint8_t i=0; i<proj->num_timelines; i++) {
	jdaw_write_timeline(f, proj->timelines[i]);
    }
    session_loading_screen_deinit();
    fprintf(stderr, "\t...done.\n");
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

    fwrite(hdr_data, 1, 4, f);

    /* Write clip sample data */
    uint32_t len_samples = clip->len_sframes * clip->channels;
    int16_t *clip_samples = malloc(sizeof(int16_t) * len_samples);
    if (!clip_samples) {
        fprintf(stderr, "Error: unable to allocate space for clip_samples\n");
        exit(1);
    }
    if (clip->channels == 2) {
        for (uint32_t i=0; i<len_samples; i+=2) {
	    clip_samples[i] = clip->L[i/2] * INT16_MAX;
	    clip_samples[i+1] = clip->R[i/2] * INT16_MAX;
        }
    } else if (clip->channels == 1) {
        for (uint32_t i=0; i<len_samples; i++) {
	    clip_samples[i] = clip->L[i] * INT16_MAX;
        }
    }
    if (SYS_BYTEORDER_LE) {
	fwrite(clip_samples, 2, len_samples, f);
    } else {
	for (uint32_t i=0; i<clip->len_sframes * clip->channels; i++) {
	    int16_ser_le(f, clip_samples + i);
	}
    }
    free(clip_samples);
}


/* static void jdaw_write_midi_note(FILE *f, Note *note); */
static void jdaw_write_midi_clip(FILE *f, MIDIClip *mclip)
{
    fwrite(hdr_mclip, 1, 5, f);
    uint8_t namelen = strlen(mclip->name);
    uint8_ser(f, &namelen);
    fwrite(&mclip->name, 1, namelen, f);    
    uint32_ser_le(f, &mclip->len_sframes);


    /* INSTEAD of writing objects, write midi stream */
    PmEvent *events;
    uint32_t num_events = midi_clip_get_all_events(mclip, &events);
    midi_serialize_events(f, events, num_events);
    free(events);
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

    int16_t track_area_num_children = (int16_t)tl->track_area->num_children;
    int16_ser_le(f, &track_area_num_children);
    
    for (uint16_t i=0; i<tl->track_area->num_children; i++) {
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

}

static void jdaw_write_clipref(FILE *f, ClipRef *cr);
static void jdaw_write_automation(FILE *f, Automation *a);
static void jdaw_write_effect(FILE *f, Effect *e);

static void jdaw_write_track(FILE *f, Track *track)
{
    fwrite(hdr_track, 1, 4, f);
    uint8_t track_namelen = strlen(track->name);
    fwrite(&track_namelen, 1, 1, f);
    fwrite(track->name, 1, track_namelen, f);
    fwrite(&track->color, 1, 4, f);

    /* vol and pan */
    float_ser40_le(f, track->vol);
    float_ser40_le(f, track->pan);
    
    /* Write mute and solo values */
    fwrite(&track->muted, 1, 1, f);
    fwrite(&track->solo, 1, 1, f);
    fwrite(&track->solo_muted, 1, 1, f);
    fwrite(&track->minimized, 1, 1, f);
    uint16_ser_le(f, &track->num_clips);
    for (uint16_t i=0; i<track->num_clips; i++) {
	jdaw_write_clipref(f, track->clips[i]);
    }


    /* Effects */
    uint8_ser(f, &track->num_effects);
    for (int i=0; i<track->num_effects; i++) {
	jdaw_write_effect(f, track->effects[i]);
    }

    /* Automations */
    uint8_ser(f, &track->num_automations);
    for (uint8_t i=0; i<track->num_automations; i++) {
	jdaw_write_automation(f, track->automations[i]);
    }
    uint8_t has_synth = track->synth != NULL;
    uint8_ser(f, &has_synth);
    if (has_synth) {
	fwrite(hdr_trck_synth, 1, 5, f);
	api_node_serialize(f, &track->synth->api_node);
    }
}

static void jdaw_write_fir_filter(FILE *f, FIRFilter *filter);
static void jdaw_write_delay(FILE *f, DelayLine *dl);
static void jdaw_write_saturation(FILE *f, Saturation *s);
static void jdaw_write_eq(FILE *f, EQ *eq);
static void jdaw_write_compressor(FILE *f, Compressor *compressor);

static void jdaw_write_effect(FILE *f, Effect *e)
{
    fwrite(hdr_trck_efct, 1, 4, f);
    uint8_t type_byte = e->type;
    uint8_ser(f, &type_byte);
    switch(e->type) {
    case EFFECT_EQ:
	jdaw_write_eq(f, e->obj);
	break;
    case EFFECT_FIR_FILTER:
	jdaw_write_fir_filter(f, e->obj);
	break;
    case EFFECT_DELAY:
	jdaw_write_delay(f, e->obj);
	break;
    case EFFECT_SATURATION:
	jdaw_write_saturation(f, e->obj);
	break;
    case EFFECT_COMPRESSOR:
	jdaw_write_compressor(f, e->obj);
	break;
    default:
	break;
    }
}

static void jdaw_write_fir_filter(FILE *f, FIRFilter *filter)
{
    fwrite(&filter->effect->active, 1, 1, f);    
    uint8_t type_byte = (uint8_t)(filter->type);
    fwrite(&type_byte, 1, 1, f);
    float_ser40_le(f, filter->cutoff_freq);
    float_ser40_le(f, filter->bandwidth);
    uint16_ser_le(f, &filter->impulse_response_len);
}

static void jdaw_write_delay(FILE *f, DelayLine *dl)
{
    /* fprintf(stderr, "WRITE Active? %d\nLen? %d\nAmp? */
    fwrite(&dl->effect->active, 1, 1, f);
    int32_ser_le(f, &dl->len);
    float_ser40_le(f, dl->stereo_offset);
    float_ser40_le(f, dl->amp);
}

static void jdaw_write_saturation(FILE *f, Saturation *s)
{
    fwrite(&s->effect->active, 1, 1, f);
    float_ser40_le(f, s->gain);
    fwrite(&s->do_gain_comp, 1, 1, f);
    uint8_t type_byte = (uint8_t)s->type;
    uint8_ser(f, &type_byte);

}

static void jdaw_write_eq(FILE *f, EQ *eq)
{
    fwrite(&eq->effect->active, 1, 1, f);
    uint8_t num_filters = EQ_DEFAULT_NUM_FILTERS;
    uint8_ser(f, &num_filters);
    for (int i=0; i<num_filters; i++) {
	EQFilterCtrl *ctrl = eq->ctrls + i;
	fwrite(&ctrl->filter_active, 1, 1, f);
	uint8_t type_byte = (uint8_t)eq->group.filters[i].type;
	uint8_ser(f, &type_byte);
	float_ser40_le(f, ctrl->freq_amp_raw[0]);
	float_ser40_le(f, ctrl->freq_amp_raw[1]);
	float_ser40_le(f, ctrl->bandwidth_scalar);
    }
}

static void jdaw_write_compressor(FILE *f, Compressor *c)
{
    fwrite(&c->effect->active, 1, 1, f);
    float_ser40_le(f, c->attack_time);
    float_ser40_le(f, c->release_time);
    float_ser40_le(f, c->threshold);
    float_ser40_le(f, c->m);
    float_ser40_le(f, c->makeup_gain);
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
    
    /* TODO: remove segment end pos from spec */
    int32_t dummy = 0;
    int32_ser_le(f, &dummy);
    
    /* int32_ser_le(f, &s->end_pos); */
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

    uint8_t type_byte = cr->type;
    uint8_ser(f, &type_byte);
    
    int src_clip_index = -1;
    if (cr->type == CLIP_AUDIO) {
	for (int i=0; i<proj->num_clips; i++) {
	    if (cr->source_clip == proj->clips[i]) {
		src_clip_index = i;
		break;
	    }
	}
    } else {
	for (int i=0; i<proj->num_midi_clips; i++) {
	    if (cr->source_clip == proj->midi_clips[i]) {
		src_clip_index = i;
		break;
	    }
	}
    }
    if (src_clip_index < 0) {
	fprintf(stderr, "CRITICAL ERROR: clipref \"%s\" source clip not found. Src clip ptr %p, proj num clips: %d and midi clips: %d\n", cr->name, cr->source_clip, proj->num_clips, proj->num_midi_clips);
	exit(1);
    }
    uint8_t src_clip_index_8 = src_clip_index;
    uint8_ser(f, &src_clip_index_8);
    /* fwrite(&src_clip_index, 1, 1, f); */

    int32_ser_le(f, &cr->tl_pos);
    int32_ser_le(f, &cr->start_in_clip);
    int32_ser_le(f, &cr->end_in_clip);

    /* Linear start/end ramps to be implemented later */
    uint32_t null_ramp_val = 0;
    uint32_ser_le(f, &null_ramp_val);
    uint32_ser_le(f, &null_ramp_val);
    float_ser40_le(f, cr->gain);
}

static void jdaw_write_auto_keyframe(FILE *f, Keyframe *k);
static void jdaw_write_automation(FILE *f, Automation *a)
{
    fwrite(hdr_auto, 1, 4, f);
    /* Automation type */
    uint8_t type_byte = (uint8_t)a->type;
    fwrite(&type_byte, 1, 1, f);
    if (type_byte == AUTO_ENDPOINT) {
	char route[255];
	uint8_t route_len = api_endpoint_get_route(a->endpoint, route, 255);
        uint8_ser(f, &route_len);
	fwrite(route, 1, route_len, f);
    }
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
static int jdaw_read_midi_clip(FILE *f, Project *proj);
static int jdaw_read_timeline(FILE *f, Project *proj);

static Project *proj_reading;


const char *get_fmt_str(SDL_AudioFormat f);
int jdaw_read_file(Project *dst, const char *path)
{
    /* Session *session_to_set_proj_reading = session_get(); */
    /* session_to_set_proj_reading->proj_reading = dst; */
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: could not find project file at path %s\n", path);
	/* session_to_set_proj_reading->proj_reading = NULL; */
        return -1;
    }
    char hdr_buffer[9];
    fread(hdr_buffer, 1, 4, f);
    hdr_buffer[4] = '\0';
    if (strncmp(hdr_buffer, hdr_jdaw, 4) != 0) {
        fprintf(stderr, "Error: unable to read file. 'JDAW' specifier missing %s\n", path);
        /* free(proj); */
	/* session_to_set_proj_reading->proj_reading = NULL; */
        return -1;
    }

    /* Check whether " VERSION" specifier exists to get the file spec version. If not, reset buffer position */
    fpos_t version_split_pos;
    fgetpos(f, &version_split_pos);
    fread(hdr_buffer, 1, 8, f);
    hdr_buffer[8] = '\0';
    if (strncmp(hdr_buffer, hdr_version, 8) != 0) {
	fprintf(stderr, "Error: \"VERSION\" specifier missing in .jdaw file\n");
	/* session_to_set_proj_reading->proj_reading = NULL; */
	return -1;
    }
    fread(read_file_spec_version, 1, 5, f);
    read_file_spec_version[5] = '\0';
    /* hdr_buffer[5] = '\0'; */
    /* read_file_spec_version = atof(hdr_buffer); */
    fprintf(stderr, "Reading JDAW file with version %s\n", read_file_spec_version);
    if (read_file_version_older_than("00.10")) {
	fprintf(stderr, "Error: .jdaw file version %s is not compatible with the current jackdaw version (%s). You may need to downgrade to open this file.\n", hdr_buffer, JACKDAW_VERSION);
        /* free(proj); */
	/* session_to_set_proj_reading->proj_reading = NULL; */
	return -1;
    }

    uint8_t proj_namelen;
    char project_name[MAX_NAMELENGTH];
    uint8_t channels;
    uint32_t sample_rate;
    SDL_AudioFormat fmt;
    uint16_t chunk_size;
    uint16_t num_clips;
    uint16_t num_midi_clips;
    uint8_t num_timelines;

    proj_namelen = uint8_deser(f);
    fread(project_name, 1, proj_namelen, f);
    project_name[proj_namelen] = '\0';
    fprintf(stderr, "Project name: %s\n", project_name);
    if (read_file_version_older_than("00.15")) {
	/* Extraneous nullterm in older versions */
	char c;
	fread(&c, 1, 1, f);
    }

    channels = uint8_deser(f);
    sample_rate = uint32_deser_le(f);
    chunk_size = uint16_deser_le(f);
    fmt = (SDL_AudioFormat)uint16_deser_le(f);
    
    if (read_file_version_older_than("00.14")) {
	uint8_t byte_num_clips;
	fread(&byte_num_clips, 1, 1, f);
	num_clips = byte_num_clips;
    } else {
	num_clips = uint16_deser_le(f);
	/* fread(&num_clips, 2, 1, f); */
    }

    if (read_file_version_older_than("00.18")) {
	num_midi_clips = 0;
    } else {
	num_midi_clips = uint16_deser_le(f);
    }
    
    num_timelines = uint8_deser(f);

    session_set_loading_screen("Loading project file", "Reading audio data...", true);
    session_loading_screen_update("Creating project...", 0.0);
    
    fprintf(stderr, "CREATING PROJECT with settings:\n");
    fprintf(stderr, "\tchannels: %d\n\tsample_rate: %d\n\tfmt: %s\n",
	    channels,
	    sample_rate,
	    get_fmt_str(fmt));

    
    project_init(
	dst,
	project_name,
	channels,
	sample_rate,
	fmt,
	chunk_size,
	DEFAULT_FOURIER_LEN_SFRAMES,
	false
	);

    proj_reading = dst;

    
    proj_reading->num_timelines = 0;

    fprintf(stderr, "Reading %d clips...\n", num_clips);
    for (int i=0; i<num_clips; i++) {
    /* while (num_clips > 0) { */
	session_loading_screen_update("Reading audio clips...", 0.8 * (float)i / num_clips);
	if (jdaw_read_clip(f, proj_reading) != 0) {
	    goto jdaw_parse_error;
	}
	/* num_clips--; */
    }

    fprintf(stderr, "Reading %d MIDI clips...\n", num_midi_clips);
    session_loading_screen_update("Reading midi clips...", 0.8);
    while (num_midi_clips > 0) {
	if (jdaw_read_midi_clip(f, proj_reading) != 0) {
	    goto jdaw_parse_error;
	}
	num_midi_clips--;
    }

    session_loading_screen_update("Reading timelines...", 0.9);
    fprintf(stderr, "Reading Timelines...\n");
    while (num_timelines > 0) {
	if (jdaw_read_timeline(f, proj_reading) != 0) {
	    goto jdaw_parse_error;
	}
	num_timelines--;
    }

    /* timeline_reset(proj->timelines[0]); */
    fclose(f);
    session_loading_screen_deinit();
    /* session_to_set_proj_reading->proj_reading = NULL; */
    return 0;
    
jdaw_parse_error:
    debug_print_bytes_around(f);
    fclose(f);
    fprintf(stderr, "Error parsing .jdaw file at %s, filespec version %s\n", path, read_file_spec_version);
    session_loading_screen_deinit();
    /* session_to_set_proj_reading->proj_reading = NULL; */
    return -1;
    
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
    clip_init(clip);
    proj->clips[proj->num_clips] = clip;
    proj->num_clips++;
    proj->active_clip_index++;
    
    uint8_t name_length;
    
    fread(&name_length, 1, 1, f);
    if (read_file_version_older_than("00.15")) {
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
    fread(interleaved_clip_samples, sizeof(int16_t), clip_len_samples, f);
    if (!SYS_BYTEORDER_LE) {
	for (uint32_t i=0; i<clip_len_samples; i++) {
	    char *buf =(char *)(interleaved_clip_samples + i);
	    uint16_t sample_u = uint16_fromstr_le(buf);
	    interleaved_clip_samples[i] = *((int16_t *)&sample_u);
	}
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

static int jdaw_read_midi_note(FILE *f, MIDIClip *mclip);

static int jdaw_read_midi_clip(FILE *f, Project *proj)
{
    char hdr_buffer[5];
    fread(hdr_buffer, 1, 5, f);
    if (strncmp(hdr_buffer, hdr_mclip, 5) != 0) {
	fprintf(stderr, "Error: MCLIP indicator missing.\n");
	return 1;
    }

    uint8_t name_length;
    MIDIClip *mclip = calloc(1, sizeof(MIDIClip));
    midi_clip_init(mclip);

    proj->midi_clips[proj->num_midi_clips] = mclip;
    proj->num_midi_clips++;
    proj->active_midi_clip_index++;

    fread(&name_length, 1, 1, f);
    fread(&mclip->name, 1, name_length, f);
    mclip->name[name_length] = '\0';
    mclip->len_sframes = uint32_deser_le(f);

    /* Read MIDI stream */
    /* mclip->num_notes = uint32_deser_le(f); */
    if (read_file_version_older_than("00.19")) {
    /* if (read_file_spec_version < 0.19f) { */
	uint32_t num_notes = uint32_deser_le(f);
	int err;
	for (uint32_t i=0; i<num_notes; i++) {
	    err = jdaw_read_midi_note(f, mclip);
	    if (err != 0) {
		fprintf(stderr, "Error: unable to deserialize MIDI note.\n");
		return err;
	    }
	}
    } else {
	PmEvent *events;
	uint32_t num_events = midi_deserialize_events(f, &events);
	midi_clip_read_events(mclip, events, num_events, MIDI_TS_SFRAMES, 0);
	free(events);
    }
    return 0;
}

static int jdaw_read_midi_note(FILE *f, MIDIClip *mclip)
{
    uint8_t note, velocity;
    int32_t start_rel, end_rel;
    note = uint8_deser(f);
    velocity = uint8_deser(f);
    start_rel = int32_deser_le(f);
    end_rel = int32_deser_le(f);
    midi_clip_insert_note(mclip, 0, note, velocity, start_rel, end_rel);
    return 0;
}


void user_tl_move_track_up(void *nullarg);
void user_tl_move_track_down(void *nullarg);
void user_tl_track_selector_up(void *nullarg);
void user_tl_track_selector_down(void *nullarg);
static void local_track_selector_down()
{
    /* Session *session = session_get(); */
    /* /\* Project *sg = proj; *\/ */
    /* /\* proj = proj_reading; *\/ */
    /* Project saved = session->proj; */
    /* session->proj = *proj_reading; */
    /* user_tl_track_selector_down(NULL); */
    /* session->proj = saved; */

}
static void local_track_selector_up()
{
    /* Session *session = session_get(); */
    /* Project saved = session->proj; */
    /* session->proj = *proj_reading; */
    /* user_tl_track_selector_up(NULL); */
    /* session->proj = saved; */

    /* Project *sg = proj; */
    /* proj = proj_reading; */
    
    /* proj = sg; */
}
static void local_move_track_down()
{
    /* Project *sg = proj; */
    /* proj = proj_reading; */
    /* user_tl_move_track_down(NULL); */
    /* proj = sg; */
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
    if (read_file_version_older_than("00.15")) {
	char c;
	fread(&c, 1, 1, f); /* Extraneous nullterm in earlier versions */
    }
    tl_name[tl_namelen] = '\0';
    fprintf(stderr, "Reading timeline \"%s\"...\n", tl_name);
    uint8_t index = project_add_timeline(proj_loc, tl_name);
    Timeline *tl = proj_loc->timelines[index];

    int16_t num_tracks;
    if (read_file_version_older_than("00.15")) {
	num_tracks = uint8_deser(f);
    } else {
	num_tracks = int16_deser_le(f);
    }
    fprintf(stderr, "Reading %d tracks...\n", num_tracks);
    if (read_file_version_older_than("00.15")) {
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
	    memset(hdr_buf, '\0', 4);
	    size_t bytes_read = fread(hdr_buf, 1, 4, f);
	    fseek(f, -1 * bytes_read, SEEK_CUR);
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

    timeline_reset(tl, true);
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
static int jdaw_read_effect(FILE *f, Track *track);


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


    uint8_t track_namelen = uint8_deser(f);
    char name[track_namelen + 1];
    fread(name, 1, track_namelen, f);
    if (read_file_version_older_than("00.15")) {
	char c;
	fread(&c, 1, 1, f);
    }
    name[track_namelen] = '\0';
    Track *track = timeline_add_track_with_name(tl, name, -1);
    /* Track *track = tl->tracks[tl->num_tracks - 1]; */
    fread(&track->color, 1, 4, f);

    char floatvals[17];
    if (read_file_version_older_than("00.15")) {
	fread(floatvals, 1, OLD_FLOAT_SER_W, f);
	floatvals[OLD_FLOAT_SER_W] = '\0';
	track->vol = atof(floatvals);
	fread(floatvals, 1, OLD_FLOAT_SER_W, f);
	track->pan = atof(floatvals);
    } else {
    	track->vol = float_deser40_le(f);
	track->pan = float_deser40_le(f);
    }
    track->vol_ctrl_val = pow(track->vol, 1.0 / VOL_EXP);
    
    bool muted = uint8_deser(f);
    bool solo = uint8_deser(f);
    bool solo_muted = uint8_deser(f);
    bool minimized = false;

    if (!read_file_version_older_than("00.16")) {
	minimized = uint8_deser(f);
    }
    
    if (muted) {
	track_mute(track);
    }
    if (solo) {
	track_solo(track);
    }
    if (solo_muted) {
	track_solomute(track);
    }
    if (minimized) {
	track_minimize(track);
    }
    
    uint16_t num_cliprefs;
    if (read_file_version_older_than("00.14")) {
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
    track->synth = synth_create(track);
    if (read_file_version_older_than("00.17")) {
	if (!read_file_version_older_than("00.13")) {
	    uint8_t num_automations = uint8_deser(f);
	    while (num_automations > 0) {
		if (jdaw_read_automation(f, track) != 0) {
		    return 1;
		}
		num_automations--;
	    }
	}
    }
    if (!read_file_version_older_than("00.11")) {
	if (read_file_version_older_than("00.17")) {
	    Effect *e;
	    uint8_t fir_filter_active = uint8_deser(f);
	    if (fir_filter_active) {
		uint8_t type_byte;
		FilterType type;
		double cutoff_freq;
		double bandwidth;
		uint16_t impulse_response_len;
	
		fread(&type_byte, 1, 1, f);
		type = (FilterType)type_byte;

		if (read_file_version_older_than("00.15")) {
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
		e = track_add_effect(track, EFFECT_FIR_FILTER);
		filter_set_impulse_response_len(e->obj, impulse_response_len);
		filter_set_params(e->obj, type, cutoff_freq, bandwidth);

	    } else {
		if (read_file_version_older_than("00.15")) {
		    fseek(f, 35, SEEK_CUR);
		} else {
		    fseek(f, 13, SEEK_CUR);
		}
	    }
	    uint8_t delay_line_active = uint8_deser(f);
	    if (delay_line_active) {
		int32_t len;
		double amp;
		double stereo_offset;
		if (read_file_version_older_than("00.15")) {
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
		e = track_add_effect(track, EFFECT_DELAY);
		delay_line_set_params(e->obj, amp, len);
		((DelayLine *)e->obj)->stereo_offset = stereo_offset;
	    } else {
		if (read_file_version_older_than("00.15")) {
		    fseek(f, 36, SEEK_CUR);
		} else {
		    fseek(f, 14, SEEK_CUR);
		}
	    }
	    if (!read_file_version_older_than("00.16")) {
		uint8_t saturation_active = uint8_deser(f);
		if (saturation_active) {
		    e = track_add_effect(track, EFFECT_SATURATION);
		    Saturation *s = e->obj;
		    saturation_set_gain(s, float_deser40_le(f));
		    s->do_gain_comp = uint8_deser(f);
		    saturation_set_type(s, uint8_deser(f));
		} else {
		    fseek(f, 7, SEEK_CUR);
		}
		
		uint8_t eq_active = uint8_deser(f);
		e = track_add_effect(track, EFFECT_EQ);
		EQ *eq = e->obj;
		eq->effect->active = eq_active;
		uint8_t num_filters = uint8_deser(f);
		for (int i=0; i<num_filters; i++) {
		    EQFilterCtrl *ctrl = eq->ctrls + i;
		    fprintf(stderr, "Reading filter %d/%d, ctrl %p\n", i,num_filters, ctrl);
		    ctrl->filter_active = uint8_deser(f);
		    IIRFilterType t = (int)uint8_deser(f);
		    eq->group.filters[i].type = t;
		    ctrl->freq_amp_raw[0] = float_deser40_le(f);
		    ctrl->freq_amp_raw[1] = float_deser40_le(f);
		    ctrl->bandwidth_scalar = float_deser40_le(f);
		    if (ctrl->filter_active) {
			eq_set_filter_from_ctrl(eq, i);
		    }
		}
	    }
	} else { /* READ FILE SPEC >= 00.17 */
	    uint8_t num_effects = uint8_deser(f);
	    for (int i=0; i<num_effects; i++) {
		int ret = jdaw_read_effect(f, track);
		if (ret != 0) {
		    return ret;
		}
	    }
	    uint8_t num_automations = uint8_deser(f);
	    while (num_automations > 0) {
		if (jdaw_read_automation(f, track) != 0) {
		    return 1;
		}
		num_automations--;
	    }
	    if (!read_file_version_older_than("00.18")) {
		uint8_t has_synth = uint8_deser(f);
		if (has_synth) {
		    fprintf(stderr, "Track \"%s\" has synth\n", track->name);
		    /* track->synth = synth_create(track); */
		    track->midi_out = track->synth;
		    track->midi_out_type = MIDI_OUT_SYNTH;
		    char buf[5];
		    fread(buf, 1, 5, f);
		    if (strncmp(buf, hdr_trck_synth, 5) != 0) {
			fprintf(stderr, "Error: \"SYNTH\" header not found\n");
			return 1;
		    }
		    fprintf(stderr, "\t->deserializing synth\n");
		    api_node_deserialize(f, &track->synth->api_node);
		}
	    }
	}	
    }
    return 0;
}

static int jdaw_read_fir_filter(FILE *f, FIRFilter *filter);
static int jdaw_read_delay(FILE *f, DelayLine *dl);
static int jdaw_read_saturation(FILE *f, Saturation *s);
static int jdaw_read_eq(FILE *f, EQ *eq);
static int jdaw_read_compressor(FILE *f, Compressor *c);


static int jdaw_read_effect(FILE *f, Track *track)
{
    char hdr_buffer[4];
    fread(hdr_buffer, 1, 4, f);
    if (strncmp(hdr_buffer, hdr_trck_efct, 4) != 0) {
	fprintf(stderr, "Error: .jdaw parsing error: \"EFCT\" indicator missing\n");
        return 1;
    }

    EffectType type = (EffectType)uint8_deser(f);
    Effect *e = track_add_effect(track, type);
    switch(type) {
    case EFFECT_EQ:
	jdaw_read_eq(f, e->obj);
	break;
    case EFFECT_FIR_FILTER:
	jdaw_read_fir_filter(f, e->obj);
	break;
    case EFFECT_DELAY:
	jdaw_read_delay(f, e->obj);
	break;
    case EFFECT_SATURATION:
	jdaw_read_saturation(f, e->obj);
	break;
    case EFFECT_COMPRESSOR:
	jdaw_read_compressor(f, e->obj);
	break;
    default:
	break;
    }
    return 0;
}

static int jdaw_read_fir_filter(FILE *f, FIRFilter *filter)
{
    filter->effect->active = uint8_deser(f);
    uint8_t type_byte;
    FilterType type;
    double cutoff_freq;
    double bandwidth;
    uint16_t impulse_response_len;
    fread(&type_byte, 1, 1, f);
    type = (FilterType)type_byte;
    cutoff_freq = float_deser40_le(f);
    bandwidth = float_deser40_le(f);
    impulse_response_len = uint16_deser_le(f);
    filter_set_impulse_response_len(filter, impulse_response_len);
    filter_set_params(filter, type, cutoff_freq, bandwidth);
    return 0;
}
static int jdaw_read_delay(FILE *f, DelayLine *dl)
{
    uint8_t delay_line_active = uint8_deser(f);
    int32_t len;
    double amp;
    double stereo_offset;
    len = int32_deser_le(f);
    stereo_offset = float_deser40_le(f);
    amp = float_deser40_le(f);
    delay_line_set_params(dl, amp, len);
    dl->len_msec = 1000 * len / session_get_sample_rate();
    dl->stereo_offset = stereo_offset;
    dl->effect->active = delay_line_active;
    return 0;
}
static int jdaw_read_saturation(FILE *f, Saturation *s)
{
    s->effect->active = uint8_deser(f);
    saturation_set_gain(s, float_deser40_le(f));
    s->do_gain_comp = uint8_deser(f);
    saturation_set_type(s, uint8_deser(f));
    return 0;
}
static int jdaw_read_eq(FILE *f, EQ *eq)
{
    eq->effect->active = uint8_deser(f);
    uint8_t num_filters = uint8_deser(f);
    for (int i=0; i<num_filters; i++) {
	EQFilterCtrl *ctrl = eq->ctrls + i;
	ctrl->filter_active = uint8_deser(f);
	IIRFilterType t = (int)uint8_deser(f);
	eq->group.filters[i].type = t;
	ctrl->freq_amp_raw[0] = float_deser40_le(f);
	ctrl->freq_amp_raw[1] = float_deser40_le(f);
	ctrl->bandwidth_scalar = float_deser40_le(f);
	if (ctrl->filter_active) {
	    eq_set_filter_from_ctrl(eq, i);
	}
    }
    return 0;
}

static int jdaw_read_compressor(FILE *f, Compressor *c)
{
    c->effect->active = uint8_deser(f);
    float attack = float_deser40_le(f);
    float release = float_deser40_le(f);
    c->attack_time = attack;
    c->release_time = release;
    compressor_set_times_msec(c, attack, release, c->effect->track->tl->proj->sample_rate);
    c->threshold = float_deser40_le(f);
    compressor_set_m(c, float_deser40_le(f));
    c->makeup_gain = float_deser40_le(f);
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

    ClipRef *cr = NULL;
    if (read_file_version_older_than("00.18")) {    
	uint8_t src_clip_index = uint8_deser(f);
	Clip *clip = track->tl->proj->clips[src_clip_index];
	cr = clipref_create(track, 0, CLIP_AUDIO, clip);
	strncpy(cr->name, clipref_name, clipref_namelen + 1);
    } else {
	enum clip_type t = uint8_deser(f);
	uint8_t src_clip_index = uint8_deser(f);
	switch (t) {
	case CLIP_AUDIO: {
	    Clip *clip = track->tl->proj->clips[src_clip_index];
	    cr = clipref_create(track, 0, CLIP_AUDIO, clip);
	}
	    break;
	case CLIP_MIDI: {
	    MIDIClip *mclip = track->tl->proj->midi_clips[src_clip_index];
	    cr = clipref_create(track, 0, CLIP_MIDI, mclip);
	}
	    break;
	default:
	    fprintf(stderr, "Error: source clip type %d not recognized\n", t);
	    return -1;
	}
    }
    if (!cr) {
	fprintf(stderr, "Unknown error: clipref not created\n");
	return -1;
    }
    cr->home = clipref_home;

    strncpy(cr->name, clipref_name, clipref_namelen + 1);
    cr->tl_pos = int32_deser_le(f);
    cr->start_in_clip = uint32_deser_le(f);
    cr->end_in_clip = uint32_deser_le(f);
    uint32_deser_le(f); /* start ramp len */
    uint32_deser_le(f); /* end ramp len */

    if (!read_file_version_older_than("00.20")) {
	cr->gain = float_deser40_le(f);
	cr->gain_ctrl = pow(cr->gain, 1.0 / VOL_EXP);
    }

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

    Automation *a = NULL;
    AutomationType t = (AutomationType)type_byte;
    if (t == AUTO_ENDPOINT) {
	uint8_t route_len = uint8_deser(f);
	char route[route_len + 1];	
	fread(route, 1, route_len, f);
	route[route_len] = '\0';

	/* fprintf(stderr, "AUTO ROUTE: %s\n", route); */
	Endpoint *ep = api_endpoint_get(route);
	if (ep) {
	    /* fprintf(stderr, "==> EP route? %s\n", ep->local_id); */
	    /* user_global_api_print_all_routes(NULL); */
	    a = track_add_automation_from_endpoint(track, ep);
	    /* exit(0); */
	} else {
	    fprintf(stderr, "Could not get ep with route %s\n", route);
	    return 1;
	}
	/* for (int i=0; i<track->api_node.num_endpoints; i++) { */
	/*     Endpoint *ep = track->api_node.endpoints[i]; */
	/*     int ep_loc_len = strlen(ep->local_id); */
	/*     if (ep_loc_len == ep_loc_id_read_len && strncmp(ep->local_id, loc_id, ep_loc_len) == 0) { */
	/* 	a = track_add_automation_from_endpoint(track, ep); */
	/* 	break; */
	/*     } */
	/* } */
	/* fwrite(a->endpoint->local_id, 1, ep_loc_id_len, f); */
    } else {
	a = track_add_automation(track, t);
    }
    if (!a) {
	return 1;
    }
    a->val_type = (ValType)uint8_deser(f);

    if (read_file_version_older_than("00.15")) {    
	a->min = jdaw_val_deserialize_OLD(f, OLD_FLOAT_SER_W, a->val_type);
	a->max = jdaw_val_deserialize_OLD(f, OLD_FLOAT_SER_W, a->val_type);
	a->range = jdaw_val_deserialize_OLD(f, OLD_FLOAT_SER_W, a->val_type);
    } else {
	a->min = jdaw_val_deserialize(f, NULL);
	a->max = jdaw_val_deserialize(f, NULL);
	a->range = jdaw_val_deserialize(f, NULL);
    }
    bool read = (bool)uint8_deser(f);
    /* TODO: figure out how to initialize this */
    a->shown = (bool)uint8_deser(f);
    if (a->shown) {
	automation_show(a);
    }
    if (!read) {
	automation_toggle_read(a);
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
    if (read_file_version_older_than("00.15")) {    
	val = jdaw_val_deserialize_OLD(f, OLD_FLOAT_SER_W, a->val_type);
    } else {
	val = jdaw_val_deserialize(f, NULL);
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
    endpoint_write(&ct->metronome_vol_ep, (Value){.float_v = metronome_vol}, false, false, false, false);
    
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
    float tempo;
    if (read_file_version_older_than("00.21")) {
	tempo = (float)int16_deser_le(f);;
    } else {
	tempo = float_deser40_le(f);
    }
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


/* #endif */


/* typedef struct project Project; */
/* int jdaw_read_file(Project *dst, const char *path) */
/* { */
/*     return 0; */
/* } */

/* void jdaw_write_project(const char *path) */
/* { */

/* } */
