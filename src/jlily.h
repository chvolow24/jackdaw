/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    jlily.h

    * Parse "jlily" (small subset of LilyPond input language) as MIDI notes and
      insert those notes into a MIDI clip
    * Read more about LilyPond at https://lilypond.org/

*****************************************************************************************************************/


/* Supported syntax:

   - note names
       e.g.        "c g g a g"
   - duration
       e.g.        "c4 g8 g a g"
   - dotted duration
       e.g.        "c4 g8. g16 a4 g"
   - accidentals
       e.g.        "c4 g8. g16 aes4 g"
   - rests
       e.g.        "c4 g8. g16 aes4 g r b c"
   - octave indicators
       e.g.        "c4 g8. g16 aes4 g r b c c,"
   - repeats
       e.g.        "\repeat 4 { c16, c' }"
                    (repeat ^ times)
   - nested repeats
       e.g.        "\repeat 16 { \repeat 4 {c, c'} \repeat 4 {g, g'} }"
   - set velocity (JLILY ONLY, not standard lilypond)
       e.g.        "\velocity 100 c c e g \velocity 90 g e c"

*/

#ifndef JDAW_JLILY_H
#define JDAW_JLILY_H

#include <stdint.h>
#include "midi_clip.h"

int jlily_string_to_mclip(
    const char *str,
    double beat_dur_sframes_loc,
    int32_t pos_offset,
    MIDIClip *mclip);

void timeline_add_jlily();

#endif
