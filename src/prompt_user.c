#include "SDL_events.h"
#include "color.h"
#include "layout.h"
#include "input.h"
#include "modal.h"
#include "session.h"
#include "window.h"
#define SEL_BUF_LEN 512

extern Window *main_win;
extern struct colors colors;

static int prompt_user_sel;
static bool prompt_user_exit;
ComponentFnDef(prompt_buttonfn) {

    Button *button = self;
    /* button targets are 1-indexed (see below) */
    prompt_user_sel = (int)(long)button->target - 1;
    prompt_user_exit = true;
    return 0;
}

void handle_window_events(SDL_Event e, Window *win);

int prompt_user(const char *header, const char *description, int num_options, const char **option_titles, int cancel_index)
{

    Session *session = session_get();
    Layout *layout = layout_add_child(main_win->layout);
    layout_set_default_dims(layout);
    Modal *modal = modal_create(layout);

    if (header)
	modal_add_header(modal, header, &colors.light_grey, 4);
    if (description) {
	ModalEl *el = modal_add_p(modal, description, &colors.white);
        layout_center_agnostic(el->layout, true, false);
        /* layout_reset(modal->layout); */
        TextArea *ta = el->obj;
        txt_area_align_center(ta);

    }
    Button *buttons[num_options];
    int max_width = 0;
    modal_add_header(modal, "", &colors.light_grey, 5);
    for (int i=0; i<num_options; i++) {
        ModalEl *el = modal_add_button(modal, option_titles[i], prompt_buttonfn);
        Button *button = el->obj;
        button->target = (void *)(long)(i + 1);
        if (button->tb->layout->w.value > max_width) {
            max_width = button->tb->layout->w.value;
        }
        buttons[i] = button;
    }
    for (int i=0; i<num_options; i++) {
        buttons[i]->tb->layout->w.value = max_width;
        layout_reset(buttons[i]->tb->layout);
        textbox_reset(buttons[i]->tb);
        layout_center_agnostic(buttons[i]->tb->layout, true, false);
        
    }
    modal_reset(modal);

    SymbolButton *saved_modal_x = modal->x;
    modal->x = NULL;
    window_end_draw(main_win);
    SDL_Event e;
    bool first_frame = true;
    prompt_user_sel = 0;
    prompt_user_exit = false;
    bool needs_draw = true;
    while (!prompt_user_exit) {
	while (SDL_PollEvent(&e)) {
	    switch (e.type) {
	    case SDL_QUIT:
		SDL_PushEvent(&e); /* Push quit event to be handled later */
		return cancel_index;
            case SDL_WINDOWEVENT:
                handle_window_events(e, main_win);
                /* modal_reset(modal); */
                needs_draw = true;
                break;
            case SDL_MOUSEMOTION:
                window_set_mouse_point(main_win, e.motion.x, e.motion.y);
                break;
            case SDL_MOUSEBUTTONDOWN:
                modal_triage_mouse(modal, &main_win->mousep, true);
                needs_draw = true;
                break;
            case SDL_KEYUP:
                /* i_state handling */
                switch (e.key.keysym.scancode) {
                case SDL_SCANCODE_LSHIFT:
                case SDL_SCANCODE_RSHIFT:
                    main_win->i_state &= ~I_STATE_SHIFT;
                    break;
                default: break;
                }
                break;
	    case SDL_KEYDOWN: {
                /* i_state handling */
                switch (e.key.keysym.scancode) {
                case SDL_SCANCODE_LSHIFT:
                case SDL_SCANCODE_RSHIFT:
                    main_win->i_state |= I_STATE_SHIFT;
                    break;
                case SDL_SCANCODE_RETURN:
                    modal_select(modal);
                    prompt_user_exit = true;
                    break;
                case SDL_SCANCODE_TAB:
                    if (main_win->i_state & I_STATE_SHIFT) {
                        modal_previous_escape(modal);
                    } else {
                        modal_next_escape(modal);
                    }
                    needs_draw = true;
                    break;
                case SDL_SCANCODE_ESCAPE:
                    prompt_user_sel = cancel_index;
                    prompt_user_exit = true;
                    break;
                default:
                    break;
                }
                if (prompt_user_exit) break;
	    }
                break;
	    case SDL_AUDIODEVICEADDED:
	    case SDL_AUDIODEVICEREMOVED:
	    default:
		break;
	    }
	}
        if (needs_draw) {
            window_start_draw(main_win, NULL);
            if (first_frame) {
                SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 150);
                SDL_RenderFillRect(main_win->rend, &main_win->layout->rect);
                first_frame = false;
            }

            modal_draw(modal);
            window_end_draw(main_win);
            needs_draw = false;
        }
	SDL_Delay(1);
    }
    modal->x = saved_modal_x;
    modal_destroy(modal);
    ACTIVE_TL->needs_redraw = true;
    main_win->i_state = 0;
    return prompt_user_sel;
}
