#include "assets.h"
#include "clipref.h"
#include "color.h"
#include "layout.h"
#include "layout_xml.h"
#include "midi_clip.h"
#include "piano_roll.h"
#include "project.h"
#include "session.h"
#include "textbox.h"
#include "timeline.h"
#include "timeview.h"


extern Window *main_win;
extern struct colors colors;

#define PIANO_TOP_NOTE 108
#define PIANO_BOTTOM_NOTE 21
#define MAX_GRABBED_NOTES 128
#define MAX_TIED_NOTES 24

enum note_dur {
    WHOLE,
    HALF,
    QUARTER,
    EIGHTH,
    SIXTEENTH,
    THIRTY_SECOND,
    SIXTY_FOURTH,
};
#define MAX_DUR 0
#define MIN_DUR 6

const char *dur_strs[] = {
    "𝅝",
    "𝅗𝅥",
    "𝅘𝅥",
    "𝅘𝅥𝅮",
    "𝅘𝅥𝅯",
    "𝅘𝅥𝅰",
    "𝅘𝅥𝅱"
};

enum chord_mode {
    CHORD_MODE_AUTO,
    CHORD_MODE_MONO,
    CHORD_MODE_CHORD
};

#define NUM_PIANO_ROLL_CONSOLE_PANELS 3

struct piano_roll_gui {
    Textbox *track_name;
    Textbox *solo_button;
    Textbox *clip_label;
    Textbox *clip_name;
    Textbox *in_label;
    Textbox *in_name;
    Textbox *dur_tb;

    SDL_Rect *info_panel;
    SDL_Rect *input_panel;
    /* Textbox * */
    /* Textbox *dur_tb; */
};

struct piano_roll_state {
    bool active;
    /* TimeView tv; */
    TimeView *tl_tv;
    MIDIClip *clip;
    ClipRef *cr;
    ClickTrack *ct;
    Note *grabbed_notes[MAX_GRABBED_NOTES];    
    Layout *layout;
    Layout *note_piano_container;
    Layout *note_canvas_lt;
    Layout *piano_lt;
    Layout *console_lt;
    int selected_note; /* midi value */

    /* Insertion params */
    enum note_dur current_dur;
    enum chord_mode chord_mode;
    bool tie;
    bool chord_auto; /* Time insertions and make chord if diff < some constant */
    Note *tied_notes[MAX_TIED_NOTES];

    struct piano_roll_gui gui;
    
};


static struct piano_roll_state state;


/* void piano_roll_load_layout() */
/* { */
/*     Layout *main_lt = layout_read_from_xml(PIANO_ROLL_LT_PATH); */
/*     state.layout = main_lt; */
/*     Layout *piano_lt = layout_get_child_by_name_recursive(main_lt, "piano"); */
/*     state.piano_lt = piano_lt; */
/*     layout_read_xml_to_lt(main_lt, PIANO_88_VERTICAL_LT_PATH); */

/* } */
static void piano_roll_init_layout(Session *session)
{
    if (state.layout) return; /* Already init'd */
    Layout *audio_rect = layout_get_child_by_name_recursive(session->gui.layout, "audio_rect");
    Layout *lt = layout_add_child(session->gui.layout);
    layout_read_xml_to_lt(lt, PIANO_ROLL_LT_PATH);
    state.console_lt = layout_get_child_by_name_recursive(lt, "piano_roll_console");
    Layout *piano_container = layout_get_child_by_name_recursive(lt, "piano");

    state.gui.info_panel = &layout_get_child_by_name_recursive(lt, "track_info")->rect;
    state.gui.input_panel = &layout_get_child_by_name_recursive(lt, "insertion_info")->rect;
    
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
    /* timeview_init(state.tl_tv, &note_canvas->rect, 600, 0, &session->proj.timelines[0]->play_pos_sframes, NULL, NULL); */
}

void piano_roll_init_gui()
{
    Layout *lt = layout_get_child_by_name_recursive(state.console_lt, "current_dur");
    state.gui.dur_tb = textbox_create_from_str(
	/* "TEST", */
	dur_strs[QUARTER],
	lt,
	main_win->music_font,
	30,
	main_win);

    textbox_style(
	state.gui.dur_tb,
	CENTER,
	false,
	NULL,
	&colors.white);

    lt = layout_get_child_by_name_recursive(state.console_lt, "solo");
    state.gui.solo_button = textbox_create_from_str(
	"S",
	lt,
	main_win->bold_font,
	14,
	main_win);
    textbox_set_border(state.gui.solo_button, &colors.black, 1, MUTE_SOLO_BUTTON_CORNER_RADIUS);
    textbox_set_background_color(state.gui.solo_button, &colors.light_grey);


    /* textbox_set_background_color(state.gui.dur_tb, NULL); */
    /* textbox_set_text_color(state.gui.dur_tb, &colors.white); */
    /* textbox_set_trunc(state.gui.dur_tb, false); */
    /* textbox_reset_full(state.gui.dur_tb); */

    lt = layout_get_child_by_name_recursive(state.console_lt, "clip_label");
    state.gui.clip_label = textbox_create_from_str(
	"Clip:",
	lt,
	main_win->mono_font,
	14,
	main_win);
    textbox_style(
	state.gui.clip_label,
	CENTER_LEFT,
	false,
	NULL,
	&colors.white);

    lt = layout_get_child_by_name_recursive(state.console_lt, "in_label");
    state.gui.in_label = textbox_create_from_str(
	"Input:",
	lt,
	main_win->mono_font,
	14,
	main_win);
    textbox_style(
	state.gui.in_label,
	CENTER_LEFT,
	false,
	NULL,
	&colors.white);	
}

void piano_roll_deinit_gui()
{
    textbox_destroy_keep_lt(state.gui.track_name);
    textbox_destroy_keep_lt(state.gui.solo_button);
    textbox_destroy_keep_lt(state.gui.clip_label);
    textbox_destroy_keep_lt(state.gui.clip_name);
    textbox_destroy_keep_lt(state.gui.in_label);
    textbox_destroy_keep_lt(state.gui.in_name);
    textbox_destroy_keep_lt(state.gui.dur_tb);
}

void piano_roll_activate(ClipRef *cr)
{
    Session *session = session_get();
    window_push_mode(main_win, MODE_PIANO_ROLL);
    session->piano_roll = true;
    state.active = true;
    state.cr = cr;
    state.clip = cr->source_clip;
    state.tl_tv = &ACTIVE_TL->timeview;
    state.selected_note = 60;
    piano_roll_init_layout(session_get());
    piano_roll_init_gui();

    state.gui.track_name = textbox_create_from_str(
	cr->track->name,
	layout_get_child_by_name_recursive(state.console_lt, "track_name"),
	main_win->mono_bold_font,
	16,
	main_win);
    /* textbox_set_border(state.gui.track_name, &colors.grey, 1, 0); */
    /* textbox_set_pad(state.gui.track_name, 6, 4); */
    textbox_style(
	state.gui.track_name,
	CENTER_LEFT,
	true,
	NULL,
	&colors.white);


    state.gui.clip_name = textbox_create_from_str(
	cr->name,
	layout_get_child_by_name_recursive(state.console_lt, "clip_name"),
	main_win->std_font,
	14,
	main_win);
    textbox_set_border(state.gui.clip_name, &colors.grey, 1, MUTE_SOLO_BUTTON_CORNER_RADIUS);
    textbox_style(
	state.gui.clip_name,
	CENTER,
	true,
	&colors.quickref_button_blue,
	&colors.white);
    textbox_set_border(state.gui.clip_name, &colors.grey, 1, MUTE_SOLO_BUTTON_CORNER_RADIUS);


    const char *in_name = cr->track->input_type == MIDI_DEVICE ? ((MIDIDevice *)cr->track->input)->name : "(none)";
    state.gui.in_name = textbox_create_from_str(
	in_name,
	layout_get_child_by_name_recursive(state.console_lt, "in_name"),
	main_win->std_font,
	14,
	main_win);
    textbox_set_border(state.gui.in_name, &colors.grey, 1, MUTE_SOLO_BUTTON_CORNER_RADIUS);
    textbox_style(
	state.gui.in_name,
	CENTER,
	true,
	&colors.quickref_button_blue,
	&colors.white);


}
void piano_roll_deactivate()
{
    state.clip = NULL;
    InputMode im = window_pop_mode(main_win);
    if (im != MODE_PIANO_ROLL) {
	fprintf(stderr, "Error: deactivating piano roll, top mode %s\n", input_mode_str(im));
    }
    piano_roll_deinit_gui();
    Session *session = session_get();
    session->piano_roll = false;
    state.active = false;
    timeline_reset_full(ACTIVE_TL);
    /* layout_destroy(state.layout); */
    
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


static void reset_dur_str()
{
    textbox_set_value_handle(
	state.gui.dur_tb,
	dur_strs[state.current_dur]);
}
void piano_roll_dur_longer()
{
    state.current_dur--;
    if ((int)state.current_dur < (int)MAX_DUR) state.current_dur = MAX_DUR;
    reset_dur_str();
}

void piano_roll_dur_shorter()
{
    /* Indexing reversed */
    state.current_dur++;
    if ((int)state.current_dur > (int)MIN_DUR) state.current_dur = MIN_DUR;
    reset_dur_str();
}

void piano_roll_insert_note()
{
}

void piano_roll_insert_rest()
{

}

void piano_roll_grab_ungrab()
{
    
}



/* Draw fns */


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

    int sel_piano_note = state.selected_note - PIANO_BOTTOM_NOTE;
    for (int i=0; i<88; i++) {
	if (i % 12 == 1) SDL_SetRenderDrawColor(main_win->rend, 100, 100, 100, 100);
	else SDL_SetRenderDrawColor(main_win->rend, 100, 100, 100, 50);
	int y = state.note_canvas_lt->rect.y + round((double)i * (double)state.note_canvas_lt->rect.h / 88.0);
	SDL_RenderDrawLine(main_win->rend, state.note_canvas_lt->rect.x, y, state.note_canvas_lt->rect.x + state.note_canvas_lt->rect.w, y);
	if (88 - i - 1 == sel_piano_note) {
	    SDL_SetRenderDrawColor(main_win->rend, 255, 100, 0, 30);
	    SDL_Rect barrect = {state.note_canvas_lt->rect.x, y, state.note_canvas_lt->rect.w, note_height_nominal};
	    SDL_RenderFillRect(main_win->rend, &barrect);
	    /* SDL_SetRenderDrawColor(main_win->rend, 100, 100, 100, 50); */
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


    /* Draw console */
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.control_bar_background_grey));
    SDL_RenderFillRect(main_win->rend, state.gui.info_panel);
    SDL_RenderFillRect(main_win->rend, state.gui.input_panel);

    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.grey));
    SDL_RenderDrawRect(main_win->rend, state.gui.info_panel);
    SDL_RenderDrawRect(main_win->rend, state.gui.input_panel);
    
    textbox_draw(state.gui.track_name);
    textbox_draw(state.gui.solo_button);
    textbox_draw(state.gui.clip_label);
    textbox_draw(state.gui.clip_name);
    textbox_draw(state.gui.in_label);
    textbox_draw(state.gui.in_name);
    textbox_draw(state.gui.dur_tb);
}


/* Getters */

Textbox *piano_roll_get_solo_button()
{
    if (state.active)
	return state.gui.solo_button;
    else return NULL;
}
