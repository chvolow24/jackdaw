/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "dsp_utils.h"
#include "endpoint_callbacks.h"
#include "midi_objs.h"
#include "text.h"
#include "thread_safety.h"
#include "assets.h"
#include "color.h"
#include "components.h"
#include "input.h"
#include "layout.h"
#include "layout_xml.h"
#include "midi_clip.h"
#include "modal.h"
#include "project.h"
#include "session.h"
#include "tempo.h"
#include "timeline.h"
#include "timeview.h"
#include "user_event.h"
#include "wav.h"

#define MAX_BPM 999.0f
#define MIN_BPM 1.0f
#define DEFAULT_BPM 120.0f

extern Window *main_win;
extern struct colors colors;

/*------ debug utils -------------------------------------------------*/

static void click_track_pos_fprint(FILE *f, ClickTrackPos pos)
{
    fprintf(f, "%p -- %d.%d.%d.%d.%d:%f\n", pos.seg, pos.measure, pos.beat, pos.sd, pos.ssd, pos.sssd, pos.remainder);
}

/*------ init/deinit metronomes --------------------------------------*/

#define MAX_METRONOME_LEN_MSEC
#define METRONOME_STD_HIGH_INDEX 0
#define METRONOME_STD_LOW_INDEX 1
#define METRONOME_SNAP_INDEX 2
#define METRONOME_SNAP_CLOSE_INDEX 3

static MetronomeBuffer *session_add_metronome(const char *filepath)
{
    Session *session = session_get();
    if (session->num_metronome_buffers >= SESSION_MAX_METRONOME_BUFFERS) {
	return NULL;
    }
    float *buf;
    int32_t buf_len = wav_load(filepath, &buf, NULL);
    MetronomeBuffer *mb = session->metronome_buffers + session->num_metronome_buffers;
    mb->buf_len = buf_len;
    mb->buf = buf;
    mb->filepath = filepath;
    mb->used_in = 0;
    mb->name = mb->filepath;
    const char *last_slash_pos = NULL;
    while (mb->name[0] != '\0') {
	if (mb->name[0] == '/') last_slash_pos = mb->name;
	mb->name++;
    }
    if (last_slash_pos) mb->name = last_slash_pos + 1;
    session->num_metronome_buffers++;
    return mb;
}

void session_init_metronomes(Session *session)
{
    session_add_metronome(asset_get_abs_path(METRONOME_STD_HIGH_PATH));
    session_add_metronome(asset_get_abs_path(METRONOME_STD_LOW_PATH));
    session_add_metronome(asset_get_abs_path(METRONOME_SNAP_PATH));
    session_add_metronome(asset_get_abs_path(METRONOME_SNAP_CLOSE_PATH));    
}

void session_destroy_metronomes(Session *session)
{
    for (int i=0; i<session->num_metronome_buffers; i++) {
	/* Metronome *m = session->metronomes + i; */
	MetronomeBuffer *mb = session->metronome_buffers + i;
	if (mb->buf) free(mb->buf);
	mb->buf = NULL;
	mb->used_in = 0;
    }
}

static void click_track_configure_metronome(
    ClickTrack *ct,
    MetronomeBuffer *bp_measure_buf,
    MetronomeBuffer *bp_beat_buf,
    MetronomeBuffer *bp_offbeat_buf,
    MetronomeBuffer *bp_subdiv_buf)
{
    ct->metronome.bp_measure_buf = bp_measure_buf;
    ct->metronome.bp_beat_buf = bp_beat_buf;
    ct->metronome.bp_offbeat_buf = bp_offbeat_buf;
    ct->metronome.bp_subdiv_buf = bp_subdiv_buf;
}

/*------ get segment at pos ------------------------------------------*/

ClickSegment *click_track_get_segment_at_pos(ClickTrack *t, int32_t pos)
{
    ClickSegment *s = t->segments;
    if (!s) return NULL;
    
    while (1) {
	if (pos < s->start_pos) {
	    if (s->prev) {
		s = s->prev;
	    } else {
		return NULL;
	    }
	} else {
	    if (!s->next || pos < s->next->start_pos) {
		return s;
	    } else {
		s = s->next;
	    }
	}
    }
    /* fprintf(stderr, "Returning seg %d-%d (POS %d)\n", s->start_pos, s->end_pos, pos); */
    return s;
}

/*------ internal utils & tl pos <> ct pos translation ---------------*/

/* TODO: finer subdivs */
/* static int32_t get_beat_pos(ClickSegment *s, int measure, int beat, int subdiv) */
/* {     */
/*     int32_t pos = s->start_pos + (measure - s->first_measure_index) * s->cfg.dur_sframes; */
/*     for (int i=0; i<beat; i++) { */
/* 	pos += s->cfg.dur_sframes * s->cfg.beat_subdiv_lens[i] / s->cfg.num_atoms; */
/*     } */
/*     pos += s->cfg.dur_sframes * subdiv / s->cfg.num_atoms; */
/*     return pos; */
/* } */

static inline int32_t get_atom_len(ClickSegment *s)
{
    return s->cfg.dur_sframes / s->cfg.num_atoms;
}

static inline int32_t get_sd_len(ClickSegment *s, int beat)
{
    int atoms = s->cfg.beat_len_atoms[beat];
    int32_t atom_len_s = get_atom_len(s);
    if (atoms > 2 && atoms % 2 == 0) {
	return 2 * atom_len_s;
    } else {
	return atom_len_s;
    }
}

static ClickTrackPos click_track_get_pos(ClickTrack *ct, int32_t tl_pos);

/* public for piano roll, jlily */

void click_segment_get_durs_at(ClickTrack *ct, int32_t at, int32_t *measure_dur, int32_t *beat_dur, int32_t *subdiv_dur)
{
    if (!ct) { /* Default to 120bpm, 4/4 */
	int32_t measure_dur_sframes = 4 * 60 * session_get_sample_rate() / 120;
	/* int32_t beat_dur_sframes = measure_dur_sframes / 4; */
	if (measure_dur) {
	    *measure_dur = measure_dur_sframes;
	}
	if (beat_dur) {
	    *beat_dur = measure_dur_sframes / 4;
	}
	if (subdiv_dur) {
	    *subdiv_dur = measure_dur_sframes / 8;
	}
	return;
    }
    if (at < ct->segments->start_pos) {
	at = ct->segments->start_pos;
    }
    ClickTrackPos ctp = click_track_get_pos(ct, at);
    if (measure_dur) {
	*measure_dur = ctp.seg->cfg.dur_sframes;
    }
    if (beat_dur) {
	*beat_dur = ctp.seg->cfg.dur_sframes / ctp.seg->cfg.num_atoms * ctp.seg->cfg.beat_len_atoms[ctp.beat];
    }
    if (subdiv_dur) {
	*subdiv_dur = get_sd_len(ctp.seg, ctp.beat);
    }
}

static inline int beat_len_subdivs(ClickSegment *s, int beat)
{
    int atoms = s->cfg.beat_len_atoms[beat];
    if (atoms > 2 && atoms % 2 == 0) {
	return atoms / 2;
    } else {
	return atoms;
    }
}

static int32_t click_track_pos_get_remainder_samples(ClickTrackPos ctp, BeatProminence from)
{
    int32_t sd_len = get_sd_len(ctp.seg, ctp.beat);
    int32_t rem = ctp.remainder * sd_len;
    int32_t atom_len = get_atom_len(ctp.seg);
    switch (from) {
    case BP_SEGMENT:
	rem += (ctp.measure - ctp.seg->first_measure_index) * ctp.seg->cfg.dur_sframes;
    case BP_MEASURE:
	for (int i=0; i<ctp.beat; i++) {
	    rem += atom_len * ctp.seg->cfg.beat_len_atoms[i];
	}
    case BP_BEAT:
	rem += ctp.sd * sd_len;
    case BP_SD:
	rem += ctp.ssd * sd_len / 2;
    case BP_SSD:
	rem += ctp.sssd * sd_len / 4;
    case BP_SSSD:
	break;
    case BP_NONE:
	break;
    }
    return rem;
}

/* TODO: redundant with to/from ctpos functions */
static int32_t get_beat_pos(ClickTrackPos ct_pos)
{
    ClickSegment *seg = ct_pos.seg;
    if (!seg) return INT32_MIN;
    int32_t pos = seg->start_pos + (ct_pos.measure - seg->first_measure_index) * seg->cfg.dur_sframes;
    for (int i=0; i<ct_pos.beat; i++) {
	pos += seg->cfg.dur_sframes * seg->cfg.beat_len_atoms[i] / seg->cfg.num_atoms;	
    }
    int32_t sd_len = get_sd_len(seg, ct_pos.beat);
    pos += ct_pos.sd * sd_len;
    pos += ct_pos.ssd * sd_len / 2;
    pos += ct_pos.sssd * sd_len / 4;
    pos += ct_pos.remainder * sd_len;
    /* fprintf(stderr, "\tGBP: "); */
    /* click_track_pos_fprint(stderr, ct_pos); */
    /* fprintf(stderr, "\n\t\t=> %d\n", pos); */
    return pos;
}

static void do_increment(ClickTrackPos *ctp, BeatProminence bp)
{
    if (!ctp->seg) {
	fprintf(stderr, "Fatal error: click track 'do_increment' expects segment populated\n");
	TESTBREAK;
	exit(1);
    }
    /* ClickTrackPos start = *ctp; */
    switch (bp) {
    case BP_MEASURE:
	ctp->measure++;
	break;
    case BP_BEAT:
	ctp->beat++;
	break;
    case BP_SD:
	ctp->sd++;
	break;
    case BP_SSD:
	ctp->ssd++;
	break;
    case BP_SSSD:
	ctp->sssd++;
	break;
    default:
	return;
    }
    /* Do wrapping. no breaks */
    switch (bp) {
    case BP_SSSD:
	if (ctp->sssd >= 2) {
	    ctp->sssd = 0;
	    ctp->ssd++;
	}
    case BP_SSD:
	if (ctp->ssd >= 2) {
	    ctp->ssd = 0;
	    ctp->sd++;
	}
    case BP_SD:
	if (ctp->sd >= beat_len_subdivs(ctp->seg, ctp->beat)) {
	    ctp->sd = 0;
	    ctp->beat++;
	}
    case BP_BEAT:
	if (ctp->beat >= ctp->seg->cfg.num_beats) {
	    ctp->beat = 0;
	    ctp->measure++;
	}
    case BP_MEASURE:
	if (ctp->seg->num_measures > 0 && ctp->measure > ctp->seg->num_measures - ctp->seg->first_measure_index) {
	    ctp->seg = ctp->seg->next;
	}
    default:
	return;
    }
    
}

static void do_decrement(ClickTrackPos *ctp, BeatProminence bp)
{
    switch (bp) {
    case BP_MEASURE:
	ctp->measure--;
	break;
    case BP_BEAT:
	ctp->beat--;
	break;
    case BP_SD:
	ctp->sd--;
	break;
    case BP_SSD:
	ctp->ssd--;
	break;
    case BP_SSSD:
	ctp->sssd--;
	break;
    default:
	return;
    }
    /* Do wrapping. no breaks */
    switch (bp) {
    case BP_SSSD:
	if (ctp->sssd < 0) {
	    ctp->sssd = 1;
	    ctp->ssd--;
	}
    case BP_SSD:
	if (ctp->ssd < 0) {
	    ctp->ssd = 1;
	    ctp->sd--;
	}
    case BP_SD:
	if (ctp->sd < 0) {// beat_len_subdivs(ctp->seg, ctp->beat)) {
	    ctp->beat--;
	    if (ctp->beat < 0) {
		ctp->sd = beat_len_subdivs(ctp->seg, ctp->seg->cfg.num_beats - 1) - 1;
	    } else {
		ctp->sd = beat_len_subdivs(ctp->seg, ctp->beat) - 1;
	    }
	}
    case BP_BEAT:
	if (ctp->beat < 0) {
	    ctp->beat = ctp->seg->cfg.num_beats - 1;
	    ctp->measure--;
	}
    case BP_MEASURE:
	if (ctp->measure < ctp->seg->first_measure_index) {
	    if (ctp->seg->prev) {
		ctp->seg = ctp->seg->prev;
	    } else {
		ctp->seg = NULL;
	    }
	}
    default:
	return;
    }	

}

static BeatProminence get_beat_prominence(ClickTrackPos ctp)
{
    if (!ctp.seg) return BP_NONE;
    if (ctp.remainder != 0) return BP_NONE;
    if (ctp.sssd != 0) return BP_SSSD;
    if (ctp.ssd != 0) return BP_SSD;
    if (ctp.sd != 0) return BP_SD;
    if (ctp.beat != 0) return BP_BEAT;
    if (ctp.measure != ctp.seg->first_measure_index) return BP_MEASURE;
    else return BP_SEGMENT;    
}


/* Simple stateless version of click_track_bar_beat_subdiv() */
static ClickTrackPos click_track_get_pos(ClickTrack *ct, int32_t tl_pos)
{
    ClickTrackPos ret = {0};
    ret.seg = click_track_get_segment_at_pos(ct, tl_pos);
    /* fprintf(stderr, "GET POS %d =>Seg: %p\n", tl_pos, ret.seg); */
    if (!ret.seg) return ret;
    int32_t pos_in_seg = tl_pos - ret.seg->start_pos;
    ret.measure = (pos_in_seg / ret.seg->cfg.dur_sframes) + ret.seg->first_measure_index;
    int32_t beat_pos;
    do {
	beat_pos = get_beat_pos(ret);
	do_increment(&ret, BP_SSSD);
    } while (beat_pos < tl_pos);
    do_decrement(&ret, BP_SSSD);
    if (beat_pos > tl_pos)
	do_decrement(&ret, BP_SSSD);
    beat_pos = get_beat_pos(ret);
    ret.remainder = (double)(tl_pos - beat_pos) / get_sd_len(ret.seg, ret.beat);
    return ret;
}

/* Convert ClickTrackPos to raw tl pos (sframes) (for repositioning elements on tempo change, e.g.) */
int32_t click_track_pos_to_tl_pos(ClickTrackPos *ctp)
{
    /* return ctp->remainder * ctp->seg->cfg.dur_sframes / ctp->seg->cfg.num_atoms + get_beat_pos(ctp->seg, ctp->measure, ctp->beat, ctp->subdiv); */
    return get_beat_pos(*ctp);
}

static void click_track_set_readout_ctp(ClickTrack *ct, ClickTrackPos ctp)
{
    if (!ctp.seg) {
	snprintf(ct->pos_str, CLICK_POS_STR_LEN, "(-∞)");
	return;
    }
    static const int sample_str_len = 7;
    char sample_str[sample_str_len];
    /* if (remainder < -99999) { */
    /* 	snprintf(sample_str, sample_str_len, "%s", "-∞"); */
    /* } else if (remainder > 999999) { */
    /* 	snprintf(sample_str, sample_str_len, "%s", "∞"); */
    /* } else { */
    snprintf(sample_str, sample_str_len, "%d", (int32_t)(ctp.remainder * get_sd_len(ctp.seg, ctp.beat)));
    /* } */
    snprintf(ct->pos_str, CLICK_POS_STR_LEN, "%d.%d.%d:%s", ctp.measure, 1 + ctp.beat, 1 + ctp.sd * 4 + ctp.ssd * 2 + ctp.sssd, sample_str);
    textbox_reset_full(ct->readout);
}

void click_track_set_readout(ClickTrack *ct, int32_t tl_pos)
{
    ClickTrackPos ctp = click_track_get_pos(ct, tl_pos);
    click_track_set_readout_ctp(ct, ctp);
}

/* Quickly increment over subdivs positions on a track (BP_SSD) */
static void click_track_get_next_pos(ClickTrack *t, bool start, int32_t start_from, int32_t *pos_dst, BeatProminence *bp, ClickSegment **seg_dst)
{
    static JDAW_THREAD_LOCAL ClickTrackPos ctp = {0};

    int32_t beat_pos;
    if (start) {
	ctp = click_track_get_pos(t, start_from);
	if (!ctp.seg) {
	    ctp.seg = t->segments;
	    ctp.measure = ctp.seg->first_measure_index;
	}
	bool on_beat = true;
	if (ctp.remainder) {
	    ctp.remainder = 0;
	    on_beat = false;
	}
	if (ctp.sssd) {
	    ctp.sssd = 0;
	    on_beat = false;
	}
	if (!on_beat) {
	    do_increment(&ctp, BP_SSD);
	}
	beat_pos = get_beat_pos(ctp);
    } else {
	do_increment(&ctp, BP_SSD);
	beat_pos = get_beat_pos(ctp);
	if (ctp.seg && ctp.seg->next && beat_pos >= ctp.seg->next->start_pos) {
	    ctp.seg = ctp.seg->next;
	    ctp.measure = ctp.seg->first_measure_index;
	    ctp.beat = 0;
	    ctp.sd = 0;
	    ctp.ssd = 0;
	    ctp.sssd = 0;
	    ctp.remainder = 0.0;
	    beat_pos = ctp.seg->start_pos;
	}
    }
    if (pos_dst) {
	*pos_dst = beat_pos;
    }
    if (seg_dst) {
	*seg_dst = ctp.seg;
    }
    if (bp) {
	*bp = get_beat_prominence(ctp);
    }
    /* if (start) { */
    /* 	fprintf(stderr, "Got next pos from %d\t", start_from); */
    /* 	click_track_pos_fprint(stderr, ctp); */
    /* } */
}


/*------ simple segment modifications --------------------------------*/

void click_segment_set_start_pos(ClickSegment *s, int32_t new_start_pos)
{
    int32_t diff = new_start_pos - s->start_pos;
    s->start_pos = new_start_pos;
    s->start_pos_internal = new_start_pos;
    if (s->prev) {
	int32_t prev_dur = new_start_pos - s->start_pos;
	double num_measures = (double)prev_dur / s->prev->cfg.dur_sframes;
	s->prev->num_measures = floor(0.9999999 + num_measures);
    }
    if (s->next) {
	click_segment_set_start_pos(s->next, s->next->start_pos + diff);
    }
}

/* Called when a new config is specified on the click track page, or when the tempo is changed */
void click_segment_set_config(ClickSegment *s, int num_measures, float bpm, uint8_t num_beats, uint8_t *beat_lens_atoms, enum ts_end_bound_behavior ebb)
{
    if (num_beats > MAX_BEATS_PER_BAR) {
	fprintf(stderr, "Error: num_beats exceeds maximum per bar (%d)\n", MAX_BEATS_PER_BAR);
	return;
    }
    s->cfg.bpm = bpm;
    s->cfg.num_beats = num_beats;

    /* First, factor out GCD (if 2) from subidivs */
    bool div2 = true;
    for (int i=0; i<num_beats; i++) {
	if (beat_lens_atoms[i] > 2 && beat_lens_atoms[i] % 2 == 0) {
	    continue;
	} else {
	    div2 = false;
	    break;
	}
    }
    if (div2) {
	for (int i=0; i<num_beats; i++) {
	    beat_lens_atoms[i] /= 2;
	}
    }
    
    

    int32_t old_dur_sframes = s->cfg.dur_sframes;
    double beat_dur = s->track->tl->proj->sample_rate * 60.0 / bpm;
    int min_beat_len_atoms = 0;
    s->cfg.num_atoms = 0;
    for (int i=0; i<num_beats; i++) {
	int beat_len_atoms = beat_lens_atoms[i];
	s->cfg.beat_len_atoms[i] = beat_len_atoms;
	s->cfg.num_atoms += beat_len_atoms;
	if (i == 0 || s->cfg.beat_len_atoms[i] < min_beat_len_atoms) {
	    min_beat_len_atoms = s->cfg.beat_len_atoms[i];
	}
    }
    double measure_dur = 0;
    for (int i=0; i<num_beats; i++) {
	measure_dur += beat_dur * s->cfg.beat_len_atoms[i] / min_beat_len_atoms;
    }
				   
    s->cfg.dur_sframes = (int32_t)(round(measure_dur));
    s->cfg.atom_dur_approx = measure_dur / s->cfg.num_atoms;

    if (!s->next) {
	/* s->end_pos = s->start_pos; */
	/* s->end_pos_internal = s->start_pos; */
	s->num_measures = -1;
    } else {
	if (ebb == SEGMENT_FIXED_NUM_MEASURES) {
	    double num_measures_f = (double)(s->next->start_pos - s->start_pos)/old_dur_sframes;
	    int32_t end_pos = s->start_pos + num_measures_f * s->cfg.dur_sframes;
	    click_segment_set_start_pos(s->next, end_pos);
	} else {
	    click_segment_set_start_pos(s->next, s->next->start_pos);
	}
    }
}

/*------ stash obj ct positions, and reset from stash ----------------*/

enum moved_obj_type {
    MOV_OBJ_AUDIO_CLIP,
    MOV_OBJ_MIDI_CLIP,
    MOV_OBJ_MIDI_NOTE,
    MOV_OBJ_MIDI_PB,
    MOV_OBJ_MIDI_CC,
    MOV_OBJ_AUTOMATION_KF
};

struct note_clip_id {
    ClipRef *cr;
    uint32_t note_id;
};

struct pitch_bend_id {
    ClipRef *cr;
    int32_t index;
};

union moved_obj_id {
    struct note_clip_id note;
    struct pitch_bend_id pb;
    void *obj;
};

struct moved_obj {
    enum moved_obj_type type;
    union moved_obj_id id;
    /* void *obj; */
    ClickTrackPos pos;
    ClickTrackPos end_pos;
};

struct move_stash {
    uint32_t num_objs;
    uint32_t alloc_len;
    struct moved_obj *objs;
    double len_prop;
    MIDIClip *midi_clips_resized[MAX_PROJ_MIDI_CLIPS];
    uint16_t num_midi_clips_resized;
};

static struct move_stash move_stash;

#define MOVE_STASH_INIT_ALLOC_LEN 64

static void clear_move_stash()
{
    if (move_stash.objs) {
	free(move_stash.objs);
	move_stash.objs = NULL;
    }
    move_stash.alloc_len = MOVE_STASH_INIT_ALLOC_LEN;
    move_stash.num_objs = 0;
}

static void init_move_stash()
{
    clear_move_stash();
    move_stash.alloc_len = MOVE_STASH_INIT_ALLOC_LEN;
    move_stash.objs = malloc(move_stash.alloc_len * sizeof(struct moved_obj));
    move_stash.num_midi_clips_resized = 0;
}

static void stash_obj(union moved_obj_id obj_id, ClickTrackPos pos, ClickTrackPos opt_end_pos, enum moved_obj_type type)
{
    /* resize array if necessary */
    if (move_stash.num_objs + 1 >= move_stash.alloc_len) {
	move_stash.alloc_len *= 2;
	move_stash.objs = realloc(move_stash.objs, move_stash.alloc_len * sizeof(struct moved_obj));
    }
    move_stash.objs[move_stash.num_objs].type = type;
    move_stash.objs[move_stash.num_objs].id = obj_id;
    move_stash.objs[move_stash.num_objs].pos = pos;
    move_stash.objs[move_stash.num_objs].end_pos = opt_end_pos;
    move_stash.num_objs++;
}

static void reset_obj_position(struct moved_obj *obj)
{
    ClickTrackPos *ct_pos = &obj->pos;
    int32_t tl_pos = click_track_pos_to_tl_pos(ct_pos);
    switch (obj->type) {
    case MOV_OBJ_AUDIO_CLIP: {
	ClipRef *cr = obj->id.obj;
	cr->tl_pos = tl_pos;
	clipref_reset(cr, false);
    }
	break;
    case MOV_OBJ_MIDI_CLIP: {
	ClipRef *cr = obj->id.obj;
	cr->tl_pos = tl_pos;
	cr->start_in_clip *= move_stash.len_prop;
	cr->end_in_clip *= move_stash.len_prop;
	MIDIClip *mclip = cr->source_clip;
	bool midi_clip_resized = false;
	for (int i=0; i<move_stash.num_midi_clips_resized; i++) {
	    if (move_stash.midi_clips_resized[i] == mclip) {
		midi_clip_resized = true;
		break;
	    }
	}
	if (!midi_clip_resized) {
	    mclip->len_sframes *= move_stash.len_prop;
	    move_stash.midi_clips_resized[move_stash.num_midi_clips_resized] = mclip;
	    move_stash.num_midi_clips_resized++;
	}
	/* cr->start_in_clip += start_move_by; */
	/* cr->end_in_clip += end_move_by; */
	clipref_reset(cr, false);
    }
	break;
    case MOV_OBJ_MIDI_NOTE: {
	int32_t end_tl_pos = click_track_pos_to_tl_pos(&obj->end_pos);
	ClipRef *cr = obj->id.note.cr;
	MIDIClip *mclip = cr->source_clip;
	uint32_t note_id = obj->id.note.note_id;
	Note *note = mclip->notes + midi_clip_get_note_by_id(mclip, note_id);
	note->start_rel = tl_pos - cr->tl_pos + cr->start_in_clip;
	note->end_rel = end_tl_pos - cr->tl_pos + cr->start_in_clip;
	midi_clip_check_reset_bounds(mclip);
    }	
	break;
    case MOV_OBJ_MIDI_PB: {
	struct pitch_bend_id id = obj->id.pb;
	ClipRef *cr = id.cr;
	MIDIClip *mclip = cr->source_clip;
	MEvent16bit *e = mclip->pitch_bend.changes + id.index;
	/* fprintf(stderr, "RESET pb %d pos %d->%d\n", id.index, e->pos_rel, tl_pos ); */
	e->pos_rel = tl_pos - cr->tl_pos + cr->start_in_clip;
    }
	break;
    case MOV_OBJ_MIDI_CC:
	break;
    case MOV_OBJ_AUTOMATION_KF: {
	Keyframe *kf = obj->id.obj;
	automation_keyframe_reset_pos(kf, tl_pos);
	if (kf->pos != tl_pos) exit(1);
    }
	break;	
    }
}
    
static void reset_positions_from_stash()
{
    for (uint32_t i=0; i<move_stash.num_objs; i++) {
	reset_obj_position(move_stash.objs + i);
    }
    clear_move_stash();
}

static void stash_all_object_positions_track(Track *track, ClickSegment *seg)
{
    
    ClickTrack *ct = seg->track;
    for (int i=0; i<track->num_clips; i++) {
	ClipRef *cr = track->clips[i];
	ClickTrackPos clip_pos = click_track_get_pos(ct, track->clips[i]->tl_pos);
	if (!clip_pos.seg) continue;
	union moved_obj_id id = {.obj = cr};
	enum moved_obj_type type = cr->type == CLIP_AUDIO ? MOV_OBJ_AUDIO_CLIP : MOV_OBJ_MIDI_CLIP;
	stash_obj(id, clip_pos, (ClickTrackPos){0}, type);
	if (cr->type == CLIP_MIDI) {
	    MIDIClip *mclip = cr->source_clip;
	    int32_t first_note = midi_clipref_check_get_first_note(cr);
	    if (first_note == -1) continue;
	    int32_t last_note = midi_clipref_check_get_last_note(cr);
	    for (int32_t i=first_note; i<=last_note; i++) {
		Note *note = mclip->notes + i;
		union moved_obj_id id = {.note = {.cr = cr, .note_id = note->id}};
		ClickTrackPos note_start = click_track_get_pos(ct, note_tl_start_pos(note, cr));
		ClickTrackPos note_end = click_track_get_pos(ct, note_tl_end_pos(note, cr));
		stash_obj(id, note_start, note_end, MOV_OBJ_MIDI_NOTE);
	    }
	    for (int32_t i=0; i<mclip->pitch_bend.num_changes; i++) {
		MEvent16bit *event = mclip->pitch_bend.changes + i;
		if (event->pos_rel >= cr->start_in_clip && event->pos_rel <= cr->end_in_clip) {
		    struct pitch_bend_id pb_id;
		    pb_id.cr = cr;
		    pb_id.index = i;
		    union moved_obj_id id = {.pb = pb_id};
		    ClickTrackPos pos = click_track_get_pos(ct, event->pos_rel + cr->tl_pos - cr->start_in_clip);
		    stash_obj(id, pos, (ClickTrackPos){0}, MOV_OBJ_MIDI_PB);
		}
	    }
	}

    }
    for (int i=0; i<track->num_automations; i++) {
	Automation *a = track->automations[i];
	for (int i=0; i<a->num_keyframes; i++) {
	    Keyframe *k = a->keyframes + i;
	    /* if (k->pos >= seg->start_pos && */
	    /* 	(!seg->next || k->pos <= seg->next->start_pos)) { */
	    union moved_obj_id id;
	    ClickTrackPos ct_pos = click_track_get_pos(ct, k->pos);
	    if (!ct_pos.seg) continue;
	    id.obj = k;
	    stash_obj(id, ct_pos, (ClickTrackPos){0}, MOV_OBJ_AUTOMATION_KF);
	    /* } */
	}
    }
}

static void stash_all_object_positions(ClickSegment *seg, double len_prop)
{
    ClickTrack *ct = seg->track;
    init_move_stash();
    move_stash.len_prop = len_prop;
    for (int i=0; i<ct->tl->num_tracks; i++) {
	Track *track = ct->tl->tracks[i];
	if (track_governing_click_track(track) != ct) continue;
	stash_all_object_positions_track(track, seg);
    }
}

/*------ bpm endpoint callback ---------------------------------------*/

static void bpm_proj_cb(Endpoint *ep)
{
    ClickSegment *s = ep->xarg1;
    double len_prop = ep->last_write_val.float_v / ep->current_write_val.float_v;
    stash_all_object_positions(s, len_prop);
    click_segment_set_config(s, s->num_measures, ep->current_write_val.float_v, s->cfg.num_beats, s->cfg.beat_len_atoms, s->track->end_bound_behavior);
    if (session_get()->dragged_component.component == s) {
	label_move(s->bpm_label, main_win->mousep.x, s->track->layout->rect.y - 20);
    }
    label_reset(s->bpm_label, ep->current_write_val);
    reset_positions_from_stash();
}

/*------ public: add segments ----------------------------------------*/

ClickSegment *click_track_add_segment(ClickTrack *t, int32_t start_pos, int16_t num_measures, float bpm, uint8_t num_beats, uint8_t *subdiv_lens)
{
    ClickSegment *s = calloc(1, sizeof(ClickSegment));
    s->track = t;
    s->num_measures = num_measures;
    s->start_pos = start_pos;
    s->start_pos_internal = start_pos;
    endpoint_init(
	&s->start_pos_ep,
	&s->start_pos_internal,
	JDAW_INT32,
	"start_pos",
	"undo/redo move click segment boundary",
	JDAW_THREAD_MAIN,
	NULL, click_segment_bound_proj_cb, NULL,
	(void *)s, NULL, NULL, NULL);
    endpoint_init(
	&s->bpm_ep,
	&s->cfg.bpm,
	JDAW_FLOAT,
	"bpm",
	"BPM",
	JDAW_THREAD_MAIN,
	NULL, bpm_proj_cb, NULL,
	s, NULL, NULL, NULL);
    endpoint_set_default_value(&s->bpm_ep, (Value){.float_v = DEFAULT_BPM});
    endpoint_set_allowed_range(&s->bpm_ep, (Value){.float_v = MIN_BPM}, (Value){.float_v=MAX_BPM});
	

    s->bpm_label = label_create(BPM_STRLEN, t->layout, label_bpm, &s->cfg.bpm, JDAW_FLOAT, main_win);
    if (!t->segments) {
	t->segments = s;
	/* s->end_pos = s->start_pos; */
	s->first_measure_index = BARS_FOR_NOTHING * -1;
	goto set_config_and_exit;
    }
    ClickSegment *interrupt = click_track_get_segment_at_pos(t, start_pos);
    if (interrupt->start_pos == start_pos) {
	fprintf(stderr, "Error: cannot insert segment at existing segment loc\n");
	free(s);
	return NULL;
    }
    if (!interrupt->next) {
	interrupt->next = s;
	s->prev = interrupt;
	click_segment_set_start_pos(s, start_pos);
    } else {
	s->next = interrupt->next;
	interrupt->next->prev = s;
	interrupt->next = s;
	s->prev = interrupt;
	click_segment_set_config(s, num_measures, bpm, num_beats, subdiv_lens, SEGMENT_FIXED_END_POS);
	click_segment_set_start_pos(s, start_pos);
	return s;
    }
set_config_and_exit:
    click_segment_set_config(s, num_measures, bpm, num_beats, subdiv_lens, SEGMENT_FIXED_END_POS); 
    return s;
}

ClickSegment *click_track_add_segment_at_measure(ClickTrack *t, int16_t measure, int16_t num_measures, int bpm, uint8_t num_beats, uint8_t *subdiv_lens)
{
    int32_t start_pos = 0;
    ClickSegment *s = t->segments;
    if (s) {
	while (1) {

	    if (s->first_measure_index > measure) {
		if (s->prev) {
		    s = s->prev;
		} else {
		    fprintf(stderr, "ERROR: attempting to insert before first measure index at head\n");
		    exit(1);
		}
		break;
	    } else if (!s->next) {
		break;
	    }
	    s = s->next;
	}
	ClickTrackPos measure_pos = {
	    .seg = s,
	    .measure = measure,
	    0
	};
	start_pos = get_beat_pos(measure_pos);
	/* start_pos = get_beat_pos(s, measure, 0, 0); */
    }
    s = click_track_add_segment(t, start_pos, num_measures, bpm, num_beats, subdiv_lens);
    return s;
}

/*------ click track insertion/deletion and supporting ---------------*/

static void click_track_remove(ClickTrack *tt)
{
    Timeline *tl = tt->tl;
    for (int i=tt->index; i<tl->num_click_tracks - 1; i++) {
	tl->click_tracks[i] = tl->click_tracks[i+1];
    }
    tl->num_click_tracks--;
    layout_remove_child(tt->layout);
    timeline_rectify_track_indices(tl);
    timeline_rectify_track_area(tl);

}

static void click_track_reinsert(ClickTrack *tt)
{
    Timeline *tl = tt->tl;
    for (int i=tl->num_click_tracks; i>tt->index; i--) {
	tl->click_tracks[i] = tl->click_tracks[i-1];
    }
    tl->click_tracks[tt->index] = tt;
    tl->num_click_tracks++;
    layout_insert_child_at(tt->layout, tl->track_area, tt->layout->index);
    tl->layout_selector = tt->layout->index;
    timeline_rectify_track_indices(tl);
    timeline_rectify_track_area(tl);
}

NEW_EVENT_FN(undo_add_click_track, "undo add tempo track")
    ClickTrack *tt = obj1;
    click_track_remove(tt);

}
NEW_EVENT_FN(redo_add_click_track, "redo add tempo track")
    ClickTrack *tt = obj1;
    click_track_reinsert(tt);
}
 
NEW_EVENT_FN(dispose_forward_add_click_track, "")
    click_track_destroy((ClickTrack *)obj1);
}

ClickTrack *timeline_add_click_track(Timeline *tl)
{
    if (tl->num_click_tracks == MAX_CLICK_TRACKS) return NULL;
    ClickTrack *t = calloc(1, sizeof(ClickTrack));
    snprintf(t->name, MAX_NAMELENGTH, "click_track_%d", tl->num_click_tracks + 1);
    t->tl = tl;
    Session *session = session_get();

    click_track_configure_metronome(
	t,
	session->metronome_buffers + METRONOME_STD_HIGH_INDEX,
	session->metronome_buffers + METRONOME_STD_LOW_INDEX,
	NULL, NULL);
	
				     
    /* t->metronome = &session->metronomes[0]; */
    snprintf(t->num_beats_str, 3, "4");
    for (int i=0; i<MAX_BEATS_PER_BAR; i++) {
	snprintf(t->subdiv_len_strs[i], 2, "4");
    }    

    endpoint_init(
	&t->metronome.vol_ep,
	&t->metronome.vol,
	JDAW_FLOAT,
	"metro_vol",
	"undo/redo adj metronome vol",
	JDAW_THREAD_MAIN,
	track_slider_cb, NULL, NULL,
	(void *)&t->metronome_vol_slider, (void *)tl, NULL, NULL);
    endpoint_set_allowed_range(
	&t->metronome.vol_ep,
	(Value){.float_v = 0.0f},
	(Value){.float_v = TRACK_VOL_MAX});

    endpoint_init(
	&t->end_bound_behavior_ep,
	&t->end_bound_behavior,
	JDAW_INT,
	"segment_end_bound_behavior",
	"click track segment end-bound behavior",
	JDAW_THREAD_MAIN,
        click_track_ebb_gui_cb, NULL, NULL,
	NULL, NULL, NULL, NULL);
    
    endpoint_set_allowed_range(
	&t->end_bound_behavior_ep,
	(Value){.int_v = 0},
	(Value){.int_v = 1});

    
    /* Layout *click_tracks_area = layout_get_child_by_name_recursive(tl->layout, "tracks_area"); */
    Layout *click_tracks_area = tl->track_area;
    if (!click_tracks_area) {
	fprintf(stderr, "Error: no tempo tracks area\n");
	exit(1);
    }
    /* Layout *lt = layout_read_xml_to_lt(click_tracks_area, CLICK_TRACK_LT_PATH); */
    Layout *lt = layout_read_from_xml(CLICK_TRACK_LT_PATH);
    if (tl->layout_selector >= 0) {
	layout_insert_child_at(lt, click_tracks_area, tl->layout_selector);
    } else {
	layout_insert_child_at(lt, click_tracks_area, 0);
    }

    t->layout = lt;
    layout_size_to_fit_children_v(click_tracks_area, true, 0);
    layout_reset(tl->track_area);
    tl->needs_redraw = true;

    layout_force_reset(lt);

    Layout *tb_lt = layout_get_child_by_name_recursive(t->layout, "display");
    snprintf(t->pos_str, CLICK_POS_STR_LEN, "0.0.0:00000");
    t->readout = textbox_create_from_str(
	t->pos_str,
	tb_lt,
	main_win->mono_bold_font,
	14,
	main_win);
    textbox_set_align(t->readout, CENTER_LEFT);
    textbox_set_background_color(t->readout, &colors.black);
    textbox_set_text_color(t->readout, &colors.white);
    /* textbox_size_to_fit_v(t->readout, 4); */
    /* layout_center_agnostic(t->readout->layout, false, true); */
    textbox_set_pad(t->readout, 10, 0);
    textbox_set_trunc(t->readout, false);
    textbox_reset_full(t->readout);

    t->colorbar_rect = &layout_get_child_by_name_recursive(t->layout, "colorbar")->rect;
    t->console_rect = &layout_get_child_by_name_recursive(t->layout, "console")->rect;
    t->right_console_rect = &layout_get_child_by_name_recursive(t->layout, "right_console")->rect;
    t->right_colorbar_rect = &layout_get_child_by_name_recursive(t->layout, "right_colorbar")->rect;

    Layout *edit_button_lt = layout_get_child_by_name_recursive(t->layout, "edit_button");
    t->edit_button = textbox_create_from_str(
	"✎",
	edit_button_lt,
	main_win->symbolic_font,
	16,
	main_win);
    t->edit_button->corner_radius = MUTE_SOLO_BUTTON_CORNER_RADIUS;
    textbox_set_text_color(t->edit_button, &colors.white);
    textbox_set_background_color(t->edit_button, &colors.quickref_button_blue);
    /* textbox_set_border(t->edit_button, &colors.black, 1); */
    textbox_set_border(t->edit_button, &colors.white, 1, BUTTON_CORNER_RADIUS);
    
    Layout *metro_button_lt = layout_get_child_by_name_recursive(t->layout, "metronome_button");
    t->metronome_button = textbox_create_from_str(
	"M",
	metro_button_lt,
	main_win->bold_font,
	14,
	main_win);
    textbox_set_background_color(t->metronome_button, &colors.play_green);
    textbox_set_border(t->metronome_button, &colors.black, 1, MUTE_SOLO_BUTTON_CORNER_RADIUS);
    /* textbox_set_background_color(track->tb_mute_button, &color_mute_solo_grey); */



    t->metronome.vol = 1.0f;
	
    Layout *vol_lt = layout_get_child_by_name_recursive(t->layout, "metronome_vol_slider");
    t->metronome_vol_slider = slider_create(
	vol_lt,
	&t->metronome.vol_ep,
	(Value){.float_v = 0.0f},
	(Value){.float_v = TRACK_VOL_MAX},
	SLIDER_HORIZONTAL,
	SLIDER_FILL,
	NULL,
	/* NULL, */
	/* /\* &slider_label_amp_to_dbstr, *\/ */
	/* NULL, */
	/* NULL, */
	/* NULL, */
	&session->dragged_component);
    slider_reset(t->metronome_vol_slider);
    /* Value min, max; */
    /* min.float_v = 0.0f; */
    /* max.float_v = TRACK_VOL_MAX; */
    /* slider_set_range(t->metronome_vol_slider, min, max); */
    /* slider_reset(t->metronome_vol_slider); */

    uint8_t subdivs[] = {4, 4, 4, 4};
    click_track_add_segment(t, 0, -1, DEFAULT_BPM, 4, subdivs);
    
    tl->click_tracks[tl->num_click_tracks] = t;
    t->index = tl->num_click_tracks;
    tl->num_click_tracks++;
    snprintf(t->name, MAX_NAMELENGTH, "Click track %d\n", t->index + 1);

    timeline_rectify_track_indices(tl);
    timeline_rectify_track_area(tl);
    
    /* timeline_reset(tl, false); */
    /* layout_force_reset(tl->layout); */
    /* layout_reset(lt); */
    Value nullval = {.int_v = 0};
    user_event_push(
	undo_add_click_track,
	redo_add_click_track,
	NULL,
	dispose_forward_add_click_track,
	(void *)t, NULL,
	nullval, nullval,
	nullval, nullval,
	0, 0, false, false);


    /* timeline_reset(tl, false); */
    return t;
}

/*------ deallocators ------------------------------------------------*/

void click_segment_destroy(ClickSegment *s)
{
    label_destroy(s->bpm_label);
    free(s);
}

void click_track_destroy(ClickTrack *tt)
{
    textbox_destroy(tt->metronome_button);
    textbox_destroy(tt->edit_button);
    slider_destroy(tt->metronome_vol_slider);
    
    ClickSegment *s = tt->segments;
    while (s) {
	ClickSegment *next = s->next;
	click_segment_destroy(s);
	s = next;
    }
    layout_destroy(tt->layout);
}

/*------ higher-level user interface functions -----------------------*/

static int set_tempo_submit_form(void *mod_v, void *target)
{
    Modal *mod = (Modal *)mod_v;
    ClickSegment *s = (ClickSegment *)mod->stashed_obj;
    for (uint8_t i=0; i<mod->num_els; i++) {
	ModalEl *el = mod->els[i];
	if (el->type == MODAL_EL_TEXTENTRY) {
	    char *value = ((TextEntry *)el->obj)->tb->text->value_handle;
	    /* TODO: FIX THIS */
	    /* float tempo = atoi(value); */
	    float tempo = txt_float_from_str(value);
	    endpoint_write(&s->bpm_ep, (Value){.float_v = tempo}, true, true, true, true);
	    /* click_segment_set_tempo(s, tempo, s->track->end_bound_behavior, false); */
	    break;
	}
    }
    window_pop_modal(main_win);
    Session *session = session_get();
    ACTIVE_TL->needs_redraw = true;
    return 0;
}

#define TEMPO_STRLEN 8
void timeline_click_track_set_tempo_at_cursor(Timeline *tl)
{
    ClickTrack *tt = timeline_selected_click_track(tl);
    if (!tt) {
	status_set_errstr("No tempo track selected");
	return;
    }
    ClickSegment *s = click_track_get_segment_at_pos(tt, tl->play_pos_sframes);
    if (!s) {
	s = tt->segments;
    }
    Layout *mod_lt = layout_add_child(main_win->layout);
    layout_set_default_dims(mod_lt);
    Modal *mod = modal_create(mod_lt);
    static char tempo_str[TEMPO_STRLEN];
    snprintf(tempo_str, TEMPO_STRLEN, "%f", s->cfg.bpm);
    modal_add_header(mod, "Set tempo:", &colors.light_grey, 4);
    ModalEl *el = modal_add_textentry(
	mod,
	tempo_str,
	TEMPO_STRLEN,
	txt_float_validation,
	/* txt_integer_validation, */
	NULL, NULL);
    el->layout->y.value += 15.0;
    TextEntry *te = (TextEntry *)el->obj;
    te->target = (void *)mod;
    te->tb->text->completion = NULL;
    te->tb->text->completion_target = NULL;

    if (s->next) {
	char opt1[64];
	char opt2[64];
	char timestr[64];
	timecode_str_at(tt->tl, timestr, 64, s->next->start_pos);
	snprintf(opt1, 64, "Fixed end pos (%s)", timestr);

	if (s->cfg.dur_sframes * s->num_measures == s->next->start_pos - s->start_pos) {
	    snprintf(opt2, 64, "Fixed num measures (%d)", s->num_measures);
	} else {
	    snprintf(opt2, 64, "Fixed num measures (%f)", (float)(s->next->start_pos - s->start_pos)/s->cfg.dur_sframes);
	}
	char *options[] = {opt1, opt2};

	
	/* TODO: FIX THIS */
	el = modal_add_radio(
	    mod,
	    &colors.white,
	    /* NULL, */
	    &tt->end_bound_behavior_ep,
	    /* &tt->end_bound_behavior, */
	    /* tempo_rb_action, */
	    (const char **)options,
	    2);	

	radio_button_reset_from_endpoint(el->obj);
	/* te->tb->text->max_len = TEMPO_STRLEN; */
    }
    mod->layout->w.value = 450;

    
    el = modal_add_button(
	mod,
	"Submit",
	set_tempo_submit_form);
    ((Button *)el->obj)->target = s;

    layout_reset(mod->layout);
    layout_center_agnostic(el->layout, true, false);

    mod->stashed_obj = (void *)s;
    mod->submit_form = set_tempo_submit_form;
    window_push_modal(main_win, mod);

    modal_reset(mod);
    modal_move_onto(mod);
    tl->needs_redraw = true;
 }

static void click_track_delete(ClickTrack *tt, bool from_undo);

NEW_EVENT_FN(undo_delete_click_track, "undo delete tempo track")
    ClickTrack *tt = (ClickTrack *)obj1;
    click_track_reinsert(tt);
}

NEW_EVENT_FN(redo_delete_click_track, "redo delete tempo track")
    ClickTrack *tt = (ClickTrack *)obj1;
    click_track_delete(tt, true);
}

NEW_EVENT_FN(dispose_delete_click_track, "")
    ClickTrack *tt = (ClickTrack *)obj1;
    click_track_destroy(tt);
}

static void click_track_delete(ClickTrack *tt, bool from_undo)
{
    click_track_remove(tt);
    Value nullval = {.int_v = 0};
    if (!from_undo) {
	user_event_push(
	    undo_delete_click_track,
	    redo_delete_click_track,
	    dispose_delete_click_track,
	    NULL,
	    (void *)tt, NULL,
	    nullval, nullval, nullval, nullval,
	    0, 0, false, false);	    
    }
}

bool timeline_click_track_delete(Timeline *tl)
{
    ClickTrack *tt = timeline_selected_click_track(tl);
    if (!tt) return false;
    if (tt == tl->click_tracks[0] && tl->click_track_frozen) {
	check_unfreeze_click_track(tl);
    }
    click_track_delete(tt, false);
    timeline_reset(tl, false);
    return true;
}

static void simple_click_segment_remove(ClickSegment *s)
{
    if (s == s->track->segments && !s->next) return;
    ClickTrack *tt = s->track;
    if (s->prev) {
	s->prev->next = s->next;
    } else {
	tt->segments = s->next;
    }
    if (s->next) {
	s->next->prev = s->prev;
	click_segment_set_start_pos(s->next, s->next->start_pos);
    }
}

static void simple_click_segment_reinsert(ClickSegment *s, int32_t segment_dur)
{
    if (!s->prev) {
	s->track->segments = s;
    } else {
	s->prev->next = s;
    }
    if (s->next) {
	s->next->prev = s;
	click_segment_set_start_pos(s->next, s->start_pos + segment_dur);
    }
    /* if ( */
    click_segment_set_start_pos(s, s->start_pos);
}

ClickSegment *click_track_cut_at(ClickTrack *tt, int32_t at)
{
    ClickSegment *s = click_track_get_segment_at_pos(tt, at);
    uint8_t subdiv_lens[s->cfg.num_beats];
    memcpy(subdiv_lens, s->cfg.beat_len_atoms, s->cfg.num_beats * sizeof(uint8_t));
    return click_track_add_segment(tt, at, -1, s->cfg.bpm, s->cfg.num_beats, subdiv_lens);
}

NEW_EVENT_FN(undo_cut_click_track, "undo cut click track")
    ClickTrack *tt = (ClickTrack *)obj1;
    ClickSegment *s = (ClickSegment *)obj2;
    /* int32_t at = val1.int32_v; */
    /* s = click_track_get_segment_at_pos(tt, at); */
    simple_click_segment_remove(s);
    tt->tl->needs_redraw = true;
}

NEW_EVENT_FN(redo_cut_click_track, "redo cut click track")
    ClickTrack *tt = (ClickTrack *)obj1;
    /* int32_t at = val1.int32_v; */
    simple_click_segment_reinsert(obj2, val2.int32_v);
    tt->tl->needs_redraw = true;
    /* ClickSegment *s = click_track_cut_at(tt, at); */
    /* self->obj2 = (void *)s; */
}

NEW_EVENT_FN(dispose_forward_cut_click_track, "")
    click_segment_destroy((ClickSegment *)obj2);
}


void timeline_cut_click_track_at_cursor(Timeline *tl)
{
    if (tl->play_pos_sframes < 0) return;
    ClickTrack *tt = timeline_selected_click_track(tl);
    if (!tt) return;
    ClickSegment *s = click_track_cut_at(tt, tl->play_pos_sframes);
    if (!s) {
	status_set_errstr("Error: cannot cut at existing segment boundary");
	return;
    }
    tl->needs_redraw = true;
    Value cut_pos = {.int32_v = tl->play_pos_sframes};
    Value new_seg_duration = {.int32_v = s->next ? s->next->start_pos - s->start_pos : -1};
    user_event_push(
	undo_cut_click_track, redo_cut_click_track,
	NULL, dispose_forward_cut_click_track,
	(void *)tt, (void *)s,
	cut_pos, new_seg_duration,
	cut_pos, new_seg_duration,
	0, 0, false, false);
}

void timeline_increment_click_at_cursor(Timeline *tl, int inc_by)
{
    ClickTrack *tt = timeline_selected_click_track(tl);
    if (!tt) return;
    ClickSegment *s = click_track_get_segment_at_pos(tt, tl->play_pos_sframes);
    int new_tempo = s->cfg.bpm + inc_by;
    uint8_t subdiv_lens[s->cfg.num_beats];
    memcpy(subdiv_lens, s->cfg.beat_len_atoms, s->cfg.num_beats * sizeof(uint8_t));
    click_segment_set_config(s, s->num_measures, new_tempo, s->cfg.num_beats, subdiv_lens, tt->end_bound_behavior);
    tl->needs_redraw = true;
}

void click_track_get_prox_beats(ClickTrack *ct, int32_t pos, BeatProminence bp, int32_t *prev_pos_dst, int32_t *next_pos_dst)
{
    ClickTrackPos ctp = click_track_get_pos(ct, pos);
    /* click_track_pos_fprint(stderr, ctp); */
    switch (bp) {
    case BP_SEGMENT:
	ctp.measure = ctp.seg->first_measure_index;
    case BP_MEASURE:
	ctp.beat = 0;
    case BP_BEAT:
	ctp.sd = 0;
    case BP_SD:
	ctp.ssd = 0;
    case BP_SSD:
	ctp.sssd = 0;
    case BP_SSSD:
	ctp.remainder = 0;
    case BP_NONE:
	break;
    }
    *prev_pos_dst = get_beat_pos(ctp);
    if (!ctp.seg) {
	ctp.seg = ct->segments;
	ctp.measure = ctp.seg->first_measure_index;
    } else {
	do_increment(&ctp, bp);
    }
    *next_pos_dst = get_beat_pos(ctp);
}

void click_track_goto_prox_beat(ClickTrack *ct, int direction, BeatProminence bp)
{

    Timeline *tl = ct->tl;
    
    int32_t pos = tl->play_pos_sframes;

    int32_t prev, next;
    click_track_get_prox_beats(ct, pos, bp, &prev, &next);
    fprintf(stderr, "PROX %d < %d < %d (%d, %d)\n", prev, pos, next, pos - prev, next - pos);
    if (direction > 0) {
	timeline_set_play_position(tl, next, true);
    } else {
	if (prev > INT32_MIN && prev == pos) {
	    click_track_get_prox_beats(ct, pos - 1, bp, &prev, &next);
	    fprintf(stderr, "\tPROX %d < %d < %d (%d, %d)\n", prev, pos, next, pos - prev, next - pos);
	}
	if (prev == INT32_MIN) {
	    status_set_errstr("No earlier beats");
	    return;
	}
	timeline_set_play_position(tl, prev, true);
    }
}


static void click_track_deferred_draw(void *click_track_v)
{
    ClickTrack *tt = (ClickTrack *)click_track_v;
    Session *session = session_get();
    SDL_Rect *audio_rect = session->gui.audio_rect;
    /* SDL_Rect cliprect = *audio_rect; */
    /* cliprect.w += cliprect.x; */
    /* cliprect.x = 0; */
    if (audio_rect->y + audio_rect->h > tt->tl->track_area->rect.y) {
	/* SDL_RenderSetClipRect(main_win->rend, audio_rect); */
	textbox_draw(tt->metronome_button);
	slider_draw(tt->metronome_vol_slider);
	/* SDL_RenderSetClipRect(main_win->rend, NULL); */
    }
}

/* not-DRY code for piano roll */
void click_track_draw_segments(ClickTrack *tt, TimeView *tv, SDL_Rect draw_rect)
{

    static const SDL_Color line_colors[] =  {
	{255, 250, 125, 60},
	{255, 255, 255, 60},
	{170, 170, 170, 60},
	{130, 130, 130, 60},
	{100, 100, 100, 60},
    };

    Timeline *tl = tt->tl;
    int32_t pos = tv->offset_left_sframes;
    BeatProminence bp;
    click_track_get_next_pos(tt, true, pos, &pos, &bp, NULL);
    int x;
    int top_y = draw_rect.y;
    int h = draw_rect.h;
    int bottom_y = top_y + h;
    int prev_draw_x = -100;
    const int subdiv_draw_thresh = 10;
    const int beat_draw_thresh = 4;
    int max_bp = BP_NONE;
    while (1) {
	if (bp == BP_SEGMENT) prev_draw_x = -100;
	/* int prev_x = x; */
	x = timeline_get_draw_x(tl, pos);
	if (x > draw_rect.x + draw_rect.w) break;
	if (bp >= max_bp) {
	    goto reset_pos_and_bp;
	}
	    
	if (max_bp > BP_BEAT) {
	    /* fprintf(stderr, "DIFF: %d\n", x - prev_draw_x); */
	    if (x - prev_draw_x < beat_draw_thresh) {
		max_bp = BP_BEAT;
	    } else if (x - prev_draw_x < subdiv_draw_thresh) {
		max_bp = BP_SD;
		/* max_bp = BP_SUBDIV; */
	    }
	    /* fprintf(stderr, "x %d->%d (max bp %d)\n", prev_draw_x, x, max_bp); */
	    prev_draw_x = x;
	    if (bp >= max_bp) {
		goto reset_pos_and_bp;
	    }
	}
	/* int x_diff; */
	/* if (draw_beats && (x_diff = x - prev_x) <= draw_beat_thresh) { */
	/*     draw_beats = false; */
	/*     draw_subdivs = false; */
	/* } else if (draw_subdivs && x_diff <= draw_subdiv_thresh) { */
	/*     draw_subdivs = false; */
	/* } */
	/* if (!draw_subdivs && (bp >= BP_SUBDIV)) continue; */
	/* if (!draw_beats && (bp >= BP_BEAT)) continue; */
	/* top_y = main_top_y + h * (int)bp / 5; */
	/* top_y = main_top_y + h / (1 + (int)bp); */
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(line_colors[(int)bp]));
	double dpi = main_win->dpi_scale_factor;
	if (bp == BP_SEGMENT) {
	    SDL_Rect seg = {x - dpi, top_y, dpi * 2, h};
	    SDL_RenderFillRect(main_win->rend, &seg);

	    /* double dpi = main_win->dpi_scale_factor; */
	    /* SDL_RenderDrawLine(main_win->rend, x-dpi, top_y, x-dpi, bttm_y); */
	    /* SDL_RenderDrawLine(main_win->rend, x+dpi, top_y, x+dpi, bttm_y); */
	} else {
	    SDL_RenderDrawLine(main_win->rend, x, top_y, x, bottom_y);
	}
    reset_pos_and_bp:
	click_track_get_next_pos(tt, false, tv->offset_left_sframes, &pos, &bp, NULL);
    }

}

void click_track_draw(ClickTrack *tt)
{
    /* ClickTrackPos start = click_track_get_pos(tt, 0); */
    /* for (int i=0; i<100; i++) { */
    /* 	do_increment(&start, BP_SSSD); */
    /* 	fprintf(stderr, "INCR\t"); */
    /* 	click_track_pos_fprint(stderr, start); */
    /* } */
    /* for (int i=0; i<100; i++) { */
    /* 	do_decrement(&start, BP_SSSD); */
    /* 	fprintf(stderr, "Decr\t"); */
    /* 	click_track_pos_fprint(stderr, start); */
    /* } */
    /* exit(0); */


    
    Timeline *tl = tt->tl;
    SDL_Rect ttr = tt->layout->rect;
    
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.click_track));
    SDL_RenderFillRect(main_win->rend, &ttr);

    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.black));
    SDL_RenderFillRect(main_win->rend, tt->console_rect);
    
    static SDL_Color line_colors[] =  {
	{255, 250, 125, 255},
	{255, 255, 255, 255},
	{170, 170, 170, 255},
	{130, 130, 130, 255},
	{100, 100, 100, 255}
    };

    int32_t pos = tt->tl->timeview.offset_left_sframes;
    BeatProminence bp;
    ClickSegment *s;
    const static int MAX_BPM_LABELS_TO_DRAW = 64;
    Label *bpm_labels_to_draw[128];
    int num_bpm_labels_to_draw = 0;

    click_track_get_next_pos(tt, true, pos, &pos, &bp, &s);
    bpm_labels_to_draw[num_bpm_labels_to_draw] = s->bpm_label;
    num_bpm_labels_to_draw++;

    int x = timeline_get_draw_x(tl, pos);
    /* fprintf(stderr, "IN DRAW first pos %d bp %d, x %d\n", pos, bp, x); */
    int main_top_y = tt->layout->rect.y;
    int bttm_y = main_top_y + tt->layout->rect.h - 1; /* TODO: figure out why decremet to bttm_y is necessary */
    int h = tt->layout->rect.h;

    int top_y = main_top_y + h * (int)bp / 5;
    double dpi = main_win->dpi_scale_factor;
    if (bp == BP_NONE) {
	fprintf(stderr, "ERROR: bp is NONE\n");
	TESTBREAK;
	exit(1);
    }
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(line_colors[(int)bp]));
    if (bp == BP_SEGMENT) {
	SDL_Rect seg = {x - dpi, top_y, dpi * 2, h};
	SDL_RenderFillRect(main_win->rend, &seg);

	/* SDL_RenderDrawLine(main_win->rend, x-dpi, top_y, x-dpi, bttm_y); */
	/* SDL_RenderDrawLine(main_win->rend, x+dpi, top_y, x+dpi, bttm_y); */
    } else {
	SDL_RenderDrawLine(main_win->rend, x, top_y, x, bttm_y);
    }
    /* SDL_RenderDrawLine(main_win->rend, x, top_y, x, bttm_y); */

    /* const int draw_subdiv_thresh = 2; */
    /* const int draw_beat_thresh = 1; */
    /* bool draw_subdivs = true; */
    /* bool draw_beats = true; */
    
    while (x > 0) {
	x = timeline_get_draw_x(tl, pos);
	/* fprintf(stderr, "IN DRAW, pos %d, x %d, bp %d\n", pos, x, bp); */
	if (x > tt->right_console_rect->x) break;
	/* int x_diff; */
	/* if (draw_beats && (x_diff = x - prev_x) <= draw_beat_thresh) { */
	/*     draw_beats = false; */
	/*     draw_subdivs = false; */
	/* } else if (draw_subdivs && x_diff <= draw_subdiv_thresh) { */
	/*     draw_subdivs = false; */
	/* } */
	/* if (!draw_subdivs && (bp >= BP_SUBDIV)) continue; */
	/* if (!draw_beats && (bp >= BP_BEAT)) continue; */
	top_y = main_top_y + h * (int)bp / 5;
	/* top_y = main_top_y + h / (1 + (int)bp); */
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(line_colors[(int)bp]));
	if (bp == BP_SEGMENT) {
	    SDL_Rect seg = {x - dpi, top_y, dpi * 2, h};
	    SDL_RenderFillRect(main_win->rend, &seg);
	    if (num_bpm_labels_to_draw < MAX_BPM_LABELS_TO_DRAW) {
		bpm_labels_to_draw[num_bpm_labels_to_draw] = s->bpm_label;
		num_bpm_labels_to_draw++;
	    }

	    /* double dpi = main_win->dpi_scale_factor; */
	    /* SDL_RenderDrawLine(main_win->rend, x-dpi, top_y, x-dpi, bttm_y); */
	    /* SDL_RenderDrawLine(main_win->rend, x+dpi, top_y, x+dpi, bttm_y); */
	} else {
	    SDL_RenderDrawLine(main_win->rend, x, top_y, x, bttm_y);
	}
	click_track_get_next_pos(tt, false, 0, &pos, &bp, &s);
    }
    textbox_draw(tt->readout);
    textbox_draw(tt->edit_button);	

    /* Fill right console */
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.control_bar_background_grey));
    SDL_RenderFillRect(main_win->rend, tt->right_console_rect);

    /* Draw right console elements */
    click_track_deferred_draw(tt);
    /* window_defer_draw(main_win, click_track_deferred_draw, tt); */

    /* Draw outline */
    SDL_SetRenderDrawColor(main_win->rend, 100, 100, 100, 255);
    SDL_RenderDrawRect(main_win->rend, &ttr);
    SDL_RenderFillRect(main_win->rend, tt->colorbar_rect);
    SDL_RenderFillRect(main_win->rend, tt->right_colorbar_rect);

    for (int i=0; i<num_bpm_labels_to_draw; i++) {
	label_draw(bpm_labels_to_draw[i]);
    }

}


/* int send_message_udp(char *message, int port); */

void click_track_mix_metronome(ClickTrack *ct, float *mixdown_buf, int32_t mixdown_buf_len, int32_t tl_start_pos_sframes, int32_t tl_end_pos_sframes, float step, int channel)
{
    /* fprintf(stderr, "MIX METRONOME start %d\n", tl_start_pos_sframes); */
    /* static float *ct->metronome_buf; */
    /* static int32_t ct->metronome_buf_len; */
    if (!ct->metronome_buf || ct->metronome_buf_len != mixdown_buf_len) {
	if (ct->metronome_buf) free(ct->metronome_buf);
	ct->metronome_buf_len = mixdown_buf_len;
	ct->metronome_buf = calloc(ct->metronome_buf_len, sizeof(float));
    }
    
    if (ct->muted) return;
    if (step < 0.0) {
	return;
    }
    if (channel != 0) {
	goto add_metronome_buf;
	/* return; */
    }
    memset(ct->metronome_buf, 0, ct->metronome_buf_len * sizeof(float));
    int32_t start_right = tl_end_pos_sframes - 1;
    /* ClickTrackPos ctp = click_track_get_pos(ct, tl_start_pos_sframes); */
    ClickTrackPos start_right_ctp = click_track_get_pos(ct, start_right);
    ClickTrackPos ctp = start_right_ctp;
    ctp.remainder = 0;
    ctp.sssd = 0;
    ctp.ssd = 0;
    if (!ctp.seg) return;
    /* do_decrement(&ctp, BP_SD); */
    /* TODO: refine ! */
    for (int i=0; i<32; i++) {
	int32_t beat_pos = get_beat_pos(ctp);
	BeatProminence bp = get_beat_prominence(ctp);
	/* int32_t elapsed_since_beat = tl_start_pos_sframes - beat_pos; */
	int32_t beat_start_in_chunk = beat_pos - tl_start_pos_sframes;
	beat_start_in_chunk /= step;
	/* int32_t available_buf = mixdown_buf_len - start_in_buf; */
	/* fprintf(stderr, "\tbeat_pos: %d\n\telapsed: %d\n\tstart in buf: %d\n", beat_pos, elapsed_since_beat, beat_start_in_buf); */
	/* First get the appropriate metronome buffer */
	MetronomeBuffer *mb;
	switch (bp) {
	case BP_SEGMENT:
	case BP_MEASURE:
	    mb = ct->metronome.bp_measure_buf;
	    break;
	case BP_BEAT:
	    if (ct->metronome.bp_offbeat_buf && ctp.beat % 2 != 0) {
		mb = ct->metronome.bp_offbeat_buf;
	    } else {
		mb = ct->metronome.bp_beat_buf;
	    }
	    break;
	case BP_SD:
	    mb = ct->metronome.bp_subdiv_buf;
	    break;
	default:
	    fprintf(stderr, "Error: beat prominence %d not allowed in metronome code\n", bp);
	    exit(1);
	    break;
	}
	if (mb) {
	    int32_t adj_buf_len = mb->buf_len / step;
	    if (beat_start_in_chunk < mixdown_buf_len && beat_start_in_chunk + adj_buf_len > 0) {
		if (fabs(step - 1.0) < 1e-6) {
		    int32_t copy_to_start, copy_from_start, copy_len;
		    if (beat_start_in_chunk < 0) {
			copy_to_start = 0;
			copy_from_start = -1 * beat_start_in_chunk;
			copy_len = mb->buf_len - copy_from_start;
			if (copy_len > mixdown_buf_len) copy_len = mixdown_buf_len;
		    } else {
			copy_to_start = beat_start_in_chunk;
			copy_from_start = 0;
			copy_len = mixdown_buf_len - copy_to_start;
			if (copy_len > mb->buf_len) copy_len = mb->buf_len;
		    }
		    float_buf_add(ct->metronome_buf + copy_to_start, mb->buf + copy_from_start, copy_len);
		    /* float_buf_add(mixdown_buf + copy_to_start, mb->buf + copy_from_start, copy_len); */
		} else {
		    double src_i = beat_start_in_chunk > 0 ? 0 : -1 * beat_start_in_chunk * step;
		    int32_t dst_i = beat_start_in_chunk > 0 ? beat_start_in_chunk : 0;
		    while (dst_i < mixdown_buf_len && (int32_t)round(src_i) < mb->buf_len) {
			ct->metronome_buf[dst_i] += mb->buf[(int32_t)round(src_i)];
			dst_i++;
			src_i += step;
		    }
		}
	    } else { /* If one buffer is overrun, consider all overrun (i.e. assume similar lengths) */
		break;
	    }
	}
	do_decrement(&ctp, BP_SD);
	if (!ctp.seg) break;

    }
    float_buf_mult_const(ct->metronome_buf, endpoint_safe_read(&ct->metronome.vol_ep, NULL).float_v, ct->metronome_buf_len);
add_metronome_buf:
    float_buf_add(mixdown_buf, ct->metronome_buf, mixdown_buf_len);
}
    
    
    
/* int bar, beat, subdiv; */
    /* ClickTrackPos ctp = {0}; */
    /* float *buf = NULL; */
    /* int32_t buf_len; */
    /* int32_t tl_chunk_len = tl_end_pos_sframes - tl_start_pos_sframes; */
    /* BeatProminence bp; */
    /* ctp = click_track_get_pos(tt, tl_end_pos_sframes); */
    /* int32_t remainder = click_track_pos_get_remainder_samples(ctp, BP_BEAT); */
    /* remainder -= tl_chunk_len; */
    /* int i=0; */
    /* while (1) { */
    /* 	i++; */
    /* 	if (i>10) { */
    /* 	    return; */
    /* 	} */

    /* 	int32_t tick_start_in_chunk = remainder * -1 / step; */

    /* 	if (tick_start_in_chunk > mixdown_buf_len) { */
    /* 	    goto previous_beat; */
    /* 	} */
    /* 	bp = get_beat_prominence(ctp); */
    /* 	/\* get_beat_prominence(s, &bp, bar, beat, subdiv); *\/ */
	
    /* 	/\* UDP -> Pd testing *\/ */
    /* 	/\* if (tick_start_in_chunk > 0) { *\/ */
    /* 	/\*     char message[255]; *\/ */
    /* 	/\*     snprintf(message, 255, "%d %d;", tick_start_in_chunk, bp); *\/ */
    /* 	/\*     send_message_udp(message, 5400); *\/ */
    /* 	/\* } *\/ */

	
    /* 	if (bp < 2) { */
    /* 	    buf = tt->metronome->buffers[0]; */
    /* 	    buf_len = tt->metronome->buf_lens[0]; */

    /* 	} else if (bp == 2) { */
    /* 	    buf = tt->metronome->buffers[1]; */
    /* 	    buf_len = tt->metronome->buf_lens[1]; */
    /* 	} else { */
    /* 	    /\* Beat is not audible *\/ */
    /* 	    goto previous_beat; */
    /* 	} */

    /* 	int32_t chunk_end_i = tick_start_in_chunk + mixdown_buf_len; */
    /* 	/\* Normal exit point *\/ */
    /* 	if (chunk_end_i < 0) { */
    /* 	    break; */
    /* 	} */
    /* 	int32_t buf_i = 0; */
    /* 	double buf_i_d = 0.0; */
    /* 	/\* double tick_i_d = (double)tick_start_in_chunk; *\/ */
    /* 	while (tick_start_in_chunk < mixdown_buf_len) { */
    /* 	    if (tick_start_in_chunk > 0 && buf_i > 0 && buf_i < buf_len) { */
    /* 		mixdown_buf[tick_start_in_chunk] += buf[buf_i] * tt->metronome_vol; */
    /* 	    } */
    /* 	    buf_i_d += step; */
    /* 	    buf_i = (int32_t)round(buf_i_d); */
    /* 	    tick_start_in_chunk++; */
    /* 	    /\* tick_start_in_chunk = (round((double)tick_start_in_chunk + step)); *\/ */
    /* 	    /\* inner_ops++; *\/ */
    /* 	} */
    /* previous_beat: */
    /* 	do_decrement(&ctp, BP_BEAT); */
    /* 	/\* do_decrement(s, &bar, &beat, &subdiv); *\/ */
    /* 	/\* c = clock(); *\/ */
    /* 	remainder = tl_start_pos_sframes - get_beat_pos(ctp); */
    /* 	/\* remainder = (tl_start_pos_sframes - get_beat_pos(s, bar, beat, subdiv)); *\/ */
    /* } */
/* } */

void click_track_mute_unmute(ClickTrack *t)
{
    t->muted = !t->muted;
    if (t->muted) {
	textbox_set_background_color(t->metronome_button, &colors.mute_red);
    } else {
	textbox_set_background_color(t->metronome_button, &colors.play_green);
    }
    t->tl->needs_redraw = true;
}

/* void track_increment_vol(Track *track) */
/* { */
/*     track->vol += TRACK_VOL_STEP; */
/*     if (track->vol > track->vol_ctrl->max.float_v) { */
/* 	track->vol = track->vol_ctrl->max.float_v; */
/*     } */
/*     slider_edit_made(track->vol_ctrl); */
/*     slider_reset(track->vol_ctrl); */
/* } */

void click_track_increment_vol(ClickTrack *tt)
{
    tt->metronome.vol += TRACK_VOL_STEP;
    if (tt->metronome.vol > tt->metronome_vol_slider->max.float_v) {
	tt->metronome.vol = tt->metronome_vol_slider->max.float_v;
    }
    /* slider_edit_made(tt->metronome_vol_slider); */
    slider_reset(tt->metronome_vol_slider);
}


void click_track_decrement_vol(ClickTrack *tt)
{
    tt->metronome.vol -= TRACK_VOL_STEP;
    if (tt->metronome.vol < 0.0f) {
	tt->metronome.vol = 0.0f;
    }
    /* slider_edit_made(tt->metronome_vol_slider); */
    slider_reset(tt->metronome_vol_slider);
}

static void click_track_delete_segment_at(ClickTrack *ct, int32_t at, bool from_undo);

NEW_EVENT_FN(undo_delete_click_segment, "undo delete click track segment")
    ClickSegment *to_cpy = (ClickSegment *)obj1;
    simple_click_segment_reinsert(to_cpy, val2.int32_v);
    /* int32_t at = val1.int32_v; */
    /* uint8_t subdiv_lens[to_cpy->cfg.num_beats]; */
    /* memcpy(subdiv_lens, to_cpy->cfg.beat_subdiv_lens, to_cpy->cfg.num_beats * sizeof(uint8_t)); */
    /* click_track_add_segment(to_cpy->track, at, -1, to_cpy->cfg.bpm, to_cpy->cfg.num_beats, subdiv_lens); */
}

NEW_EVENT_FN(redo_delete_click_segment, "redo delete click track segment")
    ClickSegment *deleted = (ClickSegment *)obj1;
    click_track_delete_segment_at(deleted->track, val1.int32_v, true);
}

NEW_EVENT_FN(dispose_delete_click_segment, "")
    click_segment_destroy(obj1);
}

static void click_track_delete_segment_at(ClickTrack *ct, int32_t at, bool from_undo)
{
    ClickSegment *s = click_track_get_segment_at_pos(ct, at);
    if (!s) return;
    int32_t start_pos = s->start_pos;
    int32_t segment_dur = s->next ? s->next->start_pos - s->start_pos : -1;
    simple_click_segment_remove(s);
    

    if (!from_undo) {
	user_event_push(
	    undo_delete_click_segment,
	    redo_delete_click_segment,
	    dispose_delete_click_segment, NULL,
	    (void *)s,
	    NULL,
	    (Value){.int32_v = start_pos}, (Value){.int32_v = segment_dur},
	    (Value){.int32_v = start_pos}, (Value){.int32_v = segment_dur},
	    0, 0,
	    false, false);
    }

}

void click_track_delete_segment_at_cursor(ClickTrack *ct)
{
    click_track_delete_segment_at(ct, ct->tl->play_pos_sframes, false);
}

static void timeline_select_click_track(Timeline *tl, ClickTrack *ct)
{
    tl->layout_selector = ct->layout->index;
    timeline_rectify_track_indices(tl);
}

/* Mouse */

static ClickTrackPos dragging_pos;
static float original_bpm;
static int32_t clicked_tl_pos;
void click_track_drag_pos(ClickSegment *s, Window *win)
{
    Session *session = session_get();
    int32_t tl_pos = timeview_get_pos_sframes(&ACTIVE_TL->timeview, win->mousep.x);
    if (tl_pos <= s->start_pos) return;
    int32_t og_dur = clicked_tl_pos - s->start_pos;
    int32_t new_dur = tl_pos - s->start_pos;
    double prop = (double)og_dur / new_dur;
    float new_tempo = original_bpm * prop;
    endpoint_write(&s->bpm_ep, (Value){.float_v=new_tempo}, true, true, true, false);
}

bool click_track_triage_click(uint8_t button, ClickTrack *t)
{
    Session *session = session_get();
    if (!SDL_PointInRect(&main_win->mousep, &t->layout->rect)) {
	return false;
    }
    if (SDL_PointInRect(&main_win->mousep, t->right_console_rect)) {
	if (slider_mouse_click(t->metronome_vol_slider, main_win)) {
	    return true;
	}
	if (SDL_PointInRect(&main_win->mousep, &t->metronome_button->layout->rect)) {
	    click_track_mute_unmute(t);
	    return true;
	}
	return true;
    }
    if (SDL_PointInRect(&main_win->mousep, t->console_rect)) {
	if (SDL_PointInRect(&main_win->mousep, &t->edit_button->layout->rect)) {
	    Timeline *tl = ACTIVE_TL;
	    timeline_select_click_track(tl, t);
	    timeline_click_track_edit(tl);
	    return true;
	}
	return true;
    }
    if (main_win->i_state & I_STATE_CMDCTRL) {
	Session *session = session_get();
	int32_t tl_pos = timeview_get_pos_sframes(&ACTIVE_TL->timeview, main_win->mousep.x);
	clicked_tl_pos = tl_pos;
	dragging_pos = click_track_get_pos(t, tl_pos);
	original_bpm = dragging_pos.seg->cfg.bpm;
	session->dragged_component.component = dragging_pos.seg;
	session->dragged_component.type = DRAG_CLICK_TRACK_POS;
	Value cur_val = endpoint_safe_read(&dragging_pos.seg->bpm_ep, NULL);
	endpoint_start_continuous_change(&dragging_pos.seg->bpm_ep, false, (Value){0}, JDAW_THREAD_MAIN, cur_val);
	label_move(dragging_pos.seg->bpm_label, main_win->mousep.x, dragging_pos.seg->track->layout->rect.y - 20);
	label_reset(dragging_pos.seg->bpm_label, cur_val);
	/* textbox_reset(dragging_pos.seg->bpm_label->tb); */
	return true;
    } else {

	ClickSegment *final = NULL;
	ClickSegment *s = t->segments;
	const int xdst_init = 5 * main_win->dpi_scale_factor;
	int xdst = xdst_init;
	while (s) {
	    int xdst_test = abs(main_win->mousep.x - timeline_get_draw_x(s->track->tl, s->start_pos));
	    if (xdst_test < xdst) {
		final = s;
		xdst = xdst_test;
	    } else if (final) {
		break;
	    }
	    s = s->next;
	}
	if (final) {
	    session->dragged_component.component = final;
	    session->dragged_component.type = DRAG_CLICK_SEG_BOUND;
	    Value current_val = endpoint_safe_read(&final->start_pos_ep, NULL);
	    endpoint_start_continuous_change(&final->start_pos_ep, false, (Value)0, JDAW_THREAD_MAIN, current_val);
	    return true;
	}
    }

    
    return false;
}

/* implicitly SSSD */
static int32_t click_segment_get_nearest_beat_pos(ClickTrack *ct, int32_t start_pos)
{
    int32_t next, prev;
    click_track_get_prox_beats(ct, start_pos, BP_SSD, &prev, &next);
    fprintf(stderr, "Prev cur next: %d, %d, %d (%d %d)\n", prev, start_pos, next, start_pos - prev, next - start_pos);
    if (start_pos - prev < next - start_pos) {
	return prev;
    } else {
	return next;
    }
    /* int bar, beat, subdiv; */
    /* ClickSegment *s; */
    /* int32_t remainder = click_track_bar_beat_subdiv(ct, start_pos, &bar, &beat, &subdiv, &s, false); */
    /* int32_t init = get_beat_pos(s, bar, beat, subdiv); */
    /* do_increment(s, &bar, &beat, &subdiv); */
    /* int32_t next = get_beat_pos(s, bar, beat, subdiv); */
    /* if (next - start_pos < remainder) { */
    /* 	return next; */
    /* } else { */
    /* 	return init; */
    /* } */
    
}

void click_track_mouse_motion(ClickSegment *s, Window *win)
{
    int32_t tl_pos = timeline_get_abspos_sframes(s->track->tl, win->mousep.x);
    if (!(main_win->i_state & I_STATE_SHIFT)) {
	tl_pos = click_segment_get_nearest_beat_pos(s->track, tl_pos);
    }
    if (tl_pos < 0 || (s->prev && tl_pos < s->prev->start_pos + session_get_sample_rate() / 100)) {
	return;
    }
    endpoint_write(&s->start_pos_ep, (Value){.int32_v = tl_pos}, true, true, true, false);
}

void click_segment_fprint(FILE *f, ClickSegment *s)
{
    fprintf(f, "\nSegment at %p start: %d (%fs)\n", s, s->start_pos, (float)s->start_pos / s->track->tl->proj->sample_rate);
    fprintf(f, "Segment tempo (bpm): %f\n", s->cfg.bpm);
    fprintf(stderr, "Segment measure start i: %d; num measures: %d\n", s->first_measure_index, s->num_measures);
    fprintf(f, "\tCfg beats: %d\n", s->cfg.num_beats);
    fprintf(f, "\tCfg beat subdiv lengths:\n");
    for (int i=0; i<s->cfg.num_beats; i++) {
	fprintf(f, "\t\t%d: %d\n", i, s->cfg.beat_len_atoms[i]);
    }
    fprintf(f, "\tCfg num atoms: %d\n", s->cfg.num_atoms);
    fprintf(f, "\tCfg measure dur: %d\n", s->cfg.dur_sframes);
    fprintf(f, "\tCfg atom dur: %d\n", s->cfg.dur_sframes / s->cfg.num_atoms);

}

void click_segment_at_cursor_fprint(FILE *f, Timeline *tl)
{
    ClickTrack *tt = timeline_selected_click_track(tl);
    if (tt) {
	ClickSegment *s = click_track_get_segment_at_pos(tt, tl->play_pos_sframes);
	click_segment_fprint(f, s);
    }
}

void click_track_fprint(FILE *f, ClickTrack *tt)
{
    fprintf(f, "\nTEMPO TRACK\n");
    ClickSegment *s = tt->segments;
    while (s) {
	fprintf(f, "SEGMENT:\n");
	click_segment_fprint(f, s);
	s = s->next;
    }
}

ClickTrack *click_track_active_at_cursor(Timeline *tl)
{
    ClickTrack *ct = NULL;
    for (int i=tl->num_click_tracks - 1; i>=0; i--) {
	if (tl->click_tracks[i]->layout->index <= tl->layout_selector) {
	    ct = tl->click_tracks[i];
	    break;
	}
    }
    if (!ct && tl->click_track_frozen) {
	ct = tl->click_tracks[0];
    }
    return ct;
}

ClickSegment *click_segment_active_at_cursor(Timeline *tl)
{
    ClickTrack *ct = click_track_active_at_cursor(tl);
    if (!ct) return NULL;
    for (int i=tl->num_click_tracks - 1; i>=0; i--) {
	if (tl->click_tracks[i]->layout->index <= tl->layout_selector) {
	    ct = tl->click_tracks[i];
	    break;
	}
    }
    ClickTrackPos pos = click_track_get_pos(ct, tl->play_pos_sframes);
    /* ClickSegment *ret; */
    /* click_track_bar_beat_subdiv(ct, tl->play_pos_sframes, NULL, NULL, NULL, &ret, false); */
    return pos.seg;
}


