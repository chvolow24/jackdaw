#ifndef JDAW_PIANO_ROLL_H
#define JDAW_PIANO_ROLL_H

#include "midi_clip.h"
#include "midi_objs.h"
#include "tempo.h"
#include "timeview.h"


void piano_roll_deactivate();
void piano_roll_draw();
void piano_roll_activate(ClipRef *cr);
void piano_roll_zoom_in();
void piano_roll_zoom_out();
void piano_roll_move_view_left();
void piano_roll_move_view_right();
void piano_roll_note_up();
void piano_roll_note_down();
void piano_roll_next_note();
void piano_roll_prev_note();
void piano_roll_dur_up();
void piano_roll_dur_down();

#endif


