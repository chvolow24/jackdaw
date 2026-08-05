/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2026 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    project_loop.c

    * (no header file)
    * main project animation loop
    * in-progress animations and updates
 *****************************************************************************************************************/


#include <time.h>
#include "SDL_video.h"
#include "audio_connection.h"
#include "audio_clip.h"
#include "automation.h"
#include "clipref.h"
#include "consts.h"
#include "context_menu.h"
#include "eq.h"
#include "fir_filter.h"
#include "function_lookup.h"
#include "input.h"
#include "io.h"
#include "layout.h"
#include "log.h"
#include "midi_clip.h"
#include "midi_qwerty.h"
#include "mouse.h"
#include "piano_roll.h"
#include "project.h"
#include "project_draw.h"
#include "route.h"
#include "screenrecord.h"
#include "session_endpoint_ops.h"
#include "session.h"
#include "settings.h"
#include "synth.h"
#include "tempo.h"
#include "thread_safety.h"
#include "timeline.h"
#include "timeview.h"
#include "transport.h"
#include "window.h"

#define MAX_MODES 8
#define STICK_DELAY_MS 500

#define IDLE_AFTER_N_FRAMES 1000

extern Window *main_win;

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

/* TabView *synth_tabviewc_create(Track *track); */

extern void user_global_quit(void *);
/* extern bool do_blep; */

void route_page_open(Track *track);

void user_tl_track_selector_up(void *nullarg);
void user_tl_track_selector_down(void *nullarg);

#define EVENT_MUTATES_STATE(type) \
    (type == SDL_KEYDOWN || type == SDL_KEYUP || type == SDL_MOUSEBUTTONUP || type == SDL_DROPFILE || type == SDL_MOUSEWHEEL)

void handle_window_events(SDL_Event e, Window *win)
{
    if (e.window.event == SDL_WINDOWEVENT_RESIZED || e.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
        int w, h;
        if (e.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
            SDL_GetWindowSize(main_win->win, &w, &h);
        } else {
            w = e.window.data1;
            h = e.window.data2;
        }
        main_win->needs_redraw = true;
        window_resize_passive(main_win, w, h);
        main_win->needs_redraw = true;
    } else if (e.window.event == SDL_WINDOWEVENT_DISPLAY_CHANGED) {
        int rw = 0, rh = 0, ww = 0, wh = 0;
        SDL_GetWindowSize(main_win->win, &ww, &wh);
        SDL_GetRendererOutputSize(main_win->rend, &rw, &rh);

        double new_dpi = (double) rw / ww;
        log_tmp(LOG_INFO, "Window display changed. DPI %f => %f\n", main_win->dpi_scale_factor, new_dpi);
        main_win->dpi_scale_factor = new_dpi;

        window_check_monitor_dpi(main_win);
    } else if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
        main_win->focused = true;
    } else if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
        main_win->focused = false;
    }
}

void loop_project_main()
{
    Session *session = session_get();
    Layout *temp_scrolling_lt = NULL;
    Layout *scrolling_lt = NULL;
    UserFn *input_fn = NULL;
    
    SDL_Event e;
    uint8_t fingersdown = 0;
    
    uint8_t animate_step = 0;
    bool set_i_state_k = false;
    bool set_i_state_g = false;

    bool first_frame = true;
    int wheel_event_recency = 0;
    int play_speed_scroll_recency = 60;
    bool scrub_block = false;
    int frames_since_event = 0;

    float pitch_bend = 0.0f;
    bool set_pitch_bend = false;
    
    main_win->current_event = &e;
    while (!(main_win->i_state & I_STATE_QUIT)) {
	while (SDL_PollEvent(&e)) {            
	    frames_since_event = 0;
	    switch (e.type) {
	    case SDL_QUIT:
		user_global_quit(NULL);
		break;
	    case SDL_WINDOWEVENT:
                handle_window_events(e, main_win);
		break;
	    case SDL_AUDIODEVICEADDED:
		if (!first_frame) {
		    audioconn_handle_connection_event(e.adevice.which, e.adevice.iscapture, e.adevice.type);
		}
		break;
	    case SDL_AUDIODEVICEREMOVED:
		if (!first_frame) {
		    /*
		      'which' is the id for AUDIODEVICEREMOVED, which makes it impossible
		      to determine which device removed when it has not been opened/
		      https://discourse.libsdl.org/t/sdl-audiodeviceevent-cant-determine-which-removed-sdl2/51613
		    */
		    audioconn_handle_disconnection_event(e.adevice.which, e.adevice.iscapture, e.adevice.type);
		}
		break;
	    case SDL_MOUSEMOTION: {
		window_set_mouse_point(main_win, e.motion.x, e.motion.y);
		if (session->dragged_component.component) {
		    draggable_mouse_motion(&session->dragged_component, main_win);
		    break;
		}
		switch (TOP_MODE) {
		case MODE_MODAL:
		    if (session->dragged_component.component) {
			draggable_mouse_motion(&session->dragged_component, main_win);
		    } else {
			mouse_triage_motion_modal();
		    }
		    break;
		case MODE_MENU_NAV:
		    mouse_triage_motion_menu();
		    break;
		case MODE_AUTOCOMPLETE_LIST:
		    mouse_triage_motion_autocompletion();
		    break;
		case MODE_TIMELINE:
		    if (!mouse_triage_motion_page() && !mouse_triage_motion_tabview()) {
			mouse_triage_motion_timeline(e.motion.xrel, e.motion.yrel);
		    }
		case MODE_TABVIEW:
		    if (!mouse_triage_motion_tabview())
			mouse_triage_motion_page();
		    break;
		case MODE_PIANO_ROLL:
		    piano_roll_mouse_motion(main_win->mousep);
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
                main_win->needs_redraw = true;
		break;
	    case SDL_KEYDOWN: {
		scrolling_lt = NULL;
		temp_scrolling_lt = NULL;
		switch (e.key.keysym.scancode) {
                case SDL_SCANCODE_6: {
                    Ctx *arr = NULL;
                    int num_ctxs = context_at_cursor(&arr);
                    fprintf(stderr, "\n");
                    for (int i=0; i<num_ctxs; i++) {
                        fprintf(stderr, "%d: %s (%p) %s\n", i, context_type_name(arr[i].type), arr[i].obj, arr[i].name);
                    }
                    context_menu_create(num_ctxs);
                }
                    break;
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
		/* K and G fall through to default input handling */
	        case SDL_SCANCODE_K:
		    if (TOP_MODE != MODE_MIDI_QWERTY) {
			if (main_win->i_state & I_STATE_K) {
			    break;
			} else {
			    set_i_state_k = true;
			}
		    }
		    /* No break! */
		case SDL_SCANCODE_G:
		    if (!set_i_state_k && TOP_MODE != MODE_MIDI_QWERTY) {
			if (main_win->i_state & I_STATE_G) {
			    break;
			} else {
			    set_i_state_g = true;
			}
		    }
		    /* No break! */
		default:
		    input_fn = input_get(main_win->i_state, e.key.keysym.sym);
		    if (input_fn && input_fn->do_fn) {
			char *keycmd_str = input_get_keycmd_str(main_win->i_state, e.key.keysym.sym);
			log_tmp(LOG_INFO, "USERFN %s (%s)\n", input_fn->fn_id, keycmd_str);
			status_set_callstr(keycmd_str);
			free(keycmd_str);
			status_cat_callstr(" : ");
			status_cat_callstr(input_fn->fn_display_name);
			input_fn->do_fn(NULL);
			if (input_fn->bound_button) {
			    button_press_color_change(
				input_fn->bound_button,
				input_fn->bound_button->pressed_color,
				input_fn->bound_button->return_color);

			}
                        main_win->needs_redraw = true;
			/* timeline_reset(ACTIVE_TL); */
		    }
		    break;
		}
		break;
	    case SDL_KEYUP:
		scrolling_lt = NULL;
		temp_scrolling_lt = NULL;
		if (session->midi_qwerty) {
		    mqwert_handle_key(e.key.keysym.sym, true);
		}
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
		    break;
		case SDL_SCANCODE_G:
		    main_win->i_state &= ~I_STATE_G;
		    break;
		case SDL_SCANCODE_J:
		case SDL_SCANCODE_L:
		    if (main_win->i_state & I_STATE_K) {
			session->playback.play_speed = 0;
			session->source_mode.src_play_speed = 0;
			transport_stop_playback();
		    }
		    break;
		default:
		    session_flush_ongoing_changes(session, JDAW_THREAD_MAIN);
		    session_flush_ongoing_changes(session, JDAW_THREAD_DSP);
		    session_flush_ongoing_changes(session, JDAW_THREAD_PLAYBACK);
		    session->playhead_scroll.playhead_do_incr = false;
		    break;
		}
		break;
	    case SDL_MOUSEWHEEL: {
		Timeline *tl = ACTIVE_TL;
		main_win->needs_redraw = true;
		if (session->dragged_component.component) {
		    draggable_handle_scroll(&session->dragged_component, e.wheel.x, e.wheel.y);
		    break;
		}
		wheel_event_recency = 0;
		Layout *modal_or_page_scrollable = NULL;
		if ((modal_or_page_scrollable = mouse_triage_wheel(e.wheel.preciseX * LAYOUT_SCROLL_SCALAR, e.wheel.preciseY * LAYOUT_SCROLL_SCALAR, fingersdown))) {
		    temp_scrolling_lt = modal_or_page_scrollable;
		} else if (TOP_MODE == MODE_MIDI_QWERTY) {
		    pitch_bend += e.wheel.preciseY * 10.0;
		    set_pitch_bend = true;
		} else if (TOP_MODE == MODE_TIMELINE || TOP_MODE == MODE_TABVIEW || TOP_MODE == MODE_PIANO_ROLL || TOP_MODE == MODE_MIDI_QWERTY) {
		    if (main_win->i_state & I_STATE_SHIFT) {
			if (fabs(e.wheel.preciseY) > fabs(e.wheel.preciseX)) {
			    scrub_block = true;
			    timeline_play_speed_adj(e.wheel.preciseY);
			    if (!session->playback.playing) transport_start_playback();
			} else if (!scrub_block) {
			    play_speed_scroll_recency = 0;
			    if (!session->playback.playing) transport_start_playback();
			    Value old_speed = endpoint_safe_read(&session->playback.play_speed_ep, NULL);
			    if (main_win->i_state & I_STATE_CMDCTRL) {
				float new_speed = (old_speed.float_v + e.wheel.preciseX) / 2;
				endpoint_write(&session->playback.play_speed_ep, (Value){.float_v = new_speed}, true, true, true, false);
			    } else if (main_win->i_state & I_STATE_META) {
				float new_speed = (old_speed.float_v + e.wheel.preciseX / 20.0) / 2;
				endpoint_write(&session->playback.play_speed_ep, (Value){.float_v = new_speed}, true, true, true, false);				
			    } else {
				float new_speed = (old_speed.float_v + e.wheel.preciseX / 3.0) / 2;
				endpoint_write(&session->playback.play_speed_ep, (Value){.float_v = new_speed}, true, true, true, false);				
			    }
			}
		    } else {
			bool allow_scroll = true;
			double scroll_x = e.wheel.preciseX * LAYOUT_SCROLL_SCALAR;
			double scroll_y = e.wheel.preciseY * LAYOUT_SCROLL_SCALAR;
			if (SDL_PointInRect(&main_win->mousep, session->gui.audio_rect) || SDL_PointInRect(&main_win->mousep, session->gui.console_column_rect)) {
			    if (main_win->i_state & I_STATE_CMDCTRL) {
				double scale_factor = pow(SFPP_STEP, e.wheel.y);
				timeline_rescale(tl, scale_factor, true);
				allow_scroll = false;
			    } else if (main_win->i_state & I_STATE_META) {
				/* Scroll cursor position */
				static int frames_since_last = 0;
				if (fabs(e.wheel.preciseX) > fabs(e.wheel.preciseY)) {
				    int32_t new_pos = tl->play_pos_sframes + e.wheel.preciseX * tl->timeview.sample_frames_per_pixel * 10;
				    timeline_set_play_position(tl, new_pos, true);
				} else if (fabs(e.wheel.preciseY) > 0.8 && frames_since_last > 2) {
				    bool up = e.wheel.preciseY > 0;
				    if (up) {
					user_tl_track_selector_up(NULL);
				    } else {
					user_tl_track_selector_down(NULL);
				    }
				    frames_since_last = 0;
				} else {
				    frames_since_last++;
				}
			    } else if (fabs(scroll_x) > fabs(scroll_y)) {
				timeline_scroll_horiz(tl, TL_SCROLL_STEP_H * e.wheel.x);
			    } else if (allow_scroll) {
				temp_scrolling_lt = tl->track_area;
				layout_scroll(tl->track_area, 0, scroll_y, fingersdown);
				timeline_reset(tl, false);
			    }
			}
		    }
	        } else if (session->piano_roll) {
		    piano_roll_move_note_selector(e.wheel.preciseY < 0.0 ? floor(e.wheel.preciseY) : ceil(e.wheel.preciseY));
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
		mouse_triage_click(e);
                main_win->needs_redraw = true;
		break;
	    case SDL_MOUSEBUTTONUP:
		scrolling_lt = NULL;
		temp_scrolling_lt = NULL;
		if (e.button.button == SDL_BUTTON_LEFT) {
		    main_win->i_state &= ~I_STATE_MOUSE_L;
		    session->dragged_component.component = NULL;
		    automation_unset_dragging_kf(ACTIVE_TL);
		} else if (e.button.button == SDL_BUTTON_RIGHT) {
		    main_win->i_state &= ~I_STATE_MOUSE_R;
		}
		session_flush_ongoing_changes(session, JDAW_THREAD_MAIN);
		session_flush_ongoing_changes(session, JDAW_THREAD_DSP);
		session_flush_ongoing_changes(session, JDAW_THREAD_PLAYBACK);
		if (session->piano_roll) {
		    piano_roll_mouse_up(main_win->mousep);
		}
                main_win->needs_redraw = true;
		break;
	    case SDL_FINGERUP:
		fingersdown = SDL_GetNumTouchFingers(-1);
		if (fingersdown == 0) {
		    if (wheel_event_recency < 2) {
			scrolling_lt = temp_scrolling_lt;
		    }
		    scrub_block = false;
		}
                main_win->needs_redraw = true;
		break;
	    case SDL_FINGERDOWN:
	        fingersdown = SDL_GetNumTouchFingers(-1);
		if (scrolling_lt && wheel_event_recency >= 2) {
		    layout_halt_scroll(scrolling_lt);
		    scrolling_lt = NULL;
		}
                main_win->needs_redraw = true;
		break;
	    case SDL_DROPFILE: {
		Timeline *tl = ACTIVE_TL;
		int x, y;
		SDL_GetMouseState(&x, &y);
		window_set_mouse_point(main_win, x, y);
		for (int i=0; i<tl->num_tracks; i++) {
		    if (SDL_PointInRect(&main_win->mousep, &tl->tracks[i]->layout->rect)) {
			timeline_select_track(tl->tracks[i]);
		    }
		}
		int32_t pos = timeview_get_pos_sframes(&tl->timeview, main_win->mousep.x);
		timeline_set_play_position(tl, pos, false);
		io_open_file(e.drop.file, IO_FILE_TYPE_UNDETERMINED, timeline_selected_track(ACTIVE_TL), pos);
		SDL_free(e.drop.file);
                main_win->needs_redraw = true;
	    }
		break;		
	    default:
		break;
	    }
	    if (set_i_state_k) {
		main_win->i_state |= I_STATE_K;
		set_i_state_k = false;
	    }
	    if (set_i_state_g) {
		main_win->i_state |= I_STATE_G;
		set_i_state_g = false;
	    }
            /* Any new event, check if unsaved changes */
            if (EVENT_MUTATES_STATE(e.type)) {
                session_check_reset_window_title();
            }
	} /* End event handling */



	Timeline *tl = ACTIVE_TL;
	if (!session->playback.playing && frames_since_event >= IDLE_AFTER_N_FRAMES) {
            /* fprintf(stderr, "IDLING!\n"); */
	    /* goto end_frame; */
	} else {
	    frames_since_event++;
	}	

	if (tl->needs_reset) {
	    timeline_reset(tl, false);
	    tl->needs_reset = false;
	}

	wheel_event_recency++;
	if (wheel_event_recency == INT_MAX)
	    wheel_event_recency = 0;
	play_speed_scroll_recency++;
	if (play_speed_scroll_recency == INT_MAX)
	    wheel_event_recency = 100;
	if (scrolling_lt) {
	    if (animate_step % 1 == 0) {
		if (layout_scroll_step(scrolling_lt) == 0) {
		    scrolling_lt = NULL;
		}
		timeline_reset(tl, false);
	    }
            main_win->needs_redraw = true;
	}
	
	first_frame = false;
	
	if (!scrub_block && fingersdown > 0 && play_speed_scroll_recency > 4 && play_speed_scroll_recency < 20) {
	    Value old_speed = endpoint_safe_read(&session->playback.play_speed_ep, NULL);
	    float new_speed = old_speed.float_v / 3.0;
            endpoint_write(&session->playback.play_speed_ep, (Value){.float_v = new_speed}, true, true, true, false);
	}	
	if (session->playback.playing && !session->source_mode.source_mode) {
	    timeline_catchup(tl);
	    timeline_set_timecode(tl);
	    /* Set click track clock displays */
	    for (int i=0; i<tl->num_click_tracks; i++) {
		click_track_set_readout(tl->click_tracks[i], tl->play_pos_sframes);
	    }
	} else if (session->source_mode.source_mode && fabs(session->source_mode.src_play_speed) > 1e-9) {
	    timeview_catchup(&session->source_mode.timeview);
	}

	if (animate_step == 255) {
	    animate_step = 0;
	} else {
	    animate_step++;
	}
        session_animations_do_frame();
	if (session->dragging) {
	    session->drag_color_pulse_phase++;
	    session->drag_color_pulse_phase %= DRAG_COLOR_PULSE_PHASE_MAX;
	    session->drag_color_pulse_prop = (sin(TAU * (double)session->drag_color_pulse_phase / DRAG_COLOR_PULSE_PHASE_MAX) + 1.0) / 2.0;
	    main_win->needs_redraw = true;
	}

	if (session->playhead_scroll.playhead_do_incr) {
	    timeline_scroll_playhead(session->playhead_scroll.playhead_frame_incr);
	}

	if (session->piano_roll) {
	    if (piano_roll_execute_queued_insertions()) {
		frames_since_event = 0;
	    }
	}
	if (set_pitch_bend) {
	    if (fingersdown < 2) {
		pitch_bend -= pitch_bend / 4.0;
	    }
	    if (fabs(pitch_bend) < 1e-6) {
		pitch_bend = 0.0f;
		set_pitch_bend = false;		
	    }
	    Synth *monitor_synth = NULL;
	    MIDIDevice *mdevice = NULL;
	    if ((monitor_synth = session->midi_io.monitor_synth) && (mdevice = session->midi_io.monitor_device)) {
		mqwert_set_pitch_bend(pitch_bend);
		/* synth_set_pitch_bend(session->midi_io.monitor_synth, pitch_bend); */
		
	    }
	}
	if (main_win->txt_editing) {
	    if (main_win->txt_editing->cursor_countdown == 0) {
		main_win->txt_editing->cursor_countdown = CURSOR_COUNTDOWN_MAX;
	    } else {
		main_win->txt_editing->cursor_countdown--;
	    }
            main_win->needs_redraw = true;
	}
	if (session->playback.recording) {
	    transport_recording_update_cliprects();
            main_win->needs_redraw = true;
	}
	if (status_frame()) {
            main_win->needs_redraw = true;
        }

	if (session_do_ongoing_changes(session, JDAW_THREAD_MAIN) > 0) {
            main_win->needs_redraw = true;
        }
	if (session_flush_val_changes(session, JDAW_THREAD_MAIN) > 0) {
            main_win->needs_redraw = true;
        }
	if (session_flush_callbacks(session, JDAW_THREAD_MAIN) > 0) {
            main_win->needs_redraw = true;
        }
	if (main_win->needs_redraw) {
	    set_clipref_at_cursor();
	}
        bool redrawn = main_win->needs_redraw;
	project_draw();


	if (main_win->screenrecording) {
	    screenshot_loop();
	}
	static const int zero_playspeed_count_thresh = 20;
        static int zero_playspeed_count = 0;
	if (session->playback.playing) {
	    if (tl->read_pos_sframes <= TL_MIN_SFRAMES || tl->read_pos_sframes >= TL_MAX_SFRAMES) {
		status_set_errstr("Reached end of timeline. S-u to return to t=0");
		transport_stop_playback();
	    }
	    if (!session->source_mode.source_mode && fabs(session->playback.play_speed) < 1e-3f) {
		zero_playspeed_count++;
	    } else {
		zero_playspeed_count = 0;
	    }
	    if (zero_playspeed_count > zero_playspeed_count_thresh) {
		transport_stop_playback();
		timeline_play_speed_set(0.0f);
	    }
	    
	    struct timespec now;
	    clock_gettime(CLOCK_MONOTONIC, &now);
	    double elapsed_s = now.tv_sec + ((double)now.tv_nsec / 1e9) - tl->play_pos_moved_at.tv_sec - ((double)tl->play_pos_moved_at.tv_nsec / 1e9);
	    if (elapsed_s > 0.05) {
		goto end_frame;
	    }
	    int32_t play_pos_adj = tl->play_pos_sframes + elapsed_s * session_get_sample_rate() * session->playback.play_speed;
	    for (uint8_t i=0; i<tl->num_tracks; i++) {
		Track *track = tl->tracks[i];
		for (uint8_t ai=0; ai<track->num_automations; ai++) {
		    Automation *a = track->automations[ai];
		    if (a->write) {
			int32_t frame_dur = session_get_sample_rate() * session->playback.play_speed / 30.0;
			Value val = endpoint_safe_read(a->endpoint, NULL);
			automation_do_write(a, val, play_pos_adj, play_pos_adj + frame_dur, session->playback.play_speed);
		    }
		}
		
	    }
	}

	if (session->do_tests) {
	    TEST_FN_CALL(chaotic_user, &session->do_tests, 60 * 10); /* 10s of really dumb tests */
	}

    end_frame:

	if ((!redrawn && frames_since_event >= IDLE_AFTER_N_FRAMES) || !main_win->focused) {
            fprintf(stderr, "IDLE\n");
	    SDL_Delay(200);
	} else {
	    SDL_Delay(1);
	}
    }
}
