#include "midi_clip.h"

void midi_clip_destroy(MIDIClip *mc)
{


}

void midi_clip_add_note(MIDIClip *mc, int note_val, int velocity, int32_t start_rel, int32_t end_rel)
{
    if (!mc->notes) {
	mc->notes_alloc_len = 32;
	mc->notes = calloc(mc->notes_alloc_len, sizeof(Note));
    }
    if (mc->num_notes == mc->notes_alloc_len) {
	mc->notes_alloc_len *= 2;
	mc->notes = realloc(mc->notes, mc->notes_alloc_len * sizeof(Note));
    }
    Note *note = mc->notes + mc->num_notes;
    note->note = note_val;
    note->velocity = velocity;
    note->start_rel = start_rel;
    note->end_rel = end_rel;
    mc->num_notes++;
}
