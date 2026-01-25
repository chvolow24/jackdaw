#include "clipref.h"
#include "color.h"
#include "midi_clip.h"
#include "midi_io.h"
#include "midi_objs.h"
#include "modal.h"
#include "project.h"
#include "tempo.h"
#include "test.h"
#include "session.h"
#include "timeline.h"
#include "user_event.h"

#define DEFAULT_NOTES_ALLOC_LEN 16
#define DEFAULT_REFS_ALLOC_LEN 2

#define MAX_INTERSECTING_NOTES 4096

extern Window *main_win;
extern struct colors colors;

/* Mandatory initialization after allocating */
void midi_clip_init(MIDIClip *mclip)
{
    mclip->notes_alloc_len = DEFAULT_NOTES_ALLOC_LEN;
    mclip->notes = calloc(mclip->notes_alloc_len, sizeof(Note));

    mclip->refs_alloc_len = DEFAULT_REFS_ALLOC_LEN;
    mclip->refs = calloc(mclip->refs_alloc_len, sizeof(ClipRef *));

    int err;
    if ((err = pthread_mutex_init(&mclip->notes_arr_lock, NULL)) != 0) {
	fprintf(stderr, "Error initializing mutex in 'midi_clip_init': %s\n", strerror(err));
	exit(1);
    }
}

MIDIClip *midi_clip_create(MIDIDevice *device, Track *target)
{
    Session *session = session_get();
    if (session->proj.num_midi_clips == MAX_PROJ_MIDI_CLIPS) {
	return NULL;
    }
    MIDIClip *mclip = calloc(1, sizeof(MIDIClip));
    if (device) {
	mclip->recorded_from = device;
    }
    midi_clip_init(mclip);
    /* mclip->notes_alloc_len = DEFAULT_NOTES_ALLOC_LEN; */
    /* mclip->notes = calloc(mclip->notes_alloc_len, sizeof(Note)); */

    /* mclip->refs_alloc_len = DEFAULT_REFS_ALLOC_LEN; */
    /* mclip->refs = calloc(mclip->refs_alloc_len, sizeof(ClipRef *)); */
    

    if (!target && device) {
	snprintf(mclip->name, sizeof(mclip->name), "%s_rec_%d", device->name, session->proj.num_clips); /* TODO: Fix this */
    } else if (target) {
	snprintf(mclip->name, sizeof(mclip->name), "%s take %d", target->name, target->num_takes + 1);
	target->num_takes++;
    } else {
	snprintf(mclip->name, sizeof(mclip->name), "anonymous");
    }

    session->proj.midi_clips[session->proj.num_midi_clips] = mclip;
    session->proj.num_midi_clips++;
    return mclip;

}

void midi_clip_destroy(MIDIClip *mc)
{
    free(mc->notes);
    for (int i=0; i<mc->num_refs; i++) {
	clipref_destroy(mc->refs[i], false);
    }
    Session *session = session_get();
    if (mc == session->source_mode.src_clip) {
	session->source_mode.src_clip = NULL;
    }
    bool displace = false;
    Project *proj = &session->proj;
    for (uint16_t i=0; i<proj->num_midi_clips; i++) {
	if (proj->midi_clips[i] == mc) {
	    displace = true;
	} else if (displace && i > 0) {
	    proj->midi_clips[i-1] = proj->midi_clips[i];
	}
    }
    proj->num_midi_clips--;
    proj->active_midi_clip_index = proj->num_midi_clips;

    pthread_mutex_destroy(&mc->notes_arr_lock);
    free(mc->refs);
    free(mc);
}

TEST_FN_DEF(check_note_order, {
	int32_t start_rel = INT32_MIN;
	for (int i=0; i<mclip->num_notes; i++) {
	    Note *note = mclip->notes + i;
	    if (note->start_rel < start_rel) {
		fprintf(stderr, "Error: MIDI clip note order error at indices %d,%d, clip %s\n", i-1, i, mclip->name);
		return 1;
	    }
	    /* fprintf(stderr, "NOTE: %d/%d, note %d, velocity %d, start: %d\n", i, mclip->num_notes, note->note, note->velocity, note->end_rel); */
	    start_rel = note->start_rel;
	}
	return 0;
    }, MIDIClip *mclip);


/* static void grabbed_notes_rebase(MIDIClip *mclip, Note *new_head); */
static void grabbed_notes_incr(MIDIClip *mclip, int32_t from_i, int32_t until_i, int by);

void midi_clip_note_check_reset_bounds(MIDIClip *mc, Note *note)
{
    if (note->start_rel < 0 && note == mc->notes) {
	int32_t adj = note->start_rel * -1;
	for (int i=0; i<mc->num_notes; i++) {
	    Note *note = mc->notes + i;
	    note->start_rel += adj;
	    note->end_rel += adj;
	}
	for (int i=0; i<mc->num_refs; i++) {
	    ClipRef *cr = mc->refs[i];
	    cr->tl_pos -= adj;
	}
    } else if (note->end_rel > mc->len_sframes) {
	for (int i=0; i<mc->num_refs; i++) {
	    if (mc->refs[i]->end_in_clip == mc->len_sframes) {
		mc->refs[i]->end_in_clip = note->end_rel;
	    }
	}
	mc->len_sframes = note->end_rel;
    }
}

void midi_clip_check_reset_bounds(MIDIClip *mc)
{
    midi_clip_note_check_reset_bounds(mc, mc->notes); /* first note */
    Note *last = NULL;
    int32_t end_pos = 0;
    for (int i=0; i<mc->num_notes; i++) {
	if (mc->notes[i].end_rel > end_pos) {
	    last = mc->notes + i;
	    end_pos = last->end_rel;
	}
    }
    if (last) {
	midi_clip_note_check_reset_bounds(mc, last);
    }
}

Note *midi_clip_insert_note(MIDIClip *mc, int channel, int note_val, int velocity, int32_t start_rel, int32_t end_rel)
{
    pthread_mutex_lock(&mc->notes_arr_lock);
    if (!mc->notes) {
	mc->notes_alloc_len = 32;
	mc->notes = calloc(mc->notes_alloc_len, sizeof(Note));
    }
    if (mc->num_notes == mc->notes_alloc_len) {
	mc->notes_alloc_len *= 2;
	mc->notes = realloc(mc->notes, mc->notes_alloc_len * sizeof(Note));
	memset(mc->notes + mc->num_notes, 0, sizeof(Note) * (mc->notes_alloc_len - mc->num_notes));
	/* grabbed_notes_rebase(mc, mc->notes);	       */
    }
    pthread_mutex_unlock(&mc->notes_arr_lock);
    Note *note = mc->notes + mc->num_notes;
    while (note > mc->notes && start_rel < (note - 1)->start_rel) {
	*note = *(note - 1);
	note--;
    }
    memset(note, '\0', sizeof(Note));
    note->channel = channel;
    note->key = note_val;
    note->velocity = velocity;
    note->start_rel = start_rel;
    note->end_rel = end_rel;
    mc->num_notes++;
    
    note->id = mc->note_id;
    mc->note_id++;

    grabbed_notes_incr(mc, note - mc->notes + 1, mc->num_notes, 1);
    midi_clip_note_check_reset_bounds(mc, note);
    /* if (note->start_rel < 0 && note - mc->notes == 0) { */
    /* 	int32_t adj = note->start_rel * -1; */
    /* 	for (int i=0; i<mc->num_notes; i++) { */
    /* 	    Note *note = mc->notes + i; */
    /* 	    note->start_rel += adj; */
    /* 	    note->end_rel += adj; */
    /* 	} */
    /* 	for (int i=0; i<mc->num_refs; i++) { */
    /* 	    ClipRef *cr = mc->refs[i]; */
    /* 	    cr->tl_pos -= adj; */
    /* 	} */
    /* } else if (note->end_rel > mc->len_sframes) { */
    /* 	for (int i=0; i<mc->num_refs; i++) { */
    /* 	    if (mc->refs[i]->end_in_clip == mc->len_sframes) { */
    /* 		mc->refs[i]->end_in_clip = note->end_rel; */
    /* 	    } */
    /* 	} */
    /* 	mc->len_sframes = note->end_rel; */
    /* } */
    TEST_FN_CALL(check_note_order, mc);
    return note;
}

void midi_clip_add_controller_change(MIDIClip *mclip, PmEvent e, int32_t pos)
{
    uint8_t status = Pm_MessageStatus(e.message);
    uint8_t channel = status & 0x0F;
    uint8_t controller_number = Pm_MessageData1(e.message);
    uint8_t value = Pm_MessageData2(e.message);
    Controller *c = mclip->controllers + controller_number;
    c->channel = channel;
    c->in_use = true;
    midi_controller_insert_change(c, pos, value);
 
}

void midi_clip_add_pitch_bend(MIDIClip *mclip, PmEvent e, int32_t pos)
{
    uint8_t status = Pm_MessageStatus(e.message);
    uint8_t channel = status & 0x0F;
    uint8_t value1 = Pm_MessageData1(e.message);
    uint8_t value2 = Pm_MessageData2(e.message);
    PitchBend *pb = &mclip->pitch_bend;
    pb->channel = channel;
    midi_pitch_bend_insert_change(pb, pos, value1, value2);
}

/* void midi_clip_add_cc(MIDIClip *mc, MIDICC cc_in) */
/* { */
/*     /\* fprintf(stderr, "(%d) ADD CC type %d val %d\n", cc_in.pos_rel, cc_in.type, cc_in.value); *\/ */
/*     if (!mc->ccs) { */
/* 	mc->ccs_alloc_len = 32; */
/* 	mc->ccs = calloc(mc->ccs_alloc_len, sizeof(MIDICC)); */
/*     } */
/*     if (mc->num_ccs == mc->ccs_alloc_len) { */
/* 	mc->ccs_alloc_len *= 2; */
/* 	mc->ccs = realloc(mc->ccs, mc->ccs_alloc_len * sizeof(MIDICC)); */
/*     } */
    
/*     MIDICC *cc = mc->ccs + mc->num_ccs; */
/*     while (cc > mc->ccs && cc_in.pos_rel < (cc - 1)->pos_rel) { */
/* 	*cc = *(cc - 1); */
/* 	cc--; */
/*     } */
/*     *cc = cc_in; */
/*     mc->num_ccs++; */
/* } */

/* void midi_clip_add_pb(MIDIClip *mc, MIDIPitchBend pb_in) */
/* { */
/*     /\* fprintf(stderr, "(%d) ADD PB val %d float %f\n", pb_in.pos_rel, pb_in.value, pb_in.floatval); *\/ */
/*     if (!mc->pbs) { */
/* 	mc->pbs_alloc_len = 32; */
/* 	mc->pbs = calloc(mc->pbs_alloc_len, sizeof(MIDIPitchBend)); */
/*     } */
/*     if (mc->num_pbs == mc->pbs_alloc_len) { */
/* 	mc->pbs_alloc_len *= 2; */
/* 	mc->pbs = realloc(mc->pbs, mc->pbs_alloc_len * sizeof(MIDIPitchBend)); */
/*     } */
    
/*     MIDIPitchBend *pb = mc->pbs + mc->num_pbs; */
/*     while (pb > mc->pbs && pb_in.pos_rel < (pb - 1)->pos_rel) { */
/* 	*pb = *(pb - 1); */
/* 	pb--; */
/*     } */
/*     *pb = pb_in; */
/*     mc->num_pbs++; */
/* } */

/* Return -1 if clip has no notes; otherwise, first note index */
int32_t midi_clipref_check_get_first_note(ClipRef *cr)
{
    MIDIClip *clip = cr->source_clip;

    if (cr->first_note == -1) cr->first_note = 0;
    while (cr->first_note > 0 && clip->notes[cr->first_note].start_rel > cr->start_in_clip) {
	cr->first_note--;
    }
    while (cr->first_note < clip->num_notes && clip->notes[cr->first_note].start_rel < cr->start_in_clip) {
	cr->first_note++;
    }
    if (cr->first_note == clip->num_notes) cr->first_note = -1;
    return cr->first_note;
}

int32_t midi_clipref_check_get_last_note(ClipRef *cr)
{
    MIDIClip *clip = cr->source_clip;
    int32_t end_in_clip = cr->end_in_clip <= cr->start_in_clip ? clip->len_sframes : cr->end_in_clip;
    if (cr->last_note <= 0 || cr->last_note >= clip->num_notes) cr->last_note = clip->num_notes - 1;
    while (cr->last_note < clip->num_notes - 1 && clip->notes[cr->last_note].start_rel < end_in_clip) {
	cr->last_note++;
    }
    while (cr->last_note > 0 && clip->notes[cr->last_note].start_rel > end_in_clip) {
	cr->last_note--;
    }
    if (cr->last_note == clip->num_notes) cr->last_note = -1;
    return cr->last_note;
}


/* /\* Return -1 if clip has no ccs; otherwise, first cc index *\/ */
/* int32_t midi_clipref_check_get_first_cc(ClipRef *cr) */
/* { */
/*     MIDIClip *clip = cr->source_clip; */

/*     if (cr->first_cc == -1) cr->first_cc = 0; */
/*     while (cr->first_cc > 0 && clip->ccs[cr->first_cc].pos_rel > cr->start_in_clip) { */
/* 	cr->first_cc--; */
/*     } */
/*     while (cr->first_cc < clip->num_ccs && clip->ccs[cr->first_cc].pos_rel < cr->start_in_clip) { */
/* 	cr->first_cc++; */
/*     } */
/*     if (cr->first_cc == clip->num_ccs) cr->first_cc = -1; */
/*     return cr->first_cc; */
/* } */

/* int32_t midi_clipref_check_get_first_pb(ClipRef *cr) */
/* { */
/*     MIDIClip *clip = cr->source_clip; */

/*     if (cr->first_pb == -1) cr->first_pb = 0; */
/*     while (cr->first_pb > 0 && clip->pbs[cr->first_pb].pos_rel > cr->start_in_clip) { */
/* 	cr->first_pb--; */
/*     } */
/*     while (cr->first_pb < clip->num_pbs && clip->pbs[cr->first_pb].pos_rel < cr->start_in_clip) { */
/* 	cr->first_pb++; */
/*     } */
/*     if (cr->first_pb == clip->num_pbs) cr->first_pb = -1; */
/*     return cr->first_pb; */
/* } */

int32_t note_tl_start_pos(Note *note, ClipRef *cr)
{
    return note->start_rel + cr->tl_pos - cr->start_in_clip;
}

int32_t note_tl_end_pos(Note *note, ClipRef *cr)
{
    /* fprintf(stderr, "END REL: %d, tl pos: %d, start in clip: %d\n", note->end_rel, cr->tl_pos, cr->start_in_clip); */
    return note->end_rel + cr->tl_pos - cr->start_in_clip;
}

void midi_clip_rectify_length(MIDIClip *mclip)
{
    if (mclip->num_notes == 0) return;
    mclip->len_sframes = mclip->notes[mclip->num_notes - 1].end_rel;
}

static int event_cmp(const void *obj1, const void *obj2)
{
    const PmEvent *e1 = obj1;
    const PmEvent *e2 = obj2;
    if (e1->timestamp == e2->timestamp) {
	return Pm_MessageStatus(e1->message) - Pm_MessageStatus(e2->message);
    }
    return e1->timestamp - e2->timestamp;
}

void midi_clip_read_events(
    MIDIClip *mclip,
    PmEvent *events,
    uint32_t num_events,
    enum midi_ts_type ts_type,
    int32_t ts_offset)
{
    Note unclosed_notes[128];
    for (uint32_t i=0; i<num_events; i++) {
	PmEvent e = events[i];
	uint8_t status = Pm_MessageStatus(e.message);
	uint8_t channel = status & 0x0F;
	uint8_t note_val = Pm_MessageData1(e.message);
	uint8_t velocity = Pm_MessageData2(e.message);
	uint8_t msg_type = status >> 4;
	int32_t pos_rel;
	if (ts_type == MIDI_TS_SFRAMES) {
	    pos_rel = e.timestamp;
	} else if (ts_type == MIDI_TS_MSEC) { /* MSEC */
	    pos_rel = ((double)e.timestamp - ts_offset) * (double)session_get_sample_rate() / 1000.0;
	} else {
	    return;
	}
	/* fprintf(stderr, "EVENT %d/%d, timestamp: %d pos rel %d (record start %d)\n", i, d->num_unconsumed_events, e.timestamp, pos_rel, d->record_start); */
	if (msg_type == 9) {
	    Note *unclosed = unclosed_notes + note_val;
	    unclosed->key = note_val;
	    unclosed->velocity = velocity;
	    unclosed->start_rel = pos_rel;
	    unclosed->unclosed = true;
	} else if (msg_type == 8) {
	    Note *unclosed = unclosed_notes + note_val;
	    /* if (d->current_clip) */
	    if (unclosed->unclosed) {
		midi_clip_insert_note(mclip, channel, note_val, unclosed->velocity, unclosed->start_rel, pos_rel);
		unclosed->unclosed = false;
	    }
	} else if (msg_type == 0xB) { /* Controller */
	    /* MIDICC cc = midi_cc_from_event(&e, pos_rel); */
	    midi_clip_add_controller_change(mclip, e, pos_rel);
	} else if (msg_type == 0xE) {
	    midi_clip_add_pitch_bend(mclip, e, pos_rel);
	    /* fprintf(stderr, "RECORD PITCH BEND!\n"); */
	    /* MIDIPitchBend pb = midi_pitch_bend_from_event(&e, pos_rel); */
	    /* midi_clip_add_pb(mclip, pb); */
	}
    }
}

static void event_buf_insert(PmEvent **dst, uint32_t *alloc_len, uint32_t *num_events, PmEvent insert)
{
    /* fprintf(stderr, "EVENT BUF insert alloc len %d\n", *alloc_len); */
    /* if (*alloc_len > 5000) exit(1); */
    if (*num_events + 1 > *alloc_len) {
	*alloc_len *= 2;
	*dst = realloc(*dst, *alloc_len * sizeof(PmEvent));	
    }
    (*dst)[*num_events] = insert;
    (*num_events)++;
}

/* Allocate an array of PmEvents and sort them by timestamp.

   "timestamp" here signifies the position IN THE CLIP.
   
   The note off ring buffer is only required for playback (not
   for file serialization, for example.
*/
uint32_t midi_clip_get_events(
    MIDIClip *mclip,
    PmEvent **dst,
    int32_t start_in_clip,
    int32_t end_in_clip,
    int32_t note_trunc_pos_rel,
    int32_t tl_start,
    MIDIEventRingBuf *rb)
{
    /* fprintf(stderr, "%d - %d\n", start_pos, end_pos); */
    uint32_t alloc_len = 128;
    uint32_t num_events = 0;
    *dst = malloc(alloc_len * sizeof(PmEvent));
    for (int i=0; i<mclip->num_notes; i++) {
	Note *note = mclip->notes + i;
	if (note->grabbed && session_get()->dragging) continue;
	/* fprintf(stderr, "\tTest start? %d\n", note->start_rel); */
	if (note->start_rel < start_in_clip) continue;
	if (note->start_rel >= end_in_clip) break;
	PmEvent note_on, note_off;
	note_on.timestamp = note->start_rel + tl_start;
	note_on.message = Pm_Message(
	    0x90 + note->channel,
	    note->key,
	    note->velocity);
	/* Insert Note ON */
	event_buf_insert(dst, &alloc_len, &num_events, note_on);
	if (note->end_rel > note_trunc_pos_rel) {
	    note_off.timestamp = note_trunc_pos_rel + tl_start;
	} else {
	    note_off.timestamp = note->end_rel + tl_start;
	}
	note_off.message = Pm_Message(
	    0x80 + note->channel,
	    note->key,
	    note->velocity);
	
	/* Insert Note OFF or queue if ring buffer provided */
	if (rb) {
	    midi_event_ring_buf_insert(rb, note_off);
	} else {
	    event_buf_insert(dst, &alloc_len, &num_events, note_off);
	}
    }
    PmEvent *note_off;
    if (rb) {
	while ((note_off = midi_event_ring_buf_pop(rb, tl_start + end_in_clip - 1 /* sample at end pos belongs to NEXT chunk */))) {
	    event_buf_insert(dst, &alloc_len, &num_events, *note_off);
	}
    }
    for (int i=0; i<MIDI_NUM_CONTROLLERS; i++) {
	Controller *c = mclip->controllers + i;
	if (!c->in_use) continue;
	for (uint16_t i=0; i<c->num_changes; i++) {
	    PmEvent e = midi_controller_make_event(c, i);
	    if (e.timestamp < start_in_clip) continue;
	    if (e.timestamp >= end_in_clip) break;
	    e.timestamp += tl_start;
	    event_buf_insert(dst, &alloc_len, &num_events, e);	    
	}
    }
    for (uint16_t i=0; i<mclip->pitch_bend.num_changes; i++) {
	PmEvent e = pitch_bend_make_event(&mclip->pitch_bend, i);
	if (e.timestamp < start_in_clip) continue;
	if (e.timestamp >= end_in_clip) break;
	e.timestamp += tl_start;
	event_buf_insert(dst, &alloc_len, &num_events, e);	
    }
    qsort(*dst, num_events, sizeof(PmEvent), event_cmp);
    /* fprintf(stderr, "Output %d events\n", num_events); */
    /* for (int i=0; i<num_events; i++) { */
    /* 	PmEvent e = (*dst)[i]; */
    /* 	fprintf(stderr, "\t(%d) %s, pitch %d vel %d\n", */
    /* 		e.timestamp, */
    /* 		(Pm_MessageStatus(e.message) & 0xF0) == 0x80 ? "note OFF" : */
    /* 		(Pm_MessageStatus(e.message) & 0xF0) == 0x90 ? "note ON" : */
    /* 		"unknown", */
    /* 		Pm_MessageData1(e.message), */
    /* 		Pm_MessageData2(e.message)); */
		
    /* } */
    /* fprintf(stderr, "BEFORE RETURN EVENTS:\n"); */
    /* for (int i=0; i<num_events; i++) { */
    /* 	PmEvent e = *dst[i]; */
    /* 	fprintf(stderr, "\t%d: Note %s ts %d\n", i, Pm_MessageStatus(e.message) >> 4 == 8? "OFF" : "ON", e.timestamp); */
    /* } */
    return num_events;
}

uint32_t midi_clip_get_all_events(MIDIClip *mclip, PmEvent **dst)
{
    uint32_t num_events = midi_clip_get_events(
	mclip,
	dst,
	0,
	mclip->len_sframes,
	mclip->len_sframes - 1, /* Send note off before clip is closed */
	0,
	NULL);
    return num_events;
}


int midi_clipref_output_chunk(ClipRef *cr, PmEvent *event_buf, int event_buf_max_len, int32_t chunk_tl_start, int32_t chunk_tl_end)
{
    if (cr->type == CLIP_AUDIO) return 0;
    
    MIDIClip *mclip = cr->source_clip;

    int32_t start_in_clip = chunk_tl_start - cr->tl_pos + cr->start_in_clip;
    int32_t end_in_clip = chunk_tl_end - cr->tl_pos + cr->start_in_clip;

    if (end_in_clip > cr->start_in_clip + clipref_len(cr)) {
	end_in_clip = cr->start_in_clip + clipref_len(cr);
    }

    PmEvent *dyn_events;
    int num_events = midi_clip_get_events(
	mclip,
	&dyn_events,
	start_in_clip,
	end_in_clip,
	cr->end_in_clip == 0 ? mclip->len_sframes - 1 : cr->end_in_clip - 1,
	cr->tl_pos - cr->start_in_clip,
	&cr->track->note_offs);
    if (num_events > event_buf_max_len) {
	fprintf(stderr, "Error! Event buf full!\n");
	num_events = event_buf_max_len;
    }
    /* fprintf(stderr, "(%d-%d): %d events\n", chunk_tl_start, chunk_tl_end, num_events); */
    memcpy(event_buf, dyn_events, num_events * sizeof(PmEvent));
    free(dyn_events);
#ifdef TESTBUILD
    for (int i=0; i<num_events; i++) {
	if (event_buf[i].timestamp < chunk_tl_start || event_buf[i].timestamp >= chunk_tl_end) {
	    breakfn();
	}
    }
#endif

    return num_events;
}

Note *midi_clipref_get_next_note(ClipRef *cr, int32_t from, int32_t *pos_dst)
{
    MIDIClip *mclip = cr->source_clip;
    int32_t pos = from;
    Note *ret = NULL;
    int32_t note_i = midi_clipref_check_get_first_note(cr);
    do {
	Note *note = mclip->notes + note_i;
	int32_t note_tl_pos = note_tl_start_pos(note, cr);
	if (note_tl_pos > from) {
	    ret = note;
	    pos = note_tl_pos;
	    break;
	}
	note_i++;
    } while (note_i < mclip->num_notes);
    if (ret)
	*pos_dst = pos;
    return ret;
}

Note *midi_clipref_get_prev_note(ClipRef *cr, int32_t from, int32_t *pos_dst)
{
    MIDIClip *mclip = cr->source_clip;
    int32_t pos = from;
    Note *ret = NULL;
    int32_t note_i = midi_clipref_check_get_last_note(cr);
    /* fprintf(stderr, "Note start i: %d\n", note_i); */
    do {
	Note *note = mclip->notes + note_i;
	int32_t note_tl_pos = note_tl_start_pos(note, cr);
	if (note_tl_pos < from) {
	    ret = note;
	    pos = note_tl_pos;
	    break;
	}
	note_i--;
    } while (note_i >= 0);
    if (ret)
	*pos_dst = pos;
    return ret;
}

Note *midi_clipref_note_at_pos(ClipRef *cr, int32_t pos)
{
    int32_t note_i = midi_clipref_check_get_last_note(cr);
    int32_t first_note = midi_clipref_check_get_first_note(cr);
    MIDIClip *mclip = cr->source_clip;
    while (note_i >= first_note) {
	Note *note = mclip->notes + note_i;
	int32_t note_tl_start = note_tl_start_pos(note, cr);
	int32_t note_tl_end = note_tl_end_pos(note, cr);
	if (note_tl_start <= pos && note_tl_end >= pos) {
	    return note;
	}	    
	note_i--;
    }
    return NULL;
}

/* If dst ptr is provided, its contents must be freed */
int midi_clipref_notes_intersecting_point(ClipRef *cr, int32_t tl_pos, Note ***dst)
{
    Note *intersecting_notes[128];
    int num_intersecting_notes = 0;
    int32_t note_i = midi_clipref_check_get_first_note(cr);
    MIDIClip *mclip = cr->source_clip;
    while (note_i < mclip->num_notes) {
	Note *note = mclip->notes + note_i;
	int32_t start_pos = note_tl_start_pos(note, cr);
	if (start_pos <= tl_pos && note_tl_end_pos(note, cr) >= tl_pos) {
	    intersecting_notes[num_intersecting_notes] = note;
	    num_intersecting_notes++;
	    if (num_intersecting_notes == 128) break;
	} else if (start_pos > tl_pos) {
	    break;
	}
	note_i++;
    }
    if (dst && num_intersecting_notes > 0) {
	*dst = malloc(sizeof(Note *) * num_intersecting_notes);
	memcpy(*dst, intersecting_notes, sizeof(Note *) * num_intersecting_notes);
    }
    return num_intersecting_notes;
}

static int midi_clipref_notes_intersecting_area(ClipRef *cr, int32_t range_start, int32_t range_end, int bottom_note, int top_note, Note ***dst)
{
    Note **intersecting_notes = malloc(MAX_INTERSECTING_NOTES * sizeof(Note *));
    int num_intersecting_notes = 0;
    int32_t note_i = midi_clipref_check_get_first_note(cr);
    MIDIClip *mclip = cr->source_clip;
    while (note_i < mclip->num_notes) {
	Note *note = mclip->notes + note_i;
	if (note->key < bottom_note || note->key > top_note) {
	    note_i++;
	    continue;
	}
	int32_t note_start = note_tl_start_pos(note, cr);
	int32_t note_end = note_tl_end_pos(note, cr);
	if (note_start < range_end && note_end > range_start) {
	    intersecting_notes[num_intersecting_notes] = note;
	    num_intersecting_notes++;
	    if (num_intersecting_notes == MAX_INTERSECTING_NOTES) break;
	} else if (note_start > range_end) {
	    break;
	}
	note_i++;
    }
    if (dst && num_intersecting_notes > 0) {
	*dst = malloc(sizeof(Note *) * num_intersecting_notes);
	memcpy(*dst, intersecting_notes, sizeof(Note *) * num_intersecting_notes);
    }
    free(intersecting_notes);
    return num_intersecting_notes;
}

int midi_clipref_notes_ending_at_pos(ClipRef *cr, int32_t tl_pos, Note ***dst, bool latest_start_time, int32_t *start_pos_dst)
{
    MIDIClip *mclip = cr->source_clip;
    int32_t note_i = midi_clipref_check_get_first_note(cr);
    int32_t last_i = midi_clipref_check_get_last_note(cr);
    if (note_i == -1) return 0;
    Note **intersecting = malloc(MAX_INTERSECTING_NOTES * sizeof(Note *));
    int num_intersecting = 0;
    while (note_i <= last_i) {
	Note *note = mclip->notes + note_i;
	if (note_tl_end_pos(note, cr) == tl_pos) {
	    intersecting[num_intersecting] = note;
	    num_intersecting++;
	    if (num_intersecting == MAX_INTERSECTING_NOTES) {
		break;
	    }
	}
	note_i++;
    }
    int num_returned = 0;
    if (latest_start_time) {
	int32_t max_start_pos = INT32_MIN;
	for (int i=0; i<num_intersecting; i++) {
	    int32_t start_pos = note_tl_start_pos(intersecting[i], cr);
	    if (start_pos > max_start_pos) max_start_pos = start_pos;
	}
	*dst = malloc(num_intersecting * sizeof(Note *));
	for (int i=0; i<num_intersecting; i++) {
	    int32_t start_pos = note_tl_start_pos(intersecting[i], cr);
	    if (start_pos == max_start_pos) {
		(*dst)[num_returned] = intersecting[i];
		num_returned++;
	    }
	}
	*start_pos_dst = max_start_pos;
	*dst = realloc(*dst, num_returned * sizeof(Note *));
    } else {
	*dst = malloc(num_intersecting * sizeof(Note *));
	memcpy(*dst, intersecting, num_intersecting * sizeof(Note *));
	num_returned = num_intersecting;
    }
    free(intersecting);
    return num_returned;
}

Note *midi_clipref_up_note_at_cursor(ClipRef *cr, int32_t cursor, int sel_key)
{
    Note **intersecting_notes = NULL;
    int num_intersecting_notes = midi_clipref_notes_intersecting_point(cr, cursor, &intersecting_notes);
    Note *ret = NULL;
    if (num_intersecting_notes > 0 && intersecting_notes) {
	int diff = 128;
	for (int i=0; i<num_intersecting_notes; i++) {
	    Note *check = intersecting_notes[i];
	    int diff_loc = check->key - sel_key;
	    if (diff_loc > 0 && diff_loc < diff) {
		ret = check;
		diff = diff_loc;
	    }
	}
	free(intersecting_notes);
    }
    return ret;
}

Note *midi_clipref_down_note_at_cursor(ClipRef *cr, int32_t cursor, int sel_key)
{
    Note **intersecting_notes = NULL;
    int num_intersecting_notes = midi_clipref_notes_intersecting_point(cr, cursor, &intersecting_notes);
    Note *ret = NULL;
    if (num_intersecting_notes > 0 && intersecting_notes) {
	int diff = 128;
	for (int i=0; i<num_intersecting_notes; i++) {
	    Note *check = intersecting_notes[i];
	    int diff_loc = sel_key - check->key;
	    if (diff_loc > 0 && diff_loc < diff) {
		ret = check;
		diff = diff_loc;
	    }
	}
	free(intersecting_notes);
    }
    return ret;
}

/* GRABBED NOTES */

static void grabbed_notes_incr(MIDIClip *mclip, int32_t from_i, int32_t until_i, int by)
{

    if (mclip->first_grabbed_note >= from_i && mclip->first_grabbed_note < until_i) {
	mclip->first_grabbed_note += by;
    }
    if (mclip->last_grabbed_note >= from_i && mclip->last_grabbed_note < until_i) {
	mclip->last_grabbed_note += by;
    }
}

static void grabbed_notes_reset(MIDIClip *mclip)
{
    mclip->num_grabbed_notes = 0;
    bool first_set = false;
    Note *last = NULL;
    for (int32_t i=0; i<mclip->num_notes; i++) {
	Note *note = mclip->notes + i;
	if (note->grabbed) {
	    if (!first_set) {
		mclip->first_grabbed_note = i;
		first_set = true;
	    }
	    mclip->num_grabbed_notes++;
	    last = note;
	}	
    }
    if (last)
	mclip->last_grabbed_note = last - mclip->notes;
}

void midi_clip_grab_note(MIDIClip *mclip, Note *note, NoteEdge edge)
{
    /* fprintf(stderr, "GRAB note %d\n", note->key); */
    if (note->grabbed) {
	/* fprintf(stderr, "\t->already grabbed, grabbing edge %d\n", edge); */
	note->grabbed_edge = edge;
	return;
    }
    note->grabbed = true;
    note->grabbed_edge = edge;
    int32_t note_i = note - mclip->notes;
    mclip->first_grabbed_note = mclip->num_notes - 1;
    for (int32_t i=0; i<mclip->num_notes; i++) {
	if (mclip->notes[i].grabbed) {
	    if (i < mclip->first_grabbed_note) {
		mclip->first_grabbed_note = i;
	    } else if (i > mclip->last_grabbed_note) {
		mclip->last_grabbed_note = i;
	    }
	} 
    }
    if (note_i < mclip->first_grabbed_note) mclip->first_grabbed_note = note_i;
    else if (note_i > mclip->last_grabbed_note) mclip->last_grabbed_note = note_i;
    mclip->num_grabbed_notes++;
}

void midi_clip_ungrab_all(MIDIClip *mclip)
{
    /* fprintf(stderr, "UNGRAB ALL (%d)\n", mclip->num_grabbed_notes); */
    if (mclip->num_grabbed_notes == 0) return;
    for (int32_t i=mclip->first_grabbed_note; i<=mclip->last_grabbed_note; i++) {
	Note *note = mclip->notes + i;
	if (note->grabbed) {
	    note->grabbed_edge = NOTE_EDGE_NONE;
	    note->grabbed = false;
	}
    }
    mclip->first_grabbed_note = 0;
    mclip->last_grabbed_note = 0;
    mclip->num_grabbed_notes = 0;
}

void midi_clipref_grab_range(ClipRef *cr, int32_t tl_start, int32_t tl_end)
{
    MIDIClip *mclip = cr->source_clip;
    Note **intersecting = NULL;
    int num_intersecting = midi_clipref_notes_intersecting_area(cr, tl_start, tl_end, 0, 127, &intersecting);
    for (int i=0; i<num_intersecting; i++) {
	midi_clip_grab_note(mclip, intersecting[i], NOTE_EDGE_NONE);
    }
    if (intersecting) free(intersecting);
}

void midi_clipref_grab_area(ClipRef *cr, int32_t tl_start, int32_t tl_end, int bottom_note, int top_note)
{
    Note **intersecting = NULL;
    int num_intersecting = midi_clipref_notes_intersecting_area(
	cr,
	tl_start,
	tl_end,
	bottom_note,
	top_note,
	&intersecting);
    /* fprintf(stderr, "(%d) TL: %d-%d; notes %d-%d\n", num_intersecting, tl_start, tl_end, bottom_note, top_note); */
    for (int i=0; i<num_intersecting; i++) {
	midi_clip_grab_note(cr->source_clip, intersecting[i], NOTE_EDGE_NONE);
    }
    if (intersecting) free(intersecting);

}

static int notecmp(const void *a, const void *b)
{
    const Note *na = a;
    const Note *nb = b;
    int32_t diff = na->start_rel - nb->start_rel;
    if (diff != 0) return diff;
    return na->id - nb->id;
    /* return ((Note *)a)->start_rel - ((Note *)b)->start_rel; */
}

static void midi_clip_resort_notes(MIDIClip *mclip)
{
    qsort(mclip->notes, mclip->num_notes, sizeof(Note), notecmp);
    midi_clip_check_reset_bounds(mclip);
    grabbed_notes_reset(mclip);
    
}

void midi_clip_grabbed_notes_move(MIDIClip *mclip, int32_t move_by)
{
    if (mclip->num_grabbed_notes == 0) return;
    pthread_mutex_lock(&mclip->notes_arr_lock);
    for (int32_t i=mclip->first_grabbed_note; i<=mclip->last_grabbed_note; i++) {
	Note *note = mclip->notes + i;
	if (!note->grabbed) continue;
	switch (note->grabbed_edge) {
	case NOTE_EDGE_NONE:
	    note->start_rel += move_by;
	    note->end_rel += move_by;
	    break;
	case NOTE_EDGE_LEFT:
	    note->start_rel += move_by;
	    if (note->start_rel > note->end_rel) note->start_rel = note->end_rel;
	    break;
	case NOTE_EDGE_RIGHT:
	    note->end_rel += move_by;
	    if (note->end_rel < note->start_rel) note->end_rel = note->start_rel;
	    break;
	}
	/* while (note > mclip->notes && note->start_rel < (note - 1)->start_rel) { */
	/*     Note save = *(note - 1); */
	/*     *(note - 1) = *note; */
	/*     *note = save; */
	/*     note--; */
	/* } */
	/* while (note < mclip->notes + mclip->num_notes - 1 && note->start_rel > (note + 1)->start_rel) { */
	/*     Note save = *(note + 1); */
	/*     *(note + 1) = *note; */
	/*     *note = save; */
	/*     note++; */
	/* } */
    }
    qsort(mclip->notes, mclip->num_notes, sizeof(Note), notecmp);
    /* static void midi_clip_check_reset_bounds(MIDIClip *mc) */
    pthread_mutex_unlock(&mclip->notes_arr_lock);
    midi_clip_check_reset_bounds(mclip);
    grabbed_notes_reset(mclip);
    TEST_FN_CALL(check_note_order, mclip);
}


void midi_clip_reinsert_notes(MIDIClip *mclip, Note *notes, int num_notes)
{
    for (int i=0; i<num_notes; i++) {
	Note *note = notes + i;
	Note *new = midi_clip_insert_note(
	    mclip,
	    note->channel,
	    note->key,
	    note->velocity,
	    note->start_rel,
	    note->end_rel);
	new->id = note->id;
    }
}

static bool note_in_list(const Note *note, Note *list, int list_len)
{
    for (int i=0; i<list_len; i++) {
	if (list[i].id == note->id) {
	    /* if (i < list_len - 1) { */
	    /* 	memmove(list + i, list + i + 1, sizeof(Note) * (list_len - i - 1)); */
	    /* } */
	    return true;
	}
    }
    return false;
}

void midi_clip_remove_notes_by_id(MIDIClip *mclip, Note *notes, int num_notes)
{
    for (int32_t note_i=0; note_i<mclip->num_notes; note_i++) {
	/* If the note is in the list of IDs */
	if (note_in_list(mclip->notes + note_i, notes, num_notes)) {
	    /* num_notes--; /\* Search list has been reduced *\/ */
	    memmove(mclip->notes + note_i, mclip->notes + note_i + 1, sizeof(Note) * (mclip->num_notes - note_i - 1));
	    mclip->num_notes--;
	    note_i--;
	}
    }
}

static Note *find_note(MIDIClip *mclip, int32_t start_rel, int key, int velocity)
{
    Note *note = mclip->notes;
    while (note - mclip->notes < mclip->num_notes && note->start_rel < start_rel) note++;
    while (note - mclip->notes < mclip->num_notes) {
	if (note->start_rel == start_rel
	    && note->key == key
	    && note->velocity == velocity) {
	    return note;
	}
	note++;
    }
    return NULL;
}
NEW_EVENT_FN(undo_delete_grabbed_notes, "undo delete grabbed notes")
    ClipRef *cr = obj1;
    MIDIClip *clip = cr->source_clip;
    Note *notes = obj2;
    int32_t num_notes = val1.int32_v;
    midi_clip_reinsert_notes(clip, notes, num_notes);
    if (session_get()->piano_roll) {
	int32_t max_end = INT32_MIN;
        for (int i=0; i<num_notes; i++) {
	    int32_t tl_end = note_tl_end_pos(notes + i, cr);
	    if (tl_end > max_end) max_end = tl_end;
	}
	if (max_end != INT32_MIN) timeline_set_play_position(cr->track->tl, max_end, false);
    }
}
static void midi_clipref_grabbed_notes_delete_internal(ClipRef *cr, bool from_undo);

NEW_EVENT_FN(redo_delete_grabbed_notes, "redo delete grabbed notes")
    ClipRef *cr = obj1;
    MIDIClip *clip = cr->source_clip;
    midi_clip_ungrab_all(clip);
    Note *notes = obj2;
    int32_t num_notes = val1.int32_v;
    /* fprintf(stderr, "RE-DELETING %d notes\n", num_notes); */
    int32_t min_start = INT32_MAX;
    for (int i=0; i<num_notes; i++) {
	Note *note = find_note(clip, notes[i].start_rel, notes[i].key, notes[i].velocity);
	if (!note) fprintf(stderr, "ERROR: note not found!\n");
	int32_t tl_start = note_tl_start_pos(note, cr);
	if (tl_start < min_start) min_start = tl_start;
	midi_clip_grab_note(clip, note, NOTE_EDGE_NONE);
    }
    if (min_start < INT32_MAX && session_get()->piano_roll) {
	timeline_set_play_position(cr->track->tl, min_start, false);
    }
    midi_clipref_grabbed_notes_delete_internal(cr, true);
}

void midi_clip_remove_note_at(MIDIClip *mclip, int32_t note_i)
{
    if (mclip->num_notes > 1 && note_i < mclip->num_notes - 1) {
	memmove(mclip->notes + note_i, mclip->notes + note_i + 1, (mclip->num_notes - note_i - 1) * sizeof(Note));
	if (mclip->last_grabbed_note > note_i) {
	    mclip->last_grabbed_note--;	    
	} else {
	    grabbed_notes_reset(mclip);
	}
    }
    mclip->num_notes--;
}

static void midi_clipref_grabbed_notes_delete_internal(ClipRef *cr, bool from_undo)
{
    MIDIClip *mclip = cr->source_clip;
    Note *grabbed_note_info = calloc(mclip->num_grabbed_notes, sizeof(Note));
    /* for (int i=0; i<mclip->num_grabbed_notes; i++) { */
    int32_t grabbed_note_i = 0;
    if (mclip->num_grabbed_notes == 0) return;
    for (int32_t note_i=mclip->first_grabbed_note; note_i<=mclip->last_grabbed_note; note_i++) {
	Note *note = mclip->notes + note_i;
	if (!note->grabbed) continue;
	note->grabbed = false;
	note->grabbed_edge = NOTE_EDGE_NONE;
	grabbed_note_info[grabbed_note_i] = *note;
	grabbed_note_i++;
	if (mclip->num_notes > 1 && note_i < mclip->num_notes - 1) {
	    memmove(mclip->notes + note_i, mclip->notes + note_i + 1, (mclip->num_notes - note_i - 1) * sizeof(Note));
	    mclip->last_grabbed_note--;
	    /* grabbed_notes_incr(mclip, note_i, mclip->num_notes, -1); */
	}
	note_i--;
	mclip->num_notes--;
    }
    mclip->num_grabbed_notes = 0;
    if (mclip->last_grabbed_note >= mclip->num_notes) {
	mclip->last_grabbed_note = mclip->num_notes - 1;
    }
    if (!from_undo) {
	user_event_push(
	    undo_delete_grabbed_notes,
	    redo_delete_grabbed_notes,
	    NULL,
	    NULL,
	    cr,
	    grabbed_note_info,
	    (Value){.int32_v = grabbed_note_i},
	    (Value){0},
	    (Value){.int32_v = grabbed_note_i},
	    (Value){0},
	    0, 0, false, true);	    
    }

}


void midi_clipref_grabbed_notes_delete(ClipRef *cr)
{
    midi_clipref_grabbed_notes_delete_internal(cr, false);
}


/* GRABBED NOTE MOVES
   - CACHE
   - PUSH EVENT
   - UNDO/REDO
*/


struct note_move_info {
    int32_t note_id;
    int32_t start_tl_before;
    int32_t start_tl_after;
    int32_t end_tl_before;
    int32_t end_tl_after;
    uint8_t key_before;
    uint8_t key_after;
};


int32_t midi_clip_get_note_by_id(MIDIClip *mclip, uint32_t id)
{
    for (int32_t i=0; i<mclip->num_notes; i++) {
	/* fprintf(stderr, "TEST note %d, id %d<=>%d\n", i, mclip->notes[i].id, id); */
	if (mclip->notes[i].id == id) {
	    return i;
	}
    }
    return -1;
}

NEW_EVENT_FN(undo_move_grabbed_notes, "undo move grabbed notes")
    ClipRef *cr = obj1;
    MIDIClip *mclip = cr->source_clip;
    struct note_move_info *info = obj2;
    int32_t num_notes = val1.int32_v;
    for (int32_t i=0; i<num_notes; i++) {
	int32_t note_i = midi_clip_get_note_by_id(mclip, info[i].note_id);
	Note *note = mclip->notes + note_i;
	note->start_rel = info[i].start_tl_before - cr->tl_pos + cr->start_in_clip;
	note->end_rel = info[i].end_tl_before - cr->tl_pos + cr->start_in_clip;;
	note->key = info[i].key_before;
    }
    midi_clip_resort_notes(mclip);
    /* for (int32_t i=0; i<num_notes; i++) { */
    /*     int32_t new_index = get_note_by_id(mclip, info[i].note_id); */
    /*     info[i].note_index = new_index; */
    /* } */
}


NEW_EVENT_FN(redo_move_grabbed_notes, "redo move grabbed notes")
    ClipRef *cr = obj1;
    MIDIClip *mclip = cr->source_clip;
    struct note_move_info *info = obj2;
    int32_t num_notes = val1.int32_v;
    for (int32_t i=0; i<num_notes; i++) {
	int32_t note_i = midi_clip_get_note_by_id(mclip, info[i].note_id);
	if (note_i == -1) {
	    fprintf(stderr, "ERROR: i=%d, note_i == -1\n", i);
	    exit(1);
	}
        Note *note = mclip->notes + note_i;
	note->start_rel = info[i].start_tl_after - cr->tl_pos + cr->start_in_clip;
	note->end_rel = info[i].end_tl_after - cr->tl_pos + cr->start_in_clip;
	note->key = info[i].key_after;
    }
    midi_clip_resort_notes(mclip);
    /* for (int32_t i=0; i<num_notes; i++) { */
    /* 	int32_t new_index = get_note_by_id(mclip, info[i].note_id); */
    /* 	info[i].note_index = new_index; */
    /* } */
}


/* Call *after* a move event has completed
   Assumes that note indices remain constant :|
 */
void midi_clipref_push_grabbed_note_move_event(ClipRef *cr)
{
    MIDIClip *mclip = cr->source_clip;
    /* fprintf(stderr, "Push! note move in progress? %d num grabbed? %d\n", mclip->note_move_in_progress, mclip->num_grabbed_notes); */
    if (mclip->num_grabbed_notes == 0) return;
    if (!mclip->note_move_in_progress) return;
    mclip->note_move_in_progress = false;
    struct note_move_info *info = calloc(mclip->num_grabbed_notes, sizeof(struct note_move_info));
    int32_t info_i = 0;
    for (int32_t i=mclip->first_grabbed_note; i<=mclip->last_grabbed_note; i++) {
	Note *note = mclip->notes + i;	    
	if (note->grabbed) {
	    if (info_i >= mclip->num_grabbed_notes) {
		fprintf(stderr, "ERROR: in push_grabbed_note_move_event: num grabbed notes (indices %d %d, num %d)\n", mclip->first_grabbed_note, mclip->last_grabbed_note, mclip->num_grabbed_notes);
		free(info);
		return;
	    }
	    /* info[info_i].note_index = i; */
	    info[info_i].note_id = note->id;
	    info[info_i].start_tl_before = note->cached_start_tl_pos;
	    info[info_i].start_tl_after = note_tl_start_pos(note, cr);
	    info[info_i].end_tl_before = note->cached_end_tl_pos;
	    info[info_i].end_tl_after = note_tl_end_pos(note, cr);;
	    info[info_i].key_before = note->cached_key;
	    info[info_i].key_after = note->key;
	    /* fprintf(stderr, "PUSH key ID %d\n\ttl range before: %d-%d\n\trange after: %d-%d\n\tkey: %d=>%d\n", */
	    /* 	    note->id, */
	    /* 	    info[info_i].start_tl_before, */
	    /* 	    info[info_i].end_tl_before, */
	    /* 	    info[info_i].start_tl_after, */
	    /* 	    info[info_i].end_tl_after, */
	    /* 	    info[info_i].key_before, */
	    /* 	    info[info_i].key_after); */		    
	    info_i++;
	}
    }
    /*
      user_event_push(EventFn undo_fn,
      EventFn redo_fn, EventFn dispose_fn, EventFn dispose_forward_fn,
      void *obj1, void *obj2, Value undo_val1, Value undo_val2, Value redo_val1, Value redo_val2,
      ValType type1, ValType type2, bool free_obj1, bool free_obj2) -> UserEvent *
    */
    /* fprintf(stderr, "PUSHING EVENT %d notes\n", info_i); */
    /* for (int32_t i=0; i<info_i; i++) { */
    /* 	fprintf(stderr, "\t->note index %d (%d-%d)=>(%d-%d), key (%d->%d)\n", info[i].note_index, info[i].start_rel_before, info[i].end_rel_before, info[i].start_rel_after, info[i].end_rel_after, info[i].key_before, info[i].key_after); */
    /* } */
    user_event_push(
	undo_move_grabbed_notes,
	redo_move_grabbed_notes,
	NULL, NULL,
	cr,
	info,
	(Value){.int32_v = info_i},
	(Value){0},
	(Value){.int32_v = info_i},
	(Value){0},
	0, 0, false, true);
}

/* Call when a move event *starts* */
void midi_clipref_cache_grabbed_note_info(ClipRef *cr)
{
    /* If a cache operation requested while a move is in progress,
     (e.g. when changing track while dragging), then an event is
     pushed immediately, effectively breaking up the larger move event. */
    MIDIClip *mclip = cr->source_clip;
    if (mclip->note_move_in_progress) {
	midi_clipref_push_grabbed_note_move_event(cr);
    }
    if (mclip->num_grabbed_notes == 0) return;
    for (int32_t i=mclip->first_grabbed_note; i<=mclip->last_grabbed_note; i++) {
	Note *note = mclip->notes + i;
	if (!note->grabbed) continue;
	note->cached_start_tl_pos = note_tl_start_pos(note, cr);
	note->cached_end_tl_pos = note_tl_end_pos(note, cr);
	/* note->cached_start_rel = note->start_rel; */
	/* note->cached_end_rel = note->end_rel; */
	note->cached_key = note->key;
	/* fprintf(stderr, "CACHED note %d\n", i); */
    }
    mclip->note_move_in_progress = true;
}

/*------ Quantize internal implementation ----------------------------*/

struct quantize_undo_info {
    uint32_t note_id;
    struct note_quantize_info old_info;
    struct note_quantize_info new_info;
};

/* Set note positions based on quantize info (and parent clipref) */
static void note_apply_quantize_amt(ClipRef *cr, Note *note)
{
    int32_t tl_pos_orig = click_track_pos_to_tl_pos(&note->quantize_info.orig_start_pos);    
    if (!note->quantize_info.quantized) {
	note->start_rel = tl_pos_orig - cr->tl_pos + cr->start_in_clip;
	tl_pos_orig = click_track_pos_to_tl_pos(&note->quantize_info.orig_end_pos);
	note->end_rel = tl_pos_orig - cr->tl_pos + cr->start_in_clip;
	return;
    }
    int32_t tl_pos_quantized = click_track_pos_to_tl_pos(&note->quantize_info.quantized_start_pos);
    int32_t diff = tl_pos_quantized - tl_pos_orig;
    int32_t dst_tl_pos = tl_pos_orig + diff * note->quantize_info.amt;
    note->start_rel = dst_tl_pos - cr->tl_pos + cr->start_in_clip;
    if (note->quantize_info.quantize_note_off) {
	tl_pos_orig = click_track_pos_to_tl_pos(&note->quantize_info.orig_end_pos);
	tl_pos_quantized = click_track_pos_to_tl_pos(&note->quantize_info.quantized_end_pos);
	diff = tl_pos_quantized - tl_pos_orig;
	dst_tl_pos = tl_pos_orig + diff * note->quantize_info.amt;
	note->end_rel = dst_tl_pos - cr->tl_pos + cr->start_in_clip;
    } else {
	tl_pos_orig = click_track_pos_to_tl_pos(&note->quantize_info.orig_end_pos);
	dst_tl_pos = tl_pos_orig + diff * note->quantize_info.amt;
	note->end_rel = dst_tl_pos - cr->tl_pos + cr->start_in_clip;	
    }
}

NEW_EVENT_FN(undo_quantize_notes, "undo quantize notes")
    ClipRef *cr = obj1;
    MIDIClip *mclip = cr->source_clip;
    struct quantize_undo_info *info = obj2;
    int32_t num_notes = val1.int32_v;
    for (int32_t i=0; i<num_notes; i++) {
	Note *note = mclip->notes + midi_clip_get_note_by_id(cr->source_clip, info[i].note_id);
	note->quantize_info = info[i].old_info;
	note_apply_quantize_amt(cr, note);
    }
    cr->track->tl->needs_redraw = true;
}

NEW_EVENT_FN(redo_quantize_notes, "redo quantize notes")
    ClipRef *cr = obj1;
    MIDIClip *mclip = cr->source_clip;
    struct quantize_undo_info *info = obj2;
    int32_t num_notes = val1.int32_v;
    for (int32_t i=0; i<num_notes; i++) {
	Note *note = mclip->notes + midi_clip_get_note_by_id(cr->source_clip, info[i].note_id);
	note->quantize_info = info[i].new_info;
	note_apply_quantize_amt(cr, note);
    }
    cr->track->tl->needs_redraw = true;

}



static void midi_clipref_quantize_notes(ClipRef *cr, Note **notes, int num, ClickTrack *ct, float amount, BeatProminence resolution, bool quantize_note_offs)
{
    struct quantize_undo_info *undo_info = malloc(sizeof(struct quantize_undo_info) * num);
    for (int i=0; i<num; i++) {
	undo_info[i].note_id = notes[i]->id;
	notes[i]->quantize_info.orig_start_pos = click_track_get_pos(ct, note_tl_start_pos(notes[i], cr));
	notes[i]->quantize_info.orig_end_pos = click_track_get_pos(ct, note_tl_end_pos(notes[i], cr));
	undo_info[i].old_info = notes[i]->quantize_info;
	notes[i]->quantize_info.amt = amount;
	notes[i]->quantize_info.quantized = true;
	ClickTrackPos quantized_start = click_track_pos_round(notes[i]->quantize_info.orig_start_pos, resolution);
	notes[i]->quantize_info.quantized_start_pos = quantized_start;
	if ((notes[i]->quantize_info.quantize_note_off = quantize_note_offs)) {
	    ClickTrackPos quantized_end = click_track_pos_round(notes[i]->quantize_info.orig_end_pos, resolution);
	    if (memcmp(&quantized_start, &quantized_end, sizeof(ClickTrackPos)) == 0) {
		quantized_end = click_track_pos_do_increment(quantized_end, resolution);
	    }
	    notes[i]->quantize_info.quantized_end_pos = quantized_end;
	}
	note_apply_quantize_amt(cr, notes[i]);
	undo_info[i].new_info = notes[i]->quantize_info;
    }
    user_event_push(
	undo_quantize_notes,
	redo_quantize_notes,
	NULL, NULL,
	cr,
	undo_info,
	(Value){.int32_v = num},
	(Value){0},
	(Value){.int32_v = num},
	(Value){0},
	0, 0, false, true);
}


/* Amt 0.0 - 1.0 describes how close to the note gets to the beat */
static void midi_clipref_quantize_notes_in_range(ClipRef *cr, float amount, BeatProminence resolution, bool quantize_note_offs)
{
    if (!cr) return;
    Timeline *tl = cr->track->tl;
    ClickTrack *ct = track_governing_click_track(cr->track);
    if (!ct) {
	status_set_errstr("Add a click track (C-t) before quantizing");
	return;
    }
    
    Note **intersecting;
    int num = midi_clipref_notes_intersecting_area(cr, tl->in_mark_sframes, tl->out_mark_sframes, 0, 127, &intersecting);
    if (num == 0) return;
    midi_clipref_quantize_notes(cr, intersecting, num, ct, amount, resolution, quantize_note_offs);
    /* uint32_t *note_ids = malloc(sizeof(uint32_t) * num);; */
    free(intersecting);
}

/* Amt 0.0 - 1.0 describes how close to the note gets to the beat */
static void midi_clipref_quantize_all_notes(ClipRef *cr, float amount, BeatProminence resolution, bool quantize_note_offs)
{
    if (!cr) return;
    Timeline *tl = cr->track->tl;
    ClickTrack *ct = track_governing_click_track(cr->track);
    if (!ct) {
	status_set_errstr("Add a click track (C-t) before quantizing");
	return;
    }
    
    Note **intersecting;
    int num = midi_clipref_notes_intersecting_area(cr, cr->tl_pos, cr->tl_pos + clipref_len(cr), 0, 127, &intersecting);
    if (num == 0) return;
    midi_clipref_quantize_notes(cr, intersecting, num, ct, amount, resolution, quantize_note_offs);
    /* uint32_t *note_ids = malloc(sizeof(uint32_t) * num);; */
    free(intersecting);
}


struct note_quant_amt {
    uint32_t note_id;
    float undo_amt;
};

static void midi_clipref_notes_adj_quantize_amount(ClipRef *cr, Note **notes, int32_t num, float new_amount, bool from_undo);
NEW_EVENT_FN(undo_adj_quantize_amt, "undo adj quantize amount")
    ClipRef *cr = obj1;
    MIDIClip *mclip = cr->source_clip;
    struct note_quant_amt *info = obj2;
    int32_t num = val1.int32_v;
    Note *note;
    for (int32_t i=0; i<num; i++) {
        note = mclip->notes + midi_clip_get_note_by_id(mclip, info[i].note_id);
	midi_clipref_notes_adj_quantize_amount(cr, &note, 1, info[i].undo_amt, true);
    }
}

NEW_EVENT_FN(redo_adj_quantize_amt, "redo adj quantize amount")
    ClipRef *cr = obj1;
    MIDIClip *mclip = cr->source_clip;
    struct note_quant_amt *info = obj2;
    int32_t num = val1.int32_v;
    Note **notes = malloc(num * sizeof(Note *));
    for (int32_t i=0; i<num; i++) {
        notes[i] = mclip->notes + midi_clip_get_note_by_id(mclip, info[i].note_id);
    }
    midi_clipref_notes_adj_quantize_amount(cr, notes, num, val2.float_v, true);
    free(notes);
}



static void midi_clipref_notes_adj_quantize_amount(ClipRef *cr, Note **notes, int32_t num, float new_amount, bool from_undo)
{
    struct note_quant_amt *undo_info;
    if (!from_undo) {
	undo_info = malloc(num * sizeof(struct note_quant_amt));
    }
    for (int32_t i=0; i<num; i++) {
	undo_info[i].undo_amt = notes[i]->quantize_info.amt;
	undo_info[i].note_id = notes[i]->id;
	if (!notes[i]->quantize_info.quantized) continue;
	notes[i]->quantize_info.amt = new_amount;
	note_apply_quantize_amt(cr, notes[i]);
	/* notes[i]->start_rel = notes[i]->quantize_info.start_rel_before + notes[i]->quantize_info.start_diff * new_amount; */
	/* notes[i]->end_rel = notes[i]->quantize_info.end_rel_before + new_amount * notes[i]->quantize_info.end_diff * new_amount; */
    }
    if (!from_undo) {
	user_event_push(
	    undo_adj_quantize_amt,
	    redo_adj_quantize_amt,
	    NULL, NULL,
	    cr, undo_info,
	    (Value){.int32_v = num},
	    (Value){0},
	    (Value){.int32_v = num},
	    (Value){.float_v = new_amount},
	    0, 0, false, true);
	    
	    
    }
}

static void midi_clipref_notes_in_range_adj_quantize_amount(ClipRef *cr, float new_amount, int32_t start_tl_pos, int32_t end_tl_pos)
{
    if (!cr) return;
    Timeline *tl = cr->track->tl;    
    Note **intersecting;
    int num_intersecting = midi_clipref_notes_intersecting_area(cr, start_tl_pos, end_tl_pos, 0, 127, &intersecting);
    midi_clipref_notes_adj_quantize_amount(cr, intersecting, num_intersecting, new_amount, false);
    if (num_intersecting > 0) free(intersecting);
}

/*------ Quantize top-level interface --------------------------------*/

enum quantize_behavior_options {
    QUANTIZE_ENTIRE_CLIP=0,
    QUANTIZE_MARKED_RANGE=1
};
static int quantize_note_selection_behavior = 0;
static int quantize_beat_prominence = 1;
static bool quantize_note_offs = true;
static float quantize_amount = 1.0;

static int quantize_form_submit(void *modal_v, void *stashed_obj)
{
    /* ClipRef *cr = clipref_at_cursor(); */
    ClipRef *cr = stashed_obj;
    BeatProminence bp = quantize_beat_prominence + 2;
    /* bool quantize_note_offs =  */
    fprintf(stderr, "Quantize submit form, bp %d\n", bp);
    switch (quantize_note_selection_behavior) {
    case QUANTIZE_MARKED_RANGE:
	midi_clipref_quantize_notes_in_range(cr, quantize_amount, bp, quantize_note_offs);
	break;
    case QUANTIZE_ENTIRE_CLIP:
	midi_clipref_quantize_all_notes(cr, quantize_amount, bp, quantize_note_offs);
	/* midi_clipref_quantize_ */
	break;
    }
    window_pop_modal(main_win);
    Session *session = session_get();
    ACTIVE_TL->needs_redraw = true;
    return 0;    
}

static int quantize_amt_form_submit(void *modal_v, void *stashed_obj)
{
    /* ClipRef *cr = clipref_at_cursor(); */
    Session *session = session_get();
    ClipRef *cr = stashed_obj;
    fprintf(stderr, "Quantize amt submit form\n");
    switch (quantize_note_selection_behavior) {
    case QUANTIZE_ENTIRE_CLIP:
	midi_clipref_notes_in_range_adj_quantize_amount(cr, quantize_amount, cr->tl_pos, cr->tl_pos + clipref_len(cr));
	/* midi_clipref_quantize_ */
	break;
    case QUANTIZE_MARKED_RANGE:
	midi_clipref_notes_in_range_adj_quantize_amount(cr, quantize_amount, ACTIVE_TL->in_mark_sframes, ACTIVE_TL->out_mark_sframes);
	/* midi_clipref_quantize_notes_in_range(cr, quantize_amount, bp, true); */
	break;
    }
    window_pop_modal(main_win);
    ACTIVE_TL->needs_redraw = true;
    return 0;
    
}


static void quantize_amt_gui_cb(Endpoint *ep)
{
    Slider *s = ep->xarg1;
    slider_reset(s);
}

static Endpoint note_selection_behavior_ep = {0};
static Endpoint amount_ep = {0};
static Endpoint beat_prominence_ep = {0};
static Endpoint quantize_note_offs_ep = {0};

static const char *note_selection_radio_options[] = {
    "Entire clip",
    "Marked range"
};

static const char *beat_prominence_radio_options[] = {
    "Beat (e.g. quarter)",
    "Subdiv (e.g. eighth)",
    "Subdiv 2 (e.g. sixteenth)",
    "Subidv 3 (e.g. thirty-second)"
};

/* Creates modal for options; top-level interface */
void midi_clipref_quantize(ClipRef *cr)
{
    Session *session = session_get();    
    Layout *mod_lt = layout_add_child(main_win->layout);
    layout_set_default_dims(mod_lt);
    Modal *mod = modal_create(mod_lt);
    modal_add_header(mod, "Quantize notes", &colors.light_grey, 3);
    modal_add_p(mod, "This function will re-quantize any affected notes. To change the quantization amount instead, use \"adjust quantize amount\" instead.", &colors.light_grey);
    if (beat_prominence_ep.local_id == NULL) {
	endpoint_init(
	    &beat_prominence_ep,
	    &quantize_beat_prominence,
	    JDAW_INT,
	    "",
	    "",
	    JDAW_THREAD_MAIN,
	    NULL, NULL, NULL,
	    NULL, NULL, NULL, NULL);
	beat_prominence_ep.block_undo = true;
    }
    if (note_selection_behavior_ep.local_id == NULL) {
	endpoint_init(
	    &note_selection_behavior_ep,
	    &quantize_note_selection_behavior,
	    JDAW_INT,
	    "",
	    "",
	    JDAW_THREAD_MAIN,
	    NULL, NULL, NULL,
	    NULL, NULL,NULL,NULL);
	note_selection_behavior_ep.block_undo = true;
    }
    if (amount_ep.local_id == NULL) {
	endpoint_init(
	    &amount_ep,
	    &quantize_amount,
	    JDAW_FLOAT,
	    "",
	    "",
	    JDAW_THREAD_MAIN,
	    quantize_amt_gui_cb, NULL, NULL,
	    NULL, NULL,NULL,NULL);
	endpoint_set_allowed_range(&amount_ep, (Value){.float_v = 0.0}, (Value){.float_v = 1.0});
	endpoint_set_default_value(&amount_ep, (Value){.float_v = 1.0});
	amount_ep.block_undo = true;
    }
    if (quantize_note_offs_ep.local_id == NULL) {
	endpoint_init(
	    &quantize_note_offs_ep,
	    &quantize_note_offs,
	    JDAW_BOOL,
	    "",
	    "",
	    JDAW_THREAD_MAIN,
	    NULL, NULL, NULL,
	    NULL, NULL,NULL,NULL);
	endpoint_set_default_value(&quantize_note_offs_ep, (Value){.bool_v = true});
	quantize_note_offs_ep.block_undo = true;
    }


    modal_add_radio(mod, &colors.light_grey, &note_selection_behavior_ep, note_selection_radio_options, 2);

    modal_add_header(mod, "Resolution:", &colors.light_grey, 5);
    ModalEl *el = modal_add_radio(mod, &colors.light_grey, &beat_prominence_ep, beat_prominence_radio_options, 4);
    el->layout->y.value -= 15;
    /* layout_reset(el->layout); */
    /* layout_reset(el->layout); */
    /* modal_add_button(mod, "Create", new_tl_submit_form); */
    modal_add_header(mod, "Amount:", &colors.light_grey, 5);
    el = modal_add_slider(mod, &amount_ep, SLIDER_HORIZONTAL, SLIDER_FILL);
    amount_ep.xarg1 = el->obj;

    modal_add_header(mod, "Quantize note offs:", &colors.light_grey, 5);
    el = modal_add_toggle(mod, &quantize_note_offs_ep);
    
    modal_add_button(mod, "Quantize", quantize_form_submit);
    mod->stashed_obj = cr;
    mod->submit_form = quantize_form_submit;
    window_push_modal(main_win, mod);
    modal_reset(mod);
    modal_move_onto(mod);
    ACTIVE_TL->needs_redraw = true;
}

void midi_clipref_adj_quantize_amt(ClipRef *cr)
{
    if (beat_prominence_ep.local_id == NULL) {
	status_set_errstr("Cannot adjust quantize amount: no notes have been quantized. Quantize with A-q");
	return;
    }

    Session *session = session_get();    
    Layout *mod_lt = layout_add_child(main_win->layout);
    layout_set_default_dims(mod_lt);
    Modal *mod = modal_create(mod_lt);
    modal_add_header(mod, "Adjust quantize amount", &colors.light_grey, 3);
    modal_add_p(mod, "Adjust how close to move notes to their quantized position.", &colors.light_grey);

    modal_add_radio(mod, &colors.light_grey, &note_selection_behavior_ep, note_selection_radio_options, 2);
    modal_add_header(mod, "Amount:", &colors.light_grey, 5);
    ModalEl *el = modal_add_slider(mod, &amount_ep, SLIDER_HORIZONTAL, SLIDER_FILL);
    amount_ep.xarg1 = el->obj;

    modal_add_button(mod, "Apply", quantize_amt_form_submit);
    mod->stashed_obj = cr;
    mod->submit_form = quantize_amt_form_submit;
    window_push_modal(main_win, mod);
    modal_reset(mod);
    modal_move_onto(mod);
    ACTIVE_TL->needs_redraw = true;

}




/*------ grabbed note vel adj ----------------------------------------*/

/* Assumes at least one note grabbed. Used in piano roll */
void midi_clip_grabbed_pitch_range(MIDIClip *mclip, int *min_dst, int *max_dst)
{
    /* MIDIClip *mclip = cr->source_clip; */
    int min = 127;
    int max = 0;
    for (int32_t i=mclip->first_grabbed_note; i<=mclip->last_grabbed_note; i++) {
	Note *note = mclip->notes + i;
	if (!note->grabbed) continue;
	if (note->key < min) {
	    min = note->key;
	}
	if (note->key > max) {
	    max = note->key;
	}
    }
    *min_dst = min;
    *max_dst = max;
}

void midi_clip_grabbed_vel_range(MIDIClip *mclip, int *min_dst, int *max_dst)
{
    /* MIDIClip *mclip = cr->source_clip; */
    int min = 127;
    int max = 0;
    for (int32_t i=mclip->first_grabbed_note; i<=mclip->last_grabbed_note; i++) {
	Note *note = mclip->notes + i;
	if (!note->grabbed) continue;
	if (note->velocity < min) {
	    min = note->velocity;
	}
	if (note->velocity > max) {
	    max = note->velocity;
	}
    }
    *min_dst = min;
    *max_dst = max;
}


struct note_id_vel {
    uint32_t note_id;
    uint8_t prev_vel;
    uint8_t new_vel;
};

NEW_EVENT_FN(undo_adj_grabbed_note_velocities, "undo adj note velocity")
    MIDIClip *mclip = obj1;
    struct note_id_vel *info = obj2;
    int32_t num_notes = val1.int32_v;
    for (int32_t i=0; i<num_notes; i++) {
	Note *note = mclip->notes + midi_clip_get_note_by_id(mclip, info[i].note_id);
	note->velocity = info[i].prev_vel;
    }
}

NEW_EVENT_FN(redo_adj_grabbed_note_velocities, "redo adj note velocity")
    MIDIClip *mclip = obj1;
    struct note_id_vel *info = obj2;
    int32_t num_notes = val1.int32_v;
    for (int32_t i=0; i<num_notes; i++) {
	Note *note = mclip->notes + midi_clip_get_note_by_id(mclip, info[i].note_id);
	note->velocity = info[i].new_vel;
    }
}



void midi_clip_adj_grabbed_velocities(MIDIClip *mclip, int by)
{
    
    struct note_id_vel *undo_info = calloc(mclip->num_grabbed_notes, sizeof(struct note_id_vel));
    int32_t undo_info_index = 0;
    for (int32_t i=mclip->first_grabbed_note; i<=mclip->last_grabbed_note; i++) {
	Note *note = mclip->notes + i;
	if (!note->grabbed) continue;
	undo_info[undo_info_index].note_id = note->id;
	undo_info[undo_info_index].prev_vel = note->velocity;	
	int new_vel = note->velocity + by;
	if (new_vel > 127) new_vel = 127;
	else if (new_vel < 0) new_vel = 0;
	note->velocity = new_vel;
	undo_info[undo_info_index].prev_vel = note->velocity;
	undo_info_index++;
    }

    user_event_push(
	undo_adj_grabbed_note_velocities,
	redo_adj_grabbed_note_velocities,
	NULL, NULL,
	mclip,
	undo_info,
	(Value){.int32_v = undo_info_index},
	(Value){0},
	(Value){.int32_v = undo_info_index},
	(Value){0},
	0, 0, false, true);	
}


/* 	user_event_push(EventFn undo_fn, EventFn redo_fn, EventFn dispose_fn, EventFn dispose_forward_fn, void *obj1, void *obj2, Value undo_val1, Value undo_val2, Value redo_val1, Value redo_val2, ValType type1, ValType type2, bool free_obj1, bool free_obj2) -> UserEvent * */
/* clang [expected_expression]: Expected expression */

