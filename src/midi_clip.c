#include "clipref.h"
#include "midi_clip.h"
#include "midi_io.h"
#include "midi_objs.h"
#include "test.h"
#include "session.h"
#include "user_event.h"

#define DEFAULT_NOTES_ALLOC_LEN 16
#define DEFAULT_REFS_ALLOC_LEN 2

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


static void grabbed_notes_rebase(MIDIClip *mclip, Note *new_head);
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
	grabbed_notes_rebase(mc, mc->notes);	      
    }
    pthread_mutex_unlock(&mc->notes_arr_lock);
    
    Note *note = mc->notes + mc->num_notes;
    while (note > mc->notes && start_rel < (note - 1)->start_rel) {
	*note = *(note - 1);
	note--;
    }
    note->channel = channel;
    note->key = note_val;
    note->velocity = velocity;
    note->start_rel = start_rel;
    note->end_rel = end_rel;
    mc->num_notes++;
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
    int32_t start_pos,
    int32_t end_pos,
    int32_t note_trunc_pos,
    MIDIEventRingBuf *rb)
{
    /* fprintf(stderr, "%d - %d\n", start_pos, end_pos); */
    uint32_t alloc_len = 128;
    uint32_t num_events = 0;
    *dst = malloc(alloc_len * sizeof(PmEvent));
    for (int i=0; i<mclip->num_notes; i++) {
	Note *note = mclip->notes + i;
	/* fprintf(stderr, "\tTest start? %d\n", note->start_rel); */
	if (note->start_rel < start_pos) continue;
	if (note->start_rel >= end_pos) break;
	PmEvent note_on, note_off;
	note_on.timestamp = note->start_rel;
	note_on.message = Pm_Message(
	    0x90 + note->channel,
	    note->key,
	    note->velocity);
	/* Insert Note ON */
	event_buf_insert(dst, &alloc_len, &num_events, note_on);
	if (note->end_rel > note_trunc_pos) {
	    note_off.timestamp = note_trunc_pos;
	} else {
	    note_off.timestamp = note->end_rel;
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
	while ((note_off = midi_event_ring_buf_pop(rb, end_pos - 1 /* sample at end pos belongs to NEXT chunk */))) {
	    event_buf_insert(dst, &alloc_len, &num_events, *note_off);
	}
    }
    for (int i=0; i<MIDI_NUM_CONTROLLERS; i++) {
	Controller *c = mclip->controllers + i;
	if (!c->in_use) continue;
	for (uint16_t i=0; i<c->num_changes; i++) {
	    PmEvent e = midi_controller_make_event(c, i);
	    if (e.timestamp < start_pos) continue;
	    if (e.timestamp >= end_pos) break;
	    event_buf_insert(dst, &alloc_len, &num_events, e);	    
	}
    }
    for (uint16_t i=0; i<mclip->pitch_bend.num_changes; i++) {
	PmEvent e = pitch_bend_make_event(&mclip->pitch_bend, i);
	if (e.timestamp < start_pos) continue;
	if (e.timestamp >= end_pos) break;
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
	&cr->track->note_offs);
    if (num_events > event_buf_max_len) {
	fprintf(stderr, "Error! Event buf full!\n");
	num_events = event_buf_max_len;
    }
    /* fprintf(stderr, "(%d-%d): %d events\n", chunk_tl_start, chunk_tl_end, num_events); */
    memcpy(event_buf, dyn_events, num_events * sizeof(PmEvent));
    free(dyn_events);
    for (int i=0; i<num_events; i++) {
	event_buf[i].timestamp += cr->tl_pos - cr->start_in_clip;
        #ifdef TESTBUILD
	if (event_buf[i].timestamp < chunk_tl_start || event_buf[i].timestamp >= chunk_tl_end) {
	    breakfn();
	    /* fprintf(stderr, "CRITICAL ERROR: outputting events not in chunk!! Event index %d, status %x, ts %d (chunk %d - %d)\n", i, Pm_MessageStatus(event_buf[i].message), event_buf[i].timestamp, chunk_tl_start, chunk_tl_end); */
	    /* exit(1); */
	}
	#endif
    }

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
	if (start_pos <= tl_pos && note_tl_end_pos(note, cr) > tl_pos) {
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

int midi_clipref_notes_intersecting_area(ClipRef *cr, int32_t range_start, int32_t range_end, int bottom_note, int top_note, Note ***dst)
{
    const int max_intersecting_notes = 4096;
    Note **intersecting_notes = malloc(max_intersecting_notes * sizeof(Note *));
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
	    if (num_intersecting_notes == max_intersecting_notes) break;
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
    for (int i=0; i<mclip->num_grabbed_notes; i++) {
	Note **grabbed = mclip->grabbed_notes + i;
	int32_t grabbed_i = *grabbed - mclip->notes;
	if (grabbed_i >= from_i && grabbed_i < until_i) {
	    *grabbed += by;
	}
    }
}

static void grabbed_notes_rebase(MIDIClip *mclip, Note *new_head)
{
    for (int i=0; i<mclip->num_grabbed_notes; i++) {
	Note **grabbed = mclip->grabbed_notes + i;
	int32_t grabbed_i = *grabbed - mclip->notes;
	*grabbed  = new_head + grabbed_i;
    }
}

static void grabbed_notes_reset(MIDIClip *mclip)
{
    mclip->num_grabbed_notes = 0;
    for (int32_t i=0; i<mclip->num_notes; i++) {
	Note *note = mclip->notes + i;
	if (note->grabbed) {
	    mclip->grabbed_notes[mclip->num_grabbed_notes] = note;
	    mclip->num_grabbed_notes++;
	}
    }
}

void midi_clip_grab_note(MIDIClip *mclip, Note *note, NoteEdge edge)
{
    if (note->grabbed) {
	note->grabbed_edge = edge;
	return;
    }
    if (mclip->num_grabbed_notes == MAX_GRABBED_NOTES) {
	char msg[128];
	snprintf(msg, 128, "Cannot grab more than %d notes", MAX_GRABBED_NOTES);
	status_set_errstr(msg);
	return;
    }
    note->grabbed = true;
    note->grabbed_edge = edge;
    mclip->grabbed_notes[mclip->num_grabbed_notes] = note;
    mclip->num_grabbed_notes++;
}

static int notecmp(const void *a, const void *b)
{
    return ((Note *)a)->start_rel - ((Note *)b)->start_rel;
}
void midi_clip_ungrab_all(MIDIClip *mclip)
{
    for (int i=0; i<mclip->num_grabbed_notes; i++) {
	Note *note = mclip->grabbed_notes[i];
	note->grabbed = false;
	note->grabbed_edge = NOTE_EDGE_NONE;
    }
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
    fprintf(stderr, "(%d) TL: %d-%d; notes %d-%d\n", num_intersecting, tl_start, tl_end, bottom_note, top_note);
    for (int i=0; i<num_intersecting; i++) {
	midi_clip_grab_note(cr->source_clip, intersecting[i], NOTE_EDGE_NONE);
    }
    if (intersecting) free(intersecting);

}

void midi_clip_grabbed_notes_move(MIDIClip *mclip, int32_t move_by)
{
    pthread_mutex_lock(&mclip->notes_arr_lock);
    for (int i=0; i<mclip->num_grabbed_notes; i++) {
	Note *note = mclip->grabbed_notes[i];
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

NEW_EVENT_FN(undo_delete_grabbed_note, "undo delete grabbed notes")
    
}


NEW_EVENT_FN(redo_delete_grabbed_note, "undo delete grabbed notes")
    
}


void midi_clip_grabbed_notes_delete(MIDIClip *mclip)
{
    Note *grabbed_note_info = calloc(mclip->num_grabbed_notes, sizeof(Note));
    /* for (int i=0; i<mclip->num_grabbed_notes; i++) { */
    int grabbed_note_i = 0;
    fprintf(stderr, "\n\nDELETE num grabbed? %d\n", mclip->num_grabbed_notes);
    while (mclip->num_grabbed_notes > 0) {
	Note *note = mclip->grabbed_notes[0];
	int32_t note_i = note - mclip->notes;
	grabbed_note_info[grabbed_note_i] = *note;
	grabbed_note_i++;
	fprintf(stderr, "REMOVING note at index %d. MOVING %d notes from %p to %p (%d - %d)\n", note_i, mclip->num_notes - note_i - 1, mclip->notes + note_i + 1, mclip->notes + note_i, note_i + 1 , note_i);
	memmove(mclip->notes + note_i, mclip->notes + note_i + 1, (mclip->num_notes - note_i - 1) * sizeof(Note));
	mclip->num_notes--;
	fprintf(stderr, "NOTES NOW:\n");
	for (int32_t i=0; i<mclip->num_notes; i++) {
	    fprintf(stderr, "\t%d == %d, grabbed? %d\n", i, mclip->notes[i].key, mclip->notes[i].grabbed);
	}

	/* grabbed_notes_incr(mclip, note_i, mclip->num_notes, -1); */
	grabbed_notes_reset(mclip);
	fprintf(stderr, "\t->numgrabbed after reset: %d\n", mclip->num_grabbed_notes);
    }
    /* mclip->num_grabbed_notes = 0; */
    free(grabbed_note_info);
}
