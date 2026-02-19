#ifndef JDAW_MIDI_FILE_H
#define JDAW_MIDI_FILE_H

#include <stdbool.h>
#include <stdio.h>
#include "portmidi.h"
/* int midi_file_read(const char *filepath); */

/* int midi_file_open(const char *filepath);//, MIDIClip **mclips) */
int midi_file_open(const char *filepath, bool automatically_add_tracks);


/* The below functions have nothing to do with standard MIDI file parsing;
   these are for writing MIDI data to .jdaw files. */

/* Writes num_events before writing events*/
void midi_serialize_events(FILE *f, PmEvent *events, int num_events);

/* Gets and returns num events; allocates array of events */
uint32_t midi_deserialize_events(FILE *f, PmEvent **events);
#endif
