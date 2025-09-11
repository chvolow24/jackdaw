#include "assets.h"
#include "clipref.h"
#include "color.h"
#include "layout_xml.h"
#include "midi_clip.h"
#include "piano_roll.h"
#include "project.h"
#include "session.h"
#include "timeline.h"
#include "timeview.h"


extern Window *main_win;
extern struct colors colors;

#define PIANO_TOP_NOTE 108
#define PIANO_BOTTOM_NOTE 21

struct piano_roll_state {
    bool active;
    /* TimeView tv; */
    TimeView *tl_tv;
    MIDIClip *clip;
    ClipRef *cr;
    ClickTrack *ct;
    Note *grabbed_notes[MAX_GRABBED_NOTES];    
    enum note_dur current_dur;
    int num_dots;
    Layout *layout;
    Layout *note_piano_container;
    Layout *note_canvas_lt;
    Layout *piano_lt;
    Layout *console_lt;
    int selected_note; /* midi value */
};

struct piano_roll_state state;


/* void piano_roll_load_layout() */
/* { */
/*     Layout *main_lt = layout_read_from_xml(PIANO_ROLL_LT_PATH); */
/*     state.layout = main_lt; */
/*     Layout *piano_lt = layout_get_child_by_name_recursive(main_lt, "piano"); */
/*     state.piano_lt = piano_lt; */
/*     layout_read_xml_to_lt(main_lt, PIANO_88_VERTICAL_LT_PATH); */

/* } */

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
    SDL_Rect rect = state.note_canvas_lt->rect;
    
    float note_height_nominal = (float)rect.h / midi_piano_range;
    float true_note_height = note_height_nominal;
    if (true_note_height < 1.0) true_note_height = 1.0;
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.midi_clip_pink));
    pthread_mutex_lock(&mclip->notes_arr_lock);
    int32_t first_note = midi_clipref_check_get_first_note(state.cr);
    if (first_note < 0) return;
	
    for (int32_t i=first_note; i<mclip->num_notes; i++) {
	Note *note = mclip->notes + i;
	/* SDL_SetRenderDrawColor(main_win->rend, colors.dark_brown.r, colors.dark_brown.g, colors.dark_brown.b, 255 * note->velocity / 128); */
	if (state.cr->end_in_clip && note->start_rel > state.cr->end_in_clip) break;
	int piano_note = note->key - 20;
	if (piano_note < 0 || piano_note > midi_piano_range) continue;
	int32_t note_start = note->start_rel - state.cr->start_in_clip + state.cr->tl_pos;
	int32_t note_end = note->end_rel - state.cr->start_in_clip + state.cr->tl_pos;
	float x = timeview_get_draw_x(state.tl_tv, note_start);
	float w = timeview_get_draw_x(state.tl_tv, note_end) - x;
	float y = rect.y + round((float)(midi_piano_range - piano_note) * note_height_nominal);
	SDL_Rect note_rect = {x, y, w, true_note_height};
	SDL_RenderFillRect(main_win->rend, &note_rect);
    }
    pthread_mutex_unlock(&mclip->notes_arr_lock);

    SDL_SetRenderDrawColor(main_win->rend, 100, 100, 100, 50);
    int sel_piano_note = state.selected_note - PIANO_BOTTOM_NOTE;
    for (int i=0; i<88; i++) {
	int y = state.note_canvas_lt->rect.y + round((double)i * (double)state.note_canvas_lt->rect.h / 88.0);
	SDL_RenderDrawLine(main_win->rend, state.note_canvas_lt->rect.x, y, state.note_canvas_lt->rect.x + state.note_canvas_lt->rect.w, y);
	if (88 - i - 1 == sel_piano_note) {
	    SDL_SetRenderDrawColor(main_win->rend, 255, 100, 0, 30);
	    SDL_Rect barrect = {state.note_canvas_lt->rect.x, y, state.note_canvas_lt->rect.w, note_height_nominal};
	    SDL_RenderFillRect(main_win->rend, &barrect);
	    SDL_SetRenderDrawColor(main_win->rend, 100, 100, 100, 50);
	}
    }
    int playhead_x = timeview_get_draw_x(state.tl_tv, *state.tl_tv->play_pos);
    SDL_SetRenderDrawColor(main_win->rend, 255, 255, 255, 255);
    SDL_RenderDrawLine(main_win->rend, playhead_x, state.note_canvas_lt->rect.y, playhead_x, state.note_canvas_lt->rect.y + state.note_canvas_lt->rect.h);

}

void piano_roll_draw()
{
    if (!state.clip) return;
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.control_bar_background_grey));
    /* SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.midi_clip_pink)); */
    SDL_RenderFillRect(main_win->rend, &state.note_canvas_lt->rect);
    piano_roll_draw_notes();
    piano_draw();
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.sea_green));

    SDL_RenderDrawRect(main_win->rend, &state.note_piano_container->rect);


    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.tl_background_grey));
    SDL_RenderFillRect(main_win->rend, &state.console_lt->rect);
    /* geom_draw_rect_thick(main_win->rend, &state.note_piano_container->rect, 2, main_win->dpi_scale_factor); */
}

static void piano_roll_init_layout(Session *session)
{
    if (state.layout) return; // Already init'd
    Layout *audio_rect = layout_get_child_by_name_recursive(session->gui.layout, "audio_rect");
    Layout *lt = layout_add_child(session->gui.layout);
    layout_read_xml_to_lt(lt, PIANO_ROLL_LT_PATH);
    Layout *piano_container = layout_get_child_by_name_recursive(lt, "piano");
    Layout *piano_lt = layout_read_xml_to_lt(piano_container, PIANO_88_VERTICAL_LT_PATH);
    state.piano_lt = piano_lt;
    Layout *note_canvas = layout_get_child_by_name_recursive(lt, "note_canvas");
    state.note_canvas_lt = note_canvas;
    state.note_piano_container = layout_get_child_by_name_recursive(lt, "note_piano_container");
    /* lt->x.value = audio_rect->rect.x / main_win->dpi_scale_factor; */
    lt->y.value = audio_rect->rect.y / main_win->dpi_scale_factor;;
    lt->w.type = REVREL;
    lt->w.value = 0; // hug the right edge
    lt->h.type = REVREL;
    lt->h.value = session->status_bar.layout->h.value; // hug the top edge of status
    /* state.piano_lt = layout_add_child(lt); */
    /* state.piano_lt->h.type = SCALE; */
    /* state.piano_lt->h.value = 1.0; */
    /* state.piano_lt->x.type = REVREL; */
    /* state.piano_lt->x.value = 0.0; */
    /* layout_read_xml_to_lt(state.piano_lt, PIANO_88_VERTICAL_LT_PATH); */
    layout_force_reset(lt);
    state.layout = lt;
    state.console_lt = layout_get_child_by_name_recursive(lt, "piano_roll_console");
    /* timeview_init(state.tl_tv, &note_canvas->rect, 600, 0, &session->proj.timelines[0]->play_pos_sframes, NULL, NULL); */
}
void piano_roll_activate(ClipRef *cr)
{
    Session *session = session_get();
    window_push_mode(main_win, MODE_PIANO_ROLL);
    session->piano_roll = true;
    state.cr = cr;
    state.clip = cr->source_clip;
    state.tl_tv = &ACTIVE_TL->timeview;
    state.selected_note = 60;
    if (!state.layout) piano_roll_init_layout(session_get());
}
void piano_roll_deactivate()
{
    state.clip = NULL;
    InputMode im = window_pop_mode(main_win);
    if (im != MODE_PIANO_ROLL) {
	fprintf(stderr, "Error: deactivating piano roll, top mode %s\n", input_mode_str(im));
    }
    Session *session = session_get();
    session->piano_roll = false;
    timeline_reset_full(ACTIVE_TL);
    
}

void piano_roll_zoom_in()
{
    timeview_rescale(state.tl_tv, 1.2, false, (SDL_Point){0});
}
void piano_roll_zoom_out()
{
    timeview_rescale(state.tl_tv, 0.8, false, (SDL_Point){0});
}
void piano_roll_move_view_left()
{
    timeview_scroll_horiz(state.tl_tv, -60);
}
void piano_roll_move_view_right()
{
    timeview_scroll_horiz(state.tl_tv, 60);
}

void piano_roll_note_up()
{
    state.selected_note++;
    if (state.selected_note > PIANO_TOP_NOTE) state.selected_note = PIANO_TOP_NOTE;
}
void piano_roll_note_down()
{
    state.selected_note--;
    if (state.selected_note < PIANO_BOTTOM_NOTE) state.selected_note = PIANO_BOTTOM_NOTE;
}

void piano_roll_next_note()
{
    Session *session = session_get();
    int32_t pos = ACTIVE_TL->play_pos_sframes;
    Note *note = midi_clipref_get_next_note(state.cr, pos, &pos);
    if (note) {
	timeline_set_play_position(ACTIVE_TL, pos);
	state.selected_note = note->key;
    }
}

void piano_roll_prev_note()
{
    Session *session = session_get();
    int32_t pos = ACTIVE_TL->play_pos_sframes;
    Note *note = midi_clipref_get_prev_note(state.cr, pos, &pos);
    if (note) {
	timeline_set_play_position(ACTIVE_TL, pos);
	state.selected_note = note->key;
    }
}

