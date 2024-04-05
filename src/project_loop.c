#include "SDL.h"
#include "draw.h"
#include "input.h"
#include "layout.h"
#include "project.h"
#include "window.h"

extern Window *main_win;
extern SDL_Color color_global_black;

InputMode active_mode = GLOBAL;

Project *proj;


void add_track()
{
    layout_add_iter(layout_get_child_by_name_recursive(main_win->layout, "track_container"), VERTICAL, true);
}

void change_mode()
{
    if (active_mode < NUM_INPUT_MODES - 1) {
	active_mode++;
    } else {
	active_mode = 0;
    }
    fprintf(stdout, "Active mode: %s\n", input_mode_str(active_mode));
}

void bind_funcs()
{
    input_bind_func(add_track, I_STATE_CMDCTRL, SDLK_t, PROJECT);
    input_bind_func(change_mode, 0, SDLK_m, GLOBAL);
}


void loop_project_main()
{

    Layout *temp_scrolling_lt = NULL;
    Layout *scrolling_lt = NULL;
    UserInput *input = NULL;
    
    uint16_t i_state = 0;
    SDL_Event e;
    uint8_t fingersdown = 0;
    uint8_t fingerdown_timer = 0;

    bind_funcs();
    

    while (!(i_state & I_STATE_QUIT)) {
	while (SDL_PollEvent(&e)) {
	    switch (e.type) {
	    case SDL_QUIT:
		i_state |= I_STATE_QUIT;
		break;
	    case SDL_WINDOWEVENT:
		if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
		    window_resize(main_win, e.window.data1, e.window.data2);
		}
		break;
	    case SDL_MOUSEMOTION:
		window_set_mouse_point(main_win, e.motion.x, e.motion.y);
		break;
	    case SDL_KEYDOWN:
		switch (e.key.keysym.scancode) {
		case SDL_SCANCODE_LGUI:
		case SDL_SCANCODE_RGUI:
		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_RCTRL:
		    i_state |= I_STATE_CMDCTRL;
		    break;
		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RSHIFT:
		    i_state |= I_STATE_SHIFT;
		    break;
		case SDL_SCANCODE_X:
		    if (i_state & I_STATE_CMDCTRL) {
			i_state |= I_STATE_C_X;
		    }
		    break;
		default:
		    /* i_state = triage_keydown(i_state, e.key.keysym.scancode); */
		    input = input_get(i_state, e.key.keysym.sym, active_mode);
		    if (input && input->func) {
			input->func();
		    }
		    break;
		}
		break;
	    case SDL_KEYUP:
		switch (e.key.keysym.scancode) {
		case SDL_SCANCODE_LGUI:
		case SDL_SCANCODE_RGUI:
		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_RCTRL:
		    i_state &= ~I_STATE_CMDCTRL;
		    break;
		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RSHIFT:
		    i_state &= ~I_STATE_SHIFT;
		    break;
		default:
		    break;
		}
		break;
	    case SDL_MOUSEWHEEL:
		temp_scrolling_lt = layout_handle_scroll(
		    main_win->layout,
		    &main_win->mousep,
		    e.wheel.preciseX * LAYOUT_SCROLL_SCALAR,
		    e.wheel.preciseY * LAYOUT_SCROLL_SCALAR * -1,
		    fingersdown);
		break;
	    case SDL_FINGERDOWN:
	        fingersdown = SDL_GetNumTouchFingers(-1);
		break;
	    case SDL_FINGERUP:
		fingersdown = SDL_GetNumTouchFingers(-1);

		if (fingersdown == 0) {
		    scrolling_lt = temp_scrolling_lt;
		}
		break;
	    default:
		break;
	    }
		
	}
	if (scrolling_lt) {
	    /* fingersdown = SDL_GetNumTouchFingers(-1); */
	    if (layout_scroll_step(scrolling_lt) == 0) {
		scrolling_lt->iterator->scroll_momentum = 0;
		scrolling_lt = NULL;
	    } else if (fingersdown > 0) {
		scrolling_lt->iterator->scroll_momentum = 0;
		scrolling_lt = NULL;
	    }
	}
	
	

	/******************** DRAW ********************/
	window_start_draw(main_win, &color_global_black);

	/***** Debug only *****/
	layout_draw(main_win, main_win->layout);
	/**********************/


	
	window_end_draw(main_win);
	/**********************************************/

	
	SDL_Delay(10);

    }
}
