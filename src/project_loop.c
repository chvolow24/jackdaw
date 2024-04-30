#include "SDL.h"
#include <time.h>
#include "draw.h"
#include "input.h"
#include "layout.h"
#include "project.h"
#include "project_draw.h"
#include "timeline.h"
#include "transport.h"
#include "window.h"

extern Window *main_win;
extern SDL_Color color_global_black;

#define MAX_MODES 8
#define STICK_DELAY_MS 500

Project *proj;



/* static int timed_stop_update_track_vol_pan(void *data) */
/* { */
/*     fprintf(stdout, "OK.......\n"); */
/*     /\* int delay_ms = *((int *)data); *\/ */
/*     SDL_Delay(STICK_DELAY_MS); */
/*     fprintf(stdout, "Done!\n"); */
/*     stop_update_track_vol_pan(); */
/*     return 0; */
/* } */

static int timed_hide_slider_label(void *data)
{
    FSlider *fs = (FSlider *)data;
    SDL_Delay(STICK_DELAY_MS);
    fs->editing = false;
}

static void hide_slider_label(FSlider *fs)
{
    SDL_CreateThread(timed_hide_slider_label, "hide_slider_label", fs);
}


static void stop_update_track_vol_pan()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    Track *trk = NULL;
    for (int i=0; i<tl->num_tracks; i++) {
	trk = tl->tracks[i];
	hide_slider_label(trk->vol_ctrl);
	hide_slider_label(trk->pan_ctrl);
        /* trk->vol_ctrl->editing = false; */
	/* trk->pan_ctrl->editing = false; */
    }
    proj->vol_changing = false;
    proj->pan_changing = false;
    

}


static void update_track_vol_pan()
{
    /* fprintf(stdout, "%p, %p\n", proj->vol_changing, proj->pan_changing); */
    if (!proj->vol_changing && !proj->pan_changing) {
	return;
    }
    Timeline *tl = proj->timelines[proj->active_tl_index];
    bool had_active_track = false;
    Track *selected_track = tl->tracks[tl->track_selector];
    /* if (proj->vol_changing) { */
    for (int i=0; i<tl->num_tracks; i++) {
	Track *trk = tl->tracks[i];
	if (trk->active) {
	    had_active_track = true;
	    if (proj->vol_changing) {
		/* hide_slider_label(trk->vol_ctrl); */
		trk->vol_ctrl->editing = true;
		if (proj->vol_up) {
		    track_increment_vol(trk);
		} else {
		    track_decrement_vol(trk);
		}
	    }
	    if (proj->pan_changing) {
		/* hide_slider_label */
		trk->pan_ctrl->editing = true;
		if (proj->pan_right) {
		    track_increment_pan(trk);
		} else {
		    track_decrement_pan(trk);
		}
	    }
	}
    }
    if (!had_active_track) {
	if (proj->vol_changing) {
	    if (proj->vol_up) {
		track_increment_vol(selected_track);
	    } else {
		track_decrement_vol(selected_track);
	    }
	    selected_track->vol_ctrl->editing = true;
	}
	if (proj->pan_changing) {
	    if (proj->pan_right) {
		track_increment_pan(selected_track);
	    } else {
		track_decrement_pan(selected_track);
	    }
	    selected_track->pan_ctrl->editing = true;
	}
    }
}


void loop_project_main()
{

    clock_t start, end;
    uint8_t frame_ctr = 0;
    float fps = 0;

    Layout *temp_scrolling_lt = NULL;
    Layout *scrolling_lt = NULL;
    UserFn *input_fn = NULL;
    
    uint16_t i_state = 0;
    SDL_Event e;
    uint8_t fingersdown = 0;
    uint8_t fingerdown_timer = 0;

    uint8_t animate_step = 0;
    bool set_i_state_k = false;

    window_push_mode(main_win, TIMELINE);
   
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
		fprintf(stdout, "Keycmd: \"%s\"\n", input_get_keycmd_str(i_state, e.key.keysym.sym));
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
		case SDL_SCANCODE_LALT:
		case SDL_SCANCODE_RALT:
		    i_state |= I_STATE_META;
		    break;
		case SDL_SCANCODE_X:
		    if (i_state & I_STATE_CMDCTRL) {
			i_state |= I_STATE_C_X;
		    }
		    break;
	        case SDL_SCANCODE_K:
		    if (i_state & I_STATE_K) {
			break;
		    } else {
			set_i_state_k = true;
		    }
		    /* No break */
		default:
		    /* i_state = triage_keypdown(i_state, e.key.keysym.scancode); */
		    input_fn  = input_get(i_state, e.key.keysym.sym, GLOBAL);
		    if (!input_fn) {
			for (int i=main_win->num_modes - 1; i>=0; i--) {
			    input_fn = input_get(i_state, e.key.keysym.sym, main_win->modes[i]);
			    if (input_fn) {
				break;
			    }
			}
		    }
		    if (input_fn && input_fn->do_fn) {
			input_fn->do_fn();
			timeline_reset(proj->timelines[proj->active_tl_index]);
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
		case SDL_SCANCODE_LALT:
		case SDL_SCANCODE_RALT:
		    i_state &= ~I_STATE_META;
		case SDL_SCANCODE_K:
		    i_state &= ~I_STATE_K;
		    proj->play_speed = 0;
		    proj->src_play_speed = 0;
		    transport_stop_playback();
		    break;
		case SDL_SCANCODE_J:
		case SDL_SCANCODE_L:
		    if (i_state & I_STATE_K) {
			proj->play_speed = 0;
			proj->src_play_speed = 0;
			transport_stop_playback();
		    }
		    break;
		default:
		    
		    /* SDL_CreateThread(timed_stop_update_track_vol_pan, "stop_vol_pan", NULL); */
		    stop_update_track_vol_pan();
		    /* if (proj->vol_changing) { */
		    /* 	proj->vol_changing = false; */
		    /* } */
		    /* if (proj->pan_changing) { */
		    /* 	proj->pan_changing = NULL; */
		    /* } */

		    /* input_fn  = input_get(i_state, e.key.keysym.sym, GLOBAL); */
		    /* if (!input_fn) { */
		    /* 	for (int i=main_win->num_modes - 1; i>=0; i--) { */
		    /* 	    input_fn = input_get(i_state, e.key.keysym.sym, main_win->modes[i]); */
		    /* 	    if (input_fn) { */
		    /* 		break; */
		    /* 	    } */
		    /* 	} */
		    /* } */
		    /* if (input_fn && input_fn->do_fn && input_fn->is_toggle) { */
		    /* 	input_fn->do_fn(); */
		    /* 	timeline_reset(proj->timelines[proj->active_tl_index]); */
		    /* } */

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
		timeline_reset(proj->timelines[proj->active_tl_index]);
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
	    if (set_i_state_k) {
		i_state |= I_STATE_K;
		set_i_state_k = false;
	    }
		
	} /* End event handling */
	if (scrolling_lt) {
	    if (animate_step % 1 == 0) {
		/* fingersdown = SDL_GetNumTouchFingers(-1); */
		if (layout_scroll_step(scrolling_lt) == 0) {
		    scrolling_lt->iterator->scroll_momentum = 0;
		    scrolling_lt = NULL;
		} else if (fingersdown > 0) {
		    scrolling_lt->iterator->scroll_momentum = 0;
		    scrolling_lt = NULL;
		}
		timeline_reset(proj->timelines[proj->active_tl_index]);
	    }
	}
	

	if (proj->play_speed != 0) {
	    timeline_set_timecode();
	}

	if (animate_step == 255) {
	    animate_step = 0;
	} else {
	    animate_step++;
	}

	update_track_vol_pan();
	/* update_track_vol_and_pan(); */
	
	/******************** DRAW ********************/
	window_start_draw(main_win, &color_global_black);

	/* if (proj->play_speed != 0) { */
	/*     timeline_move_play_position((int32_t) 500 * proj->play_speed); */
	/* } */

	project_draw(proj);
	window_draw_menus(main_win);
	
	/***** Debug only *****/
	/* layout_draw(main_win, main_win->layout); */
	/**********************/


	
	window_end_draw(main_win);
	/**********************************************/

	SDL_Delay(1);


        end = clock();
	fps += (float)CLOCKS_PER_SEC / (end - start);
	start = end;
	if (frame_ctr > 100) {
	    fps /= 100;
	    fprintf(stdout, "FPS: %f\n", fps);
	    frame_ctr = 0;
	} else {
	    frame_ctr++;
	}

    }
}
