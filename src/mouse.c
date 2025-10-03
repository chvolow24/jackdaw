/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "autocompletion.h"
#include "clipref.h"
#include "input.h"
#include "menu.h"
#include "modal.h"
#include "page.h"
#include "panel.h"
#include "project.h"
#include "session.h"
#include "timeline.h"
#include "userfn.h"

extern Window *main_win;
extern Project *proj;

//SDL_BUTTON_LEFT | SDL_BUTTON_RIGHT

static void mouse_triage_motion_track(Track *track)
{
    /* NO OP */
    /* if (main_win->i_state & I_STATE_MOUSE_L && SDL_PointInRect(&main_win->mousep, track->console_rect)) { */
    /* 	if (slider_mouse_motion(track->vol_ctrl, main_win)) return; */
    /* 	/\* if (SDL_PointInRect(&main_win->mousep, &track->vol_ctrl->layout->rect)) { *\/ */
    /* 	/\*     Value newval = slider_val_from_coord(track->vol_ctrl, main_win->mousep.x); *\/ */
    /* 	/\*     track->vol = newval.float_v; *\/ */
    /* 	/\*     slider_reset(track->vol_ctrl); *\/ */
    /* 	/\*     /\\* proj->vol_changing = true; *\\/ *\/ */
    /* 	/\*     return; *\/ */
    /* 	/\* } *\/ */
    /* 	if (slider_mouse_motion(track->pan_ctrl, main_win)) return; */
    /* 	/\* if (SDL_PointInRect(&main_win->mousep, &track->pan_ctrl->layout->rect)) { *\/ */
    /* 	/\*     Value newval = slider_val_from_coord(track->pan_ctrl, main_win->mousep.x); *\/ */
    /* 	/\*     track->pan = newval.float_v; *\/ */
    /* 	/\*     slider_reset(track->pan_ctrl); *\/ */
    /* 	/\*     /\\* proj->pan_changing = true; *\\/ *\/ */
    /* 	/\*     return; *\/ */
    /* 	/\* } *\/ */
    /* } */

}


static bool mouse_triage_click_track(uint8_t button, Track *track)
{
    if (SDL_PointInRect(&main_win->mousep, track->console_rect)) {
	if (SDL_PointInRect(&main_win->mousep, &track->tb_input_name->layout->rect)) {
	    track_set_input(track);
	    return true;
	}
	Layout *solo = layout_get_child_by_name_recursive(track->layout, "solo_button");
	if (SDL_PointInRect(&main_win->mousep, &solo->rect)) {
	    track_or_tracks_solo(track->tl, track);
	    return true;
	}
	Layout *mute = layout_get_child_by_name_recursive(track->inner_layout, "mute_button");
	if (SDL_PointInRect(&main_win->mousep, &mute->rect)) {
	    track_mute(track);
	    return true;
	}
	if (SDL_PointInRect(&main_win->mousep, &track->tb_name->tb->layout->rect)) {
	    track_rename(track);
	    return true;
	}
	if (slider_mouse_click(track->vol_ctrl, main_win)) return true;
	if (slider_mouse_click(track->pan_ctrl, main_win)) return true;
	if (symbol_button_click(track->automation_dropdown, main_win)) return true;
    }
    return false;
}

static void mouse_triage_click_audiorect(Timeline *tl, uint8_t button)
{
    switch (button) {
    case SDL_BUTTON_LEFT: {
	for (int i=0; i<tl->num_tracks; i++) {
	    Track *track = tl->tracks[i];
	    if (SDL_PointInRect(&main_win->mousep, &track->layout->rect)) {
		timeline_select_track(track);
		break;
	    }
	}
	int32_t abs_pos = timeline_get_abspos_sframes(tl, main_win->mousep.x);
	timeline_set_play_position(tl, abs_pos);
	break;
    }
	break;
    default:
	break;
    }
    if (main_win->i_state & I_STATE_CMDCTRL) {
	for (uint8_t i=0; i<tl->num_tracks; i++) {
	    Track *track = tl->tracks[i];
	    if (SDL_PointInRect(&main_win->mousep, &track->inner_layout->rect)) {
		ClipRef *cr = clipref_at_cursor_in_track(track);
		if (cr) {
		    if (cr->grabbed) {
			timeline_ungrab_all_cliprefs(tl);
		    } else {
			timeline_clipref_grab(cr, CLIPREF_EDGE_NONE);
			/* clipref_grab(cr); */
		    }
		}
	    }
	}
    }
}

static void mouse_triage_motion_audiorect(Timeline *tl)
{
    if (main_win->i_state & I_STATE_MOUSE_L) {
	int32_t abs_pos = timeline_get_abspos_sframes(tl, main_win->mousep.x);
	timeline_set_play_position(tl, abs_pos);
    }
}


static void mouse_triage_click_timeline(uint8_t button)
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;

    for (uint8_t i=0; i<tl->num_tracks; i++) {
	Track *track = tl->tracks[i];
	if (mouse_triage_click_track(button, track)) {
	    break;
	}
	for (uint8_t i = 0; i<track->num_automations; i++) {
	    if (automation_triage_click(button, track->automations[i])) {
		timeline_select_track(track);
		return;
	    }
	}
    }
    for (uint8_t i=0; i<tl->num_click_tracks; i++) {
	ClickTrack *tt = tl->click_tracks[i];
	if (click_track_triage_click(button, tt)) return;
    }
    if (SDL_PointInRect(&main_win->mousep, session->gui.audio_rect)) {
	mouse_triage_click_audiorect(tl, button);
	/* return; */
    }
}

static void mouse_triage_click_control_bar(uint8_t button)
{
    Session *session = session_get();
    if (SDL_PointInRect(&main_win->mousep, session->gui.hamburger)) {
	user_global_menu(NULL);
	return;
    } else if (SDL_PointInRect(&main_win->mousep, &session->gui.panels->layout->rect)) {
	panel_area_mouse_click(session->gui.panels);
    }
}

void mouse_triage_click_project(uint8_t button)
{
    Session *session = session_get();
    Layout *tl_lt = layout_get_child_by_name_recursive(session->gui.layout, "timeline");
    if (SDL_PointInRect(&main_win->mousep, &tl_lt->rect)) {
	mouse_triage_click_timeline(button);
    } else if (SDL_PointInRect(&main_win->mousep, session->gui.control_bar_rect)) {
	mouse_triage_click_control_bar(button);
    }
}

void mouse_triage_motion_timeline(int xrel, int yrel)
{
    Session *session = session_get();
    if (session->dragged_component.component) {
	draggable_mouse_motion(&session->dragged_component, main_win);
	return;
    }
    Timeline *tl = ACTIVE_TL;
    if (automations_triage_motion(tl, xrel, yrel)) return;
    if (SDL_PointInRect(&main_win->mousep, session->gui.audio_rect)) {
	mouse_triage_motion_audiorect(tl);
	return;
    }
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	Track *track = tl->tracks[i];
	if (SDL_PointInRect(&main_win->mousep, &track->inner_layout->rect)) {
	    mouse_triage_motion_track(track);
	}
    }
}

void mouse_triage_motion_menu()
{
    if (main_win->num_menus == 0) return;
    Menu *top_menu = main_win->menus[main_win->num_menus - 1];
    if (top_menu) {
	menu_triage_mouse(top_menu, &main_win->mousep, false);
    }
}

void mouse_triage_motion_modal()
{
    if (main_win->num_modals == 0) return;
    Modal *top_modal = main_win->modals[main_win->num_modals - 1];
    if (top_modal) {
	modal_triage_mouse(top_modal, &main_win->mousep, false);
    }

}

void mouse_triage_click_menu(uint8_t button)
{
    Session *session = session_get();
    if (main_win->num_menus == 0) return;
    Menu *top_menu = main_win->menus[main_win->num_menus -1];
    if (top_menu) {
	if (!menu_triage_mouse(top_menu, &main_win->mousep, true))
	    ACTIVE_TL->needs_redraw = true;
    }
}

void mouse_triage_click_modal(uint8_t button)
{
    Session *session = session_get();
    if (main_win->num_modals == 0) return;
    Modal *top_modal = main_win->modals[main_win->num_modals -1];
    if (top_modal) {
	if (!modal_triage_mouse(top_modal, &main_win->mousep, true))
	    ACTIVE_TL->needs_redraw = true;
    }
}

void mouse_triage_motion_autocompletion()
{
    if (!main_win->ac_active) return;
    autocompletion_triage_mouse_motion(&main_win->ac);
}

void mouse_triage_click_autocompletion()
{
    if (!main_win->ac_active) return;
    autocompletion_triage_mouse_click(&main_win->ac);
}

Layout *mouse_triage_wheel(int x, int y, bool dynamic)
{
    if (main_win->ac_active) {
	return autocompletion_scroll(y, dynamic);
    }
    if (main_win->num_modals == 0) return NULL;
    Modal *top_modal = main_win->modals[main_win->num_modals -1];
    if (top_modal) {
	return modal_triage_wheel(top_modal, &main_win->mousep, x, y, dynamic);
    }
    return NULL;
}

bool mouse_triage_click_text_edit(uint8_t button)
{
    Text *txt = main_win->txt_editing;
    if (!txt) return false;
    if (!SDL_PointInRect(&main_win->mousep, &txt->container->rect)) {
	txt_stop_editing(txt);
	return false;
    }
    return true;
}


bool mouse_triage_motion_page()
{
    Session *session = session_get();
    Page *page;
    if (session->dragged_component.component) {
	draggable_mouse_motion(&session->dragged_component, main_win);
	return true;
    }
    if ((page = main_win->active_page)) {
	return page_mouse_motion(page, main_win);
    }
    return false;
}

bool mouse_triage_click_page()
{
    Page *page;
    if ((page = main_win->active_page)) {
	return page_mouse_click(page, main_win);
    }
    return false;
}
bool mouse_triage_click_tabview()
{
    TabView *tv;
    if ((tv = main_win->active_tabview)) {
	if (!tabview_mouse_click(tv)) {
	    tabview_close(tv);
	} else {
	    return true;
	}
    }
    return false;
}
bool mouse_triage_motion_tabview()
{
    Session *session = session_get();
    TabView *tv;
    if (session->dragged_component.component) {
	draggable_mouse_motion(&session->dragged_component, main_win);
	return true;
    }
    if ((tv = main_win->active_tabview)) {
	return tabview_mouse_motion(tv);
    }
    return false;
}
