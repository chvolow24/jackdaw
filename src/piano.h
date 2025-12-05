/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    piano.h

    * drawable 18-key piano keyboard
    * includes labels for QWERTY piano emulator (see midi_qwerty.h)

*****************************************************************************************************************/

#ifndef JDAW_PIANO_H
#define JDAW_PIANO_H

#include "layout.h"
#include "textbox.h"

#define PIANO_NUM_KEYS 18

typedef struct piano {
    Layout *container;
    Layout *layout;
    Textbox *key_labels[PIANO_NUM_KEYS];
    SDL_Rect *key_rects[PIANO_NUM_KEYS];
} Piano;

typedef struct piano_area {
    Layout *container;
    Layout *layout;
    Piano piano;
} PianoArea;

/* Initializes members, allocates textboxes */
void piano_init(Piano *piano, Layout *container);

/* Allocates space for piano and inits */
Piano *piano_create(Layout *container);
void piano_destroy(Piano *piano);

void piano_draw(Piano *piano);

void piano_reset(Piano *piano);

#endif
