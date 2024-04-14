#include "color.h"
#include "geometry.h"
#include "project.h"
#include "textbox.h"

extern Window *main_win;

SDL_Color track_bckgrnd = {120, 130, 150, 255};
SDL_Color track_bckgrnd_active = {170, 180, 200, 255};
SDL_Color console_bckgrnd = {140, 140, 140, 255};
SDL_Color console_bckgrnd_selector = {150, 150, 150, 255};

SDL_Color track_selector_color = {100, 190, 255, 255};
extern SDL_Color color_global_black;

/* SDL_Color track_vol_bar_clr = (SDL_Color) {200, 100, 100, 255}; */
/* SDL_Color track_pan_bar_clr = (SDL_Color) {100, 200, 100, 255}; */
/* SDL_Color track_in_bar_clr = (SDL_Color) {100, 100, 200, 255}; */

void track_draw(Track *track)
{
    if (track->deleted) {
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
    
    
    
}

void timeline_draw(Timeline *tl)
{
    for (int i=0; i<tl->num_tracks; i++) {
	track_draw(tl->tracks[i]);
    }
    
}

void project_draw(Project *proj)
{
    for (int i=0; i<proj->num_timelines; i++) {
	timeline_draw(proj->timelines[i]);
    }
}


