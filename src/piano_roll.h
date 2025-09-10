#ifndef JDAW_PIANO_ROLL_H
#define JDAW_PIANO_ROLL_H

#include "midi_clip.h"
#include "midi_objs.h"
#include "tempo.h"
#include "timeview.h"

#define MAX_GRABBED_NOTES 128

enum note_dur {
    WHOLE,
    HALF,
    QUARTER,
    EIGHTH,
    SIXTEENTH,
    THIRTY_SECOND
};

void piano_roll_draw();
void piano_roll_activate(ClipRef *cr);
void piano_roll_zoom_in();
void piano_roll_zoom_out();
void piano_roll_move_view_left();
void piano_roll_move_view_right();

#endif


