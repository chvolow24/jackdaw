#ifndef JDAW_MIDI_FILE_H
#define JDAW_MIDI_FILE_H

#include "midi_clip.h"

/* int midi_file_read(const char *filepath); */
int midi_file_read(const char *filepath, MIDIClip **mclips);

#endif
