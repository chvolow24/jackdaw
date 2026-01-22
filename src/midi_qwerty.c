/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "endpoint.h"
#include "midi_io.h"
#include "porttime.h"
#include "project.h"
#include "session.h"
#include "status.h"
#include "window.h"

#define VELOCITY_INCR_AMT 9

extern Window *main_win;

struct mqwert_state {
    bool active;
    int octave;
    int transpose;
    int velocity;
    bool live; /* Live mode handles keyups */
    MIDIDevice v_device; /* Virtual device */
    uint8_t key_note_table[96]; /* Held notes; table indices are ASCII printable characters */
    char octave_str[3];
    char transpose_str[4];
    char velocity_str[4];
    char monitor_device_name[MAX_NAMELENGTH];
    Endpoint active_ep;

    float pitch_bend_cents;
};

static struct mqwert_state state = {
    false, 0, 0, 100, false, {0}, {0}, "+0", "+0", "100", "(none)", {0}
};

void mqwert_activate();
void mqwert_deactivate();

void mqwert_active_cb(Endpoint *ep)
{
    if (ep->current_write_val.bool_v) {
	mqwert_activate();
    } else {
	mqwert_deactivate();
    }
}

void mqwert_init()
{
    endpoint_init(
	&state.active_ep,
	&state.active,
	JDAW_BOOL,
	"",
	"",
	JDAW_THREAD_MAIN,
	NULL, mqwert_active_cb, NULL,
	NULL, NULL, NULL, NULL);
}

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
    if (note < 1) return 1; /* Zero has special meaning in note-key map */
    if (note > 127) return 127;
    return note;
}

void mqwert_activate()
{
    state.active = true;
    Session *session = session_get();
    if (session->midi_qwerty) return; /* already activated */
    window_push_mode(main_win, MODE_MIDI_QWERTY);
    session->midi_qwerty = true;
    snprintf(state.octave_str, 3, "%s%d", state.octave >= 0 ? "+" : "", state.octave);
    if (!timeline_check_set_midi_monitoring()) {
	Track *track = timeline_selected_track(ACTIVE_TL);
	if (track) {
	    track_set_out_builtin_synth(track);
	} else {
	    status_set_errstr("Error: no track. Add with C-t");
	    mqwert_deactivate();
	    return;
	}
	/* status_set_errstr("Error: no instrument is active. Add synth to selected track with S-s"); */
	/* mqwert_deactivate(); */
	/* return; */
    }
    status_set_alert_str("QWERTY Piano active; ESC to exit");
    panel_page_refocus(session->gui.panels, "QWERTY piano", 1);
    ACTIVE_TL->needs_redraw = true;
}

void mqwert_deactivate()
{
    status_set_alert_str(NULL);
    state.active = false;
    window_extract_mode(main_win, MODE_MIDI_QWERTY);
    Session *session = session_get();
    session->midi_qwerty = false;
    midi_device_close_all_notes(&state.v_device);
    memset(state.key_note_table, '\0', sizeof(state.key_note_table));
}

void mqwert_octave(int incr)
{
    state.octave += incr;
    if (state.octave > 4) state.octave = 4;
    else if (state.octave < -4) state.octave = -4;
    snprintf(state.octave_str, 3, "%s%d", state.octave >= 0 ? "+" : "", state.octave);
}

void mqwert_transpose(int incr)
{
    state.transpose += incr;
    if (state.transpose > 11) state.transpose = 11;
    if (state.transpose < -11) state.transpose = -11;
    snprintf(state.transpose_str, 4, "%s%d", state.transpose >= 0 ? "+" : "", state.transpose);
}

void mqwert_velocity(int incr)
{
    state.velocity += incr * VELOCITY_INCR_AMT;
    if (state.velocity < 0) state.velocity = 1; /* from 100, decr 9, min = 1 */
    else if (state.velocity > 127) state.velocity = 127;
    snprintf(state.velocity_str, 4, "%d", state.velocity);
}

void mqwert_handle_key(char key, bool is_keyup)
{
    int note;
    /* fprintf(stderr, "\tmqwert %s: %d\n", is_keyup ? "key UP" : "key DOWN", key); */
    if (!is_keyup) {
	if (state.key_note_table[key - ' '] != 0) { /* Key is occupied */
	    return; /* No restrikes! */
	}
	note = note_transpose(raw_note_from_key(key));
	state.key_note_table[key - ' '] = note;
    } else {
	note = state.key_note_table[key - ' '];
	state.key_note_table[key - ' '] = 0;
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

void mqwert_pitch_bend(float cents)
{
    state.pitch_bend_cents += cents;
    cents = state.pitch_bend_cents;
    if (cents > 200) cents = 200;
    else if (cents < -200) cents = -200;
    state.pitch_bend_cents = cents;
    PmEvent e;
    e.timestamp = Pt_Time();
    /* Default to MIDI channel 0 */
    uint8_t type = 0xE0;
    uint16_t raw_val = 16384.0f * (cents + 200) / 200.0f;

    uint8_t lsb = raw_val & 0xFF;
    uint8_t msb = raw_val >> 8;

    e.message = Pm_Message(
	type,
	lsb,
	msb
    );
    
    if (midi_device_add_event(&state.v_device, e) == 0) {
	fprintf(stderr, "Error in midi_qwert_handle_note: device event buf full\n");
    }

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
}

/* returns note if pressed, 0 if not */
uint8_t mqwert_get_key_state(char key)
{
    if (key >= 'A' && key <= 'Z') key = 'a' + (key - 'A');
    return state.key_note_table[key - ' '];
}

char *mqwert_get_octave_str()
{
    return state.octave_str;
}
char *mqwert_get_transpose_str()
{
    return state.transpose_str;
}
char *mqwert_get_velocity_str()
{
    return state.velocity_str;
}

void mqwert_set_monitor_device_name(const char *device_name)
{
    int cpy_len = strlen(device_name) + 1;
    if (cpy_len > MAX_NAMELENGTH) cpy_len = MAX_NAMELENGTH;
    memcpy(state.monitor_device_name, device_name, cpy_len);
}

char *mqwert_get_monitor_device_str()
{
    return state.monitor_device_name;
}

Endpoint *mqwert_get_active_ep()
{
    return &state.active_ep;
}
