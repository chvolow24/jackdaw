#include "assets.h"
#include "clipref.h"
#include "color.h"
#include "layout_xml.h"
#include "piano_roll.h"
#include "project.h"
#include "session.h"

extern Window *main_win;
extern struct colors colors;

struct piano_roll_state {
    bool active;
    TimeView tv;
    MIDIClip *clip;
    ClipRef *cr;
    ClickTrack *ct;
    Note *grabbed_notes[MAX_GRABBED_NOTES];    
    enum note_dur current_dur;
    int num_dots;
    Layout *layout;
    Layout *piano_lt;
};

struct piano_roll_state state;


void piano_roll_load_layout()
{
    Layout *main_lt = layout_read_from_xml(PIANO_ROLL_LT_PATH);
    state.layout = main_lt;
    Layout *piano_lt = layout_get_child_by_name_recursive(main_lt, "piano");
    state.piano_lt = piano_lt;
    layout_read_xml_to_lt(main_lt, PIANO_88_VERTICAL_LT_PATH);
}

static void piano_draw()
{
    SDL_SetRenderDrawColor(main_win->rend, 255, 255, 255, 255);
    SDL_RenderFillRect(main_win->rend, &state.piano_lt->rect);
    SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
    for (int i=0; i<state.piano_lt->num_children; i++) {
	Layout *lt = state.piano_lt->children[i];
	if (lt->name[1] == 'b') {
	    SDL_RenderFillRect(main_win->rend, &lt->rect);
	} else {
	    SDL_RenderDrawLine(main_win->rend, lt->rect.x, lt->rect.y, lt->rect.x + lt->rect.w, lt->rect.y);
	}
    }
}

static void piano_roll_draw_notes()
{
    static const int midi_piano_range = 88;
    MIDIClip *mclip = state.clip;
    SDL_Rect rect = *state.tv.rect;
    
    float note_height_nominal = (float)rect.h / midi_piano_range;
    float true_note_height = note_height_nominal;
    if (true_note_height < 1.0) true_note_height = 1.0;
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.dark_brown));
    pthread_mutex_lock(&mclip->notes_arr_lock);
    int32_t first_note = midi_clipref_check_get_first_note(state.cr);
    if (first_note < 0) return;
	
    for (int32_t i=first_note; i<mclip->num_notes; i++) {
	Note *note = mclip->notes + i;
	/* SDL_SetRenderDrawColor(main_win->rend, colors.dark_brown.r, colors.dark_brown.g, colors.dark_brown.b, 255 * note->velocity / 128); */
	if (state.cr->end_in_clip && note->start_rel > state.cr->end_in_clip) break;
	int piano_note = note->key - 21;
	if (piano_note < 0 || piano_note > midi_piano_range) continue;
	int32_t note_start = note->start_rel - state.cr->start_in_clip + state.cr->tl_pos;
	int32_t note_end = note->end_rel - state.cr->start_in_clip + state.cr->tl_pos;
	float x = timeview_get_draw_x(&state.tv, note_start);
	float w = timeview_get_draw_x(&state.tv, note_end) - x;
	float y = rect.y + (midi_piano_range - piano_note) * note_height_nominal;
	SDL_Rect note_rect = {x, y - true_note_height / 2, w, true_note_height};
	/* fprintf(stderr, "\t->start->end: %d-%d\n", note_start, note_end); */
	/* fprintf(stderr, "\t->(rel): %d-%d\n", note->start_rel, note->end_rel); */
	SDL_RenderFillRect(main_win->rend, &note_rect);
    }
    pthread_mutex_lock(&mclip->notes_arr_lock);


}

void piano_roll_draw()
{
    static const int midi_piano_range = 88;
    if (!state.clip) return;

    piano_roll_draw_notes();
    piano_draw();
}

void piano_roll_init(Session *session)
{
    Layout *audio_rect = layout_get_child_by_name_recursive(session->gui.layout, "audio_rect");
    Layout *lt = layout_add_child(session->gui.layout);
    layout_set_name(lt, "piano_roll");
    lt->x.value = audio_rect->x.value;
    lt->y.value = audio_rect->y.value;
    
}
void piano_roll_activate(ClipRef *cr)
{
    state.cr = cr;
}
