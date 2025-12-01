/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "assets.h"
#include "color.h"
#include "geometry.h"
#include "layout.h"
#include "midi_qwerty.h"
#include "piano.h"
#include "layout_xml.h"
#include "textbox.h"

#define NUM_WHITE_KEYS 11

extern Window *main_win;
extern struct colors colors;

const static char *key_label_map[2][18] = {
    {"c", /* note names */
     "d",
     "e",
     "f",
     "g",
     "a",
     "b",
     "c2",
     "d2",
     "e2",
     "f2",
     "cis", /* black keys (index 11) */
     "dis",
     "fis",
     "gis",
     "ais",
     "cis2",
     "dis2"
    },
    {"A", /* labels */
     "S",
     "D",
     "F",
     "G",
     "H",
     "J",
     "K",
     "L",
     ";",
     "'",
     "W", /* black keys (index 11) */
     "E",
     "T",
     "Y",
     "U",
     "O",
     "P"
    }
};

void piano_init(Piano *piano, Layout *container)
{
    layout_force_reset(container);
    piano->container = container;
    piano->layout = layout_read_xml_to_lt(container, PIANO_LT_PATH);
    for (int i=0; i<PIANO_NUM_KEYS; i++) {
	Layout *key_lt = layout_get_child_by_name_recursive(piano->layout, key_label_map[0][i]);
	layout_reset(key_lt);
	piano->key_rects[i] = &key_lt->rect;
	Layout *label_lt = layout_add_child(key_lt);
	label_lt->x.type = SCALE;
	label_lt->y.type = SCALE;
	label_lt->w.type = SCALE;
	label_lt->h.type = SCALE;
	/* layout_set_default_dims(label_lt); */
	label_lt->w.value = 1.0;
	label_lt->h.value = 1.0;
	const char *label_str = key_label_map[1][i];
	piano->key_labels[i] = textbox_create_from_str(
	    label_str,
	    label_lt,
	    main_win->mono_bold_font,
	    12,
	    main_win);
	textbox_set_background_color(piano->key_labels[i], NULL);
	textbox_set_text_color(piano->key_labels[i], i < NUM_WHITE_KEYS ? &colors.black : &colors.white);
	textbox_size_to_fit(piano->key_labels[i], 0, 2);
	piano->key_labels[i]->layout->rect.w = piano->key_labels[i]->layout->rect.h;
	layout_set_values_from_rect(piano->key_labels[i]->layout);
	layout_reset(piano->key_labels[i]->layout);
	layout_center_scale(piano->key_labels[i]->layout, true, true);
	if (i < NUM_WHITE_KEYS) {
	    piano->key_labels[i]->layout->y.value = 0.65;
	    layout_reset(piano->key_labels[i]->layout);
	}
	textbox_reset_full(piano->key_labels[i]);
    }
}

void piano_draw(Piano *piano)
{
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.white));
    int i=0;
    bool key_states[PIANO_NUM_KEYS];
    while (i < NUM_WHITE_KEYS) {
	key_states[i] = mqwert_get_key_state(key_label_map[1][i][0]);
	if (key_states[i])
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.cerulean));
	else
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.white));
	SDL_RenderFillRect(main_win->rend, piano->key_rects[i]);
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.black));
	SDL_RenderDrawRect(main_win->rend, piano->key_rects[i]);
	i++;
    }
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.black));
    while (i < PIANO_NUM_KEYS) {
	key_states[i] = mqwert_get_key_state(key_label_map[1][i][0]);
	if (key_states[i])
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.sea_green));
	else
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.black));
	SDL_RenderFillRect(main_win->rend, piano->key_rects[i]);
	i++;
    }
    for (int i=0; i<PIANO_NUM_KEYS; i++) {
        if (key_states[i]) {
	    if (i < NUM_WHITE_KEYS) {
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.yellow));
		geom_fill_rounded_rect(main_win->rend, &piano->key_labels[i]->layout->rect, 6);
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.grey));
	    } else {
		textbox_set_text_color(piano->key_labels[i], &colors.black);
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.amber));
		geom_fill_rounded_rect(main_win->rend, &piano->key_labels[i]->layout->rect, 6);
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.dark_grey));
	    }

	} else {
	    if (i < NUM_WHITE_KEYS) {
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.black));
	    } else {
		textbox_set_text_color(piano->key_labels[i], &colors.white);
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.white));
	    }
	}
	geom_draw_rounded_rect_thick(main_win->rend, &piano->key_labels[i]->layout->rect, 2, 6 * main_win->dpi_scale_factor);
	textbox_draw(piano->key_labels[i]);
    }
}

Piano *piano_create(Layout *container)
{
    Piano *piano = calloc(1, sizeof(Piano));
    piano_init(piano, container);
    return piano;
}

void piano_destroy(Piano *piano)
{
    for (int i=0; i<PIANO_NUM_KEYS; i++) {
	textbox_destroy(piano->key_labels[i]);
    }
    free(piano);
}
