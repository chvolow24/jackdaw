#include "clipref.h"
#include "midi_clip.h"
#include "test.h"
#include "session.h"

#define DEFAULT_NOTES_ALLOC_LEN 16
#define DEFAULT_REFS_ALLOC_LEN 2

MIDIClip *midi_clip_create(MIDIDevice *device, Track *target)
{
    Session *session = session_get();
    if (session->proj.num_midi_clips == MAX_PROJ_MIDI_CLIPS) {
	return NULL;
    }
    MIDIClip *mclip = calloc(1, sizeof(MIDIClip));
    if (device) {
	mclip->recorded_from = device;
    }
    mclip->notes_alloc_len = DEFAULT_NOTES_ALLOC_LEN;
    mclip->notes = calloc(mclip->notes_alloc_len, sizeof(Note));

    mclip->refs_alloc_len = DEFAULT_REFS_ALLOC_LEN;
    mclip->refs = calloc(mclip->refs_alloc_len, sizeof(ClipRef *));
    

    if (!target && device) {
	snprintf(mclip->name, sizeof(mclip->name), "%s_rec_%d", device->info->name, session->proj.num_clips); /* TODO: Fix this */
    } else if (target) {
	snprintf(mclip->name, sizeof(mclip->name), "%s take %d", target->name, target->num_takes + 1);
	target->num_takes++;
    } else {
	snprintf(mclip->name, sizeof(mclip->name), "anonymous");
    }

    session->proj.midi_clips[session->proj.num_midi_clips] = mclip;
    session->proj.num_midi_clips++;
    fprintf(stderr, "IN CREATE num midi clips = %d (ADDR VALUE: %p)\n", session->proj.num_midi_clips, &session->proj.num_midi_clips);
    return mclip;

}

void midi_clip_destroy(MIDIClip *mc)
{


}

TEST_FN_DEF(check_note_order, {
	int32_t start_rel = 0;
	for (int i=0; i<mclip->num_notes; i++) {
	    Note *note = mclip->notes + i;
	    if (note->start_rel < start_rel) {
		fprintf(stderr, "Error: MIDI clip note order error at indices %d,%d, clip %s\n", i-1, i, mclip->name);
		return 1;
	    }
	    /* fprintf(stderr, "NOTE: %d/%d, note %d, velocity %d, start: %d\n", i, mclip->num_notes, note->note, note->velocity, note->end_rel); */
	    start_rel = note->start_rel;
	}
	return 0;
    }, MIDIClip *mclip);

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
    while (note > mc->notes && start_rel < (note - 1)->start_rel) {
	*note = *(note - 1);
	note--;
    }
    note->note = note_val;
    note->velocity = velocity;
    note->start_rel = start_rel;
    note->end_rel = end_rel;
    mc->num_notes++;
    fprintf(stderr, "NOTE ADDED %d num notes: %d, start rel: %d end rel: %d\n",note_val, mc->num_notes, start_rel, end_rel);
    TEST_FN_CALL(check_note_order, mc);
}

/* Return -1 if clip has no notes; otherwise, first note index */
int32_t midi_clipref_check_get_first_note(ClipRef *cr)
{
    MIDIClip *clip = cr->source_clip;

    if (cr->first_note == -1) cr->first_note = 0;
    while (cr->first_note > 0 && clip->notes[cr->first_note].start_rel > cr->start_in_clip) {
	cr->first_note--;
    }
    while (cr->first_note < clip->num_notes && clip->notes[cr->first_note].start_rel < cr->start_in_clip) {
	cr->first_note++;
    }
    if (cr->first_note == clip->num_notes) cr->first_note = -1;
    return cr->first_note;
}
