#include "SDL_events.h"
#include "color.h"
#include "endpoint.h"
#include "layout.h"
#include "modal.h"
#include "window.h"
#define SEL_BUF_LEN 512

extern Window *main_win;
extern struct colors colors;

int prompt_user(const char *header, const char *description, int num_options, char **option_titles)
{
    Layout *layout = layout_add_child(main_win->layout);
    layout_set_default_dims(layout);
    Modal *modal = modal_create(layout);
    
    modal_add_header(modal, header, &colors.light_grey, 3);
    modal_add_p(modal, description, &colors.white);
    char *sel_buf = malloc(SEL_BUF_LEN);
    char sel_char = '1';
    int buf_i = 0;
    for (int i=0; i<num_options; i++) {
	buf_i += snprintf(sel_buf + buf_i, SEL_BUF_LEN - buf_i, "%c:\t%s\n", sel_char, option_titles[i]);
	if (sel_char == '9') {
	    sel_char = 'a';
	} else {
	    sel_char++;
	}
    }
    sel_buf[buf_i] = '\0';
    ModalEl *el = modal_add_p_custom_fontsize(modal, sel_buf, &colors.white, 14);
    /* RadioButton *r = modal_add_radio(modal, &colors.light_grey, NULL, (const char **)option_titles, num_options)->obj; */
    modal_reset(modal);
    el->layout->w.type = REL;
    /* layout_write(stderr, el->layout, 0); */
    layout_size_to_fit_children_h(el->layout, true, 0);
    layout_center_agnostic(el->layout, true, false);
    modal_reset(modal);

    SymbolButton *saved_modal_x = modal->x;
    modal->x = NULL;
    window_end_draw(main_win);
    SDL_Event e;
    bool shift_down = false;
    int i=10000;
    int sel = 0;
    while (i > 0) {
	while (SDL_PollEvent(&e)) {
	    switch (e.type) {
	    case SDL_QUIT:
		SDL_PushEvent(&e); /* Push quit event to be handled later */
		i=1000;
		return 1;
	    case SDL_KEYDOWN: {
		int sym = e.key.keysym.sym;
		if (sym >= '1' && sym <= '9') {
		    sel = sym - '1';
		    i=0;
		} else if (sym >= 'a' && sym <= 'z') {
		    sel = 10 + sym - 'a';
		    i=0;
		} else {
		    switch (e.key.keysym.scancode) {
			/* case SDL_SCANCODE_LSHIFT: */
			/* case SDL_SCANCODE_RSHIFT: */
			/*     shift_down = true; */
			/*     break; */
		    case SDL_SCANCODE_ESCAPE:
		    case SDL_SCANCODE_RETURN:
			i=0;
			break;
		    default:
			break;
		    }
		}
		break;
	    }
	    case SDL_KEYUP:
		switch (e.key.keysym.scancode) {
		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RSHIFT:
		    shift_down = false;
		    break;
		default:
		    break;
		}
		break;
	    case SDL_MOUSEBUTTONUP:
	    case SDL_AUDIODEVICEADDED:
	    case SDL_AUDIODEVICEREMOVED:
		SDL_PushEvent(&e);
		break;
	    default:
		break;
	    }
	}
	window_start_draw(main_win, NULL);
	modal_draw(modal);
	window_end_draw(main_win);
	SDL_Delay(1);
	i--;
    }
    /* int sel = r->selected_item; */
    free(sel_buf);
    modal->x = saved_modal_x;
    modal_destroy(modal);
    return sel;
}
