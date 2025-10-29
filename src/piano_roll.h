#ifndef JDAW_PIANO_ROLL_H
#define JDAW_PIANO_ROLL_H

#include "midi_clip.h"

void piano_roll_deactivate();
void piano_roll_draw();
void piano_roll_activate(ClipRef *cr);
void piano_roll_zoom_in();
void piano_roll_zoom_out();
void piano_roll_move_view_left();
void piano_roll_move_view_right();
/* void piano_roll_note_up(); */
/* void piano_roll_note_down(); */
void piano_roll_move_note_selector(int by);
void piano_roll_forward_dur();
void piano_roll_back_dur();
void piano_roll_next_note();
void piano_roll_prev_note();
void piano_roll_up_note();
void piano_roll_down_note();
void piano_roll_dur_longer();
void piano_roll_dur_shorter();
Note *piano_roll_insert_note();
void piano_roll_insert_rest();

void piano_roll_grab_ungrab();
void piano_roll_grab_left_edge();
void piano_roll_grab_right_edge();
void piano_roll_grab_marked_range();
void piano_roll_delete();
/* void piano_roll_delete_grabbed_notes(); */
void piano_roll_stop_moving();
void piano_roll_start_moving();
void piano_roll_stop_dragging();

void piano_roll_grabbed_notes_move(int32_t move_by_sframes);

void piano_roll_toggle_tie();
void piano_roll_toggle_chord_mode();

/* externalize state */
Textbox *piano_roll_get_solo_button();
int piano_roll_get_num_grabbed_notes();

void piano_roll_mouse_click(SDL_Point mousep);
void piano_roll_mouse_motion(SDL_Point mousep);
void piano_roll_mouse_up(SDL_Point mousep);


/* MIDI I/O */
void piano_roll_feed_midi(const PmEvent *events, int num_events);
int piano_roll_execute_queued_insertions();

#endif


