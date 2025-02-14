/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    status.c

    * write text to the project status bar
 *****************************************************************************************************************/

#include "layout.h"
#include "project.h"

#define ERR_TIMER_MAX 100
#define CALL_TIMER_MAX 100
#define STAT_TIMER_MAX 50

extern Project *proj;
extern SDL_Color color_global_red;
extern SDL_Color color_global_light_grey;
extern SDL_Color color_global_white;
extern SDL_Color color_global_undo_purple;

void status_frame()
{

    if (proj->status_bar.stat_timer > 0) {
	proj->status_bar.stat_timer--;
	return;
    }
    
    uint8_t *alpha = &(proj->status_bar.error->text->color.a);
    if (proj->status_bar.err_timer > 0) {
	proj->status_bar.err_timer--;
    } else if (*alpha > 0) {
	txt_reset_drawable(proj->status_bar.error->text);
	(*alpha)--;
	/* The alpha decrement appears to NEED to come after reset drawable.
	   This makes no sense. */
    }

    alpha = &(proj->status_bar.call->text->color.a);
    if (proj->status_bar.call_timer > 0) {
	proj->status_bar.call_timer--;
    } else if (*alpha > 0) {
	txt_reset_drawable(proj->status_bar.call->text);
	(*alpha)--;
    }
}

void status_set_errstr(const char *errstr)
{
    strcpy(proj->status_bar.errstr, errstr);
    /* textbox_set_text_color(proj->status_bar.error, &color_global_red); */
    textbox_size_to_fit(proj->status_bar.error, 0, 0);
    textbox_set_text_color(proj->status_bar.error, &color_global_red);
    layout_center_agnostic(proj->status_bar.error->layout, false, true);

    textbox_reset_full(proj->status_bar.error);
    proj->status_bar.err_timer = ERR_TIMER_MAX;
    proj->status_bar.error->text->color.a = 255;
}

void status_set_undostr(const char *undostr)
{
    strcpy(proj->status_bar.errstr, undostr);
    /* textbox_set_text_color(proj->status_bar.error, &color_global_red); */
    textbox_size_to_fit(proj->status_bar.error, 0, 0);
    textbox_set_text_color(proj->status_bar.error, &color_global_undo_purple);
    layout_center_agnostic(proj->status_bar.error->layout, false, true);
    textbox_reset_full(proj->status_bar.error);
    proj->status_bar.err_timer = ERR_TIMER_MAX;
    proj->status_bar.error->text->color.a = 255;
}

void status_set_statstr(const char *statstr)
{
    strcpy(proj->status_bar.callstr, statstr);
    textbox_set_text_color(proj->status_bar.call, &color_global_white);
    /* textbox_size_to_status);fit(proj->status_bar.callor, 0, 0); */
    textbox_reset_full(proj->status_bar.call);
    proj->status_bar.stat_timer = STAT_TIMER_MAX;
    proj->status_bar.call->text->color.a = 255;

}

void status_set_callstr(const char *callstr)
{
    /* if (proj->status_bar.stat_timer > 0) { */
    /* 	return; */
    /* } */
    layout_force_reset(proj->status_bar.layout);
    strcpy(proj->status_bar.callstr, callstr);
    textbox_size_to_fit(proj->status_bar.call, 0, 0);
    textbox_set_text_color(proj->status_bar.call, &color_global_light_grey);
    layout_center_agnostic(proj->status_bar.call->layout, false, true);
    /* textbox_set_pad(proj->status_bar.call, 2, 2); */
    textbox_reset_full(proj->status_bar.call);
    proj->status_bar.call_timer = CALL_TIMER_MAX;
    proj->status_bar.call->text->color.a = 255;
    /* layout_size_to_fit_children(proj->status_bar.call->layout, false, 0); */
    /* layout_reset(proj->status_bar.layout); */
}

void status_cat_callstr(const char *catstr)
{
    /* if (proj->status_bar.stat_timer > 0) { */
    /* 	return; */
    /* } */
    strcat(proj->status_bar.callstr, catstr);
    textbox_size_to_fit(proj->status_bar.call, 0, 0);
    layout_center_agnostic(proj->status_bar.call->layout, false, true);
    textbox_reset_full(proj->status_bar.call);

    proj->status_bar.call_timer = CALL_TIMER_MAX;
    proj->status_bar.call->text->color.a = 255;
    /* layout_size_to_fit_children(proj->status_bar.call->layout, false, 0); */
    /* layout_force_reset(proj->status_bar.layout); */
}

static void status_set_dragstr(char *dragstr)
{
    strcpy(proj->status_bar.dragstr, dragstr);
    textbox_size_to_fit(proj->status_bar.dragstat, 0, 0);
    layout_center_agnostic(proj->status_bar.dragstat->layout, false, true);
    /* layout_center_agnostic(proj->status_bar.dragstat->layout, false, true); */
    textbox_reset_full(proj->status_bar.dragstat);
    layout_reset(proj->status_bar.layout);
    /* fprintf(stdout, "w: %d\n", proj->status_bar.dragstat->layout->rect.w); */
}


void status_stat_playspeed()
{
    /* fprintf(stdout, "stat playspeed\n"); */
    char buf[64];
    snprintf(buf, sizeof(buf), "Play speed: %0.3f",proj->play_speed);
    status_set_statstr(buf);
}

void status_stat_drag()
{
    if (!proj->dragging) {
        proj->status_bar.dragstat->layout->w.value = 0.0f;
	layout_reset(proj->status_bar.layout);
	return;
    }
    Timeline *tl = proj->timelines[proj->active_tl_index];
    char buf[64];
    snprintf(buf, sizeof(buf), "Dragging %d clips\n", tl->num_grabbed_clips);
    status_set_dragstr(buf);
}

    
