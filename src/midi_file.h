#ifndef JDAW_MIDI_FILE_H
#define JDAW_MIDI_FILE_H

#include <stdbool.h>
/* int midi_file_read(const char *filepath); */

/* int midi_file_open(const char *filepath);//, MIDIClip **mclips) */
int midi_file_open(const char *filepath, bool automatically_add_tracks);

#endif
