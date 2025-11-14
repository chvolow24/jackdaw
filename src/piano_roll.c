#include "assets.h"
#include "clipref.h"
#include "color.h"
#include "geometry.h"
#include "input.h"
#include "layout.h"
#include "layout_xml.h"
#include "midi_clip.h"
#include "piano_roll.h"
#include "project.h"
#include "session.h"
#include "tempo.h"
#include "textbox.h"
#include "timeline.h"
#include "timeview.h"

extern Window *main_win;
extern struct colors colors;

#define PIANO_TOP_NOTE 108
#define PIANO_BOTTOM_NOTE 21
#define MAX_GRABBED_NOTES 256
#define MAX_QUEUED_EVENTS 64
#define CHORD_THRESHOLD_MSEC 40
/* #define MAX_TIED_NOTES 24 */

enum note_dur {
    DUR_WHOLE,
    DUR_HALF,
    DUR_QUARTER,
    DUR_EIGHTH,
    DUR_SIXTEENTH,
    DUR_THIRTY_SECOND,
    DUR_SIXTY_FOURTH
};

#define MAX_DUR 0
#define MIN_DUR 6

const char *dur_strs[] = {
    "𝅝",
    "𝅗𝅥",
    "𝅘𝅥",
    "𝅘𝅥𝅮",
    "𝅘𝅥𝅯",
    "𝅘𝅥𝅰",
    "𝅘𝅥𝅱"
};


#define NUM_PIANO_ROLL_CONSOLE_PANELS 3

struct piano_roll_gui {
    Textbox *track_name;
    Textbox *solo_button;
    Textbox *clip_label;
    Textbox *clip_name;
    Textbox *in_label;
    Textbox *in_name;
    Textbox *dur_longer_button;
    Textbox *dur_shorter_button;
    Textbox *dur_tb;
    Textbox *tie_button;
    Textbox *chord_button;

    SDL_Rect *info_panel;
    SDL_Rect *input_panel;
    /* Textbox * */
    /* Textbox *dur_tb; */
};

struct note_clipboard {
    Note *notes;
    int32_t num_notes;
};

struct piano_roll_state {
    bool active;
    /* TimeView tv; */
    TimeView *tl_tv;
    MIDIClip *clip;
    ClipRef *cr;
    ClickTrack *ct;
    /* Note *grabbed_notes[MAX_GRABBED_NOTES]; */
    /* int num_grabbed_notes; */
    Layout *layout;
    Layout *note_piano_container;
    Layout *note_canvas_lt;
    Layout *piano_lt;
    Layout *console_lt;
    int selected_note; /* midi value */
    int insert_velocity;

    /* Insertion params */
    enum note_dur current_dur;
    bool chord_mode;
    bool tie;
    /* Note *tied_notes[MAX_TIED_NOTES]; */

    struct piano_roll_gui gui;

    bool mouse_sel_rect_active;
    SDL_Rect mouse_sel_rect;
    SDL_Point mouse_sel_rect_origin;


    /* Insertion Queue */
    PmEvent event_queue[MAX_QUEUED_EVENTS];
    int num_queued_events;
    pthread_mutex_t event_queue_lock;

    struct note_clipboard clipboard;

    /* int32_t last_inserted_note_ids[128]; */
    /* int num_last_inserted_notes; */
    
};


static struct piano_roll_state state;


/* void piano_roll_load_layout() */
/* { */
/*     Layout *main_lt = layout_read_from_xml(PIANO_ROLL_LT_PATH); */
/*     state.layout = main_lt; */
/*     Layout *piano_lt = layout_get_child_by_name_recursive(main_lt, "piano"); */
/*     state.piano_lt = piano_lt; */
/*     layout_read_xml_to_lt(main_lt, PIANO_88_VERTICAL_LT_PATH); */

/* } */

static int32_t get_input_dur_samples()
{
    Session *session = session_get();
    int32_t sample_rate = session->proj.sample_rate;
    double measure_dur;
    double beat_dur;
    double subdiv_dur;
    if (!state.ct) { /* No click track; assume 120bmp 4/4 */
	beat_dur = (double)sample_rate / 120.0 * 60.0;
	measure_dur = beat_dur * 4.0;
	subdiv_dur = beat_dur / 2.0;

    } else {
	ClickSegment *s = click_track_get_segment_at_pos(state.ct, ACTIVE_TL->play_pos_sframes);
	measure_dur = s->cfg.dur_sframes;
	beat_dur = (double)s->cfg.dur_sframes / s->cfg.num_atoms * s->cfg.beat_subdiv_lens[0];
	subdiv_dur = beat_dur / s->cfg.beat_subdiv_lens[0];
    }
    switch (state.current_dur) {
    case DUR_WHOLE:
	return measure_dur;
    case DUR_HALF:
	return beat_dur * 2;
    case DUR_QUARTER:
	return beat_dur;
    default:
	return (int32_t)(subdiv_dur * pow(2.0, (double)((int)DUR_EIGHTH - (int)state.current_dur)));
    }
}
static void piano_roll_init_layout(Session *session)
{
    if (state.layout) return; /* Already init'd */
    Layout *lt = layout_add_child(session->gui.layout);
    layout_read_xml_to_lt(lt, PIANO_ROLL_LT_PATH);
    state.console_lt = layout_get_child_by_name_recursive(lt, "piano_roll_console");
    Layout *piano_container = layout_get_child_by_name_recursive(lt, "piano");

    state.gui.info_panel = &layout_get_child_by_name_recursive(lt, "track_info")->rect;
    state.gui.input_panel = &layout_get_child_by_name_recursive(lt, "insertion_info")->rect;
    
    Layout *piano_lt = layout_read_xml_to_lt(piano_container, PIANO_88_VERTICAL_LT_PATH);
    state.piano_lt = piano_lt;
    Layout *note_canvas = layout_get_child_by_name_recursive(lt, "note_canvas");
    state.note_canvas_lt = note_canvas;
    state.note_piano_container = layout_get_child_by_name_recursive(lt, "note_piano_container");
    /* lt->x.value = audio_rect->rect.x / main_win->dpi_scale_factor; */
    lt->y.value = 0;
    /* lt->y.value = audio_rect->rect.y / main_win->dpi_scale_factor;; */
    lt->w.type = REVREL;
    lt->w.value = 0; // hug the right edge
    lt->h.type = REVREL;
    lt->h.value = session->status_bar.layout->h.value; // hug the top edge of status
    /* state.piano_lt = layout_add_child(lt); */
    /* state.piano_lt->h.type = SCALE; */
    /* state.piano_lt->h.value = 1.0; */
    /* state.piano_lt->x.type = REVREL; */
    /* state.piano_lt->x.value = 0.0; */
    /* layout_read_xml_to_lt(state.piano_lt, PIANO_88_VERTICAL_LT_PATH); */
    layout_force_reset(lt);
    state.layout = lt;
    /* timeview_init(state.tl_tv, &note_canvas->rect, 600, 0, &session->proj.timelines[0]->play_pos_sframes, NULL, NULL); */
}

void piano_roll_init_gui()
{
    Layout *lt = layout_get_child_by_name_recursive(state.console_lt, "current_dur");
    state.gui.dur_tb = textbox_create_from_str(
	/* "TEST", */
	dur_strs[state.current_dur],
	lt,
	main_win->music_font,
	30,
	main_win);

    textbox_style(
	state.gui.dur_tb,
	CENTER,
	false,
	NULL,
	&colors.white);

    lt = layout_get_child_by_name_recursive(state.console_lt, "solo");
    state.gui.solo_button = textbox_create_from_str(
	"S",
	lt,
	main_win->bold_font,
	14,
	main_win);
    textbox_set_border(state.gui.solo_button, &colors.black, 1, MUTE_SOLO_BUTTON_CORNER_RADIUS);
    if (state.cr->track->solo) {
	textbox_set_background_color(state.gui.solo_button, &colors.solo_yellow);
    } else {
	textbox_set_background_color(state.gui.solo_button, &colors.light_grey);
    }


    /* textbox_set_background_color(state.gui.dur_tb, NULL); */
    /* textbox_set_text_color(state.gui.dur_tb, &colors.white); */
    /* textbox_set_trunc(state.gui.dur_tb, false); */
    /* textbox_reset_full(state.gui.dur_tb); */

    lt = layout_get_child_by_name_recursive(state.console_lt, "clip_label");
    state.gui.clip_label = textbox_create_from_str(
	"Clip:",
	lt,
	main_win->mono_font,
	14,
	main_win);
    textbox_style(
	state.gui.clip_label,
	CENTER_LEFT,
	false,
	NULL,
	&colors.white);

    lt = layout_get_child_by_name_recursive(state.console_lt, "in_label");
    state.gui.in_label = textbox_create_from_str(
	"Input:",
	lt,
	main_win->mono_font,
	14,
	main_win);
    textbox_style(
	state.gui.in_label,
	CENTER_LEFT,
	false,
	NULL,
	&colors.white);

    lt = layout_get_child_by_name_recursive(state.console_lt, "dur_longer_button");
    state.gui.dur_longer_button = textbox_create_from_str(
	"1↓",
	lt,
	main_win->mono_font,
	16,
	main_win);	
    textbox_set_border(state.gui.dur_longer_button, &colors.grey, 4, BUTTON_CORNER_RADIUS);
    textbox_style(
	state.gui.dur_longer_button,
	CENTER,
	false,
	&colors.quickref_button_blue,
	&colors.white);

    lt = layout_get_child_by_name_recursive(state.console_lt, "dur_shorter_button");
    state.gui.dur_shorter_button = textbox_create_from_str(
	"2↑",
	lt,
	main_win->mono_font,
	16,
	main_win);
    textbox_set_border(state.gui.dur_shorter_button, &colors.grey, 4, BUTTON_CORNER_RADIUS);
    textbox_style(
	state.gui.dur_shorter_button,
	CENTER,
	false,
	&colors.quickref_button_blue,
	&colors.white);

    lt = layout_get_child_by_name_recursive(state.console_lt, "tie");
    state.gui.tie_button = textbox_create_from_str(
	"Tie (S-t)",
	lt,
	main_win->mono_bold_font,
	14,
	main_win);
    textbox_set_border(state.gui.tie_button, &colors.black, 1, BUTTON_CORNER_RADIUS);
    textbox_style(
	state.gui.tie_button,
	CENTER,
	false,
	&colors.grey,
	&colors.black);


    lt = layout_get_child_by_name_recursive(state.console_lt, "chord");
    state.gui.chord_button = textbox_create_from_str(
	"Chord (S-h)",
	lt,
	main_win->mono_bold_font,
	14,
	main_win);
    textbox_set_border(state.gui.chord_button, &colors.black, 1, BUTTON_CORNER_RADIUS);
    textbox_style(
	state.gui.chord_button,
	CENTER,
	false,
	&colors.grey,
	&colors.black);
}

void piano_roll_deinit_gui()
{
    textbox_destroy_keep_lt(state.gui.track_name);
    textbox_destroy_keep_lt(state.gui.solo_button);
    textbox_destroy_keep_lt(state.gui.clip_label);
    textbox_destroy_keep_lt(state.gui.clip_name);
    textbox_destroy_keep_lt(state.gui.in_label);
    textbox_destroy_keep_lt(state.gui.in_name);
    textbox_destroy_keep_lt(state.gui.dur_tb);
    textbox_destroy_keep_lt(state.gui.dur_longer_button);
    textbox_destroy_keep_lt(state.gui.dur_shorter_button);
    textbox_destroy_keep_lt(state.gui.tie_button);
    textbox_destroy_keep_lt(state.gui.chord_button);
}

void piano_roll_activate(ClipRef *cr)
{
    Session *session = session_get();
    memset(&state, 0, sizeof(struct piano_roll_state));
    window_push_mode(main_win, MODE_PIANO_ROLL);
    session->piano_roll = true;
    state.active = true;
    state.cr = cr;
    if (!cr->track->midi_out) {
	track_set_out_builtin_synth(cr->track);
    }
    state.clip = cr->source_clip;
    state.tl_tv = &ACTIVE_TL->timeview;
    state.selected_note = 60;
    state.insert_velocity = 100;

    state.current_dur = DUR_EIGHTH;
    state.ct = timeline_governing_click_track(ACTIVE_TL);

    int err = pthread_mutex_init(&state.event_queue_lock, NULL);
    if (err != 0) {
	fprintf(stderr, "Error initializing piano roll event queue lock: %s\n", strerror(err));
    }
    
    piano_roll_init_layout(session_get());
    piano_roll_init_gui();

    state.gui.track_name = textbox_create_from_str(
	cr->track->name,
	layout_get_child_by_name_recursive(state.console_lt, "track_name"),
	main_win->mono_bold_font,
	16,
	main_win);
    /* textbox_set_border(state.gui.track_name, &colors.grey, 1, 0); */
    /* textbox_set_pad(state.gui.track_name, 6, 4); */
    textbox_style(
	state.gui.track_name,
	CENTER_LEFT,
	true,
	NULL,
	&colors.white);


    state.gui.clip_name = textbox_create_from_str(
	cr->name,
	layout_get_child_by_name_recursive(state.console_lt, "clip_name"),
	main_win->std_font,
	14,
	main_win);
    textbox_set_border(state.gui.clip_name, &colors.grey, 1, MUTE_SOLO_BUTTON_CORNER_RADIUS);
    textbox_style(
	state.gui.clip_name,
	CENTER,
	true,
	&colors.quickref_button_blue,
	&colors.white);
    textbox_set_border(state.gui.clip_name, &colors.grey, 1, MUTE_SOLO_BUTTON_CORNER_RADIUS);


    const char *in_name = cr->track->input_type == MIDI_DEVICE ? ((MIDIDevice *)cr->track->input)->name : "(none)";
    state.gui.in_name = textbox_create_from_str(
	in_name,
	layout_get_child_by_name_recursive(state.console_lt, "in_name"),
	main_win->std_font,
	14,
	main_win);
    textbox_set_border(state.gui.in_name, &colors.grey, 1, MUTE_SOLO_BUTTON_CORNER_RADIUS);
    textbox_style(
	state.gui.in_name,
	CENTER,
	true,
	&colors.quickref_button_blue,
	&colors.white);

    state.tl_tv->rect = &state.note_canvas_lt->rect;
}

void piano_roll_deactivate()
{
    midi_clip_ungrab_all(state.clip);
    state.clip = NULL;
    InputMode im = window_pop_mode(main_win);
    if (im != MODE_PIANO_ROLL) {
	fprintf(stderr, "Error: deactivating piano roll, top mode %s\n", input_mode_str(im));
    }
    piano_roll_deinit_gui();

    Session *session = session_get();
    state.tl_tv->rect = session->gui.audio_rect;
    session->piano_roll = false;
    state.active = false;
    timeline_reset_full(ACTIVE_TL);
    /* layout_destroy(state.layout); */
    
}

void piano_roll_zoom_in()
{
    timeview_rescale(state.tl_tv, 1.2, false, (SDL_Point){0});
}
void piano_roll_zoom_out()
{
    timeview_rescale(state.tl_tv, 0.8, false, (SDL_Point){0});
}
void piano_roll_move_view_left()
{
    timeview_scroll_horiz(state.tl_tv, -60);
}
void piano_roll_move_view_right()
{
    timeview_scroll_horiz(state.tl_tv, 60);
}



void piano_roll_grabbed_notes_move_vertical(int move_by);
void piano_roll_move_note_selector(int by)
{
    int orig = state.selected_note;
    state.selected_note += by;
    if (state.selected_note > PIANO_TOP_NOTE) state.selected_note = PIANO_TOP_NOTE;
    else if (state.selected_note < PIANO_BOTTOM_NOTE) state.selected_note = PIANO_BOTTOM_NOTE;
    piano_roll_grabbed_notes_move_vertical(state.selected_note - orig);
}

void piano_roll_forward_dur()
{
    int32_t dur_sframes = get_input_dur_samples();
    timeline_set_play_position(state.cr->track->tl, state.cr->track->tl->play_pos_sframes + dur_sframes, true);
}

void piano_roll_back_dur()
{
    int32_t dur_sframes = get_input_dur_samples();
    timeline_set_play_position(state.cr->track->tl, state.cr->track->tl->play_pos_sframes - dur_sframes, true);
}

/* void piano_roll_note_up(int count) */
/* { */
/*     state.selected_note += count; */
/*     if (state.selected_note > PIANO_TOP_NOTE) state.selected_note = PIANO_TOP_NOTE; */
/* } */
/* void piano_roll_note_down(int count) */
/* { */
/*     state.selected_note -= count; */
/*     if (state.selected_note < PIANO_BOTTOM_NOTE) state.selected_note = PIANO_BOTTOM_NOTE; */
/* } */


void piano_roll_next_note()
{
    if (state.clip->num_notes == 0) return;
    Session *session = session_get();
    int32_t play_pos_rel = ACTIVE_TL->play_pos_sframes - state.cr->tl_pos + state.cr->start_in_clip;
    /* Note *note = midi_clipref_get_next_note(state.cr, pos, &pos); */
    int32_t note_i = midi_clipref_check_get_first_note(state.cr);
    int32_t last_note_i = midi_clipref_check_get_last_note(state.cr);
    int32_t diff_min = INT32_MAX;
    int32_t set_pos;
    Note *sel_note = NULL;
    while (note_i <= last_note_i) {
	Note *note = state.clip->notes + note_i;
	if (note->grabbed && session->dragging) {
	    note_i++;
	    continue;
	}

	int32_t diff = note->start_rel - play_pos_rel;
	if (diff > 0 && diff <= diff_min) { /* <= bc prioritize start pos over end pos */
	    diff_min = diff;
	    set_pos = note_tl_start_pos(note, state.cr);
	    sel_note = note;
	    /* ret = note; */
	}
	diff = note->end_rel - play_pos_rel;
	if (diff > 0 && diff < diff_min) {
	    diff_min = diff;
	    set_pos = note_tl_end_pos(note, state.cr);
	    sel_note = note;
	}
	note_i++;
    }
    if (sel_note) {
	timeline_set_play_position(ACTIVE_TL, set_pos, true);
	if (!(session->dragging && state.clip->num_grabbed_notes > 0)) {
	    state.selected_note = sel_note->key;
	}
    }
}

void piano_roll_prev_note()
{
    if (state.clip->num_notes == 0) return;
    Session *session = session_get();
    int32_t play_pos_rel = ACTIVE_TL->play_pos_sframes - state.cr->tl_pos + state.cr->start_in_clip;
    /* Note *note = midi_clipref_get_next_note(state.cr, pos, &pos); */
    int32_t note_i = midi_clipref_check_get_first_note(state.cr);
    int32_t last_note_i = midi_clipref_check_get_last_note(state.cr);
    int32_t diff_min = INT32_MAX;
    int32_t set_pos;
    Note *sel_note = NULL;
    while (note_i <= last_note_i) {
	Note *note = state.clip->notes + note_i;
	if (note->grabbed && session->dragging) {
	    note_i++;
	    continue;
	}
	int32_t diff = play_pos_rel - note->start_rel;
	if (diff > 0 && diff <= diff_min) { /* <= bc prioritize start pos over end pos */
	    diff_min = diff;
	    set_pos = note_tl_start_pos(note, state.cr);
	    sel_note = note;
	    /* ret = note; */
	}
	diff = play_pos_rel - note->end_rel;
	if (diff > 0 && diff < diff_min) {
	    diff_min = diff;
	    set_pos = note_tl_end_pos(note, state.cr);
	    sel_note = note;
	}
	note_i++;
    }
    if (sel_note) {
	timeline_set_play_position(ACTIVE_TL, set_pos, true);
	if (!(session->dragging && state.clip->num_grabbed_notes > 0)) {
	    state.selected_note = sel_note->key;
	}
    }
}

void piano_roll_up_note()
{
    if (state.clip->num_notes == 0) return;
    Session *session = session_get();
    int32_t pos = ACTIVE_TL->play_pos_sframes;
    Note *note = midi_clipref_up_note_at_cursor(state.cr, pos, state.selected_note);
    if (note) {
	timeline_set_play_position(ACTIVE_TL, pos, false);
	state.selected_note = note->key;
    }
}

void piano_roll_down_note()
{
    if (state.clip->num_notes == 0) return;
    Session *session = session_get();
    int32_t pos = ACTIVE_TL->play_pos_sframes;
    Note *note = midi_clipref_down_note_at_cursor(state.cr, pos, state.selected_note);
    if (note) {
	timeline_set_play_position(ACTIVE_TL, pos, false);
	state.selected_note = note->key;
    }
}



static void reset_dur_str()
{
    textbox_set_value_handle(
	state.gui.dur_tb,
	dur_strs[state.current_dur]);
}
void piano_roll_dur_longer()
{
    state.current_dur--;
    if ((int)state.current_dur < (int)MAX_DUR) state.current_dur = MAX_DUR;
    reset_dur_str();
}

void piano_roll_dur_shorter()
{
    /* Indexing reversed */
    state.current_dur++;
    if ((int)state.current_dur > (int)MIN_DUR) state.current_dur = MIN_DUR;
    reset_dur_str();
}

struct insert_note_info {
    uint32_t id;
    int key;
    int velocity;
    int channel;
    int start_tl_pos;
    int end_tl_pos;
};

NEW_EVENT_FN(undo_insert_note, "undo insert note")
    ClipRef *cr = obj1;
    MIDIClip *mclip = cr->source_clip;
    int32_t note_id = val1.int32_v;
    int32_t note_i = midi_clip_get_note_by_id(mclip, note_id);
    midi_clip_remove_note_at(mclip, note_i);
    if (state.active) {
        timeline_set_play_position(state.cr->track->tl, val2.int32_v, false);
    }
}

NEW_EVENT_FN(redo_insert_note, "redo insert note")
    ClipRef *cr = obj1;
    MIDIClip *mclip = cr->source_clip;
    struct insert_note_info *info = obj2;
    Note *new = midi_clip_insert_note(mclip, info->channel, info->key, info->velocity, info->start_tl_pos - cr->tl_pos + cr->start_in_clip, info->end_tl_pos - cr->tl_pos + cr->start_in_clip);
    if (state.active) {
	timeline_set_play_position(state.cr->track->tl, val2.int32_v, false);
    }
    new->id = info->id;
}

NEW_EVENT_FN(undo_redo_extend_tied_note, "undo/redo extend tied note")
    ClipRef *cr = obj1;
    MIDIClip *mclip = cr->source_clip;
    int32_t note_id = val1.int32_v;
    int32_t note_i = midi_clip_get_note_by_id(mclip, note_id);
    Note *note = mclip->notes + note_i;
    note->end_rel = val2.int32_v;
    if (state.active) {
	timeline_set_play_position(state.cr->track->tl, note_tl_end_pos(note, cr), false);
	/* midi_clip_grab_note(mclip, note, NOTE_EDGE_RIGHT); */
    }
}

/* static void piano_roll_insert_chord(int *notes, int num_notes) */
/* { */
/*     Session *session = session_get(); */
/*     /\* fprintf(stderr, "INSERT NUM NOTES BEFORE: %d (%d grabbed)\n", state.clip->num_notes, state.clip->num_grabbed_notes); *\/ */
/*     Timeline *tl = ACTIVE_TL; */
/*     int32_t clip_chord_pos = tl->play_pos_sframes - state.cr->tl_pos + state.cr->start_in_clip; */
/*     int32_t input_dur = get_input_dur_samples(); */
/*     int32_t end_pos = clip_chord_pos + input_dur; */

/*     Note *tied_notes[num_notes]; */
/*     int num_tied = 0; */
/*     int32_t chord_tl_end; */
/*     if (state.tie && state.clip->num_grabbed_notes > 0) { */
/* 	for (int32_t i=state.clip->first_grabbed_note; i<=state.clip->last_grabbed_note; i++) { */
/* 	    Note *note = state.clip->notes + i; */
/* 	    if (note->grabbed && note->grabbed_edge == NOTE_EDGE_RIGHT && note_tl_end_pos(note, state.cr) == tl->play_pos_sframes) { */
/* 		for (int i=0; i<num_notes; i++) { */
/* 		    if (note->key == notes[i]) { */
/* 			tied_notes[num_tied] = note; */
/* 			num_tied++; */
/* 		    } */
/* 		} */
/* 		break; */
/* 	    } */
/* 	} */
/*     } */
/*     if (num_tied > 0) { */
/* 	int32_t old_end_rel = tied_notes[0]->end_rel; */
/* 	for (int i=0; i<num_tied; i++) { */
/* 	    tied_notes[i]->end_rel += input_dur; */
/* 	} */
/* 	chord_tl_end = note_tl_end_pos(tied_notes[0], state.cr); */

/* 	/\* TODO: Undo tied CHORD *\/ */
/* 	/\* user_event_push( *\/ */
/* 	/\*     undo_redo_extend_tied_note, *\/ */
/* 	/\*     undo_redo_extend_tied_note, *\/ */
/* 	/\*     NULL, NULL, *\/ */
/* 	/\*     state.cr, *\/ */
/* 	/\*     NULL, *\/ */
/* 	/\*     (Value){.int32_v = tied_note->id}, *\/ */
/* 	/\*     (Value){.int32_v = old_end_rel}, *\/ */
/* 	/\*     (Value){.int32_v = tied_note->id}, *\/ */
/* 	/\*     (Value){.int32_v = tied_note->end_rel}, *\/ */
/* 	/\*     0, 0, false, false); *\/ */
	    
/* 	/\* midi_clip_rectify_length(state.clip); *\/ */
/*     } else { */
/* 	midi_clip_ungrab_all(state.clip); */
/* 	Note *note; */
/* 	for (int i=0; i<num_notes; i++) { */
/* 	    note = midi_clip_insert_note(state.clip, 0, notes[i], 100, clip_chord_pos, end_pos); */
/* 	    midi_clip_grab_note(state.clip, note, NOTE_EDGE_RIGHT); */
/* 	} */
/* 	chord_tl_end = note_tl_end_pos(note, state.cr); */

/* 	/\* struct insert_note_info *info = malloc(sizeof(struct insert_note_info)); *\/ */
/* 	/\* info->id = note->id; *\/ */
/* 	/\* info->key = note->key; *\/ */
/* 	/\* info->channel = note->channel; *\/ */
/* 	/\* info->velocity = note->velocity; *\/ */
/* 	/\* info->start_tl_pos = note_tl_start_pos(note, state.cr); *\/ */
/* 	/\* info->end_tl_pos = note_tl_end; *\/ */
/* 	/\* user_event_push( *\/ */
/* 	/\*     undo_insert_note, *\/ */
/* 	/\*     redo_insert_note, *\/ */
/* 	/\*     NULL, NULL, *\/ */
/* 	/\*     state.cr, info, *\/ */
/* 	/\*     (Value){.int32_v = note->id}, *\/ */
/* 	/\*     (Value){.int32_v = note_tl_start_pos(note, state.cr)}, *\/ */
/* 	/\*     (Value){.int32_v = note - state.clip->notes}, *\/ */
/* 	/\*     (Value){.int32_v = note_tl_end_pos(note, state.cr)}, *\/ */
/* 	/\*     0, 0, false, true); *\/ */

/*     } */
/*     midi_clip_check_reset_bounds(state.clip); */
/*     /\* midi_clip_rectify_length(state.clip); *\/ */
/*     if (!state.chord_mode) { */
/* 	timeline_set_play_position(tl, chord_tl_end, false); */
/*     } */
/* } */

Note *piano_roll_insert_note(bool make_synth_note)
{
    Session *session = session_get();
    /* fprintf(stderr, "INSERT NUM NOTES BEFORE: %d (%d grabbed)\n", state.clip->num_notes, state.clip->num_grabbed_notes); */
    Timeline *tl = ACTIVE_TL;
    int32_t clip_note_pos = tl->play_pos_sframes - state.cr->tl_pos + state.cr->start_in_clip;
    int32_t input_dur = get_input_dur_samples();
    int32_t end_pos = clip_note_pos + input_dur;

    Note *ret = NULL;
    Note *tied_note = NULL;
    int32_t note_tl_end;
    if (state.tie) {
	Note **tied_notes = NULL;
	int num_tied = midi_clipref_notes_ending_at_pos(state.cr, tl->play_pos_sframes, &tied_notes, false, NULL);
	for (int i=0; i<num_tied; i++) {
	    if (tied_notes[i]->key == state.selected_note) {
		tied_note = tied_notes[i];
		break;
	    }
	}
	if (num_tied > 0)
	    free(tied_notes);
	
	/* if (state.clip->num_grabbed_notes > 0) { */
	/*     for (int32_t i=state.clip->first_grabbed_note; i<=state.clip->last_grabbed_note; i++) { */
	/* 	Note *note = state.clip->notes + i; */
	/* 	if (note->grabbed */
	/* 	    && note->key == state.selected_note */
	/* 	    && note->grabbed_edge == NOTE_EDGE_RIGHT */
	/* 	    && note_tl_end_pos(note, state.cr) == tl->play_pos_sframes) */
	/* 	{ */
	/* 	    tied_note = note; */
	/* 	    break; */
	/* 	} */
	/*     } */
	/* } */
    }
    if (tied_note) {
	int32_t old_end_rel = tied_note->end_rel;
	tied_note->end_rel += input_dur;
	note_tl_end = note_tl_end_pos(tied_note, state.cr);

	user_event_push(
	    undo_redo_extend_tied_note,
	    undo_redo_extend_tied_note,
	    NULL, NULL,
	    state.cr,
	    NULL,
	    (Value){.int32_v = tied_note->id},
	    (Value){.int32_v = old_end_rel},
	    (Value){.int32_v = tied_note->id},
	    (Value){.int32_v = tied_note->end_rel},
	    0, 0, false, false);
	    
	/* midi_clip_rectify_length(state.clip); */
    } else {
	midi_clip_ungrab_all(state.clip);
	Note *note = midi_clip_insert_note(state.clip, 0, state.selected_note, state.insert_velocity, clip_note_pos, end_pos);
	/* fprintf(stderr, "Fresh note %d, grabbed? %d\n",note->key, note->grabbed); */
	/* int32_t note_tl_start = note_tl_start_pos(note, state.cr); */
	note_tl_end = note_tl_end_pos(note, state.cr);
	/* if (state.tie) { */
	/* midi_clip_grab_note(state.clip, note, NOTE_EDGE_RIGHT); */

	struct insert_note_info *info = malloc(sizeof(struct insert_note_info));
	info->id = note->id;
	info->key = note->key;
	info->channel = note->channel;
	info->velocity = note->velocity;
	info->start_tl_pos = note_tl_start_pos(note, state.cr);
	info->end_tl_pos = note_tl_end;
	user_event_push(
	    undo_insert_note,
	    redo_insert_note,
	    NULL, NULL,
	    state.cr, info,
	    (Value){.int32_v = note->id},
	    (Value){.int32_v = note_tl_start_pos(note, state.cr)},
	    (Value){.int32_v = note - state.clip->notes},
	    (Value){.int32_v = note_tl_end_pos(note, state.cr)},
	    0, 0, false, true);
	ret = note;
    }
    midi_clip_check_reset_bounds(state.clip);
    if (make_synth_note) {
	float *buf_L, *buf_R;
	int notes[] = {state.selected_note};
	int velocities[] = {state.insert_velocity};
	int32_t buf_len = synth_make_notes(state.cr->track->synth, notes, velocities, 1, &buf_L, &buf_R);
	session_queue_audio(2, buf_L, buf_R, buf_len, 0, true);
    }
    /* midi_clip_rectify_length(state.clip); */
    if (!state.chord_mode) {
	timeline_set_play_position(tl, note_tl_end, false);
    }
    return ret;
}

NEW_EVENT_FN(undo_redo_insert_rest, "undo insert rest")
     Timeline *tl = obj1;
     timeline_set_play_position(tl, val1.int32_v, false);
 }


void piano_roll_insert_rest()
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    int32_t dur = get_input_dur_samples();
    midi_clip_ungrab_all(state.clip);
    int32_t old_pos = tl->play_pos_sframes;
    timeline_set_play_position(tl, tl->play_pos_sframes + dur, false);
    int32_t clipref_end = state.cr->tl_pos + clipref_len(state.cr);
    int32_t add = tl->play_pos_sframes - clipref_end;
    if (add > 0) {
	state.clip->len_sframes += add;
	state.cr->end_in_clip = state.clip->len_sframes;
    }
    if (state.chord_mode) {
	piano_roll_toggle_chord_mode();
    }
    user_event_push(
	undo_redo_insert_rest,
	undo_redo_insert_rest,
	NULL, NULL,
	tl, NULL,
	(Value){.int32_v = old_pos},
	(Value){0},
	(Value){.int32_v = tl->play_pos_sframes},
	(Value){0},
	0, 0, false, false);
}

static Note *note_at_cursor(bool include_end)
{
    if (state.clip->num_notes == 0) return NULL;
    int32_t note_i = midi_clipref_check_get_last_note(state.cr);
    int32_t first_note = midi_clipref_check_get_first_note(state.cr);
    int32_t playhead = state.cr->track->tl->play_pos_sframes;
    while (note_i >= first_note) {
	Note *note = state.clip->notes + note_i;
	int32_t tl_start = note_tl_start_pos(note, state.cr);
	int32_t tl_end = note_tl_end_pos(note, state.cr);
	if (note->key == state.selected_note
	    && playhead >= tl_start
	    && (include_end ? playhead <= tl_end : playhead < tl_end)) {
	    return note;
	}
	note_i--;
    }
    return NULL;
}


/* NOTE GRAB INTERFACE */

/* Utility fns */

/* void grab_note(Note *note, NoteEdge edge) */
/* { */
/*     if (note->grabbed) { */
/* 	note->grabbed_edge = edge; */
/* 	return; */
/*     } */
/*     if (state.num_grabbed_notes == MAX_GRABBED_NOTES) { */
/* 	char msg[128]; */
/* 	snprintf(msg, 128, "Cannot grab more than %d notes", MAX_GRABBED_NOTES); */
/* 	status_set_errstr(msg); */
/* 	return; */
/*     } */
/*     note->grabbed = true; */
/*     note->grabbed_edge = edge; */
/*     state.grabbed_notes[state.num_grabbed_notes] = note; */
/*     state.num_grabbed_notes++; */
/* } */

/* void ungrab_all() */
/* { */
/*     for (int i=0; i<state.num_grabbed_notes; i++) { */
/* 	Note *note = state.grabbed_notes[i]; */
/* 	note->grabbed = false; */
/* 	note->grabbed_edge = NOTE_EDGE_NONE; */
/*     } */
/*     state.num_grabbed_notes = 0; */
/* } */
void piano_roll_grabbed_notes_move_vertical(int move_by)
{
    Session *session = session_get();
    if (session->dragging && state.clip->num_grabbed_notes > 0) {
	if (!session->playback.playing) {
	    midi_clipref_cache_grabbed_note_info(state.cr);
	}
	/* int notes_to_play[16]; */
	/* int velocities[16]; */
	/* int note_to_play_i = 0; */
	for (int32_t i=state.clip->first_grabbed_note; i<=state.clip->last_grabbed_note; i++) {
	    Note *note = state.clip->notes + i;
	    if (!note->grabbed) continue;
	    /* Note *note = state.clip->grabbed_notes[i]; */
	    if ((int)note->key + move_by < 0) note->key = 0;
	    else if ((int)note->key + move_by > 127) note->key = 127;
	    else note->key += move_by;
	    
	    /* Prepare note info to send to MIDI out */
	    /* if (note_to_play_i < 16) { */
	    /* 	notes_to_play[note_to_play_i] = note->key; */
	    /* 	velocities[note_to_play_i] = note->velocity; */
	    /* 	note_to_play_i++; */
	    /* } */

	}
	/* if (note_to_play_i > 0) { */
	/*     float *buf_L; */
	/*     float *buf_R; */
	/*     int32_t buf_len = synth_make_notes(state.cr->track->synth, notes_to_play, velocities, note_to_play_i, &buf_L, &buf_R); */
	/*     session_queue_audio(2, buf_L, buf_R, buf_len, 0, true); */
	/* } */

	if (!session->playback.playing) {
	    midi_clipref_push_grabbed_note_move_event(state.cr);
	}
    }
}

TEST_FN_DECL(check_note_order, MIDIClip *mclip);

void piano_roll_grabbed_notes_move(int32_t move_by)
{
    midi_clip_grabbed_notes_move(state.clip, move_by);
}

/* HIGH-LEVEL INTERFACE */
void piano_roll_grab_ungrab()
{
    Note *note = note_at_cursor(true);
    if (note && !note->grabbed) {
	/* grab_note(note, NOTE_EDGE_NONE); */
	midi_clip_grab_note(state.clip, note, NOTE_EDGE_NONE);
	/* Play all grabbed notes */
	int notes_to_play[16];
	int velocities[16];
	int note_to_play_i = 0;
	for (int32_t i=state.clip->first_grabbed_note; i<=state.clip->last_grabbed_note; i++) {
	    Note *note = state.clip->notes + i;
	    if (!note->grabbed) continue;
	    /* Prepare note info to send to MIDI out */
	    if (note_to_play_i < 16) {
		notes_to_play[note_to_play_i] = note->key;
		velocities[note_to_play_i] = note->velocity;
		note_to_play_i++;
	    }
	}
	if (note_to_play_i > 0) {
	    float *buf_L;
	    float *buf_R;
	    int32_t buf_len = synth_make_notes(state.cr->track->synth, notes_to_play, velocities, note_to_play_i, &buf_L, &buf_R);
	    session_queue_audio(2, buf_L, buf_R, buf_len, 0, true);
	}

	
	if (session_get()->dragging) {
	    midi_clipref_cache_grabbed_note_info(state.cr);
	}
    } else {
	if (session_get()->dragging) {
	    midi_clipref_push_grabbed_note_move_event(state.cr);
	}
	midi_clip_ungrab_all(state.clip);
    }
    if (session_get()->dragging) {
	status_stat_drag();
    }    
}

void piano_roll_grab_left_edge()
{
    Session *session = session_get();
    Note *note = note_at_cursor(true);
    if (!note) return;
    timeline_set_play_position(ACTIVE_TL, note_tl_start_pos(note, state.cr), false);
    midi_clip_grab_note(state.clip, note, NOTE_EDGE_LEFT);
    /* grab_note(note, NOTE_EDGE_RIGHT); */
    if (session_get()->dragging) {
	status_stat_drag();
    }
}

void piano_roll_grab_right_edge()
{
    Session *session = session_get();
    Note *note = note_at_cursor(true);
    if (!note) return;
    timeline_set_play_position(ACTIVE_TL, note_tl_end_pos(note, state.cr), false);
    midi_clip_grab_note(state.clip, note, NOTE_EDGE_RIGHT);
    /* grab_note(note, NOTE_EDGE_RIGHT); */
    if (session_get()->dragging) {
	status_stat_drag();
    }    
}

void piano_roll_grab_marked_range()
{
    Timeline *tl = state.cr->track->tl;
    midi_clipref_grab_range(state.cr, tl->in_mark_sframes, tl->out_mark_sframes);
    if (session_get()->dragging) {
	midi_clipref_cache_grabbed_note_info(state.cr);
    }
}


void piano_roll_delete()
{
    if (state.clip->num_grabbed_notes > 0) {
	midi_clipref_grabbed_notes_delete(state.cr);
    } else {
	Note **intersecting = NULL;
	int32_t play_pos_sframes = state.cr->track->tl->play_pos_sframes;
	int32_t start_pos = 0;
	int num_intersecting = midi_clipref_notes_ending_at_pos(state.cr, play_pos_sframes, &intersecting, true, &start_pos);
	for (int i=0; i<num_intersecting; i++) {
	    Note *note = intersecting[i];
	    midi_clip_grab_note(state.clip, note, NOTE_EDGE_NONE);
	}
	midi_clipref_grabbed_notes_delete(state.cr);
	if (num_intersecting > 0) {
	    timeline_set_play_position(state.cr->track->tl, start_pos, false);
	} else {
	    piano_roll_prev_note();
	}
	/* EventFn undo_options[2] = { */
	/*     undo_insert_note, */
	/*     undo_redo_insert_rest */
	/* }; */
	/* user_event_do_undo_selective(undo_options, 2); */
    }
}

void piano_roll_start_moving()
{
    if (session_get()->dragging && state.clip->num_grabbed_notes > 0)
	midi_clipref_cache_grabbed_note_info(state.cr);
}
void piano_roll_stop_moving()
{
    if (session_get()->dragging && state.clip->num_grabbed_notes > 0)
	midi_clipref_push_grabbed_note_move_event(state.cr);
}

void piano_roll_stop_dragging()
{
    midi_clipref_push_grabbed_note_move_event(state.cr);
}


void piano_roll_copy_grabbed_notes()
{
    MIDIClip *clip = state.cr->source_clip;
    if (state.clipboard.notes) free(state.clipboard.notes);
    state.clipboard.notes = malloc(clip->num_grabbed_notes * sizeof(Note));
    state.clipboard.num_notes = 0;
    int32_t first_note_start_rel = clip->notes[clip->first_grabbed_note].start_rel;
    for (int32_t i=clip->first_grabbed_note; i<=clip->last_grabbed_note; i++) {
	Note *note = clip->notes + i;
	if (note->grabbed) {
	    state.clipboard.notes[state.clipboard.num_notes] = *note;
	    state.clipboard.notes[state.clipboard.num_notes].start_rel = note->start_rel - first_note_start_rel;
	    state.clipboard.notes[state.clipboard.num_notes].end_rel = note->end_rel - first_note_start_rel;
	    state.clipboard.num_notes++;
	}
    }
}

NEW_EVENT_FN(undo_paste_grabbed_notes, "undo paste grabbed notes")
    ClipRef *cr = obj1;
    Note *pasted = obj2;
    int32_t num_pasted = val1.int32_v;
    midi_clip_remove_notes_by_id(cr->source_clip, pasted, num_pasted);

}

NEW_EVENT_FN(redo_paste_grabbed_notes, "redo paste grabbed notes")
    ClipRef *cr = obj1;
    Note *pasted = obj2;
    int32_t num_pasted = val1.int32_v;
    midi_clip_reinsert_notes(cr->source_clip, pasted, num_pasted);
}

void piano_roll_paste_grabbed_notes()
{
    if (!state.clipboard.notes) return;
    midi_clip_ungrab_all(state.cr->source_clip);
    Session *session = session_get();
    int32_t tl_pos_rel = ACTIVE_TL->play_pos_sframes - state.cr->tl_pos + state.cr->start_in_clip;
    Note *pasted_notes = malloc(state.clipboard.num_notes * sizeof(Note));
    for (int32_t i=0; i<state.clipboard.num_notes; i++) {
	Note *note = state.clipboard.notes + i;
	note = midi_clip_insert_note(state.cr->source_clip, note->channel, note->key, note->velocity, tl_pos_rel + note->start_rel, tl_pos_rel + note->end_rel);
	pasted_notes[i] = *note;
	midi_clip_grab_note(state.cr->source_clip, note, NOTE_EDGE_NONE);
    }

    

    user_event_push(
	undo_paste_grabbed_notes,
	redo_paste_grabbed_notes,
        NULL,
	NULL,
	state.cr,
	pasted_notes,
	(Value){.int32_v=state.clipboard.num_notes}, (Value){0},
	(Value){.int32_v=state.clipboard.num_notes}, (Value){0},
	0, 0,
	false, true);
}



/* INSERTIONS */

void piano_roll_toggle_tie()
{
    state.tie = !state.tie;
    if (state.tie) {
	textbox_set_background_color(state.gui.tie_button, &colors.green);
	/* Note *note = note_at_cursor(true); */
	/* if (note && note_tl_end_pos(note, state.cr) == state.cr->track->tl->play_pos_sframes) { */
	/*     midi_clip_grab_note(state.clip, note, NOTE_EDGE_RIGHT); */
	/* } */
    } else {
	textbox_set_background_color(state.gui.tie_button, &colors.grey);
	midi_clip_ungrab_all(state.clip);
    }
}

void piano_roll_toggle_chord_mode()
{
    state.chord_mode = !state.chord_mode;
    if (state.chord_mode) {
	textbox_set_background_color(state.gui.chord_button, &colors.solo_yellow);
    } else {
	textbox_set_background_color(state.gui.chord_button, &colors.grey);
    }
}


/* void piano_roll_quantize_notes_in_marked_range() */
/* { */
/*     if (!state.ct) { */
/* 	status_set_errstr("Add a click track (C-t in Timeline view) before quantizing"); */
/* 	return; */
/*     } */
/*     Note **intersecting; */
/*     Timeline *tl = state.cr->track->tl; */
/*     int num_intersecting = midi_clipref_notes_intersecting_area(state.cr, tl->in_mark_sframes, tl->out_mark_sframes, 0, 127, &intersecting); */
/*     for (int i=0; i<num_intersecting; i++) { */
/* 	int32_t tl_start = note_tl_start_pos(intersecting[i], state.cr); */
/* 	int32_t tl_end = note_tl_end_pos(intersecting[i], state.cr); */
/* 	int32_t prev_beat, next_beat; */
	
/* 	click_track_get_prox_beats(state.ct, tl_start, BP_SUBDIV, &prev_beat, &next_beat); */
/* 	int32_t diff_prev = tl_start - prev_beat; */
/* 	int32_t diff_next = next_beat - tl_start; */
/* 	if (diff_prev < diff_next) { */
/* 	    intersecting[i]->start_rel = prev_beat - state.cr->tl_pos + state.cr->start_in_clip; */
/* 	} else if (diff_next < diff_prev) { */
/* 	    intersecting[i]->start_rel = next_beat - state.cr->tl_pos + state.cr->start_in_clip;	     */
/* 	} */

/* 	click_track_get_prox_beats(state.ct, tl_end, BP_SUBDIV, &prev_beat, &next_beat); */
/* 	diff_prev = tl_end - prev_beat; */
/* 	diff_next = next_beat - tl_end; */
/* 	if (diff_prev < diff_next) { */
/* 	    intersecting[i]->end_rel = prev_beat - state.cr->tl_pos + state.cr->start_in_clip; */
/* 	} else if (diff_next < diff_prev) { */
/* 	    intersecting[i]->end_rel = next_beat - state.cr->tl_pos + state.cr->start_in_clip; */
/* 	} */
/* 	if (intersecting[i]->end_rel == intersecting[i]->start_rel) { */
/* 	    intersecting[i]->end_rel += (state.ct->segments + state.ct->current_segment)->cfg.atom_dur_approx; */
/* 	} */

/*     } */
/* } */

/* Draw fns */


static void piano_draw()
{
    Session *session = session_get();
    static bool lit_notes[88] = {0};
    static int32_t last_draw_tl_pos = INT32_MIN;
    if (ACTIVE_TL->play_pos_sframes != last_draw_tl_pos) {
	memset(lit_notes, 0, sizeof(lit_notes));
	Note **notes_at_cursor = NULL;
	int num_notes = midi_clipref_notes_intersecting_point(state.cr, ACTIVE_TL->play_pos_sframes, &notes_at_cursor);
	for (int i=0; i<num_notes; i++) {
	    lit_notes[notes_at_cursor[i]->key - PIANO_BOTTOM_NOTE] = true;
	}
	if (notes_at_cursor) free(notes_at_cursor);
    }
    last_draw_tl_pos = ACTIVE_TL->play_pos_sframes;

    SDL_SetRenderDrawColor(main_win->rend, 255, 255, 255, 255);
    SDL_RenderFillRect(main_win->rend, &state.piano_lt->rect);
    SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
    for (int i=0; i<state.piano_lt->num_children; i++) {
	Layout *lt = state.piano_lt->children[i];
	if (lt->name[1] == 'b') {
	    continue;
	} else {
	    int piano_note = 87 - i;
	    if (piano_note + PIANO_BOTTOM_NOTE == state.selected_note) {
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.solo_yellow));
		SDL_RenderFillRect(main_win->rend, &lt->rect);
		SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
	    } else if (lit_notes[piano_note]) {
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.cerulean));
		SDL_RenderFillRect(main_win->rend, &lt->rect);
		SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
	    }
	    SDL_RenderDrawLine(main_win->rend, lt->rect.x, lt->rect.y, lt->rect.x + lt->rect.w, lt->rect.y);
	}
    }
    for (int i=0; i<state.piano_lt->num_children; i++) {
	Layout *lt = state.piano_lt->children[i];
	SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
	if (lt->name[1] == 'b') {
	    int piano_note = 87 - i;
	    if (piano_note + PIANO_BOTTOM_NOTE == state.selected_note) {
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.solo_yellow));
		SDL_RenderFillRect(main_win->rend, &lt->rect);
		SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
		SDL_RenderDrawRect(main_win->rend, &lt->rect);
	    } else if (lit_notes[piano_note]) {
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.cerulean));
		SDL_RenderFillRect(main_win->rend, &lt->rect);
		SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
		SDL_RenderDrawRect(main_win->rend, &lt->rect);
	    } else {
		SDL_RenderFillRect(main_win->rend, &lt->rect);
	    }
	}
    }

}

static void piano_roll_draw_notes()
{
    Session *session = session_get();

    static const int midi_piano_range = 88;
    static ColorDiff grab_diff = {0};
    /* static SDL_Color grab_diff = {0}; */
    if (grab_diff.r == 0) {
	color_diff_set(&grab_diff, colors.midi_note_orange_grabbed, colors.midi_note_orange);
    }
    MIDIClip *mclip = state.clip;
    SDL_Rect rect = state.note_canvas_lt->rect;
    
    float note_height_nominal = (float)rect.h / midi_piano_range;
    float true_note_height = note_height_nominal;
    if (true_note_height < 1.0) true_note_height = 1.0;
    /* SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.midi_note_orange)); */
    pthread_mutex_lock(&mclip->notes_arr_lock);
    int32_t first_note = midi_clipref_check_get_first_note(state.cr);
    if (first_note < 0) {
	goto end_draw_notes;	
    }

    int32_t last_note = midi_clipref_check_get_last_note(state.cr);
    for (int32_t i=0; i<=last_note; i++) {
	Note *note = mclip->notes + i;
	/* SDL_SetRenderDrawColor(main_win->rend, colors.dark_brown.r, colors.dark_brown.g, colors.dark_brown.b, 255 * note->velocity / 128); */
	/* if (state.cr->end_in_clip && note->start_rel > state.cr->end_in_clip) break; */
	int piano_note = note->key - 20;
	if (piano_note < 0 || piano_note > midi_piano_range) continue;
	int32_t note_start = note->start_rel - state.cr->start_in_clip + state.cr->tl_pos;
	int32_t note_end = note->end_rel - state.cr->start_in_clip + state.cr->tl_pos;
	float x = timeview_get_draw_x(state.tl_tv, note_start);
	float w = timeview_get_draw_x(state.tl_tv, note_end) - x;
	float y = rect.y + round((float)(midi_piano_range - piano_note) * note_height_nominal);
	SDL_Rect note_rect = {x, y, w, true_note_height};
	if (note->grabbed && note->grabbed_edge == NOTE_EDGE_NONE) {
	    if (session->dragging) {
		SDL_Color pulse_color;
		color_diff_apply(&grab_diff, colors.midi_note_orange, session->drag_color_pulse_prop, &pulse_color);
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(pulse_color));
	    } else {
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.midi_note_orange_grabbed));
	    }
	} else {
	    if (i < first_note) {
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.grey));
	    } else {
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.midi_note_orange));
	    }
	}
	SDL_RenderFillRect(main_win->rend, &note_rect);
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.black));
	SDL_RenderDrawRect(main_win->rend, &note_rect);

	/* DRAW BUMPERS */
	
	static const int bumper_w = 10;
	static const int bumper_pad_h = 1;
	static const int bumper_pad_v = -4;
	static const int bumper_corner_r = 6;
	int bumper_gb = 0;
	if (session->dragging) {
	    bumper_gb = 200 * session->drag_color_pulse_prop;
	}
	if (note->grabbed_edge == NOTE_EDGE_LEFT) {
	    SDL_SetRenderDrawColor(main_win->rend, 255, bumper_gb, bumper_gb, 200);
	    SDL_Rect bumper = {note_rect.x + bumper_pad_h, note_rect.y + bumper_pad_v, bumper_w, note_rect.h - bumper_pad_v * 2};
	    geom_fill_rounded_rect(main_win->rend, &bumper, bumper_corner_r);	
	} else if (note->grabbed_edge == NOTE_EDGE_RIGHT) {
	    SDL_SetRenderDrawColor(main_win->rend, 255, bumper_gb, bumper_gb, 200);
	    SDL_Rect bumper = {note_rect.x + note_rect.w - bumper_pad_h - bumper_w, note_rect.y + bumper_pad_v, bumper_w, note_rect.h - bumper_pad_v * 2};
	    geom_fill_rounded_rect(main_win->rend, &bumper, bumper_corner_r);	
	}
    }
end_draw_notes:
    pthread_mutex_unlock(&mclip->notes_arr_lock);

    int sel_piano_note = state.selected_note - PIANO_BOTTOM_NOTE;
    for (int i=0; i<88; i++) {
	if (i % 12 == 1) SDL_SetRenderDrawColor(main_win->rend, 100, 100, 100, 100);
	else SDL_SetRenderDrawColor(main_win->rend, 100, 100, 100, 50);
	int y = state.note_canvas_lt->rect.y + round((double)i * (double)state.note_canvas_lt->rect.h / 88.0);
	SDL_RenderDrawLine(main_win->rend, state.note_canvas_lt->rect.x, y, state.note_canvas_lt->rect.x + state.note_canvas_lt->rect.w, y);
	if (88 - i - 1 == sel_piano_note) {
	    SDL_SetRenderDrawColor(main_win->rend, 255, 100, 0, 30);
	    SDL_Rect barrect = {state.note_canvas_lt->rect.x, y, state.note_canvas_lt->rect.w, note_height_nominal};
	    SDL_RenderFillRect(main_win->rend, &barrect);
	    /* SDL_SetRenderDrawColor(main_win->rend, 100, 100, 100, 50); */
	}
    }
    int playhead_x = timeview_get_draw_x(state.tl_tv, *state.tl_tv->play_pos);
    SDL_SetRenderDrawColor(main_win->rend, 255, 255, 255, 255);
    SDL_RenderDrawLine(main_win->rend, playhead_x, state.note_canvas_lt->rect.y, playhead_x, state.note_canvas_lt->rect.y + state.note_canvas_lt->rect.h);

}

#include "project_draw.h"
void piano_roll_draw()
{
    if (!state.clip) return;
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.control_bar_background_grey));
    /* SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.midi_note_orange)); */
    SDL_RenderFillRect(main_win->rend, &state.note_canvas_lt->rect);


    /* Draw click track */

    if (state.ct) {
	click_track_draw_segments(state.ct, state.tl_tv, state.layout->rect);
    }
    
    
    piano_roll_draw_notes();
    piano_draw();
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.sea_green));

    SDL_RenderDrawRect(main_win->rend, &state.note_piano_container->rect);

    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.tl_background_grey));
    SDL_RenderFillRect(main_win->rend, &state.console_lt->rect);

    static const SDL_Color marked_background = {255, 255, 255, 6};
    timeline_draw_marks(state.cr->track->tl, 0, colors.white, marked_background);

    /* Draw console */
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.control_bar_background_grey));
    SDL_RenderFillRect(main_win->rend, state.gui.info_panel);
    SDL_RenderFillRect(main_win->rend, state.gui.input_panel);

    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.grey));
    SDL_RenderDrawRect(main_win->rend, state.gui.info_panel);
    SDL_RenderDrawRect(main_win->rend, state.gui.input_panel);
    
    textbox_draw(state.gui.track_name);
    textbox_draw(state.gui.solo_button);
    textbox_draw(state.gui.clip_label);
    textbox_draw(state.gui.clip_name);
    textbox_draw(state.gui.in_label);
    textbox_draw(state.gui.in_name);
    textbox_draw(state.gui.dur_tb);
    textbox_draw(state.gui.dur_longer_button);
    textbox_draw(state.gui.dur_shorter_button);
    textbox_draw(state.gui.tie_button);
    textbox_draw(state.gui.chord_button);

    if (state.mouse_sel_rect_active) {
	SDL_SetRenderDrawColor(main_win->rend, 200, 200, 255, 30);
	SDL_RenderFillRect(main_win->rend, &state.mouse_sel_rect);
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.grey));
	SDL_RenderDrawRect(main_win->rend, &state.mouse_sel_rect);
    }

    Session *session = session_get();
    if (session->midi_qwerty) {
	Page *midi_qwerty = panel_area_get_page_by_title(session->gui.panels, "QWERTY piano");
	page_draw(midi_qwerty);
    }
}


/* Getters */

Textbox *piano_roll_get_solo_button()
{
    if (state.active)
	return state.gui.solo_button;
    else return NULL;
}

int piano_roll_get_num_grabbed_notes()
{
    return state.clip->num_grabbed_notes;
}




/* Mouse functions */

static void reset_mouse_sel_rect(SDL_Point mousep)
{
    SDL_Point true_orig = state.mouse_sel_rect_origin;
    int w;
    int h;
    if (mousep.x < true_orig.x) {
	w = true_orig.x - mousep.x;
	true_orig.x = mousep.x;
    } else {
	w = mousep.x - true_orig.x;
    }
    if (mousep.y < true_orig.y) {
	h = true_orig.y - mousep.y;
	true_orig.y = mousep.y;
    } else {
	h = mousep.y - true_orig.y;
    }
    state.mouse_sel_rect = (SDL_Rect){true_orig.x, true_orig.y, w, h};
}

static bool mouse_motion_occurred = false;
void piano_roll_mouse_click(SDL_Point mousep)
{
    state.mouse_sel_rect_active = true;
    state.mouse_sel_rect_origin = mousep;
    reset_mouse_sel_rect(mousep);
    mouse_motion_occurred = false;
}

void piano_roll_mouse_motion(SDL_Point mousep)
{
    if (main_win->i_state & I_STATE_MOUSE_L) {
	reset_mouse_sel_rect(mousep);
    }
    mouse_motion_occurred = true;
}

void piano_roll_mouse_up(SDL_Point mousep)
{
    if (!mouse_motion_occurred) {
	timeline_set_play_position(state.cr->track->tl, timeview_get_pos_sframes(state.tl_tv, mousep.x), false);
	return;
    }
    mouse_motion_occurred = false;
    if (state.clip->num_grabbed_notes > 0 && session_get()->dragging) {
	midi_clipref_push_grabbed_note_move_event(state.cr);
    }
    midi_clip_ungrab_all(state.clip);
    state.mouse_sel_rect_active = false;
    int sel_piano_note_top = 88 - 88 * (state.mouse_sel_rect.y - state.layout->rect.y) / state.layout->rect.h;
    int sel_piano_note_bottom = 88 - 88 * (state.mouse_sel_rect.y + state.mouse_sel_rect.h - state.layout->rect.y) / state.layout->rect.h;
    midi_clipref_grab_area(
	state.cr,
	timeview_get_pos_sframes(state.tl_tv, state.mouse_sel_rect.x),
	timeview_get_pos_sframes(state.tl_tv, state.mouse_sel_rect.x + state.mouse_sel_rect.w),
	sel_piano_note_bottom + 20,
	sel_piano_note_top + 20);
    if (session_get()->dragging) {
	midi_clipref_cache_grabbed_note_info(state.cr);
    }


}



/* MIDI Device input */

/* called in project_loop.c on main thread. returns number executed to prevent idling */
int piano_roll_execute_queued_insertions()
{
    int err = pthread_mutex_lock(&state.event_queue_lock);
    if (err != 0) {
	fprintf(stderr, "Error locking piano roll event queue lock from main thread: %s\n", strerror(err));
    }
    static int32_t last_insert_ts = INT32_MIN;
    static int32_t last_insert_note_id = -1;
    static int32_t last_insert_tl_pos = 0;

    if (state.num_queued_events !=0 ){
	/* fprintf(stderr, "\n"); */
	/* fprintf(stderr, "\nDEQUEUEING %d events\n", state.num_queued_events); */
    }
    int saved_vel = state.insert_velocity;
    for (int i=0; i<state.num_queued_events; i++) {
	/* PmEvent e = s->device.buffer[i]; */
	PmEvent e = state.event_queue[i];
	uint8_t status = Pm_MessageStatus(e.message);
	uint8_t note_val = Pm_MessageData1(e.message);
	uint8_t velocity = Pm_MessageData2(e.message);
	uint8_t msg_type = status >> 4;
        if (msg_type == 9) {
	    /* fprintf(stderr, "\t%d: type %d note %d vel %d\tchord diff %d, last insert id: %d", i, msg_type, note_val, velocity, e.timestamp - last_insert_ts, last_insert_note_id); */
	    state.selected_note = note_val;
	    
	    state.insert_velocity = velocity;
	    if (e.timestamp - last_insert_ts < CHORD_THRESHOLD_MSEC && last_insert_note_id >= 0) {
		/* fprintf(stderr, "\tCHORD time diff: %d", e.timestamp - last_insert_ts); */
		/* int32_t last_insert_note_i = midi_clip_get_note_by_id(state.clip, last_insert_note_id); */
	        /* if (last_insert_note_i < 0) { */
		    /* fprintf(stderr, "Error forming chord: could not find note by id %d\n", last_insert_note_id); */
		/* } else { */
		    /* timeline_set_play_position(state.cr->track->tl, note_tl_start_pos(state.clip->notes + last_insert_note_i, state.cr), false); */
		/* fprintf(stderr, "\tGOTO %d", last_insert_tl_pos); */
		timeline_set_play_position(state.cr->track->tl, last_insert_tl_pos, false);
		/* } */
	    }
	    last_insert_tl_pos = state.cr->track->tl->play_pos_sframes;
	    /* fprintf(stderr, "\tINSERT %d, SET last tl pos %d\n", state.selected_note, last_insert_tl_pos); */
	    Note *new = piano_roll_insert_note(false);
	    last_insert_ts = e.timestamp;
	    if (new) {
		last_insert_note_id = new->id;
		/* fprintf(stderr, "SET LAST insert id %d\n", new->id); */
	    }
	} else {
	    /* fprintf(stderr, "\n"); */
	}
    }
    state.insert_velocity = saved_vel;
    int ret = state.num_queued_events;
    state.num_queued_events = 0;
    pthread_mutex_unlock(&state.event_queue_lock);
    return ret;
}

/* Called on audio thread */
void piano_roll_feed_midi(const PmEvent *events, int num_events)
{
    if (num_events == 0) return;
    int err = pthread_mutex_lock(&state.event_queue_lock);
    if (err != 0) {
	fprintf(stderr, "Error locking piano roll event queue lock from audio thread: %s\n", strerror(err));
    }
    int buf_rem = MAX_QUEUED_EVENTS - state.num_queued_events;
    if (num_events > buf_rem) {
	fprintf(stderr, "Error: %d spaces left in event buf (%d requested)\n", buf_rem, num_events);
	num_events = buf_rem;
    }
    memcpy(state.event_queue + state.num_queued_events, events, num_events * sizeof(PmEvent));
    state.num_queued_events += num_events;
    pthread_mutex_unlock(&state.event_queue_lock);
}


