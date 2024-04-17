#include "color.h"
#include "geometry.h"
#include "project.h"
#include "textbox.h"
#include "timeline.h"

#define PLAYHEAD_TRI_H (10 * main_win->dpi_scale_factor)

extern Window *main_win;

SDL_Color track_bckgrnd = {120, 130, 150, 255};
SDL_Color track_bckgrnd_active = {170, 180, 200, 255};
SDL_Color console_bckgrnd = {140, 140, 140, 255};
SDL_Color console_bckgrnd_selector = {150, 150, 150, 255};
SDL_Color timeline_bckgrnd =  {50, 52, 55, 255};
SDL_Color ruler_bckgrnd = {10, 10, 10, 255};
SDL_Color control_bar_bckgrnd = {20, 20, 20, 255};

SDL_Color track_selector_color = {100, 190, 255, 255};


SDL_Color clip_ref_bckgrnd = {20, 200, 120, 200};
SDL_Color clip_ref_home_bckgrnd = {90, 180, 245, 200};
SDL_Color clip_ref_grabbed_bckgrnd = {200, 240, 200, 255};
SDL_Color clip_ref_home_grabbed_bckgrnd = {200, 200, 250, 255};
extern SDL_Color color_global_black;
extern SDL_Color color_global_white;

/* SDL_Color track_vol_bar_clr = (SDL_Color) {200, 100, 100, 255}; */
/* SDL_Color track_pan_bar_clr = (SDL_Color) {100, 200, 100, 255}; */
/* SDL_Color track_in_bar_clr = (SDL_Color) {100, 100, 200, 255}; */

static void clipref_draw(ClipRef *cr)
{
    if (cr->deleted) {
	return;
    }

    /* Only check horizontal out-of-bounds; track handles vertical */
    if (cr->rect.x > main_win->w || cr->rect.x + cr->rect.w < 0) {
	return;
    }

    if (cr->home) {
	if (cr->grabbed) {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(clip_ref_home_grabbed_bckgrnd));
	} else {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(clip_ref_home_bckgrnd));
	}
    } else {
	if (cr->grabbed) {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(clip_ref_grabbed_bckgrnd));
	} else {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(clip_ref_bckgrnd));
	}
    }
    SDL_RenderFillRect(main_win->rend, &cr->rect);
}


static void track_draw(Track *track)
{
    if (track->deleted) {
	return;
    }
    if (track->layout->rect.y < 0 || track->layout->rect.y > main_win->layout->rect.h) {
	return;
    }

    if (track->active) {
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(track_bckgrnd_active));
    } else {
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(track_bckgrnd));
    }
    SDL_RenderFillRect(main_win->rend, &track->layout->rect);

    if (track->tl->track_selector == track->tl_rank) {
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(console_bckgrnd_selector));
    } else {
	
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(console_bckgrnd));
    }
    SDL_RenderFillRect(main_win->rend, track->console_rect);
    
    textbox_draw(track->tb_name);
    textbox_draw(track->tb_input_label);
    textbox_draw(track->tb_mute_button);
    textbox_draw(track->tb_solo_button);

    if (track->tl->track_selector == track->tl_rank) {
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_black));
	geom_draw_rect_thick(main_win->rend, &track->layout->rect, 5, main_win->dpi_scale_factor);
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(track_selector_color));
	geom_draw_rect_thick(main_win->rend, &track->layout->rect, 3, main_win->dpi_scale_factor);

    }

    for (uint8_t i=0; i<track->num_clips; i++) {
	clipref_draw(track->clips[i]);
    }
    
    
    
}

static void ruler_draw(Timeline *tl)
{
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(ruler_bckgrnd));
    SDL_RenderFillRect(main_win->rend, tl->proj->ruler_rect);
    
}




static void timeline_draw(Timeline *tl)
{
    
    /* Draw the timeline background */
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(timeline_bckgrnd));
    SDL_RenderFillRect(main_win->rend, &tl->layout->rect);
    
    /* set_rend_color(rend, &bckgrnd_color); */
    /* SDL_Rect top_mask = {0, 0, proj->jwin->w, proj->tl->rect.y}; */
    /* SDL_RenderFillRect(rend, &top_mask); */

    
    /* SDL_Rect mask_left = {0, 0, proj->tl->rect.x, proj->jwin->h}; */
    /* SDL_RenderFillRect(rend, &mask_left); */
    /* SDL_Rect mask_left_2 = {proj->tl->rect.x, proj->tl->rect.y, PADDING, proj->tl->rect.h}; */
    /* set_rend_color(rend, &tl_bckgrnd); */
    /* SDL_RenderFillRect(rend, &mask_left_2); */
    /* // fprintf(stderr, "\t->end draw\n"); */

    /* // SDL_SetRenderDrawColor(rend, 255, 0, 0, 255); */
    /* // SDL_RenderDrawRect(rend, &(proj->ctrl_rect)); */
    /* // SDL_SetRenderDrawColor(rend, 0, 255, 0, 255); */
    /* // SDL_RenderDrawRect(rend, &(proj->ctrl_rect_col_a)); */
    /* draw_textbox(rend, proj->audio_out_label); */
    /* draw_textbox(rend, proj->audio_out); */

    /* set_rend_color(rend, &white); */

    /* /\* Draw t=0 *\/ */
    /* if (proj->tl->display_offset_sframes< 0) { */
    /*     set_rend_color(rend, &black); */
    /*     int zero_x = get_tl_draw_x(0); */
    /*     SDL_RenderDrawLine(rend, zero_x, proj->tl->audio_rect.y, zero_x, proj->tl->audio_rect.y + proj->tl->audio_rect.h); */
    /* } */



    /* Draw tracks */
    for (int i=0; i<tl->num_tracks; i++) {
	track_draw(tl->tracks[i]);
    }
    /* Draw ruler */
    ruler_draw(tl);
    if (tl->timecode_tb) {
	textbox_draw(tl->timecode_tb);
    }

    /* Draw play head line */
    /* set_rend_color(rend, &white); */
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_white));
    int ph_y = tl->proj->ruler_rect->y + PLAYHEAD_TRI_H;
    /* int tri_y = tl->proj->ruler_rect->y; */
    if (tl->play_pos_sframes >= tl->display_offset_sframes) {
        int play_head_x = timeline_get_draw_x(tl->play_pos_sframes);
        SDL_RenderDrawLine(main_win->rend, play_head_x, ph_y, play_head_x, tl->proj->audio_rect->y + tl->proj->audio_rect->h);

        /* Draw play head triangle */
        int tri_x1 = play_head_x;
        int tri_x2 = play_head_x;
        /* int ph_y = tl->rect.y; */
        for (int i=0; i<PLAYHEAD_TRI_H; i++) {
            SDL_RenderDrawLine(main_win->rend, tri_x1, ph_y, tri_x2, ph_y);
            ph_y -= 1;
            tri_x2 += 1;
            tri_x1 -= 1;
        }
    }


    /* /\* draw mark in *\/ */
    /* int in_x, out_x = -1; */
    /* if (proj->tl->in_mark_sframes >= proj->tl->display_offset_sframes && proj->tl->in_mark_sframes < proj->tl->display_offset_sframes + get_tl_abs_w(proj->tl->audio_rect.w)) { */
    /*     in_x = get_tl_draw_x(proj->tl->in_mark_sframes); */
    /*     int i_tri_x2 = in_x; */
    /*     tri_y = proj->tl->rect.y; */
    /*     for (int i=0; i<PLAYHEAD_TRI_H; i++) { */
    /*         SDL_RenderDrawLine(rend, in_x, tri_y, i_tri_x2, tri_y); */
    /*         tri_y -= 1; */
    /*         i_tri_x2 += 1;     */
    /*     }             */
    /* } else if (proj->tl->in_mark_sframes < proj->tl->display_offset_sframes) { */
    /*     in_x = proj->tl->audio_rect.x; */
    /* } */

    /* /\* draw mark out *\/ */
    /* if (proj->tl->out_mark_sframes > proj->tl->display_offset_sframes && proj->tl->out_mark_sframes < proj->tl->display_offset_sframes + get_tl_abs_w(proj->tl->audio_rect.w)) { */
    /*     out_x = get_tl_draw_x(proj->tl->out_mark_sframes); */

    /*     int o_tri_x1 = out_x; */
    /*     tri_y = proj->tl->rect.y; */
    /*     for (int i=0; i<PLAYHEAD_TRI_H; i++) { */
    /*         SDL_RenderDrawLine(rend, o_tri_x1, tri_y, out_x, tri_y); */
    /*         tri_y -= 1; */
    /*         o_tri_x1 -= 1; */
    /*     } */
    /* } else if (proj->tl->out_mark_sframes > proj->tl->display_offset_sframes + get_tl_abs_w(proj->tl->audio_rect.w)) { */
    /*     out_x = proj->tl->audio_rect.x + proj->tl->audio_rect.w; */
    /* } */
    /* if (in_x < out_x && out_x != 0) { */
    /*     SDL_Rect in_out = (SDL_Rect) {in_x, proj->tl->audio_rect.y, out_x - in_x, proj->tl->audio_rect.h}; */
    /*     set_rend_color(rend, &marked_bckgrnd); */
    /*     SDL_RenderFillRect(rend, &(in_out)); */
    /* } */


    
}

static void control_bar_draw(Project *proj)
{
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(control_bar_bckgrnd));
    SDL_RenderFillRect(main_win->rend, proj->control_bar_rect);

}

void project_draw(Project *proj)
{
    timeline_draw(proj->timelines[proj->active_tl_index]);
    control_bar_draw(proj);

    /* layout_draw(main_win, proj->timelines[proj->active_tl_index]->timecode_tb->layout); */
}


