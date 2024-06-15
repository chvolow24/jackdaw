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
    mouse.c

    * Functions related to mouse clicks
 *****************************************************************************************************************/

#include "input.h"
#include "menu.h"
#include "modal.h"
#include "project.h"
#include "timeline.h"
#include "userfn.h"

extern Window *main_win;
extern Project *proj;

//SDL_BUTTON_LEFT | SDL_BUTTON_RIGHT



static void mouse_triage_motion_track(Track *track)
{
    if (main_win->i_state & I_STATE_MOUSE_L && SDL_PointInRect(&main_win->mousep, track->console_rect)) {
	if (SDL_PointInRect(&main_win->mousep, &track->vol_ctrl->layout->rect)) {
	    float newval = fslider_val_from_coord(track->vol_ctrl, main_win->mousep.x);
	    track->vol = newval;
	    fslider_reset(track->vol_ctrl);
	    /* proj->vol_changing = true; */
	    return;
	}
	if (SDL_PointInRect(&main_win->mousep, &track->pan_ctrl->layout->rect)) {
	    float newval = fslider_val_from_coord(track->pan_ctrl, main_win->mousep.x);
	    track->pan = newval;
	    fslider_reset(track->pan_ctrl);
	    /* proj->pan_changing = true; */
	    return;
	}
    }

}


static void mouse_triage_click_track(uint8_t button, Track *track)
{
    if (SDL_PointInRect(&main_win->mousep, track->console_rect)) {
	if (SDL_PointInRect(&main_win->mousep, &track->tb_input_name->layout->rect)) {
	    track_set_input(track);
	    return;
	}
	Layout *solo = layout_get_child_by_name_recursive(track->layout, "solo_button");
	if (SDL_PointInRect(&main_win->mousep, &solo->rect)) {
	    track_or_tracks_solo(track->tl, track);
	    return;
	}
	Layout *mute = layout_get_child_by_name_recursive(track->layout, "mute_button");
	if (SDL_PointInRect(&main_win->mousep, &mute->rect)) {
	    track_mute(track);
	    return;
	}
	if (SDL_PointInRect(&main_win->mousep, &track->tb_name->layout->rect)) {
	    track_rename(track);
	    return;
	}
	if (SDL_PointInRect(&main_win->mousep, &track->vol_ctrl->layout->rect)) {
	    float newval = fslider_val_from_coord(track->vol_ctrl, main_win->mousep.x);
	    track->vol = newval;
	    fslider_reset(track->vol_ctrl);
	    return;
	}
	if (SDL_PointInRect(&main_win->mousep, &track->pan_ctrl->layout->rect)) {
	    float newval = fslider_val_from_coord(track->pan_ctrl, main_win->mousep.x);
	    track->pan = newval;
	    fslider_reset(track->pan_ctrl);
	}
    }

}

static void mouse_triage_click_audiorect(Timeline *tl, uint8_t button)
{
    switch (button) {
    case SDL_BUTTON_LEFT: {
	int32_t abs_pos = timeline_get_abspos_sframes(main_win->mousep.x);
	timeline_set_play_position(abs_pos);
	break;
    }
	break;
    default:
	break;
    }
    if (main_win->i_state & I_STATE_CMDCTRL) {
	for (uint8_t i=0; i<tl->num_tracks; i++) {
	    Track *track = tl->tracks[i];
	    if (SDL_PointInRect(&main_win->mousep, &track->layout->rect)) {
		ClipRef *cr = clipref_at_point_in_track(track);
		if (cr->grabbed) {
		    timeline_ungrab_all_cliprefs(tl);
		} else {
		    clipref_grab(cr);
		}
	    }
	}
    }
}

static void mouse_triage_motion_audiorect(Timeline *tl)
{

    if (main_win->i_state & I_STATE_MOUSE_L) {
	int32_t abs_pos = timeline_get_abspos_sframes(main_win->mousep.x);
	timeline_set_play_position(abs_pos);
    }
}


static void mouse_triage_click_timeline(uint8_t button)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    if (SDL_PointInRect(&main_win->mousep, proj->audio_rect)) {
	mouse_triage_click_audiorect(tl, button);
	return;
    }
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	Track *track = tl->tracks[i];
	if (SDL_PointInRect(&main_win->mousep, &track->layout->rect)) {
	    mouse_triage_click_track(button, track);
	}
    }
}

static void mouse_triage_click_control_bar(uint8_t button)
{
    if (SDL_PointInRect(&main_win->mousep, &proj->tb_out_value->layout->rect)) {
	user_tl_set_default_out();
    }

}

void mouse_triage_click_project(uint8_t button)
{
    Layout *tl_lt = layout_get_child_by_name_recursive(proj->layout, "timeline");
    if (SDL_PointInRect(&main_win->mousep, &tl_lt->rect)) {
	mouse_triage_click_timeline(button);
    } else if (SDL_PointInRect(&main_win->mousep, proj->control_bar_rect)) {
	mouse_triage_click_control_bar(button);
    }
}

void mouse_triage_motion_timeline()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    if (SDL_PointInRect(&main_win->mousep, proj->audio_rect)) {
	mouse_triage_motion_audiorect(tl);
	return;
    }
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	Track *track = tl->tracks[i];
	if (SDL_PointInRect(&main_win->mousep, &track->layout->rect)) {
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
    if (main_win->num_menus == 0) return;
    Menu *top_menu = main_win->menus[main_win->num_menus -1];
    if (top_menu) {
	menu_triage_mouse(top_menu, &main_win->mousep, true);
    }
}

void mouse_triage_click_modal(uint8_t button)
{
    if (main_win->num_modals == 0) return;
    Modal *top_modal = main_win->modals[main_win->num_modals -1];
    if (top_modal) {
	modal_triage_mouse(top_modal, &main_win->mousep, true);
    }
}

void mouse_triage_wheel(int x, int y)
{
    if (main_win->num_modals == 0) return;
    Modal *top_modal = main_win->modals[main_win->num_modals -1];
    if (top_modal) {
	modal_triage_wheel(top_modal, &main_win->mousep, x, y);
    }
}

void mouse_triage_click_text_edit(uint8_t button)
{
    Text *txt = main_win->txt_editing;
    if (!txt) return;
    if (!SDL_PointInRect(&main_win->mousep, &txt->container->rect))
    {
	txt_stop_editing(txt);
    }

}

