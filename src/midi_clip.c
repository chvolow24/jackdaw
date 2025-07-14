#include "clipref.h"
#include "midi_clip.h"
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
	snprintf(mclip->name, sizeof(mclip->name), "%s_rec_%d", device->info->name, session->proj.num_clips); /* TODO: Fix this */
    } else if (target) {
	snprintf(mclip->name, sizeof(mclip->name), "%s take %d", target->name, target->num_takes + 1);
	target->num_takes++;
    } else {
	snprintf(mclip->name, sizeof(mclip->name), "anonymous");
    }

    session->proj.midi_clips[session->proj.num_midi_clips] = mclip;
    session->proj.num_midi_clips++;
    fprintf(stderr, "IN CREATE num midi clips = %d (ADDR VALUE: %p)\n", session->proj.num_midi_clips, &session->proj.num_midi_clips);
    return mclip;

}

void midi_clip_destroy(MIDIClip *mc)
{
    free(mc->notes);
    for (int i=0; i<mc->num_refs; i++) {
	clipref_destroy(mc->refs[i], false);
    }
    free(mc->refs);
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

void midi_clip_add_note(MIDIClip *mc, int note_val, int velocity, int32_t start_rel, int32_t end_rel)
{
    /* #ifdef TESTBUILD */
    /* if (start_rel < 0) { */
    /* 	fprintf(stderr, "Error: call to add note with start rel %d\n", start_rel)} */
    /* 	exit(1); */
    /* } */
    /* #endif */
    /* fprintf(stderr, "ADDING note of rel %d (end %d) to clip of num notes %d\n", start_rel, end_rel, mc->num_notes); */
    if (!mc->notes) {
	mc->notes_alloc_len = 32;
	mc->notes = calloc(mc->notes_alloc_len, sizeof(Note));
    }
    if (mc->num_notes == mc->notes_alloc_len) {
	mc->notes_alloc_len *= 2;
	mc->notes = realloc(mc->notes, mc->notes_alloc_len * sizeof(Note));
    }
    
    Note *note = mc->notes + mc->num_notes;
    while (note > mc->notes && start_rel < (note - 1)->start_rel) {
	*note = *(note - 1);
	note--;
    }
    note->note = note_val;
    note->velocity = velocity;
    note->start_rel = start_rel;
    note->end_rel = end_rel;
    mc->num_notes++;
    TEST_FN_CALL(check_note_order, mc);
}

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

int32_t note_tl_start_pos(Note *note, ClipRef *cr)
{
    return note->start_rel + cr->tl_pos - cr->start_in_clip;
}

int32_t note_tl_end_pos(Note *note, ClipRef *cr)
{
    return note->end_rel + cr->tl_pos - cr->start_in_clip;
}


/* Return the number of events written */
int midi_clipref_output_chunk(ClipRef *cr, PmEvent *event_buf, int event_buf_max_len, int32_t chunk_tl_start, int32_t chunk_tl_end)
{
    if (cr->type == CLIP_AUDIO) return 0;
    /* int32_t chunk_len = chunk_tl_end - chunk_tl_start; */
    /* fprintf(stderr, "\nOutput chunk %d-%d\n", chunk_tl_start, chunk_tl_end); */
    MIDIClip *mclip = cr->source_clip;
    int32_t note_i = midi_clipref_check_get_first_note(cr);
    PmEvent e;
    PmEvent e_off;
    int event_buf_index = 0;
    /* const static int note_off_buflen = 128; */
    /* static PmEvent note_offs_array_base[note_off_buflen]; */
    /* static PmEvent *note_offs = note_offs_array_base; */
    /* static int num_note_offs = 0; */
    Track *track = cr->track;
    int32_t clipref_end = cr->tl_pos + clipref_len(cr);
    while (note_i < mclip->num_notes) {
	Note *note = mclip->notes + note_i;
	int32_t note_tl_start;
	int32_t note_tl_end;
	

	/* Skip notes that are not initiated in this chunk */
	if (note_tl_start_pos(note, cr) < chunk_tl_start) {
	    if (note_i < mclip->num_notes) {
		note_i++;
		note++;
	    } else {
		break; /* No more notes */
	    }
	    continue;
	/* If note starts after chunk end, skip */
	}
	if (note_tl_start_pos(note, cr) >= chunk_tl_end) {
	    break;
	}
	/* fprintf(stderr, "\t\tNote i: %d, start-end: %d-%d\n", note_i, note_start_tl_pos, note_end_tl_pos); */
	/* if (note_end_tl_pos - note_start_tl_pos == 0) { */
	/*     fprintf(stderr, "Warn!!!! note val %d has zero dur\n", note->note); */
	/* } */
	note_tl_start = note_tl_start_pos(note, cr);
	note_tl_end = note_tl_end_pos(note, cr);
	if (note_tl_end >= clipref_end) {
	    note_tl_end = clipref_end - 1;
	}

	/* TODO: set channel */
	e.message = Pm_Message(0x90, note->note, note->velocity);
	e.timestamp = note_tl_start;
	
	/* Insert any earlier note offs first */
	bool event_buf_full = false;
	while (1) {
	    /* fprintf(stderr, "\t\t->note off check above buf indices: %d %d\n", track->note_offs_read_i, track->note_offs_write_i); */
	    if (track->note_offs_read_i == 128) {
		track->note_offs_read_i = 0;
	    }
	    if (track->note_offs_read_i == track->note_offs_write_i) {
		break;
	    }

		
	    PmEvent note_off = track->note_offs[track->note_offs_read_i];
	    if (note_off.timestamp <= e.timestamp) {
		/* Insert note off */
		if (event_buf_index + 1 < event_buf_max_len) {
		    event_buf[event_buf_index] = note_off;
		    event_buf_index++;
		} else {
		    event_buf_full = true;
		    break;
		}
		track->note_offs_read_i++;
	    } else {
		break;
	    }
	}
	/* Now insert the note on */
	if (!event_buf_full) {
	    event_buf[event_buf_index] = e;
	    event_buf_index++;
	} else {
	    break;
	}

	e_off.message = Pm_Message(0x80, note->note, note->velocity);
	e_off.timestamp = note_tl_end;
	if (track->note_offs_write_i + 1 == track->note_offs_read_i) {
	    fprintf(stderr, "Error! reached end of buf, cannot write\n");
	} else {
	    track->note_offs[track->note_offs_write_i] = e_off;
	    track->note_offs_write_i++;
	    if (track->note_offs_write_i == 128)
		track->note_offs_write_i = 0;
	}
	
	if (event_buf_index >= event_buf_max_len) {
	    fprintf(stderr, "Warning: Losing notes\n");
	    break;
	}
	note_i++;
    }

    while (1) {
	/* fprintf(stderr, "\t\t->note off check %d %d\n", track->note_offs_read_i, track->note_offs_write_i); */
	if (track->note_offs_read_i == 128) {
	    track->note_offs_read_i = 0;
	}
	if (track->note_offs_read_i == track->note_offs_write_i) {
	    break;
	}
	
	PmEvent note_off = track->note_offs[track->note_offs_read_i];
	if (note_off.timestamp < chunk_tl_end) {
	    /* Insert note off */
	    if (event_buf_index < event_buf_max_len) {
		event_buf[event_buf_index] = note_off;
		event_buf_index++;
		track->note_offs_read_i++;
	    } else {
		break;
	    }
	} else {
	    break;
	}

    }
    #ifdef TESTBUILD
    if (event_buf_index == 256) exit(1);
    #endif
    return event_buf_index;
}

