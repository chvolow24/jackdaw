/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/

/*****************************************************************************************************************
    project_loop.c

    * main project animation loop
    * in-progress animations and updates
 *****************************************************************************************************************/


#include <time.h>
#include "SDL.h"
#include "audio_connection.h"
#include "draw.h"
#include "input.h"
#include "layout.h"
#include "modal.h"
#include "mouse.h"
#include "project.h"
#include "project_draw.h"
#include "screenrecord.h"
#include "status.h"
#include "timeline.h"
#include "transport.h"
#include "window.h"

#define MAX_MODES 8
#define STICK_DELAY_MS 500
#define PLAYSPEED_ADJUST_SCALAR_LARGE 0.1f
#define PLAYSPEED_ADJUST_SCALAR_SMALL 0.015f


#define TOP_MODE (main_win->modes[main_win->num_modes - 1])


#ifndef INSTALL_DIR
#define INSTALL_DIR "."
#endif


extern Window *main_win;
extern SDL_Color color_global_black;
extern SDL_Color color_global_white;


extern Project *proj;


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
    if (fs->editing) {
	SDL_Delay(STICK_DELAY_MS);
	fs->editing = false;
    }
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

/* TODO: SDL bug workaround. I haven't been able to get this to work reliably cross-platform. */
/* https://discourse.libsdl.org/t/window-event-at-initiation-of-window-resize/50963/3 */
/* static int SDLCALL window_resize_callback(void *userdata, SDL_Event *event) */
/* { */
/*     if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_EXPOSED) { */
/* 	window_resize_passive(main_win, event->window.data1, event->window.data2); */
/* 	project_draw(); */
/*     } */
/*     return 0; */
/* } */

/* Modal *test_modal; */

void layout_write(FILE *f, Layout *lt, int indent);
void loop_project_main()
{

    clock_t start, end;
    uint8_t frame_ctr = 0;
    float fps = 0;

    Layout *temp_scrolling_lt = NULL;
    Layout *scrolling_lt = NULL;
    UserFn *input_fn = NULL;
    
    /* uint16_t i_state = 0; */
    SDL_Event e;
    uint8_t fingersdown = 0;
    uint8_t fingerdown_timer = 0;

    uint8_t animate_step = 0;
    bool set_i_state_k = false;

    window_push_mode(main_win, TIMELINE);



    /* /\* TESTING *\/ */
    /* const char *modal_p = "Hello. My name is charlie volow. I am here to test this modal thing I am implementing. It is very unlikely that I will be happy with how it works; however, I think it is necessary, and good, and this is the best I can do to get what I want. I will say that it is better than other code I have written"; */
    /* Layout *mod_lt = layout_add_child(proj->layout); */
    /* layout_set_default_dims(mod_lt); */
    /* Modal *test_modal = modal_create(mod_lt); */
    /* modal_add_header(test_modal, "Hello world!", &color_global_black, 1); */
    /* modal_add_dirnav(test_modal, INSTALL_DIR "/assets", true, true); */
    /* modal_add_header(test_modal, "Subtitle", &color_global_black, 3); */
    /* modal_add_p(test_modal, modal_p, &color_global_black); */
    /* modal_add_header(test_modal, "Another thing...", &color_global_black, 2); */
    /* modal_add_header(test_modal, "AND", &color_global_black, 1); */
    /* modal_add_p(test_modal, "This is also a paragraph.", &color_global_black); */
    /* /\* layout_force_reset(test_modal->layout); *\/ */
    /* modal_reset(test_modal); */
    /* window_push_modal(main_win, test_modal); */
    /* END TEST */
    
    /* layout_write(stdout, main_win->layout, 0); */
    /* SDL_AddEventWatch(window_resize_callback, NULL); */
    /* window_resize_passive(main_win, main_win->w, main_win->h); */
    bool first_frame = true;
    while (!(main_win->i_state & I_STATE_QUIT)) {
	/* fprintf(stdout, "About to poll...\n"); */
	while (SDL_PollEvent(&e)) {
	    /* fprintf(stdout, "Polled!\n"); */
	    switch (e.type) {
	    case SDL_QUIT:
		main_win->i_state |= I_STATE_QUIT;
		break;
	    case SDL_WINDOWEVENT:
		if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
		    window_resize_passive(main_win, e.window.data1, e.window.data2);
		}
		break;
	    case SDL_AUDIODEVICEADDED:
	    case SDL_AUDIODEVICEREMOVED:
		/* fprintf(stdout, "%s iscapture: %d, %d\n", e.type == SDL_AUDIODEVICEADDED ? "ADDED device" : "REMOVED device", e.adevice.iscapture, e.adevice.which); */
		if (!first_frame) {
		    if (proj->recording) transport_stop_recording();
		    /* transport_stop_playback(); */
		    audioconn_handle_connection_event(e.adevice.which, e.adevice.iscapture, e.adevice.type);
		}
		break;
	    case SDL_MOUSEMOTION: {
		switch (TOP_MODE) {
		case MENU_NAV:
		    mouse_triage_motion_menu();
		    break;
		case TIMELINE:
		    mouse_triage_motion_timeline();
		default:
		    break;
		}
		window_set_mouse_point(main_win, e.motion.x, e.motion.y);
		break;
	    }
	    case SDL_KEYDOWN:
		/* for (uint8_t i=0; i<proj->num_clips; i++) { */
		/*     Clip *clip = proj->clips[i]; */
		/*     fprintf(stdout, "\n\nClip: %p\n", clip); */
		/*     for (uint8_t t=0; t<proj->timelines[0]->num_tracks; t++) { */
		/* 	Track *trk = proj->timelines[0]->tracks[t]; */
		/* 	for (uint8_t c=0;c<trk->num_clips; c++) { */
		/* 	    ClipRef *cr= trk->clips[c]; */
		/* 	    fprintf(stdout, "\t\t testing clip %p in trk %d\n", cr->clip, t); */
		/* 	    if (cr->clip == clip) { */
		/* 		fprintf(stdout, "\tFound in track %d\n", t); */
		/* 	    } */
		/* 	} */
		/*     } */
		/* } */
		switch (e.key.keysym.scancode) {
		case SDL_SCANCODE_LGUI:
		case SDL_SCANCODE_RGUI:
		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_RCTRL:
		    /* modal_reset(test_modal); */
		    /* layout_reset(main_win->layout); */
		    main_win->i_state |= I_STATE_CMDCTRL;
		    break;
		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RSHIFT:
		    main_win->i_state |= I_STATE_SHIFT;
		    break;
		case SDL_SCANCODE_LALT:
		case SDL_SCANCODE_RALT:
		    main_win->i_state |= I_STATE_META;
		    break;
		/* case SDL_SCANCODE_X: */
		/*     if (main_win->i_state & I_STATE_CMDCTRL) { */
		/* 	main_win->i_state |= I_STATE_C_X; */
		/*     } */
		/*     break; */
	        case SDL_SCANCODE_K:
		    if (main_win->i_state & I_STATE_K) {
			break;
		    } else {
			set_i_state_k = true;
		    }
		    /* No break */
		default: 
		    /* i_state = triage_keypdown(i_state, e.key.keysym.scancode); */
		    input_fn  = input_get(main_win->i_state, e.key.keysym.sym, GLOBAL);
		    if (!input_fn) {
			for (int i=main_win->num_modes - 1; i>=0; i--) {
			    input_fn = input_get(main_win->i_state, e.key.keysym.sym, main_win->modes[i]);
			    if (input_fn) {
				break;
			    }
			}
		    }
		    /* fprintf(stdout, "Input fn? %p, do fn? %p\n", input_fn, input_fn->do_fn); */
		    if (input_fn && input_fn->do_fn) {
			input_fn->do_fn();
			status_set_callstr(input_get_keycmd_str(main_win->i_state, e.key.keysym.sym));
			status_cat_callstr(" : ");
			status_cat_callstr(input_fn->fn_display_name);
			/* timeline_reset(proj->timelines[proj->active_tl_index]); */
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
		    main_win->i_state &= ~I_STATE_CMDCTRL;
		    break;
		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RSHIFT:
		    main_win->i_state &= ~I_STATE_SHIFT;
		    break;
		case SDL_SCANCODE_LALT:
		case SDL_SCANCODE_RALT:
		    main_win->i_state &= ~I_STATE_META;
		    break;
		case SDL_SCANCODE_K:
		    main_win->i_state &= ~I_STATE_K;
		    proj->play_speed = 0;
		    proj->src_play_speed = 0;
		    transport_stop_playback();
		    break;
		case SDL_SCANCODE_J:
		case SDL_SCANCODE_L:
		    if (main_win->i_state & I_STATE_K) {
			proj->play_speed = 0;
			proj->src_play_speed = 0;
			transport_stop_playback();
		    }
		    break;
		default:
		    stop_update_track_vol_pan();
		    break;
		}
		break;
	    case SDL_MOUSEWHEEL: {
		if (main_win->i_state & I_STATE_SHIFT && main_win->i_state & I_STATE_CMDCTRL) {
		    if (main_win->i_state & I_STATE_META)
		    proj->play_speed += e.wheel.y * PLAYSPEED_ADJUST_SCALAR_LARGE;
		    else {
			proj->play_speed += e.wheel.y * PLAYSPEED_ADJUST_SCALAR_SMALL;
		    }
		} else {
		    bool allow_scroll = true;
		    if (SDL_PointInRect(&main_win->mousep, proj->audio_rect)) {
			if (main_win->i_state & I_STATE_CMDCTRL) {
			    double scale_factor = pow(SFPP_STEP, e.wheel.y);
			    timeline_rescale(scale_factor, true);
			    allow_scroll = false;
			} else {
			    timeline_scroll_horiz(TL_SCROLL_STEP_H * e.wheel.x);
			}
		    }
		    if (allow_scroll) {
			temp_scrolling_lt = layout_handle_scroll(
			    main_win->layout,
			    &main_win->mousep,
			    e.wheel.preciseX * LAYOUT_SCROLL_SCALAR,
			    e.wheel.preciseY * LAYOUT_SCROLL_SCALAR * -1,
			    fingersdown);
			timeline_reset(proj->timelines[proj->active_tl_index]);
		    }
		}
	    }
		break;
	    case SDL_MOUSEBUTTONDOWN:
		if (e.button.button == SDL_BUTTON_LEFT) {
		    main_win->i_state |= I_STATE_MOUSE_L;
		} else if (e.button.button == SDL_BUTTON_RIGHT) {
		    main_win->i_state |= I_STATE_MOUSE_R;
		}
		switch(TOP_MODE) {
		case TIMELINE:
		    fprintf(stdout, "top mode tl\n");
		    mouse_triage_click_project(e.button.button);
		    break;
		case MENU_NAV:
		    mouse_triage_click_menu(e.button.button);
		    break;
		default:
		    break;
		}
		break;
	    case SDL_MOUSEBUTTONUP:
		if (e.button.button == SDL_BUTTON_LEFT) {
		    main_win->i_state &= ~I_STATE_MOUSE_L;
		} else if (e.button.button == SDL_BUTTON_RIGHT) {
		    main_win->i_state &= ~I_STATE_MOUSE_R;
		}
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
		main_win->i_state |= I_STATE_K;
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
	
	first_frame = false;
	if (proj->play_speed != 0 && !proj->source_mode) {
	    timeline_catchup();
	    timeline_set_timecode();
	}

	if (animate_step == 255) {
	    animate_step = 0;
	} else {
	    animate_step++;
	}

	update_track_vol_pan();
	if (proj->recording) {
	    transport_recording_update_cliprects();
	}
	
	status_frame();
	/* update_track_vol_and_pan(); */
	
	/******************** DRAW ********************/
	/* window_start_draw(main_win, &color_global_black); */

	/* if (proj->play_speed != 0) { */
	/*     timeline_move_play_position((int32_t) 500 * proj->play_speed); */
	/* } */

	project_draw();


	if (main_win->screenrecording) {
	    screenshot_loop();
	}
	/* window_draw_menus(main_win); */
	
	/***** Debug only *****/
	/* layout_draw(main_win, main_win->layout); */
	/**********************/


	
	/* window_end_draw(main_win); */
	/**********************************************/

	SDL_Delay(1);


        end = clock();
	fps += (float)CLOCKS_PER_SEC / (end - start);
	start = end;
	if (frame_ctr > 250) {
	    fps /= 250;
	    fprintf(stdout, "FPS: %f\n", fps);
	    frame_ctr = 0;
	} else {
	    frame_ctr++;
	}

    }
}
