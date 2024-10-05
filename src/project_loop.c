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
#include "automation.h"
#include "components.h"
#include "draw.h"
#include "dsp.h"
#include "input.h"
#include "layout.h"
#include "modal.h"
#include "mouse.h"
#include "page.h"
#include "panel.h"
#include "project.h"
#include "project_draw.h"
#include "screenrecord.h"
#include "settings.h"
#include "status.h"
#include "timeline.h"
#include "transport.h"
#include "user_event.h"
#include "value.h"
#include "waveform.h"
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
extern SDL_Color color_global_grey;

extern SDL_Color freq_L_color;
extern SDL_Color freq_R_color;

extern Project *proj;

extern volatile bool cancel_threads;

/* static int timed_stop_update_track_vol_pan(void *data) */
/* { */
/*     fprintf(stdout, "OK.......\n"); */
/*     /\* int delay_ms = *((int *)data); *\/ */
/*     SDL_Delay(STICK_DELAY_MS); */
/*     fprintf(stdout, "Done!\n"); */
/*     stop_update_track_vol_pan(); */
/*     return 0; */
/* } */

/* static int timed_hide_slider_label(void *data) */
/* { */
/*     Slider *fs = (Slider *)data; */
/*     if (fs->editing) { */
/* 	for (int i=0; i<STICK_DELAY_MS; i++) { */
/* 	    if (cancel_threads) return 0; */
/* 	    SDL_Delay(1); */
/* 	} */
/* 	fs->editing = false; */
/*     } */
/*     return 0; */
/* } */

/* static void hide_slider_label(Slider *fs) */
/* { */
/*     SDL_CreateThread(timed_hide_slider_label, "hide_slider_label", fs); */
/* } */


static void stop_update_track_vol_pan()
{
    /* Timeline *tl = proj->timelines[proj->active_tl_index]; */
    /* Track *trk = NULL; */
    /* for (int i=0; i<tl->num_tracks; i++) { */
    /* 	trk = tl->tracks[i]; */
    /* 	/\* hide_slider_label(trk->vol_ctrl); *\/ */
    /* 	/\* hide_slider_label(trk->pan_ctrl); *\/ */
    /*     /\* trk->vol_ctrl->editing = false; *\/ */
    /* 	/\* trk->pan_ctrl->editing = false; *\/ */
    /* } */
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
    tl->needs_redraw = true;
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

void loop_project_main()
{
    /* clock_t start, end; */
    /* uint8_t frame_ctr = 0; */
    /* float fps = 0; */

    Layout *temp_scrolling_lt = NULL;
    Layout *scrolling_lt = NULL;
    UserFn *input_fn = NULL;
    
    /* uint16_t i_state = 0; */
    SDL_Event e;
    uint8_t fingersdown = 0;
    /* uint8_t fingerdown_timer = 0; */
    
    uint8_t animate_step = 0;
    bool set_i_state_k = false;

    window_push_mode(main_win, TIMELINE);

    bool first_frame = true;
    int wheel_event_recency = 0;
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
		    proj->timelines[proj->active_tl_index]->needs_redraw = true;
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
		window_set_mouse_point(main_win, e.motion.x, e.motion.y);
		switch (TOP_MODE) {
		case MODAL:
		    mouse_triage_motion_modal();
		    break;
		case MENU_NAV:
		    mouse_triage_motion_menu();
		    break;
		case TIMELINE:
		    if (!mouse_triage_motion_page() && !mouse_triage_motion_tabview()) {
			mouse_triage_motion_timeline();
		    }
		case TABVIEW:
		    if (!mouse_triage_motion_tabview())
			mouse_triage_motion_page();
		    break;
		default:
		    break;
		}
		break;
	    }
	    case SDL_TEXTINPUT:
		if (main_win->txt_editing) {
		    txt_input_event_handler(main_win->txt_editing, &e);
		}
		break;
	    case SDL_KEYDOWN: {
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
		case SDL_SCANCODE_6: {
		    Timeline *tl = proj->timelines[proj->active_tl_index];
		    Track *track = tl->tracks[tl->track_selector];
		    track_add_new_automation(track);
		    /* if (track->num_automations == 0) { */
		    /* 	track_add_automation(track, AUTO_VOL); */
		    /* } else if (track->num_automations == 1) { */
		    /* 	track_add_automation(track, AUTO_PAN); */
		    /* } else if (track->num_automations == 2) { */
		    /* 	track_add_automation(track, AUTO_PLAY_SPEED); */
		    /* } else if (track->num_automations == 3) { */
		    /* 	/\* track_add_automation(track, AUTO_PLAY_SPEED); *\/ */
		    /* 	track_add_automation(track, AUTO_FIR_FILTER_CUTOFF); */
		    /* } else if (track->num_automations == 4) { */
		    /* 	track_add_automation(track, AUTO_DEL_TIME); */
		    /* } else if (track->num_automations == 5) { */
		    /* 	/\* track *\/ */
		    /* 	track_add_automation(track, AUTO_DEL_AMP); */
		    /* } */
			    
		    track_automations_show_all(track);


		} break;
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
		    /* fprintf(stdout, "== SDLK_ */
		    /* i_state = triage_keypdown(i_state, e.key.keysym.scancode); */
		    input_fn  = input_get(main_win->i_state, e.key.keysym.sym);
		    /* if (main_win->modes[main_win->num_modes - 1] == TEXT_EDIT) break; /\* Text Edit mode blocks all other modes *\/ */
		    /* if (!input_fn) { */
		    /* 	for (int i=main_win->num_modes - 1; i>=0; i--) { */
		    /* 	    input_fn = input_get(main_win->i_state, e.key.keysym.sym); */
		    /* 	    if (input_fn) { */
		    /* 		break; */
		    /* 	    } */
		    /* 	} */
		    /* } */
		    /* fprintf(stdout, "Input fn? %p, do fn? %p\n", input_fn, input_fn->do_fn); */
		    if (input_fn && input_fn->do_fn) {
			input_fn->do_fn(NULL);
			char *keycmd_str = input_get_keycmd_str(main_win->i_state, e.key.keysym.sym);
			status_set_callstr(keycmd_str);
			free(keycmd_str);
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
		    /* proj->play_speed = 0; */
		    /* proj->src_play_speed = 0; */
		    /* transport_stop_playback(); */
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
		wheel_event_recency = 0;
		Layout *modal_scrollable = NULL;
		if ((modal_scrollable = mouse_triage_wheel(e.wheel.x * TL_SCROLL_STEP_H, e.wheel.y * TL_SCROLL_STEP_V, fingersdown))) {
		    temp_scrolling_lt = modal_scrollable;
		} else if (main_win->modes[main_win->num_modes - 1] == TIMELINE || main_win->modes[main_win->num_modes - 1] == TABVIEW) {
		    if (main_win->i_state & I_STATE_SHIFT) {
			if (main_win->i_state & I_STATE_CMDCTRL)
			    /* if (main_win->i_state & I_STATE_META) { */
			    /* 	Timeline *tl = proj->timelines[0]; */
			    /* 	Track *track = tl->tracks[0]; */
			    /* 	double current_cutoff = track->fir_filter->cutoff_freq; */
			    /* 	FilterType type = track->fir_filter->type; */
			    /* 	double filter_adj = 0.001; */
			    /* 	filter_set_params(track->fir_filter, type, current_cutoff + filter_adj * e.wheel.y, 0.05); */
			    /* } else  */
			    proj->play_speed += e.wheel.y * PLAYSPEED_ADJUST_SCALAR_LARGE;
			else 
			    proj->play_speed += e.wheel.y * PLAYSPEED_ADJUST_SCALAR_SMALL;
			
			status_stat_playspeed();
		    } else {
			bool allow_scroll = true;
			double scroll_x = e.wheel.preciseX * LAYOUT_SCROLL_SCALAR;
			double scroll_y = e.wheel.preciseY * LAYOUT_SCROLL_SCALAR;
			if (SDL_PointInRect(&main_win->mousep, proj->audio_rect)) {
			    if (main_win->i_state & I_STATE_CMDCTRL) {
				double scale_factor = pow(SFPP_STEP, e.wheel.y);
				timeline_rescale(scale_factor, true);
				allow_scroll = false;
			    } else if (fabs(scroll_x) > fabs(scroll_y)) {
				timeline_scroll_horiz(TL_SCROLL_STEP_H * e.wheel.x);
			    } else if (allow_scroll) {
				temp_scrolling_lt = proj->timelines[proj->active_tl_index]->track_area;
				layout_scroll(proj->timelines[proj->active_tl_index]->track_area, 0, scroll_y, fingersdown);
			    }
			    /* temp_scrolling_lt = proj->timelines[proj->active_tl_index]->track_area; */
			    /* temp_scrolling_lt->scroll_momentum_v = scroll_y; */
			}
			timeline_reset(proj->timelines[proj->active_tl_index]);
			/* layout_reset(proj->timelines[proj->active_tl_index]->track_area); */
			/* layout_reset(proj->track_area); */
			/* if (allow_scroll) { */
			/*     if (fabs(scroll_x) > fabs(scroll_y)) { */
			/* 	scroll_y = 0; */
			/*     } else { */
			/* 	scroll_x = 0; */
			/*     } */
			/*     temp_scrolling_lt = layout_handle_scroll( */
			/* 	main_win->layout, */
			/* 	&main_win->mousep, */
			/* 	scroll_x, */
			/* 	scroll_y, */
			/* 	fingersdown); */
			/*     timeline_reset(proj->timelines[proj->active_tl_index]); */
			/* } */
		    }
		}
	    }
		break;
	    }
	    case SDL_MOUSEBUTTONDOWN:
		if (e.button.button == SDL_BUTTON_LEFT) {
		    main_win->i_state |= I_STATE_MOUSE_L;
		} else if (e.button.button == SDL_BUTTON_RIGHT) {
		    main_win->i_state |= I_STATE_MOUSE_R;
		}
		switch(TOP_MODE) {
		case TIMELINE:
		    /* if (!mouse_triage_click_page() && !mouse_triage_click_tabview()) */
			mouse_triage_click_project(e.button.button);
		    break;
		case MENU_NAV:
		    mouse_triage_click_menu(e.button.button);
		    break;
		case MODAL:
		    mouse_triage_click_modal(e.button.button);
		    break;
		case TEXT_EDIT:
		    mouse_triage_click_text_edit(e.button.button);
		    break;
		case TABVIEW:
		    if (!mouse_triage_click_tabview())
			mouse_triage_click_page();
		    break;
		default:
		    break;
		}
		break;
	    case SDL_MOUSEBUTTONUP:
		if (e.button.button == SDL_BUTTON_LEFT) {
		    main_win->i_state &= ~I_STATE_MOUSE_L;
		    proj->dragged_component.component = NULL;
		} else if (e.button.button == SDL_BUTTON_RIGHT) {
		    main_win->i_state &= ~I_STATE_MOUSE_R;
		}
		/* if (proj->currently_editing_slider) { */
		/*     EventFn undofn; */
		/*     EventFn redofn; */
		/*     switch (proj->cached_slider_type) { */
		/*     case SLIDER_TARGET_TRACK_VOL: */
		/* 	undofn = undo_set_track_vol; */
		/* 	redofn = redo_set_track_vol; */
		/* 	break; */
		/*     case SLIDER_TARGET_TRACK_PAN: */
		/* 	undofn = undo_set_track_pan; */
		/* 	redofn = redo_set_track_pan; */
		/* 	break; */
		/*     case SLIDER_TARGET_TRACK_EFFECT_PARAM: */
		/* 	undofn = undo_set_track_effect_param; */
		/* 	redofn = redo_set_track_effect_param; */
		/* 	break; */
		/*     } */
		/*     Slider *s = proj->currently_editing_slider; */
		/*     Value v_old = proj->cached_slider_val; */
		/*     Value v_new = jdaw_val_from_ptr(s->value, s->val_type); */
		/*     user_event_push( */
		/* 	&proj->history, */
		/* 	undofn, */
		/* 	redofn, */
		/* 	NULL, NULL, */
		/* 	proj->currently_editing_slider->value, */
		/* 	NULL, */
		/* 	v_old, v_old, v_new, v_new, */
		/* 	s->val_type, s->val_type, false, false); */
			
		/*     proj->currently_editing_slider = NULL; */
		/* } */
		break;
	    case SDL_FINGERUP:
		fingersdown = SDL_GetNumTouchFingers(-1);
		if (fingersdown == 0 && wheel_event_recency < 2)
		    scrolling_lt = temp_scrolling_lt;
		/* } else { */
		/*     scrolling_lt = NULL; */
		/* } */
		break;
	    case SDL_FINGERDOWN:
	        fingersdown = SDL_GetNumTouchFingers(-1);
		if (scrolling_lt && wheel_event_recency >= 2) {
		    layout_halt_scroll(scrolling_lt);
		    scrolling_lt = NULL;
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

	wheel_event_recency++;
	if (wheel_event_recency == INT_MAX)
	    wheel_event_recency = 0;
	if (scrolling_lt) {
	    if (animate_step % 1 == 0) {
		/* fingersdown = SDL_GetNumTouchFingers(-1); */
		if (layout_scroll_step(scrolling_lt) == 0) {
		    /* scrolling_lt->iterator->scroll_momentum = 0; */
		    scrolling_lt = NULL;
		} else if (fingersdown > 0) {
		    /* scrolling_lt->iterator->scroll_momentum = 0; */
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

	if (main_win->txt_editing) {
	    if (main_win->txt_editing->cursor_countdown == 0) {
		main_win->txt_editing->cursor_countdown = CURSOR_COUNTDOWN_MAX;
	    } else {
		main_win->txt_editing->cursor_countdown--;
	    }
	}
	/* if (proj->recording) { */
	/*     transport_recording_update_cliprects(); */
	/* } */

	if (proj->recording) {
	    transport_recording_update_cliprects();
	}
	status_frame();
	
	project_draw();


	if (main_win->screenrecording) {
	    screenshot_loop();
	}
	/* window_draw_menus(main_win); */
	
	/***** Debug only *****/
	/* layout_draw(main_win, main_win->layout); */
	/**********************/


	Timeline *tl = proj->timelines[proj->active_tl_index];
	for (uint8_t i=0; i<tl->num_tracks; i++) {
	    Track *track = tl->tracks[i];
	    for (uint8_t ai=0; ai<track->num_automations; ai++) {
		Automation *a = track->automations[ai];
		if (a->write) {
		    if (!a->current) a->current = automation_get_segment(a, tl->play_pos_sframes);
		    int32_t frame_dur = proj->sample_rate * proj->play_speed / 60.0;
		    automation_do_write(a, tl->play_pos_sframes, tl->play_pos_sframes + frame_dur, proj->play_speed);
		    /* automation_insert_maybe(a, a->current, a->target_val, tl->play_pos_sframes, tl->play_pos_sframes + proj->play_speed * 10, 1); */

		}
	    }
	}
	

	
	/* window_end_draw(main_win); */
	/**********************************************/

	SDL_Delay(1);


        /* end = clock(); */
	/* fps += (float)CLOCKS_PER_SEC / (end - start); */
	/* start = end; */
	/* if (frame_ctr > 250) { */
	/*     fps /= 250; */
	/*     fprintf(stdout, "FPS: %f\n", fps); */
	/*     frame_ctr = 0; */
	/* } else { */
	/*     frame_ctr++; */
	/* } */
    }
}
