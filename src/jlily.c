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
#define MAX_NOTES 32
#define MAX_JLILY_BUFLEN 1024

enum token_type {
    JLILY_NONE,
    JLILY_WHITESPACE,
    JLILY_NOTE,
    JLILY_REST,
    JLILY_DURATION,
    JLILY_SHARP,
    JLILY_FLAT,
    JLILY_OCTAVE,
    JLILY_CHORD_OPEN,
    JLILY_CHORD_CLOSE,
    JLILY_VELOCITY
};

typedef struct jlily_note {
    bool closed;
    char name;
    int accidental;
    uint8_t pitch;
    uint8_t velocity;
    double ts;
    int32_t dur;
} JLilyNote;

static double beat_dur_sframes;
static double ts;
static double current_dur_sframes;
static uint8_t velocity;

static JLilyNote notes[MAX_NOTES];
static JLilyNote *current_note;
static int num_notes;
static bool chord_open;
static enum token_type last_token_type;

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
    case JLILY_WHITESPACE:
	current_note->closed = true;
	break;
    case JLILY_NOTE: {
	if (num_notes != 0) 
	    current_note++;
	num_notes++;
	/* fprintf(stderr, "SETTING JLilyNote index %lu\n", current_note - notes); */
	current_note->name = text[index];
	current_note->dur = current_dur_sframes;
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
	current_note->accidental = accidental;
	char last_note_name = 'c';
	int last_note_accidental = 0;
	uint8_t  last_note_pitch = 60;
	if (current_note - notes > 0) { /* Relative */
	    JLilyNote *last_note = current_note - 1;
	    last_note_name = last_note->name;
	    last_note_accidental = last_note->accidental;
	    last_note_pitch = last_note->pitch;
	}
	int interval = note_name_interval(last_note_name, current_note->name);
	interval += accidental;
	interval -= last_note_accidental;
	/* fprintf(stderr, "Raw interval: %d (%d->%d) (accidentals: %d %d)\n", interval, last_note_pitch, last_note_pitch + interval, last_note_accidental, accidental); */
	current_note->pitch = last_note_pitch + interval;
	if (interval > 6) {
	    current_note->pitch -= 12;
	    /* fprintf(stderr, "\t\tDROP to %d\n", current_note->pitch); */
	}
	/* fprintf(stderr, "JLilyNote %d ts: %f\n", num_notes, ts); */
	current_note->ts = ts;
	/* fprintf(stderr, "SET NOTE %d VEL %d\n", num_notes, velocity); */
	current_note->velocity = velocity;
	/* fprintf(stderr, "\tSetting ts %d, incr %d\n", ts, current_dur); */
	if (!chord_open) {
	    ts += current_dur_sframes;
	}
    }
	break;
    case JLILY_REST:
	ts += current_dur_sframes;
	break;
    case JLILY_DURATION: {
	/* fprintf(stderr, "SETTING duration\n"); */
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
	/* fprintf(stderr, "DOTS: %d, dur literal %d\n", dots, dur_literal); */
	double dur_predot = beat_dur_sframes / (dur_literal / 4.0);
	double dur = dur_predot;
	for (int i=0; i<dots; i++) {
	    dur_predot /= 2;
	    dur += dur_predot;
	}
	/* fprintf(stderr, "DUR: %f (beat dur %f, dur_literal %d)\n", dur, beat_dur_sframes, dur_literal); */
	/* if (fabs(dur) < 1e-9) { */
	/*     fprintf(stderr, "chars? %c %c\n", char1, char2); */
	/*     exit(0); */
	/* } */

	ts -= current_dur_sframes;
	current_note->dur -= current_dur_sframes;
	
	current_dur_sframes = dur;
	
	ts += current_dur_sframes;
	current_note->dur += current_dur_sframes;
	/* current_note->ts += current_dur; */
	/* fprintf(stderr, "Dur: %d current: %d\n", dur, current_dur); */
    }
	break;
    case JLILY_SHARP:
	break;
    case JLILY_FLAT:
	break;
    case JLILY_OCTAVE:
	if (text[index] == ',') {
	    current_note->pitch -= 12;
	} else if (text[index] == '\'') {
	    current_note->pitch += 12;
	}
	break;
    case JLILY_CHORD_OPEN:
	chord_open = true;
	break;
    case JLILY_CHORD_CLOSE:
	chord_open = false;
	ts += current_dur_sframes;
	break;
    case JLILY_VELOCITY: {
	int subindex = index + 1;
	int velocity_loc = 0;
	while (subindex < max_i) {
	    char c = text[subindex];
	    if (c == '=') {
		subindex++;
		tok_len++;
	    } else if (c >= '0' && c <= '9') {
		/* fprintf(stderr, "\t\t(%c): loc*10 = %d;  + %d\n", c, velocity_loc * 10, c- '0'); */
		velocity_loc = (velocity_loc * 10) + c - '0';
		subindex++;
		tok_len++;
	    } else {
		break;
	    }
	}
	/* fprintf(stderr, "Set vel from loc: %d\n", velocity_loc); */
	velocity = velocity_loc > 127 ? 127 : velocity_loc;
	/* fprintf(stderr, "VEL: %d\n", velocity); */
    }
	break;
	
    }
    last_token_type = type;
    return tok_len;
}

int do_next_token(const char *text, int index, int max_i)
{
    enum token_type type;
    char c = text[index];
    /* fprintf(stderr, "CHAR: %c, GLOB VEL: %d\n", c, velocity); */
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
    } else if (c == 'v') {
	type = JLILY_VELOCITY;
    } else {
	fprintf(stderr, "Parse error, char %c\n", c);
	return 0;
    }
    return handle_next_token(text, index, type, max_i);
}

/* Translation unit entrypoint; sets globals
   Returns num notes */
int jlily_string_to_mclip(
    const char *str,
    double beat_dur_sframes_loc,
    int32_t pos_offset,
    MIDIClip *mclip)
{

    /* fprintf(stderr, "ARGS? %p, %f, %d, %p\n", str, beat_dur_sframes_loc, pos_offset, mclip); */
    beat_dur_sframes = beat_dur_sframes_loc;
    /* fprintf(stderr, "BEAT DUR SFRAMES? %f LOC? %f\n", beat_dur_sframes, beat_dur_sframes_loc); */
    ts = 0.0;
    current_dur_sframes = beat_dur_sframes_loc / 4.0;
    velocity = 100;
    current_note = notes;
    num_notes = 0;
    chord_open = false;
    last_token_type = JLILY_NONE;

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
	for (int i=0; i<num_notes; i++) {
	    JLilyNote *jnote = notes + i;
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
	for (int i=0; i<num_notes; i++) {
	    JLilyNote *jnote = notes + i;
	    fprintf(stderr, "(%d) pitch %d vel %d\n", (int32_t)jnote->ts + pos_offset, jnote->pitch, jnote->velocity);
	}
    }
    return num_notes;
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
    if (!cr) {
	mclip = midi_clip_create(NULL, t);
	cr = clipref_create(t, tl->play_pos_sframes, CLIP_MIDI, mclip);
    } else {
	if (cr->type == CLIP_AUDIO) return -1;
	mclip = cr->source_clip;
    }
    int32_t pos_rel = tl->play_pos_sframes - cr->tl_pos + cr->start_in_clip;
    
    jlily_string_to_mclip(target, (double)beat_dur, pos_rel, mclip);
    mclip->len_sframes = mclip->notes[mclip->num_notes - 1].end_rel + 1;
    clipref_reset(cr, true);
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
	snprintf(buf, MAX_BUFLEN, "Insert JLily string");
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

