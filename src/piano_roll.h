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
void piano_roll_next_note();
void piano_roll_prev_note();
void piano_roll_dur_longer();
void piano_roll_dur_shorter();
void piano_roll_insert_note();


/* externalize state */
Textbox *piano_roll_get_solo_button();

#endif


