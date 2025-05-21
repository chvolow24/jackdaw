/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

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
/* #include "dsp.h" */
#include "eq.h"
#include "fir_filter.h"
#include "function_lookup.h"
#include "input.h"
#include "layout.h"
#include "modal.h"
#include "mouse.h"
#include "page.h"
#include "panel.h"
#include "project.h"
#include "project_endpoint_ops.h"
#include "project_draw.h"
#include "saturation.h"
#include "screenrecord.h"
#include "settings.h"
#include "status.h"
#include "tempo.h"
#include "timeline.h"
#include "transport.h"
#include "user_event.h"
#include "value.h"
#include "waveform.h"
#include "window.h"

#define MAX_MODES 8
#define STICK_DELAY_MS 500

extern Window *main_win;
extern SDL_Color color_global_black;
extern SDL_Color color_global_white;
extern SDL_Color color_global_grey;

extern SDL_Color freq_L_color;
extern SDL_Color freq_R_color;

extern Project *proj;

extern pthread_t DSP_THREAD_ID;

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

void user_global_quit(void *);
void loop_project_main()
{
    /* clock_t start, end; */
    /* uint8_t frame_ctr = 0; */
    /* float fps = 0; */

    /* layout_force_reset(proj->layout); */
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
    int play_speed_scroll_recency = 60;
    bool scrub_block = false;
    main_win->current_event = &e;
    while (!(main_win->i_state & I_STATE_QUIT)) {
	while (SDL_PollEvent(&e)) {
	    /* fprintf(stdout, "Polled!\n"); */
	    switch (e.type) {
	    case SDL_QUIT:
		/* proj->quit_count++; */
		/* if (proj->quit_count > 1) { */
		/*     main_win->i_state |= I_STATE_QUIT; */
		/* } else if (proj->quit_count == 0) { */
		    user_global_quit(NULL);
		/* } else { */
		/*     proj->quit_count++; */
		/* } */
		/* main_win->i_state |= I_STATE_QUIT; */
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

		
		/* static double bandwidth_scalar = 0.4; */
		/* static double freq_raw = 0.253; */
	        /* static double amp_raw = 2.0; */

		/* if (main_win->i_state & I_STATE_MOUSE_L) { */
		/*     if (main_win->i_state * I_STATE_CMDCTRL) { */
		/* 	/\* bandwidth_scalar += (double)e.motion.yrel / 100; *\/ */
		/* 	eq_set_filter_from_mouse(&glob_eq, filter_selector, main_win->mousep); */
		/* 	/\* eq_set_peak(&glob_eq, filter_selector, freq_raw, amp_raw, bandwidth_scalar * freq_raw); *\/ */
		/* 	break; */
		/*     } */
		/*     /\* freq_raw = waveform_freq_plot_freq_from_x_abs(glob_eq.fp, e.motion.x * main_win->dpi_scale_factor); *\/ */
		/*     /\* amp_raw = waveform_freq_plot_amp_from_x_abs(glob_eq.fp, e.motion.y * main_win->dpi_scale_factor, 0, true); *\/ */
		/*     eq_set_filter_from_mouse(&glob_eq, filter_selector, main_win->mousep); */
		/* } */
                /* eq_set_peak(&glob_eq, filter_selector, freq_raw, amp_raw, bandwidth_scalar * freq_raw); */
		/* glob_eq.ctrls[filter_selector].x = e.motion.x * main_win->dpi_scale_factor; */
		/* glob_eq.ctrls[filter_selector].y = e.motion.y * main_win->dpi_scale_factor; */
		
		/* if (eqfp) { */
		/*     double bandwidth = freq_raw * bandwidth_scalar; */
		/*     if (main_win->i_state & I_STATE_CMDCTRL) { */
		/* 	bandwidth_scalar += (double)e.motion.yrel / 100; */
		/* 	if (bandwidth_scalar < 0 ) bandwidth_scalar = 0.0; */
		/* 	fprintf(stderr, "BW SCALAR: %f\n", bandwidth_scalar); */
		/* 	iir_set_coeffs_peaknotch(&iir, freq_raw, amp_raw, bandwidth); */
			
		/* 	break; */
		/*     } */

		/*     freq_raw = waveform_freq_plot_freq_from_x_abs(eqfp, e.motion.x * main_win->dpi_scale_factor); */

		/*     amp_raw = waveform_freq_plot_amp_from_x_abs(eqfp, e.motion.y * main_win->dpi_scale_factor, 0, true); */
		/*     for (int channel =0; channel < 2; channel++) { */
		/* 	if (!inited) { */
		/* 	    fprintf(stderr, "INITING CHANNEL %d, filter at %p\n", channel, &iir); */
		/* 	    /\* double A[] = {1, -1.9781447870471984, 0.9978788059141444}; *\/ */
		/* 	    /\* double B[] = {1.918821765339674, -0.9379639763484257}; *\/ */
		/* 	    iir_init(&iir, 2, 2); */
		/* 	    /\* iir_set_coeffs(iir, A, B); *\/ */
		/* 	    /\* iir_set_coeffs_peaknotch(iir, 0.04, 50.0, 0.001); *\/ */
		/* 	    inited = 1; */
		/* 	} */
		/*     } */

		/*     double xprop = 2.0 * e.motion.x / main_win->w_pix; */
		/*     double yprop = 2.0 * e.motion.y / main_win->h_pix; */

		/*     xprop = pow(xprop, 4.0); */
		/*     yprop  = 1 - yprop; */
		/*     yprop *= 20; */
		/*     yprop -= 1; */

		
		/*     if (yprop < 0.0) { */
		/* 	/\* bandwidth *= 10; *\/ */
		/* 	yprop = 0.0; */
		/*     } */
		    
		/*     /\* fprintf(stderr, "%f, %f\n", xprop, yprop); *\/ */
		/*     if (freq_raw < 0) freq_raw = 0.0; */
		/*     if (freq_raw > 1.0) freq_raw = 1.0; */
		/*     if (amp_raw < 0.0) amp_raw = 0.0; */
		/*     iir_set_coeffs_peaknotch(&iir, freq_raw, amp_raw, bandwidth); */
		/* } */
		

		switch (TOP_MODE) {
		case MODAL:
		    mouse_triage_motion_modal();
		    break;
		case MENU_NAV:
		    mouse_triage_motion_menu();
		    break;
		case AUTOCOMPLETE_LIST:
		    mouse_triage_motion_autocompletion();
		    break;
		case TIMELINE:
		    if (!mouse_triage_motion_page() && !mouse_triage_motion_tabview()) {
			mouse_triage_motion_timeline(e.motion.xrel, e.motion.yrel);
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
		break;
	    case SDL_TEXTINPUT:
		if (main_win->txt_editing) {
		    txt_input_event_handler(main_win->txt_editing, &e);
		}
		break;
	    case SDL_KEYDOWN: {
		scrolling_lt = NULL;
		temp_scrolling_lt = NULL;
		switch (e.key.keysym.scancode) {
		/* case SDL_SCANCODE_6: { */
		/*     do_interpolation = !do_interpolation; */
		/* } */
		/*     break; */
		/* case SDL_SCANCODE_6: { */
		/*     Timeline *tl = proj->timelines[proj->active_tl_index]; */
		/*     Track* sel = timeline_selected_track(tl); */
		/*     Effect *e = track_add_effect(sel, i); */
		/*     fprintf(stderr, "Applying %s\n", effect_type_str(i)); */
		/*     /\* Saturation *s = e->obj; *\/ */
		/*     /\* s->active = true; *\/ */
		/*     /\* saturation_set_gain(s, 20.0); *\/ */
		/*     i++; */
		/*     i%=4; */
		    
		/* } */
		/*     break; */
		/* case SDL_SCANCODE_6: { */
		/*     Timeline *tl = proj->timelines[proj->active_tl_index]; */
		/*     Track* sel = timeline_selected_track(tl); */
		/*     track_set_bus_out(sel, tl->tracks[0]); */
		/* } */
		/*     break; */
		/* case SDL_SCANCODE_6: { */
		/*     filter_selector++; */
		/*     filter_selector %=4; */
		/* } */
		    /* break; */
		/* case SDL_SCANCODE_6: { */
		/*     ClipRef *cr = clipref_at_cursor(); */
		/*     if (cr) { */
		/* 	FIRFilter *f = cr->track->effects[0]->obj; */
		/* 	Clip *c = cr->clip; */
		/* 	int32_t pos = cr->track->tl->play_pos_sframes - cr->pos_sframes; */
		/* 	filter_set_arbitrary_IR(f, c->L + pos, 2048); */
		/*     } */
		/* } */
		    break;
		/* case SDL_SCANCODE_6: { */
		/*     create_global_ac(); */
		    /* const char *words[] = {"a", "b", "c"}; */
		    /* static int word_i = 0; */

		    /* TEST_lookup_print_all_matches(words[word_i]); */
		    
		    /* word_i++; */
		    /* word_i %= 3; */
		/* } */
		/*     break;    */

		/* case SDL_SCANCODE_6: { */
		/*     Timeline *tl = proj->timelines[proj->active_tl_index]; */
		/*     ClipRef *cr = clipref_at_cursor(); */
		/*     if (cr) { */
		/* 	ClipRef *new1, *new2; */
		/* 	clipref_split_stereo_to_mono(cr, &new1, &new2); */
		/* 	/\* cr->clip->channels = 1; *\/ */
		/* 	/\* Clip *c = cr->clip; *\/ */
		/* 	/\* free(c->R); *\/ */
		/* 	/\* c->R = malloc(sizeof(float) * c->len_sframes); *\/ */
		/* 	/\* memcpy(c->R, c->L, sizeof(float) * c->len_sframes); *\/ */
		/* 	timeline_reset_full(tl); */
		/*     } */
		/* } */
		/*     break; */
		case SDL_SCANCODE_LGUI:
		case SDL_SCANCODE_RGUI:
		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_RCTRL:
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
	        case SDL_SCANCODE_K:
		    if (main_win->i_state & I_STATE_K) {
			break;
		    } else {
			set_i_state_k = true;
		    }
		    /* No break */
		default:
		    input_fn  = input_get(main_win->i_state, e.key.keysym.sym);
		    if (input_fn && input_fn->do_fn) {
			char *keycmd_str = input_get_keycmd_str(main_win->i_state, e.key.keysym.sym);
			status_set_callstr(keycmd_str);
			free(keycmd_str);
			status_cat_callstr(" : ");
			status_cat_callstr(input_fn->fn_display_name);
			input_fn->do_fn(NULL);
			/* timeline_reset(proj->timelines[proj->active_tl_index]); */
		    }
		    break;
		}
		break;
	    case SDL_KEYUP:
		scrolling_lt = NULL;
		temp_scrolling_lt = NULL;
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
		    project_flush_ongoing_changes(proj, JDAW_THREAD_MAIN);
		    project_flush_ongoing_changes(proj, JDAW_THREAD_DSP);
		    proj->playhead_do_incr = false;
		    /* stop_update_track_vol_pan(); */
		    break;
		}
		break;
	    case SDL_MOUSEWHEEL: {
		Timeline *tl = proj->timelines[proj->active_tl_index];
		tl->needs_redraw = true;
		if (proj->dragged_component.component) {
		    draggable_handle_scroll(&proj->dragged_component, e.wheel.x, e.wheel.y);
		    break;
		}
		wheel_event_recency = 0;
		Layout *modal_scrollable = NULL;
		if ((modal_scrollable = mouse_triage_wheel(e.wheel.x * TL_SCROLL_STEP_H, e.wheel.y * TL_SCROLL_STEP_V, fingersdown))) {
		    temp_scrolling_lt = modal_scrollable;
		} else if (main_win->modes[main_win->num_modes - 1] == TIMELINE || main_win->modes[main_win->num_modes - 1] == TABVIEW) {
		    if (main_win->i_state & I_STATE_SHIFT) {
			if (fabs(e.wheel.preciseY) > fabs(e.wheel.preciseX)) {
			    scrub_block = true;
			    timeline_play_speed_adj(e.wheel.preciseY);
			    if (!proj->playing) transport_start_playback();
			} else if (!scrub_block) {
			    play_speed_scroll_recency = 0;
			    if (!proj->playing) transport_start_playback();
			    Value old_speed = endpoint_safe_read(&proj->play_speed_ep, NULL);
			    if (main_win->i_state & I_STATE_CMDCTRL) {
				float new_speed = (old_speed.float_v + e.wheel.preciseX) / 2;
				endpoint_write(&proj->play_speed_ep, (Value){.float_v = new_speed}, true, true, true, false);				
			    } else {
				float new_speed = (old_speed.float_v + e.wheel.preciseX / 3.0) / 2;
				endpoint_write(&proj->play_speed_ep, (Value){.float_v = new_speed}, true, true, true, false);				
			    }

			    /* timeline_scroll_playhead(e.wheel.preciseX); */
			}
			/* if (main_win->i_state & I_STATE_CMDCTRL) */
			/*     /\* if (main_win->i_state & I_STATE_META) { *\/ */
			/*     /\* 	Timeline *tl = proj->timelines[0]; *\/ */
			/*     /\* 	Track *track = tl->tracks[0]; *\/ */
			/*     /\* 	double current_cutoff = track->fir_filter->cutoff_freq; *\/ */
			/*     /\* 	FilterType type = track->fir_filter->type; *\/ */
			/*     /\* 	double filter_adj = 0.001; *\/ */
			/*     /\* 	filter_set_params(track->fir_filter, type, current_cutoff + filter_adj * e.wheel.y, 0.05); *\/ */
			/*     /\* } else  *\/ */
			/*     proj->play_speed += e.wheel.y * PLAYSPEED_ADJUST_SCALAR_LARGE; */
			/* else  */
			/*     proj->play_speed += e.wheel.y * PLAYSPEED_ADJUST_SCALAR_SMALL; */
			
			/* status_stat_playspeed(); */
		    } else {
			bool allow_scroll = true;
			double scroll_x = e.wheel.preciseX * LAYOUT_SCROLL_SCALAR;
			double scroll_y = e.wheel.preciseY * LAYOUT_SCROLL_SCALAR;
			if (SDL_PointInRect(&main_win->mousep, proj->audio_rect)) {
			    if (main_win->i_state & I_STATE_CMDCTRL) {
				double scale_factor = pow(SFPP_STEP, e.wheel.y);
				timeline_rescale(tl, scale_factor, true);
				allow_scroll = false;
			    } else if (fabs(scroll_x) > fabs(scroll_y)) {
				timeline_scroll_horiz(tl, TL_SCROLL_STEP_H * e.wheel.x);
			    } else if (allow_scroll) {
				temp_scrolling_lt = tl->track_area;
				layout_scroll(tl->track_area, 0, scroll_y, fingersdown);
				timeline_reset(tl, false);
			    }
			    /* temp_scrolling_lt = proj->timelines[proj->active_tl_index]->track_area; */
			    /* temp_scrolling_lt->scroll_momentum_v = scroll_y; */
			}


			
			/* timeline_reset(proj->timelines[proj->active_tl_index]); */


			
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
		scrolling_lt = NULL;
		temp_scrolling_lt = NULL;
		if (e.button.button == SDL_BUTTON_LEFT) {
		    main_win->i_state |= I_STATE_MOUSE_L;
		} else if (e.button.button == SDL_BUTTON_RIGHT) {
		    main_win->i_state |= I_STATE_MOUSE_R;
		}
	    escaped_text_edit:
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
		case AUTOCOMPLETE_LIST:
		    mouse_triage_click_autocompletion();
		    break;
		case TEXT_EDIT:
		    if (!mouse_triage_click_text_edit(e.button.button)) {
			if (TOP_MODE == TEXT_EDIT) {
			    fprintf(stderr, "Error: text edit escaped improperly");
			    window_pop_mode(main_win);
			}
			goto escaped_text_edit;
		    }
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
		scrolling_lt = NULL;
		temp_scrolling_lt = NULL;
		if (e.button.button == SDL_BUTTON_LEFT) {
		    main_win->i_state &= ~I_STATE_MOUSE_L;
		    proj->dragged_component.component = NULL;
		    automation_unset_dragging_kf(proj->timelines[proj->active_tl_index]);
		} else if (e.button.button == SDL_BUTTON_RIGHT) {
		    main_win->i_state &= ~I_STATE_MOUSE_R;
		}
		project_flush_ongoing_changes(proj, JDAW_THREAD_MAIN);
		project_flush_ongoing_changes(proj, JDAW_THREAD_DSP);

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
		if (fingersdown == 0) {
		    if (wheel_event_recency < 2) {
			scrolling_lt = temp_scrolling_lt;
		    }
		    scrub_block = false;
		}
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
	Timeline *tl = proj->timelines[proj->active_tl_index];
	wheel_event_recency++;
	if (wheel_event_recency == INT_MAX)
	    wheel_event_recency = 0;
	play_speed_scroll_recency++;
	if (play_speed_scroll_recency == INT_MAX)
	    wheel_event_recency = 100;
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
		timeline_reset(tl, false);
	    }
	}
	
	first_frame = false;
	
	if (!scrub_block && fingersdown > 0 && play_speed_scroll_recency > 4 && play_speed_scroll_recency < 20) {
	    Value old_speed = endpoint_safe_read(&proj->play_speed_ep, NULL);
	    float new_speed = old_speed.float_v / 3.0;
	    if (fabs(new_speed) < 1E-6) {
		transport_stop_playback();
		play_speed_scroll_recency = 20;
		
	    } else {
		endpoint_write(&proj->play_speed_ep, (Value){.float_v = new_speed}, true, true, true, false);
	    }

	}
	
	if (proj->play_speed != 0 && !proj->source_mode) {
	    timeline_catchup(tl);
	    timeline_set_timecode(tl);
	    for (int i=0; i<tl->num_click_tracks; i++) {
		int bar, beat, subdiv;
		click_track_bar_beat_subdiv(tl->click_tracks[i], tl->play_pos_sframes, &bar, &beat, &subdiv, NULL, true);
	    }
	}

	if (animate_step == 255) {
	    animate_step = 0;
	} else {
	    animate_step++;
	}
        project_animations_do_frame();

	if (proj->playhead_do_incr) {
	    timeline_scroll_playhead(proj->playhead_frame_incr);
	}

	/* update_track_vol_pan(); */

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

	project_do_ongoing_changes(proj, JDAW_THREAD_MAIN);
	project_flush_val_changes(proj, JDAW_THREAD_MAIN);
	project_flush_callbacks(proj, JDAW_THREAD_MAIN);
	project_draw();


	if (main_win->screenrecording) {
	    screenshot_loop();
	}
	/* window_draw_menus(main_win); */
	
	/***** Debug only *****/
	/* layout_draw(main_win, main_win->layout); */
	/**********************/

	if (proj->playing) {
	    /* Timeline *tl = proj->timelines[proj->active_tl_index]; */
	    struct timespec now;
	    clock_gettime(CLOCK_MONOTONIC, &now);
	    double elapsed_s = now.tv_sec + ((double)now.tv_nsec / 1e9) - tl->play_pos_moved_at.tv_sec - ((double)tl->play_pos_moved_at.tv_nsec / 1e9);
	    if (elapsed_s > 0.05) {
		goto end_auto_write;
	    }
	    int32_t play_pos_adj = tl->play_pos_sframes + elapsed_s * proj->sample_rate * proj->play_speed;
	    for (uint8_t i=0; i<tl->num_tracks; i++) {
		Track *track = tl->tracks[i];
		for (uint8_t ai=0; ai<track->num_automations; ai++) {
		    Automation *a = track->automations[ai];
		    if (a->write) {
			/* if (!a->current) a->current = automation_get_segment(a, play_pos_adj); */
			int32_t frame_dur = proj->sample_rate * proj->play_speed / 30.0;
			Value val = endpoint_safe_read(a->endpoint, NULL);
			/* fprintf(stderr, "READ FLOATVAL (%s): %f\n", a->endpoint->local_id, val.float_v); */
			automation_do_write(a, val, play_pos_adj, play_pos_adj + frame_dur, proj->play_speed);
		    }
		    /* if (a->num_kclips > 0) { */
		    /* 	kclipref_move(a->kclips, 500); */
		    /* } */
		    /* TEST_automation_keyframe_order(a); */
		    /* TEST_kclipref_bounds(a); */
		}
		
	    }
	}

    end_auto_write:

	
	SDL_Delay(1);


	if (proj->do_tests) {
	    TEST_FN_CALL(chaotic_user, &proj->do_tests, 60 * 10); /* 10s of really dumb tests */
	}
    }
}
