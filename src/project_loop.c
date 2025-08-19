/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    project_loop.c

    * (no header file)
    * main project animation loop
    * in-progress animations and updates
 *****************************************************************************************************************/


#include <time.h>
#include "audio_connection.h"
#include "audio_clip.h"
#include "automation.h"
#include "clipref.h"
#include "eq.h"
#include "fir_filter.h"
#include "function_lookup.h"
#include "input.h"
#include "layout.h"
#include "midi_clip.h"
#include "midi_qwerty.h"
#include "mouse.h"
#include "project.h"
#include "project_draw.h"
#include "screenrecord.h"
#include "session_endpoint_ops.h"
#include "session.h"
#include "settings.h"
#include "tempo.h"
#include "timeline.h"
#include "transport.h"
#include "window.h"

#define MAX_MODES 8
#define STICK_DELAY_MS 500

#define IDLE_AFTER_N_FRAMES 100

extern Window *main_win;


extern Project *proj;

/* extern pthread_t DSP_THREAD_ID; */

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

TabView *synth_tabview_create(Track *track);
void user_global_quit(void *);
void timeline_add_jlily();

extern bool do_blep;
void loop_project_main()
{
    Session *session = session_get();
    /* clock_t start, end; */
    /* uint8_t frame_ctr = 0; */
    /* float fps = 0; */

    /* layout_force_reset(session->gui.layout); */
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
    int frames_since_event = 0;
    main_win->current_event = &e;
    while (!(main_win->i_state & I_STATE_QUIT)) {
	while (SDL_PollEvent(&e)) {
	    frames_since_event = 0;
	    switch (e.type) {
	    case SDL_QUIT:
		user_global_quit(NULL);
		break;
	    case SDL_WINDOWEVENT:
		if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
		    window_resize_passive(main_win, e.window.data1, e.window.data2);
		    ACTIVE_TL->needs_redraw = true;
		}
		break;
	    case SDL_AUDIODEVICEADDED:
	    case SDL_AUDIODEVICEREMOVED:
		if (!first_frame) {
		    if (session->playback.recording) transport_stop_recording();
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
		/* PmMessage m; */
		/* uint8_t status = main_win->i_state & I_STATE_SHIFT ? 0x80 : 0x90; */
		/* /\* status = status == 0x90 ? 0x80 : 0x90; *\/ */
		/* uint8_t velocity = 50; */
		/* uint8_t note = e.key.keysym.sym; */
		/* m = Pm_Message(status, note, velocity); */
		/* PmEvent pme; */
		/* pme.message = m; */
		/* pme.timestamp = 0; */
		/* if (session->midi_io.primary_output) { */
		/*     session->midi_io.primary_output->buffer[0] = pme; */
		/*     status = 0x80; */
		/*     m = Pm_Message(status, note, velocity); */
		/*     pme.message = m; */
		/*     session->midi_io.primary_output->buffer[1] = pme; */
		/*     PmError err = Pm_Write( */
		/* 	session->midi_io.primary_output->stream, */
		/* 	session->midi_io.primary_output->buffer, */
		/* 	2); */
		/*     if (err == TRUE) { */
		/* 	fprintf(stderr, "Sent! note %d, velocity %d, type %#xh\n", note, velocity, status); */
		/*     } else if (err == FALSE) { */
		/* 	fprintf(stderr, "uhhh note %d, velocity %d, type %#x\n", note, velocity, status); */
		/*     } else { */
		/* 	fprintf(stderr, "ERROR (%s) note %d, velocity %d, type %#x\n", Pm_GetErrorText(err), note, velocity, status); */
		/*     } */
			    
		/* } */

		switch (e.key.keysym.scancode) {
		case SDL_SCANCODE_2: {
		    /* synth_save_preset(); */
		/*     Timeline *tl = ACTIVE_TL; */
		/*     Track *t = timeline_selected_track(tl); */
		/*     if (t && t->synth) */
		/* 	synth_write_preset_file("synth_preset.jsynth", t->synth); */
		/*     /\* FILE *f = fopen("test_ser.txt", "w"); *\/ */
		/*     /\* api_node_serialize(f, &session->proj.timelines[0]->api_node); *\/ */
		/*     /\* fclose(f); *\/ */

		}
		    break;
		case SDL_SCANCODE_3: {
		    /* synth_open_preset(); */
		/*     Timeline *tl = ACTIVE_TL; */
		/*     Track *t = timeline_selected_track(tl); */
		/*     if (t && t->synth) */
		/* 	synth_read_preset_file("synth_preset.jsynth", t->synth); */

		/*     /\* FILE *f = fopen("test_ser.txt", "r"); *\/ */
		/*     /\* api_node_deserialize(f, &session->proj.timelines[0]->api_node); *\/ */
		/*     /\* fclose(f); *\/ */
		}
		    break;
		case SDL_SCANCODE_4:
		    do_blep = !do_blep;
		    if (do_blep) {
			fprintf(stderr, "DOING BLEP!\n");
		    } else {
			fprintf(stderr, "No blep...\n");
		    }
		    break;
		/* case SDL_SCANCODE_5: { */
		/*     breakfn(); */
		/*     Timeline *tl = ACTIVE_TL; */
		/*     Track *t = timeline_selected_track(tl); */
		/*     if (!t->synth) t->synth = synth_create(t); */
		/*     TabView *tv = synth_tabview_create(t); */
		/*     tabview_activate(tv); */

		/* } */
		/*     break; */
		case SDL_SCANCODE_5: {
		    timeline_add_jlily();
		}
		    break;
		/* case SDL_SCANCODE_6: { */
		/*     Timeline *tl = ACTIVE_TL; */
		/*     Track *track = timeline_selected_track(tl); */
		/*     if (!track) break; */
		/*     MIDIClip *mclip = midi_clip_create(NULL, track); */
		/*     int32_t start = 96000 * 5; */
		/*     /\* int32_t end = 10000; *\/ */
		/*     midi_clip_add_note(mclip, 0, 69, 108, 100, 100 + 96000); */
		/*     srand(time(NULL)); */
		/*     mclip->len_sframes = 96000 * 5; */
		/*     for (int i=0; i<900; i++) { */
		/* 	int32_t dur = rand() % 30000; */
		/* 	int32_t interval = rand() % 11 + 1; */
		/* 	int32_t t_interval = rand() % 5000 + 1000; */
		/* 	uint8_t velocity = rand() % 128; */
		/* 	for (int i=25; i<25 + 80; i+=interval) { */
		/* 	    interval += rand() % 4 - 2; */
		/* 	    if (interval <= 0) interval += rand() %4 + 1; */
		/* 	    midi_clip_add_note(mclip, 0, i, velocity, start, start + dur); */
		/* 	    start += t_interval; */
		/* 	    /\* end += 3000; *\/ */
		/* 	    /\* mclip->len_sframes += t_interval; *\/ */
		/* 	    mclip->len_sframes += t_interval; */
		/* 	} */
		/*     } */

		/*     /\* midi_clip_add_note(&mclip, 78, 127, 3, 1000); *\/ */
		/*     /\* midi_clip_add_note(&mclip, 85, 127, 3, 1000); *\/ */
		/*     /\* midi_clip_add_note(&mclip, 97, 127, 3, 1000); *\/ */
		/*     /\* midi_clip_add_note(&mclip, 96, 127, 1500, 6000); *\/ */
		/*     /\* midi_clip_add_note(&mclip, 95, 127, 6000, 10000); *\/ */

		/*     clipref_create( */
		/* 	track, */
		/* 	tl->play_pos_sframes, */
		/* 	CLIP_MIDI, */
		/* 	mclip); */
			
		/* } */
		/*     break; */
		/* case SDL_SCANCODE_6: { */
		/*     Timeline *tl = ACTIVE_TL; */
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
		/* 	Clip *c = cr->source_clip; */
		/* 	int32_t pos = cr->track->tl->play_pos_sframes - cr->tl_pos; */
		/* 	filter_set_arbitrary_IR(f, c->L + pos, 2048); */
		/*     } */
		/* } */
		/*     break; */
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
		/*     Timeline *tl = ACTIVE_TL; */
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
			/* timeline_reset(ACTIVE_TL); */
		    }
		    break;
		}
		break;
	    case SDL_KEYUP:
		scrolling_lt = NULL;
		temp_scrolling_lt = NULL;
		if (session->midi_qwerty) {
		    mqwert_handle_keyup(e.key.keysym.sym);
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
		    /* session->playback.play_speed = 0; */
		    /* session->source_mode.src_play_speed = 0; */
		    /* transport_stop_playback(); */
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

		    session->playhead_scroll.playhead_do_incr = false;
		    /* stop_update_track_vol_pan(); */
		    break;
		}
		break;
	    case SDL_MOUSEWHEEL: {
		Timeline *tl = ACTIVE_TL;
		tl->needs_redraw = true;
		if (session->dragged_component.component) {
		    draggable_handle_scroll(&session->dragged_component, e.wheel.x, e.wheel.y);
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
			if (SDL_PointInRect(&main_win->mousep, session->gui.audio_rect)) {
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
			}
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
		    session->dragged_component.component = NULL;
		    automation_unset_dragging_kf(ACTIVE_TL);
		} else if (e.button.button == SDL_BUTTON_RIGHT) {
		    main_win->i_state &= ~I_STATE_MOUSE_R;
		}
		session_flush_ongoing_changes(session, JDAW_THREAD_MAIN);
		session_flush_ongoing_changes(session, JDAW_THREAD_DSP);
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

	if (!session->playback.playing && frames_since_event >= IDLE_AFTER_N_FRAMES) {
	    goto end_frame;
	} else {
	    frames_since_event++;
	}

	
	Timeline *tl = ACTIVE_TL;
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
	    Value old_speed = endpoint_safe_read(&session->playback.play_speed_ep, NULL);
	    float new_speed = old_speed.float_v / 3.0;
	    if (fabs(new_speed) < 1E-6) {
		transport_stop_playback();
		play_speed_scroll_recency = 20;
		
	    } else {
		endpoint_write(&session->playback.play_speed_ep, (Value){.float_v = new_speed}, true, true, true, false);
	    }

	}
	
	if (fabs(session->playback.play_speed) > 1e-9 && !session->source_mode.source_mode) {
	    timeline_catchup(tl);
	    timeline_set_timecode(tl);
	    for (int i=0; i<tl->num_click_tracks; i++) {
		int bar, beat, subdiv;
		click_track_bar_beat_subdiv(tl->click_tracks[i], tl->play_pos_sframes, &bar, &beat, &subdiv, NULL, true);
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

	if (session->playhead_scroll.playhead_do_incr) {
	    timeline_scroll_playhead(session->playhead_scroll.playhead_frame_incr);
	}

	/* update_track_vol_pan(); */

	if (main_win->txt_editing) {
	    if (main_win->txt_editing->cursor_countdown == 0) {
		main_win->txt_editing->cursor_countdown = CURSOR_COUNTDOWN_MAX;

	    } else {
		main_win->txt_editing->cursor_countdown--;
	    }
	}
	/* if (session->playback.recording) { */
	/*     transport_recording_update_cliprects(); */
	/* } */

	if (session->playback.recording) {
	    transport_recording_update_cliprects();
	}
	status_frame();

	session_do_ongoing_changes(session, JDAW_THREAD_MAIN);
	session_flush_val_changes(session, JDAW_THREAD_MAIN);
	session_flush_callbacks(session, JDAW_THREAD_MAIN);
	project_draw();


	if (main_win->screenrecording) {
	    screenshot_loop();
	}
	/* window_draw_menus(main_win); */
	
	/***** Debug only *****/
	/* layout_draw(main_win, main_win->layout); */
	/**********************/

	if (session->playback.playing) {
	    /* Timeline *tl = ACTIVE_TL; */
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
			/* if (!a->current) a->current = automation_get_segment(a, play_pos_adj); */
			int32_t frame_dur = session_get_sample_rate() * session->playback.play_speed / 30.0;
			Value val = endpoint_safe_read(a->endpoint, NULL);
			/* fprintf(stderr, "READ FLOATVAL (%s): %f\n", a->endpoint->local_id, val.float_v); */
			automation_do_write(a, val, play_pos_adj, play_pos_adj + frame_dur, session->playback.play_speed);
		    }
		    /* if (a->num_kclips > 0) { */
		    /* 	kclipref_move(a->kclips, 500); */
		    /* } */
		    /* TEST_automation_keyframe_order(a); */
		    /* TEST_kclipref_bounds(a); */
		}
		
	    }
	}

	if (session->do_tests) {
	    TEST_FN_CALL(chaotic_user, &session->do_tests, 60 * 10); /* 10s of really dumb tests */
	}

    end_frame:

	if (!session->playback.playing && frames_since_event >= IDLE_AFTER_N_FRAMES) {
	    SDL_Delay(100);
	} else {
	    SDL_Delay(1);
	}
    }
}
