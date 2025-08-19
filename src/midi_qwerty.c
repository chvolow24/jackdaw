#include <stdint.h>
#include <stdbool.h>
#include "midi_io.h"
#include "porttime.h"
#include "session.h"

extern Window *main_win;

struct mqwert_state {
    int octave;
    int transpose;
    uint8_t velocity;
    bool live; /* Live mode handles keyups */
    MIDIDevice v_device; /* Virtual device */
    int key_note_table[96]; /* Held notes; table indices are ASCII printable characters */
};

static struct mqwert_state state = {
    0, 0, 100, false, {0}
};

static int raw_note_from_key(char key)
{
    switch (key) {
    case 'a':
	return 60;
    case 'w':
	return 61;
    case 's':
	return 62;
    case 'e':
	return 63;
    case 'd':
	return 64;
    case 'f':
	return 65;
    case 't':
	return 66;
    case 'g':
	return 67;
    case 'y':
	return 68;
    case 'h':
	return 69;
    case 'u':
	return 70;
    case 'j':
	return 71;
    case 'k':
	return 72;
    case 'o':
	return 73;
    case 'l':
	return 74;
    case 'p':
	return 75;
    case ';':
	return 76;
    case '\'':
	return 77;
    default:
	return 0;
    }
}

static int note_transpose(int note_raw)
{
    int note = note_raw;
    note += 12 * state.octave;
    note += state.transpose;
    if (note < 0) return 0;
    if (note > 127) return 127;
    return note;
}

void mqwert_activate()
{
    Session *session = session_get();
    if (session->midi_qwerty) return; /* already activated */
    window_push_mode(main_win, MIDI_QWERTY);
    session->midi_qwerty = true;
}

void mqwert_deactivate()
{
    if (TOP_MODE != MIDI_QWERTY) {
	fprintf(stderr, "Error: call to mqwert_deactivate, top mode not MIDI_QWERTY\n");
    } else {
	window_pop_mode(main_win);
    }
    Session *session = session_get();
    session->midi_qwerty = false;
    midi_device_close_all_notes(&state.v_device);
}

void mqwert_octave(int incr)
{
    state.octave += incr;
}

void mqwert_transpose(int incr)
{
    state.transpose += incr;
}

void mqwert_handle_key(char key, bool is_keyup)
{
    int note;
    

    if (!is_keyup) {
	if (state.key_note_table[key - ' '] != -1) { /* Key is occupied */
	    return; /* No restrikes! */
	}
	note = note_transpose(raw_note_from_key(key));
	state.key_note_table[key - ' '] = note;
    } else {
	note = state.key_note_table[key - ' '];
	state.key_note_table[key - ' '] = -1;
    }

    /* Note *unclosed = state.v_device.unclosed_notes + note; */
    
    PmEvent e;
    e.timestamp = Pt_Time();
    /* Default to MIDI channel 0 */
    uint8_t type = is_keyup ? 0x80 : 0x90;

    e.message = Pm_Message(
	type,
	(uint8_t)note,
	state.velocity
	);
    
    if (midi_device_add_event(&state.v_device, e) == 0) {
	fprintf(stderr, "Error in midi_qwert_handle_note: device event buf full\n");
    }
    /* if (!is_keyup) { */
    /* 	unclosed->unclosed = true; */
    /* 	unclosed->key = note; */
    /* 	unclosed->channel = 0; */
    /* 	unclosed->start_rel = e.timestamp; */
    /* 	unclosed->velocity = state.velocity; */
    /* } else { */
    /* 	unclosed->unclosed = false; */
    /* } */
}

/* void mqwert_handle_keyup(char key) */
/* { */
/*     /\* int note = raw_note_from_key(key); *\/ */
/*     mqwert_handle_key(key, true); */
/* } */


void mqwert_get_current_notes(MIDIDevice *dst_device)
{
    dst_device->num_unconsumed_events = state.v_device.num_unconsumed_events;
    memcpy(&dst_device->buffer, &state.v_device.buffer, dst_device->num_unconsumed_events * sizeof(PmEvent));
    state.v_device.num_unconsumed_events = 0;
    /* for (int i=0; i<128; i++) { */
    /* 	Note *unclosed_src = state.v_device.unclosed_notes + i; */
    /* 	if (unclosed_src->unclosed) { */
    /* 	    Note *unclosed_dst = dst_device->unclosed_notes + i; */
    /* 	    *unclosed_dst = *unclosed_src; */
    /* 	    memset(unclosed_src, '\0', sizeof(Note)); */
    /* 	} */
    /* } */
    /* memcpy(&dst_device->unclosed_notes, &state.v_device.unclosed_notes, sizeof(state.v_device.unclosed_notes)); */
    /* memset(&state.v_device, '\0', sizeof(state.v_device)); */
}

