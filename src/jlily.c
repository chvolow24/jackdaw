#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "clipref.h"
#include "midi_clip.h"
#include "project.h"
#include "session.h"

#define MAX_CHORD_NOTES 128
#define MAX_NOTES 1024
#define MAX_JLILY_BUFLEN 1024
#define MAX_REPEAT_BLOCK_DEPTH 4

enum token_type {
    JLILY_NONE,
    JLILY_ESCAPE,
    JLILY_WHITESPACE,
    JLILY_NOTE,
    JLILY_REST,
    JLILY_DURATION,
    JLILY_SHARP,
    JLILY_FLAT,
    JLILY_OCTAVE,
    JLILY_CHORD_OPEN,
    JLILY_CHORD_CLOSE,
    JLILY_TIE,
    JLILY_BARLINE,
    JLILY_BLOCK_OPEN,
    JLILY_BLOCK_CLOSE,
};

enum jlily_escape_type {
    JLILY_REPEAT,
    JLILY_VELOCITY
};


typedef struct jlily_repeat_block {
    int repeat_n_times;
    double ts_start;
    double ts_end;
    int first_note_index;
    int last_note_index;    
} JLilyRepeatBlock;

typedef struct jlily_note {
    bool closed;
    bool tied;
    char name;
    int accidental;
    uint8_t pitch;
    uint8_t velocity;
    double ts;
    int32_t dur;
} JLilyNote;


struct jlily_state {
    double beat_dur_sframes;
    double ts;
    double current_dur_sframes;
    uint8_t velocity;

    JLilyNote notes[MAX_NOTES];
    JLilyNote *current_note;
    int num_notes;
    bool chord_open;
    enum token_type last_token_type;
    int repeat_block_index;
    JLilyRepeatBlock repeat_blocks[MAX_REPEAT_BLOCK_DEPTH];
};

struct jlily_state state;

static int note_name_interval(char note1, char note2)
{
    int interval = 0;
    while (note1 != note2) {
	note1++;
	switch(note1) {
	case 'f':
	case 'c':
	    interval++;
	    break;
	case 'h':
	    interval+=2;
	    note1 = 'a';
	    break;
	default:
	    interval+=2;
	}
    }
    return interval;
}

void handle_repeat(JLilyRepeatBlock *block)
{
    int block_num_notes =  1 + block->last_note_index - block->first_note_index;
    double ts_add = block->ts_end - block->ts_start;
    fprintf(stderr, "\t\tHANDLE REPEAT: %d times, irange %d-%d, num_notes : %d\n", block->repeat_n_times, block->first_note_index, block->last_note_index, block_num_notes);
    for (int i=1; i<block->repeat_n_times; i++) {
	fprintf(stderr, "\t\t\tInsert %d notes; space remeaining? %d\n", block_num_notes, MAX_NOTES - state.num_notes);
	if (block_num_notes + state.num_notes >= MAX_NOTES) {
	    fprintf(stderr, "ERROR! Too many notes!\n");
	    return;
	}
	memcpy(state.notes + state.num_notes, state.notes + block->first_note_index, block_num_notes * sizeof(JLilyNote));
	for (int j=0; j<block_num_notes; j++) {
	    JLilyNote *note = state.notes + state.num_notes;
	    note->ts += (ts_add * i);
	    state.num_notes++;
	    state.current_note++;
	}
	state.ts += ts_add;
    }
}


static int handle_next_token(const char *text, int index, enum token_type type, int max_i)
{
    /* Token is at most 2 chars long */
    char tok_buf[3];
    memset(tok_buf, '\0', sizeof(tok_buf));
    int tok_len = 1;
    memcpy(tok_buf, text + index, 2);
    /* fprintf(stderr, "TOK: %s, type %d\n", tok_buf, type); */
    
    
    switch (type) {
    case JLILY_NONE:
	return -1;
    case JLILY_ESCAPE: {
	if (state.chord_open) break;
	char esc_word_buf[24];
	int i=0;
	char c;
	while (i < 24 - 1 && i + index < max_i - 1 && (c = text[index + i + 1]) >= 'a' && c <= 'z') {
	    tok_len++;
	    esc_word_buf[i] = c;
	    i++;
	}
	esc_word_buf[i] = '\0';
	int value = 0;
	int subindex = index + i + 1;
	while (subindex < max_i) {
	    char c = text[subindex];
	    if (c == ' ') {
		tok_len++;
		subindex++;
		continue;
	    } else if (c >= '0' && c <= '9') {
		tok_len++;
		value  = (value * 10) + c - '0';				
	    } else {
		break;
	    }
	    subindex++;
	}
	if (strncmp(esc_word_buf, "repeat", 6) == 0) {
	    state.repeat_block_index++;
	    state.repeat_blocks[state.repeat_block_index].repeat_n_times = value;
	    fprintf(stderr, "REPEAT!!!\n");
	} else if (strncmp(esc_word_buf, "velocity", 8) == 0) {
	    state.velocity = value > 127 ? 127 : value;
	    fprintf(stderr, "VELOCITY!!! value %d\n", value);
	}
    }
	break;
    case JLILY_WHITESPACE:
	if (!state.current_note->closed) {
	    fprintf(stderr, "CLOSING note %ld\n", state.current_note - state.notes);
	    if (state.current_note > state.notes && (state.current_note - 1)->tied) {
		(state.current_note - 1)->dur += state.current_note->dur;
		fprintf(stderr, "\t actually tied\n");
		memset(state.current_note, '\0', sizeof(JLilyNote));
		state.current_note--;
		state.current_note->tied = false;
		state.num_notes--;
	    }
	    state.current_note->closed = true;
	}
	break;
    case JLILY_NOTE: {
	if (state.num_notes != 0) { 
	    state.current_note++;
	}
	state.num_notes++;
	fprintf(stderr, "SETTING JLilyNote index %lu\n", state.current_note - state.notes);
	state.current_note->name = text[index];
	state.current_note->dur = state.current_dur_sframes;
	int accidental = 0;
	int subindex = index + 1;
	while (subindex < max_i) {
	    if (text[subindex] == 'e') {
		accidental--;
		tok_len += 2;
		subindex += 2;
	    } else if (text[subindex] == 'i') {
		accidental++;
		tok_len += 2;
		subindex += 2;
	    } else {
		break;
	    }
	}
	state.current_note->accidental = accidental;
	char last_note_name = 'c';
	int last_note_accidental = 0;
	uint8_t  last_note_pitch = 60;
	if (state.current_note - state.notes > 0) { /* Relative */
	    JLilyNote *last_note = state.current_note - 1;
	    last_note_name = last_note->name;
	    last_note_accidental = last_note->accidental;
	    last_note_pitch = last_note->pitch;
	}
	int interval = note_name_interval(last_note_name, state.current_note->name);
	interval += accidental;
	interval -= last_note_accidental;
	/* fprintf(stderr, "Raw interval: %d (%d->%d) (accidentals: %d %d)\n", interval, last_note_pitch, last_note_pitch + interval, last_note_accidental, accidental); */
	state.current_note->pitch = last_note_pitch + interval;
	if (interval > 6) {
	    state.current_note->pitch -= 12;
	    /* fprintf(stderr, "\t\tDROP to %d\n", state.current_note->pitch); */
	}
	/* fprintf(stderr, "JLilyNote %d ts: %f\n", state.num_notes, ts); */
	state.current_note->ts = state.ts;
	/* fprintf(stderr, "SET NOTE %d VEL %d\n", state.num_notes, state.velocity); */
	state.current_note->velocity = state.velocity;
	/* fprintf(stderr, "\tSetting ts %d, incr %d\n", ts, current_dur); */
	if (!state.chord_open) {
	    state.ts += state.current_dur_sframes;
	}
    }
	break;
    case JLILY_REST:
	if (state.chord_open) break;
	state.ts += state.current_dur_sframes;
	break;
    case JLILY_DURATION: {
	if (state.chord_open) break;
	int dur_literal;
	int char1 = text[index] - '0';
	int char2;
	int subindex = index + 1;
	if (subindex < max_i && (char2 = text[subindex] - '0') >= 0 && char2 <= 9) {
	    tok_len++;
	    subindex++;
	    dur_literal = char1 * 10 + char2;
	} else {
	    dur_literal = char1;
	}
	int dots = 0;
	while (subindex < max_i && text[subindex] == '.') {
	    dots++;
	    tok_len++;
	    subindex++;
	}

	double dur_predot = state.beat_dur_sframes / (dur_literal / 4.0);
	double dur = dur_predot;
	for (int i=0; i<dots; i++) {
	    dur_predot /= 2;
	    dur += dur_predot;
	}

	state.ts -= state.current_dur_sframes;

	double edit_ts = state.current_note->ts;
	JLilyNote *n = state.current_note;
	if (!state.current_note->closed) {

	    JLilyNote *n = state.current_note;
	    while (n >= state.notes && fabs(n->ts - edit_ts) < 1e-9) {
		n->dur -= state.current_dur_sframes;
		n--;
	    }
	}
	state.current_dur_sframes = dur;
	state.ts += state.current_dur_sframes;

	if (!state.current_note->closed) { 
	    n = state.current_note;
	    while (n >= state.notes && fabs(n->ts - edit_ts) < 1e-9) {
		n->dur += state.current_dur_sframes;
		n--;
	    }
	}
    }
	break;
    case JLILY_SHARP:
	break;
    case JLILY_FLAT:
	break;
    case JLILY_OCTAVE:
	if (text[index] == ',') {
	    state.current_note->pitch -= 12;
	} else if (text[index] == '\'') {
	    state.current_note->pitch += 12;
	}
	break;
    case JLILY_CHORD_OPEN:
	state.chord_open = true;
	break;
    case JLILY_CHORD_CLOSE:
	state.chord_open = false;
	state.ts += state.current_dur_sframes;
	break;
    /* case JLILY_VELOCITY: { */
    /* 	int subindex = index + 1; */
    /* 	int velocity_loc = 0; */
    /* 	while (subindex < max_i) { */
    /* 	    char c = text[subindex]; */
    /* 	    if (c == '=') { */
    /* 		subindex++; */
    /* 		tok_len++; */
    /* 	    } else if (c >= '0' && c <= '9') { */
    /* 		/\* fprintf(stderr, "\t\t(%c): loc*10 = %d;  + %d\n", c, velocity_loc * 10, c- '0'); *\/ */
    /* 		velocity_loc = (velocity_loc * 10) + c - '0'; */
    /* 		subindex++; */
    /* 		tok_len++; */
    /* 	    } else { */
    /* 		break; */
    /* 	    } */
    /* 	} */
    /* 	/\* fprintf(stderr, "Set vel from loc: %d\n", velocity_loc); *\/ */
    /* 	state.velocity = velocity_loc > 127 ? 127 : velocity_loc; */
    /* 	/\* fprintf(stderr, "VEL: %d\n", velocity); *\/ */
    /* } */
    /* 	break; */
    case JLILY_TIE:
	state.current_note->tied = true;
	break;
    case JLILY_BARLINE:
	/* Ignore barlines; for input clarity only */
	break;
    case JLILY_BLOCK_OPEN: {
	if (state.repeat_block_index < 0) break;
	JLilyRepeatBlock *block = state.repeat_blocks + state.repeat_block_index;
	block->first_note_index = state.num_notes <= 0 ? 0 : state.current_note + 1 - state.notes;
	block->ts_start = state.ts;
    }
	break;
    case JLILY_BLOCK_CLOSE: {
	if (state.repeat_block_index < 0) break;
	JLilyRepeatBlock *block = state.repeat_blocks + state.repeat_block_index;
        block->last_note_index = state.current_note - state.notes;
	block->ts_end = state.ts;
	state.repeat_block_index--;
	handle_repeat(block);
    }
	break;
    }
    state.last_token_type = type;
    return tok_len;
}

int do_next_token(const char *text, int index, int max_i)
{
    enum token_type type;
    char c = text[index];
    if (c >= 'a' && c <= 'g') {
	type = JLILY_NOTE;
    } else if (c == 'r') {
	type = JLILY_REST;
    } else if (c >= '0' && c <= '9') {
	type = JLILY_DURATION;
    } else if (c == 'i') {
	type = JLILY_SHARP;
    } else if (c == '\'' || c == ',') {
	type = JLILY_OCTAVE;
    } else if (c == '<') {
	type = JLILY_CHORD_OPEN;
    } else if (c == '>') {
	type = JLILY_CHORD_CLOSE;
    } else if (c == ' ') {
	type = JLILY_WHITESPACE;
    } else if (c == '~') {
	type = JLILY_TIE;
    } else if (c == '|') {
	type = JLILY_BARLINE;
    } else if (c == '{') {
	type = JLILY_BLOCK_OPEN;
    } else if (c == '}') {
	type = JLILY_BLOCK_CLOSE;
    } else if (c == '\\') {
	type = JLILY_ESCAPE;
    } else {
	fprintf(stderr, "Parse error, char %c\n", c);
	return 0;
    }
    return handle_next_token(text, index, type, max_i);
}

/* Translation unit entrypoint; sets globals
   Returns num state.notes */
int jlily_string_to_mclip(
    const char *str,
    double beat_dur_sframes_loc,
    int32_t pos_offset,
    MIDIClip *mclip)
{
    memset(&state, '\0', sizeof(state));
    state.beat_dur_sframes = beat_dur_sframes_loc;
    state.current_dur_sframes = beat_dur_sframes_loc;
    state.current_note = state.notes;
    state.last_token_type = JLILY_NONE;
    state.velocity = 100;
    state.repeat_block_index = -1;

    int str_index = 0;
    int max_i = strlen(str);

    while (str_index < max_i) {
	int incr = do_next_token(str, str_index, max_i);
	if (incr <= 0) {
	    fprintf(stderr, "JLily parsing error\n");
	    return 0;
	} else {
	    str_index += incr;
	}
    }
    if (mclip) {
	for (int i=0; i<state.num_notes; i++) {
	    JLilyNote *jnote = state.notes + i;
	    midi_clip_add_note(
		mclip,
		0,
		jnote->pitch,
		jnote->velocity,
		(int32_t)jnote->ts + pos_offset,
		(int32_t)jnote->ts + pos_offset + jnote->dur);
		/* (int32_t)jnote->ts + pos_offset + 1000); */
	}
    } else {
	for (int i=0; i<state.num_notes; i++) {
	    JLilyNote *jnote = state.notes + i;
	    fprintf(stderr, "(%d) pitch %d vel %d\n", (int32_t)jnote->ts + pos_offset, jnote->pitch, jnote->velocity);
	}
    }
    return state.num_notes;
}

#include "color.h"
#include "modal.h"
extern Window *main_win;
extern struct colors colors;

static int add_jlily_modalfn(void *mod_v, void *target)
{

    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    Track *t = timeline_selected_track(tl);
    ClickSegment *s = click_segment_active_at_cursor(tl);
    int32_t beat_dur;
    if (s) {
	beat_dur = s->cfg.dur_sframes / s->cfg.num_atoms * s->cfg.beat_subdiv_lens[0];
    } else {
	beat_dur = (double)session->proj.sample_rate / 120.0 * 60.0;
    }

    ClipRef *cr = clipref_at_cursor();
    MIDIClip *mclip;
    bool clip_created = false;
    if (!cr) {
	mclip = midi_clip_create(NULL, t);
	cr = clipref_create(t, tl->play_pos_sframes, CLIP_MIDI, mclip); 
	clip_created = true;
    } else {
	if (cr->type == CLIP_AUDIO) return -1;
	mclip = cr->source_clip;
    }
    int32_t pos_rel = tl->play_pos_sframes - cr->tl_pos + cr->start_in_clip;

    fprintf(stderr, "ADDING JLILY STRING: %s\n", (char *)target);
    jlily_string_to_mclip(target, (double)beat_dur, pos_rel, mclip);
    if (clip_created) {
	if (mclip->num_notes == 0) {
	    midi_clip_destroy(mclip);
	    goto pop_modal_and_exit;
	} else {
	    session->proj.active_clip_index++;
	}
    }
    
    int32_t end_rest_dur = 0;
    if (state.num_notes > 0) {
	end_rest_dur = state.ts - state.notes[state.num_notes - 1].ts - state.notes[state.num_notes - 1].dur;
    }
    if (mclip->num_notes > 0) {
	mclip->len_sframes =  mclip->notes[mclip->num_notes - 1].end_rel + end_rest_dur + 1;
    } else {
	mclip->len_sframes = state.ts;
    }

    clipref_reset(cr, true);
pop_modal_and_exit:
    tl->needs_redraw = true;
    window_pop_modal(main_win);
    return 0;

}

void timeline_add_jlily()
{
    static const int MAX_BUFLEN = 1024;
    Layout *mod_lt = layout_add_child(main_win->layout);
    layout_set_default_dims(mod_lt);
    Modal *m = modal_create(mod_lt);
    modal_add_header(m, "Insert JLily notes (LilyPond)", &colors.light_grey, 5);
    static char buf[MAX_BUFLEN];
    static bool buf_initialized = false;
    if (!buf_initialized) {
	memset(buf, '\0', MAX_BUFLEN);
	buf_initialized = true;
    }
    modal_add_textentry(
	m,
        buf,
	MAX_BUFLEN,
	NULL,
	NULL,
	NULL);
    m->stashed_obj = buf;


    modal_add_button(m, "Insert", add_jlily_modalfn);
    /* button->target = buf; */
    m->submit_form = add_jlily_modalfn;
    window_push_modal(main_win, m);
    modal_reset(m);
    /* fprintf(stdout, "about to call move onto\n"); */
    modal_move_onto(m);

} 

