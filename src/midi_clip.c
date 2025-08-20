#include "clipref.h"
#include "midi_clip.h"
#include "midi_io.h"
#include "midi_objs.h"
#include "test.h"
#include "session.h"

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
	int32_t start_rel = 0;
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

void midi_clip_add_note(MIDIClip *mc, int channel, int note_val, int velocity, int32_t start_rel, int32_t end_rel)
{
    pthread_mutex_lock(&mc->notes_arr_lock);
    if (!mc->notes) {
	mc->notes_alloc_len = 32;
	mc->notes = calloc(mc->notes_alloc_len, sizeof(Note));
    }
    if (mc->num_notes == mc->notes_alloc_len) {
	mc->notes_alloc_len *= 2;
	mc->notes = realloc(mc->notes, mc->notes_alloc_len * sizeof(Note));
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
    TEST_FN_CALL(check_note_order, mc);
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
    return note->end_rel + cr->tl_pos - cr->start_in_clip;
}


/* int32_t cc_tl_pos(MIDICC *cc, ClipRef *cr) */
/* { */
/*     return cc->pos_rel + cr->tl_pos - cr->start_in_clip; */
/* } */

/* int32_t pb_tl_pos(MIDIPitchBend *pb, ClipRef *cr) */
/* { */
/*     return pb->pos_rel + cr->tl_pos - cr->start_in_clip; */
/* } */

/* static void note_off_rb_print(struct midi_event_ring_buf *rb) */
/* { */
/*     fprintf(stderr, "RING BUF has %d queued, read i %d....\n", rb->num_queued, rb->read_i); */
/*     for (int i=0; i<rb->num_queued; i++) { */
/* 	int index = (rb->read_i + i) % rb->size; */
/* 	/\* fprintf(stderr, "\t%d/%d (index %d): %d\n", i, rb->num_queued, index, rb->buf[index].timestamp); *\/ */
/*     } */
/* } */

/* static PmEvent *note_off_rb_check_pop_event(MIDIEventRingBuf *rb, int32_t pop_if_before_or_at) */
/* { */
/*     PmEvent *ret = NULL; */
/*     for (int i=0; i<rb->num_queued; i++) { */
/* 	int index = (rb->read_i + i) % rb->size; */
/* 	if (rb->buf[index].timestamp <= pop_if_before_or_at) { */
/* 	    ret = rb->buf + index; */
/* 	    rb->read_i++; */
/* 	    if (rb->read_i == rb->size) rb->read_i = 0; */
/* 	    rb->num_queued--; */
/* 	    break; */
/* 	} */
/*     } */
/*     return ret; */
/* } */

/* static void note_off_rb_insert(MIDIEventRingBuf *rb, PmEvent note_off) */
/* { */
/*     if (rb->num_queued == rb->size) { */
/* 	fprintf(stderr, "Note Off buf full\n"); */
/* 	return; */
/*     } */
/*     int index = rb->read_i; */
/*     int place_in_queue = 0; */
/*     for (int i=0; i<rb->num_queued; i++) { */
/* 	index = (rb->read_i + i) % rb->size; */
/* 	/\* Insert here *\/ */
/* 	if (note_off.timestamp <= rb->buf[index].timestamp) { */
/* 	    break; */
/* 	} */
/* 	place_in_queue++; */
/*     } */
/*     for (int i=rb->num_queued; i>place_in_queue; i--) { */
/* 	index = (rb->read_i + i) % rb->size; */
/* 	int prev_index = (rb->read_i + i - 1) % rb->size; */
/* 	rb->buf[index] = rb->buf[prev_index]; */
/*     } */
/*     int dst_index = (rb->read_i + place_in_queue) % rb->size; */
/*     rb->buf[dst_index] = note_off; */
/*     rb->num_queued++; */
/* } */

/* Output an array of PmEvents */
/* int midi_clip_output_chunk( */
/*     MIDIClip *mclip, */
/*     int32_t *start_pos_rel, */
/*     const int32_t chunk_len_sframes, */
/*     PmEvent *event_buf, */
/*     int event_buf_max_len, */
/*     struct midi_event_ring_buf *note_offs_rb) */
/* { */

/*     int32_t end_pos = *start_pos_rel + chunk_len_sframes; */
/*     if (end_pos > mclip->len_sframes) end_pos = mclip->len_sframes; */
/*     int event_i = 0; */
    
/*     int32_t note_i = mclip->num_notes > 0 ? 0 : -1; */
/*     /\* int32_t cc_i = mclip->num_ccs > 0 ? 0 : -1; *\/ */
/*     /\* int32_t pb_i = mclip->num_pbs > 0 ? 0 : -1; *\/ */

/*     Note *note; */
/*     /\* MIDICC *cc; *\/ */
/*     /\* MIDIPitchBend *pb; *\/ */

/*     bool notes_remaining = true; */
/*     bool ccs_remaining = true; */
/*     bool pbs_remaining = true; */

/*     /\* fprintf(stderr, "\n\nCHUNK %d-%d\n", chunk_tl_start, chunk_tl_end); *\/ */
/*     while (1) { */

/* 	/\* Get the next event: */
/* 	   - if obj, */
/* 	   - and obj ts before chunk, */
/* 	   - skip ahead til obj ts after chunk start */
/* 	   - if obj ts in chunk, */
/* 	   - cmp to running_ts to find min */
/* 	   - insert min */
/* 	   - repeat */
/* 	*\/ */

/* 	bool insert_note = false; */
/* 	bool insert_cc = false; */
/* 	bool insert_pb = false; */
/* 	bool insert_note_off = false; */

/* 	/\* int32_t note_pos; *\/ */
/* 	/\* int32_t cc_pos; *\/ */
/* 	/\* int32_t pb_pos; *\/ */

/* 	while (notes_remaining) { */
/* 	    if (note_i < 0 || note_i >= mclip->num_notes) { */
/* 		note = NULL; */
/* 		notes_remaining = false; */
/* 		break; */
/* 	    } */
/* 	    note = mclip->notes + note_i; */
/* 	    if (note->start_rel >= *start_pos_rel) { */
/* 		/\* Found next note in chunk *\/ */
/* 		break; */
/* 	    } else if (note->start_rel > end_pos) { */
/* 		note = NULL; */
/* 		notes_remaining = false; */
/* 		break; */
/* 	    } */
/* 	    note_i++; */
/* 	} */

/* 	while (ccs_remaining) { */
/* 	    if (cc_i < 0 || cc_i >= mclip->num_ccs) { */
/* 		cc = NULL; */
/* 		ccs_remaining = false; */
/* 		break; */
/* 	    } */
/* 	    cc = mclip->ccs + cc_i; */
/* 	    if (cc->pos_rel >= *start_pos_rel) { */
/* 		/\* Found next CC in chunk *\/ */
/* 		break; */
/* 	    } else if (cc->pos_rel > end_pos) { */
/* 		cc = NULL; */
/* 		ccs_remaining = false; */
/* 	    } */
/* 	    cc_i++; */
/* 	} */

/* 	while (pbs_remaining) { */
/* 	    if (pb_i < 0 || pb_i >= mclip->num_pbs) { */
/* 		pb = NULL; */
/* 		pbs_remaining = false; */
/* 		break; */
/* 	    } */
/* 	    pb = mclip->pbs + pb_i; */
/* 	    if (pb->pos_rel >= *start_pos_rel) { */
		
/* 		/\* Found first PB in chunk *\/ */
/* 		break; */
/* 	    } else if (pb->pos_rel > end_pos) { */
/* 		pb = NULL; */
/* 		pbs_remaining = false; */
/* 	    } */
/* 	    pb_i++; */
/* 	} */
	
	
/* 	/\* fprintf(stderr, "\n\nCHUNK %d - %d\n", chunk_tl_start, chunk_tl_end); *\/ */
/* 	/\* fprintf(stderr, "\tindices? %d %d %d\n", note_i, cc_i, pb_i); *\/ */
/* 	/\* fprintf(stderr, "\tobjs? %p %p %p\n", note, cc, pb); *\/ */
/* 	/\* fprintf(stderr, "\tRUNNING TS: %d, ts: %d\n", running_ts, ts); *\/ */
/* 	int32_t running_ts = INT32_MAX; */
/* 	/\* fprintf(stderr, "Note pos: %d, cc_pos: %d, pb_pos: %d\n", note_pos, cc_pos, pb_pos); *\/ */
/* 	if (note && note->start_rel < running_ts) { */
/* 	    insert_note = true; */
/* 	    running_ts = note->start_rel;		 */
/* 	} */
/* 	if (cc && cc->pos_rel < running_ts) { */
/* 	    insert_note = false; */
/* 	    insert_cc = true; */
/* 	    running_ts = cc->pos_rel; */
/* 	} */
/* 	if (pb && pb->pos_rel < running_ts) { */
/* 	    insert_note = false; */
/* 	    insert_cc = false; */
/* 	    insert_pb = true; */
/* 	    running_ts = pb->pos_rel; */
/* 	} */

/* 	/\* Event exceeds chunk (still at INT32_MAX or event ts past chunk): we're done *\/ */

/* 	PmEvent *note_off = NULL; */
/* 	/\* If 'running_ts' note set by other object, use chunk end *\/ */
/* 	int32_t note_off_bound = running_ts > end_pos ? end_pos - 1 : running_ts; */
/* 	/\* fprintf(stderr, "\t\tBound: %d; num queued? %d; read i? %d; cmp ts? %d\n", note_off_bound, rb->num_queued, rb->read_i, rb->buf[rb->read_i].timestamp); *\/ */
/* 	/\* fprintf(stderr, "note off bound: %d\n", note_off_bound); *\/ */
/* 	if ((note_off = note_off_rb_check_pop_event(note_offs_rb, note_off_bound))) { */
/* 	    /\* fprintf(stderr, "\tPopped! pos %d\n", note_off->timestamp); *\/ */
/* 	    running_ts = note_off->timestamp; */
/* 	    insert_note = false; */
/* 	    insert_cc = false; */
/* 	    insert_pb = false; */
/* 	    insert_note_off = true; */
/* 	    /\* exit(0); *\/ */
/* 	} */

/* 	if (!insert_note_off && running_ts >= end_pos) { */
/* 	    /\* fprintf(stderr, "EXIT due to running ts: %d\n", running_ts); *\/ */
/* 	    break; */
/* 	} */



/* 	/\* fprintf(stderr, "\tEvent? %d %d %d %d\n", insert_note, insert_cc, insert_pb, insert_note_off); *\/ */
/* 	/\* Do the insertion if available *\/ */
/* 	PmEvent e; */
/* 	if (insert_note) { */
/* 	    e = note_create_event_no_ts(note, 0, false); */
/* 	    note_i++; */
/* 	    /\* fprintf(stderr, "(%d) NOTE %d\n", running_ts, note->note); *\/ */
/* 	    /\* Queue the note off! *\/ */
/* 	    PmEvent e_off = note_create_event_no_ts(note, 0, true); */
	    
/* 	    /\* int32_t note_end = note_tl_end_pos(note, cr); *\/ */
/* 	    /\* int32_t clipref_end = cr->tl_pos + clipref_len(cr); *\/ */
/* 	    e_off.timestamp = note->end_rel; */
/* 	    note_off_rb_insert(note_offs_rb, e_off); */
/* 	} else if (insert_cc) { */
/* 	    e = midi_cc_create_event_no_ts(cc); */
/* 	    cc_i++; */
/* 	} else if (insert_pb) { */
/* 	    e = midi_pitch_bend_create_event_no_ts(pb); */
/* 	    pb_i++; */
/* 	} else if (insert_note_off) { */
/* 	    /\* fprintf(stderr, "(%d) NOTE OFF %d\n", running_ts, Pm_MessageData1(note_off->message)); *\/ */
/* 	    e = *note_off; */
/* 	} else { */
/* 	    /\* fprintf(stderr, "Break! no more\n"); *\/ */
/* 	    break; /\* No more events to insert *\/ */
/* 	} */
/* 	e.timestamp = running_ts; */
/* 	fprintf(stderr, "\tInsert event at %d\n", running_ts); */
/* 	event_buf[event_i] = e; */
/* 	event_i++; */
/* 	if (event_i == event_buf_max_len) { */
/* 	    /\* fprintf(stderr, "EVENT BUF FULL\n"); *\/ */
/* 	    break; */
/* 	}	 */
/*     } */
/*     return event_i; */
/* } */

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
		midi_clip_add_note(mclip, channel, note_val, unclosed->velocity, unclosed->start_rel, pos_rel);
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
    
    

/*     struct midi_event_ring_buf *rb = &cr->track->note_offs; */

/*     int32_t start_pos = chunk_tl_start - cr->tl_pos + cr->start_in_clip; */
/*     /\* fprintf(stderr, "GET CHUNK clip start %d\n", start_pos); *\/ */
/*     int num_events = midi_clip_output_chunk( */
/* 	mclip, */
/* 	&start_pos, */
/* 	chunk_tl_end - chunk_tl_start, */
/* 	event_buf, */
/* 	event_buf_max_len, */
/* 	rb); */
/*     /\* fprintf(stderr, "\tNUM EVENTS: %d\n", num_events); *\/ */
/*     for (int i=0; i<num_events; i++) { */
/* 	event_buf[i].timestamp += chunk_tl_start; */
/*     } */
/*     if (chunk_tl_end > cr->tl_pos + clipref_len(cr)) { */
/* 	PmEvent *e; */
/* 	while ((e = note_off_rb_check_pop_event(rb, INT32_MAX)) && num_events < event_buf_max_len) { */
/* 	    event_buf[num_events] = *e; */
/* 	    event_buf[num_events].timestamp += chunk_tl_start; */
/* 	    num_events++; */
/* 	} */
/*     } */
/*     return num_events; */
    
/*     /\* int32_t note_i = midi_clipref_check_get_first_note(cr); *\/ */
/*     /\* int32_t cc_i = midi_clipref_check_get_first_cc(cr); *\/ */
/*     /\* int32_t pb_i = midi_clipref_check_get_first_pb(cr); *\/ */

/*     /\* Note *note; *\/ */
/*     /\* MIDICC *cc; *\/ */
/*     /\* MIDIPitchBend *pb; *\/ */

/*     /\* bool notes_remaining = true; *\/ */
/*     /\* bool ccs_remaining = true; *\/ */
/*     /\* bool pbs_remaining = true; *\/ */

/*     /\* /\\* fprintf(stderr, "\n\nCHUNK %d-%d\n", chunk_tl_start, chunk_tl_end); *\\/ *\/ */
/*     /\* while (1) { *\/ */

/*     /\* 	/\\* Get the next event: *\/ */
/*     /\* 	   - if obj, *\/ */
/*     /\* 	   - and obj ts before chunk, *\/ */
/*     /\* 	   - skip ahead til obj ts after chunk start *\/ */
/*     /\* 	   - if obj ts in chunk, *\/ */
/*     /\* 	   - cmp to running_ts to find min *\/ */
/*     /\* 	   - insert min *\/ */
/*     /\* 	   - repeat *\/ */
/*     /\* 	*\\/ *\/ */

/*     /\* 	bool insert_note = false; *\/ */
/*     /\* 	bool insert_cc = false; *\/ */
/*     /\* 	bool insert_pb = false; *\/ */
/*     /\* 	bool insert_note_off = false; *\/ */

/*     /\* 	int32_t note_pos; *\/ */
/*     /\* 	int32_t cc_pos; *\/ */
/*     /\* 	int32_t pb_pos; *\/ */

/*     /\* 	while (notes_remaining) { *\/ */
/*     /\* 	    if (note_i < 0 || note_i >= mclip->num_notes) { *\/ */
/*     /\* 		note = NULL; *\/ */
/*     /\* 		notes_remaining = false; *\/ */
/*     /\* 		break; *\/ */
/*     /\* 	    } *\/ */
/*     /\* 	    note = mclip->notes + note_i; *\/ */
/*     /\* 	    if ((note_pos = note_tl_start_pos(note, cr)) >= chunk_tl_start) { *\/ */
/*     /\* 		/\\* Found next note in chunk *\\/ *\/ */
/*     /\* 		break; *\/ */
/*     /\* 	    } else if (note_pos > chunk_tl_end) { *\/ */
/*     /\* 		note = NULL; *\/ */
/*     /\* 		notes_remaining = false; *\/ */
/*     /\* 		break; *\/ */
/*     /\* 	    } *\/ */
/*     /\* 	    note_i++; *\/ */
/*     /\* 	} *\/ */

/*     /\* 	while (ccs_remaining) { *\/ */
/*     /\* 	    if (cc_i < 0 || cc_i >= mclip->num_ccs) { *\/ */
/*     /\* 		cc = NULL; *\/ */
/*     /\* 		ccs_remaining = false; *\/ */
/*     /\* 		break; *\/ */
/*     /\* 	    } *\/ */
/*     /\* 	    cc = mclip->ccs + cc_i; *\/ */
/*     /\* 	    if ((cc_pos = cc_tl_pos(cc, cr)) >= chunk_tl_start) { *\/ */
/*     /\* 		/\\* Found next CC in chunk *\\/ *\/ */
/*     /\* 		break; *\/ */
/*     /\* 	    } else if (cc_pos > chunk_tl_end) { *\/ */
/*     /\* 		cc = NULL; *\/ */
/*     /\* 		ccs_remaining = false; *\/ */
/*     /\* 	    } *\/ */
/*     /\* 	    cc_i++; *\/ */
/*     /\* 	} *\/ */

/*     /\* 	while (pbs_remaining) { *\/ */
/*     /\* 	    if (pb_i < 0 || pb_i >= mclip->num_pbs) { *\/ */
/*     /\* 		pb = NULL; *\/ */
/*     /\* 		pbs_remaining = false; *\/ */
/*     /\* 		break; *\/ */
/*     /\* 	    } *\/ */
/*     /\* 	    pb = mclip->pbs + pb_i; *\/ */
/*     /\* 	    if ((pb_pos = pb_tl_pos(pb, cr)) >= chunk_tl_start) { *\/ */
		
/*     /\* 		/\\* Found first PB in chunk *\\/ *\/ */
/*     /\* 		break; *\/ */
/*     /\* 	    } else if (pb_pos > chunk_tl_end) { *\/ */
/*     /\* 		pb = NULL; *\/ */
/*     /\* 		pbs_remaining = false; *\/ */
/*     /\* 	    } *\/ */
/*     /\* 	    pb_i++; *\/ */
/*     /\* 	} *\/ */
	
	
/*     /\* 	/\\* fprintf(stderr, "\n\nCHUNK %d - %d\n", chunk_tl_start, chunk_tl_end); *\\/ *\/ */
/*     /\* 	/\\* fprintf(stderr, "\tindices? %d %d %d\n", note_i, cc_i, pb_i); *\\/ *\/ */
/*     /\* 	/\\* fprintf(stderr, "\tobjs? %p %p %p\n", note, cc, pb); *\\/ *\/ */
/*     /\* 	/\\* fprintf(stderr, "\tRUNNING TS: %d, ts: %d\n", running_ts, ts); *\\/ *\/ */
/*     /\* 	int32_t running_ts = INT32_MAX; *\/ */
/*     /\* 	/\\* fprintf(stderr, "Note pos: %d, cc_pos: %d, pb_pos: %d\n", note_pos, cc_pos, pb_pos); *\\/ *\/ */
/*     /\* 	if (note && note_pos < running_ts) { *\/ */
/*     /\* 	    insert_note = true; *\/ */
/*     /\* 	    running_ts = note_pos;		 *\/ */
/*     /\* 	} *\/ */
/*     /\* 	if (cc && cc_pos < running_ts) { *\/ */
/*     /\* 	    insert_note = false; *\/ */
/*     /\* 	    insert_cc = true; *\/ */
/*     /\* 	    running_ts = cc_pos; *\/ */
/*     /\* 	} *\/ */
/*     /\* 	if (pb && pb_pos < running_ts) { *\/ */
/*     /\* 	    insert_note = false; *\/ */
/*     /\* 	    insert_cc = false; *\/ */
/*     /\* 	    insert_pb = true; *\/ */
/*     /\* 	    running_ts = pb_pos; *\/ */
/*     /\* 	} *\/ */

/*     /\* 	/\\* Event exceeds chunk (still at INT32_MAX or event ts past chunk): we're done *\\/ *\/ */

/*     /\* 	PmEvent *note_off = NULL; *\/ */
/*     /\* 	/\\* If 'running_ts' note set by other object, use chunk end *\\/ *\/ */
/*     /\* 	int32_t note_off_bound = running_ts > chunk_tl_end ? chunk_tl_end - 1 : running_ts; *\/ */
/*     /\* 	/\\* fprintf(stderr, "\t\tBound: %d; num queued? %d; read i? %d; cmp ts? %d\n", note_off_bound, rb->num_queued, rb->read_i, rb->buf[rb->read_i].timestamp); *\\/ *\/ */
/*     /\* 	/\\* fprintf(stderr, "note off bound: %d\n", note_off_bound); *\\/ *\/ */
/*     /\* 	if ((note_off = note_off_rb_check_pop_event(rb, note_off_bound))) { *\/ */
/*     /\* 	    /\\* fprintf(stderr, "\tPopped! pos %d\n", note_off->timestamp); *\\/ *\/ */
/*     /\* 	    running_ts = note_off->timestamp; *\/ */
/*     /\* 	    insert_note = false; *\/ */
/*     /\* 	    insert_cc = false; *\/ */
/*     /\* 	    insert_pb = false; *\/ */
/*     /\* 	    insert_note_off = true; *\/ */
/*     /\* 	    /\\* exit(0); *\\/ *\/ */
/*     /\* 	} *\/ */

/*     /\* 	if (!insert_note_off && running_ts >= chunk_tl_end) { *\/ */
/*     /\* 	    /\\* fprintf(stderr, "EXIT due to running ts: %d\n", running_ts); *\\/ *\/ */
/*     /\* 	    break; *\/ */
/*     /\* 	} *\/ */



/*     /\* 	/\\* fprintf(stderr, "\tEvent? %d %d %d %d\n", insert_note, insert_cc, insert_pb, insert_note_off); *\\/ *\/ */
/*     /\* 	/\\* Do the insertion if available *\\/ *\/ */
/*     /\* 	PmEvent e; *\/ */
/*     /\* 	if (insert_note) { *\/ */
/*     /\* 	    e = note_create_event_no_ts(note, 0, false); *\/ */
/*     /\* 	    note_i++; *\/ */
/*     /\* 	    /\\* fprintf(stderr, "(%d) NOTE %d\n", running_ts, note->note); *\\/ *\/ */
/*     /\* 	    /\\* Queue the note off! *\\/ *\/ */
/*     /\* 	    PmEvent e_off = note_create_event_no_ts(note, 0, true); *\/ */
/*     /\* 	    int32_t note_end = note_tl_end_pos(note, cr); *\/ */
/*     /\* 	    int32_t clipref_end = cr->tl_pos + clipref_len(cr); *\/ */
/*     /\* 	    e_off.timestamp = note_end > clipref_end - 1 ? clipref_end - 1 : note_end; *\/ */
/*     /\* 	    note_off_rb_insert(rb, e_off); *\/ */
/*     /\* 	} else if (insert_cc) { *\/ */
/*     /\* 	    e = midi_cc_create_event_no_ts(cc); *\/ */
/*     /\* 	    cc_i++; *\/ */
/*     /\* 	} else if (insert_pb) { *\/ */
/*     /\* 	    e = midi_pitch_bend_create_event_no_ts(pb); *\/ */
/*     /\* 	    pb_i++; *\/ */
/*     /\* 	} else if (insert_note_off) { *\/ */
/*     /\* 	    /\\* fprintf(stderr, "(%d) NOTE OFF %d\n", running_ts, Pm_MessageData1(note_off->message)); *\\/ *\/ */
/*     /\* 	    e = *note_off; *\/ */
/*     /\* 	} else { *\/ */
/*     /\* 	    /\\* fprintf(stderr, "Break! no more\n"); *\\/ *\/ */
/*     /\* 	    break; /\\* No more events to insert *\\/ *\/ */
/*     /\* 	} *\/ */
/*     /\* 	e.timestamp = running_ts; *\/ */
/*     /\* 	/\\* fprintf(stderr, "\tInsert event at %d\n", running_ts); *\\/ *\/ */
/*     /\* 	event_buf[event_i] = e; *\/ */
/*     /\* 	event_i++; *\/ */
/*     /\* 	if (event_i == event_buf_max_len) { *\/ */
/*     /\* 	    /\\* fprintf(stderr, "EVENT BUF FULL\n"); *\\/ *\/ */
/*     /\* 	    break; *\/ */
/*     /\* 	}	 *\/ */
/*     /\* } *\/ */
/*     /\* return event_i; *\/ */
/* } */

/* /\* Return the number of events written *\/ */
/* int midi_clipref_output_chunk_old(ClipRef *cr, PmEvent *event_buf, int event_buf_max_len, int32_t chunk_tl_start, int32_t chunk_tl_end) */
/* { */
/*     struct midi_event_ring_buf *rb = &cr->track->note_offs; */

/*     bool event_buf_full = false; */
    
/*     if (cr->type == CLIP_AUDIO) return 0; */
/*     /\* int32_t chunk_len = chunk_tl_end - chunk_tl_start; *\/ */
/*     /\* fprintf(stderr, "\nOutput chunk %d-%d\n", chunk_tl_start, chunk_tl_end); *\/ */
/*     MIDIClip *mclip = cr->source_clip; */
/*     int32_t note_i = midi_clipref_check_get_first_note(cr); */
/*     int32_t cc_i = midi_clipref_check_get_first_cc(cr); */
/*     int32_t pb_i = midi_clipref_check_get_first_pb(cr); */
/*     MIDICC *cc = NULL; */
/*     MIDIPitchBend *pb = NULL; */
/*     if (cc_i >= 0) cc = mclip->ccs + cc_i; */
/*     if (pb_i >= 0) pb = mclip->pbs + pb_i; */
    
/*     PmEvent e; */
/*     PmEvent e_off; */
/*     int event_buf_index = 0; */
/*     /\* const static int note_off_buflen = 128; *\/ */
/*     /\* static PmEvent note_offs_array_base[note_off_buflen]; *\/ */
/*     /\* static PmEvent *note_offs = note_offs_array_base; *\/ */
/*     /\* static int num_note_offs = 0; *\/ */
/*     /\* Track *track = cr->track; *\/ */
/*     int32_t clipref_end = cr->tl_pos + clipref_len(cr); */
/*     while (note_i < mclip->num_notes) { */
/* 	Note *note = mclip->notes + note_i; */
/* 	int32_t note_tl_start; */
/* 	int32_t note_tl_end; */
	

/* 	/\* Skip notes that are not initiated in this chunk *\/ */
/* 	if (note_tl_start_pos(note, cr) < chunk_tl_start) { */
/* 	    if (note_i < mclip->num_notes) { */
/* 		note_i++; */
/* 		note++; */
/* 	    } else { */
/* 		break; /\* No more notes *\/ */
/* 	    } */
/* 	    continue; */
	    
/* 	/\* If note starts after chunk end, skip *\/ */
/* 	} */
/* 	if (note_tl_start_pos(note, cr) >= chunk_tl_end) { */
/* 	    break; */
/* 	} */
/* 	note_tl_start = note_tl_start_pos(note, cr); */
/* 	note_tl_end = note_tl_end_pos(note, cr); */
/* 	if (note_tl_end >= clipref_end) { */
/* 	    note_tl_end = clipref_end - 1; */
/* 	} */

/* 	/\* TODO: set channel *\/ */
/* 	e.message = Pm_Message(0x90, note->note, note->velocity); */
/* 	e.timestamp = note_tl_start; */
	
/* 	/\* Insert any earlier note offs first *\/ */
/* 	PmEvent *note_off = NULL; */
/* 	while ((note_off = note_off_rb_check_pop_event(rb, e.timestamp))) { */
/* 	    if (event_buf_index + 1 < event_buf_max_len) { */
/* 		event_buf[event_buf_index] = *note_off; */
/* 		event_buf_index++; */
/* 	    } else { */
/* 		event_buf_full = true; */
/* 		break; */
/* 	    } */
/* 	} */

/* 	if (event_buf_index + 1 < event_buf_max_len) { */
/* 	    event_buf[event_buf_index] = e; */
/* 	    event_buf_index++; */
/* 	} else { */
/* 	    event_buf_full = true; */
/* 	} */

/* 	/\* And queue the note off *\/ */
/* 	/\* TODO: channels *\/ */
/* 	e_off.message = Pm_Message(0x80, note->note, note->velocity); */
/* 	e_off.timestamp = note_tl_end; */
/* 	if (note_tl_end <= chunk_tl_end) { */
/* 	    if (event_buf_index < event_buf_max_len) { */
/* 		event_buf[event_buf_index] = e; */
/* 		event_buf_index++; */
/* 	    } else { */
/* 		event_buf_full = true; */
/* 	    } */
/* 	} */
/* 	note_off_rb_insert(rb, e_off); */
/* 	note_i++; */
/*     } */

/*     /\* Insert note offs before chunk end *\/ */
/*     PmEvent *note_off; */
/*     while ((note_off = note_off_rb_check_pop_event(rb, chunk_tl_end))) { */
/* 	if (event_buf_index + 1 < event_buf_max_len) { */
/* 	    event_buf[event_buf_index] = *note_off; */
/* 	    event_buf_index++; */
/* 	} else { */
/* 	    event_buf_full = true; */
/* 	} */
/*     } */

/*     /\* while (1) { *\/ */
/*     /\* /\\* for (int i=track->note_offs_read_i; i<track->note_offs_write_i; i++) { *\\/ *\/ */
/*     /\* 	/\\* fprintf(stderr, "\t\t->note off check %d %d\n", track->note_offs_read_i, track->note_offs_write_i); *\\/ *\/ */
/*     /\* 	if (track->note_offs_read_i == 128) { *\/ */
/*     /\* 	    track->note_offs_read_i = 0; *\/ */
/*     /\* 	} *\/ */
/*     /\* 	if (track->note_offs_read_i == track->note_offs_write_i) { *\/ */
/*     /\* 	    break; *\/ */
/*     /\* 	} *\/ */
/*     /\* 	PmEvent note_off = track->note_offs[track->note_offs_read_i]; *\/ */
/*     /\* 	fprintf(stderr, "\tTEST Note off index %d, %d, ts %d, chunk end: %d\n", track->note_offs_read_i, Pm_MessageData1(note_off.message), note_off.timestamp, chunk_tl_end); *\/ */
/*     /\* 	if (note_off.timestamp < chunk_tl_end) { *\/ */
/*     /\* 	    /\\* Insert note off *\\/ *\/ */
/*     /\* 	    if (event_buf_index < event_buf_max_len) { *\/ */
/*     /\* 		fprintf(stderr, "\tInsert REGULAR note off\n"); *\/ */
/*     /\* 		event_buf[event_buf_index] = note_off; *\/ */
/*     /\* 		event_buf_index++; *\/ */
/*     /\* 		add++; *\/ */
/*     /\* 		/\\* track->note_offs_read_i++; *\\/ *\/ */
/*     /\* 	    } else { *\/ */
/*     /\* 		break; *\/ */
/*     /\* 	    } *\/ */
/*     /\* 	} *\/ */
/*     /\* 	track->note_offs_read_i++; *\/ */

/*     /\* } *\/ */
/*     /\* track->note_offs_read_i = saved_read_i + add; *\/ */
/*     if (event_buf_full) { */
/* 	fprintf(stderr, "ERROR: event buf is full\n"); */
/*     } */
/*     #ifdef TESTBUILD */
/*     if (event_buf_index == 256) exit(1); */
/*     #endif */
/*     return event_buf_index; */
/* } */

