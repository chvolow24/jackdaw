#include "color.h"
#include "geometry.h"
#include "menu.h"
#include "layout_xml.h"
#include "session.h"
#include "textbox.h"

extern Window *main_win;

extern SDL_Color menu_std_clr_inner_border;
extern SDL_Color menu_std_clr_outer_border;

static const SDL_Color txt_clr = {10, 245, 10, 255};

void session_loading_screen_deinit(Session *session)
{
    LoadingScreen *ls = &session->loading_screen;
    if (ls->title_tb) textbox_destroy(ls->title_tb);
    if (ls->subtitle_tb) textbox_destroy(ls->subtitle_tb);
    ls->title_tb = NULL;
    ls->subtitle_tb = NULL;
}

static void loading_screen_init(
    LoadingScreen *ls,
    const char *title,
    const char *subtitle,
    bool draw_progress_bar)
{
    if (title)
	strncpy(ls->title, title, MAX_LOADSTR_LEN);
    if (subtitle)
	strncpy(ls->subtitle, subtitle, MAX_LOADSTR_LEN);
    ls->draw_progress_bar = draw_progress_bar;

    if (ls->layout) layout_destroy(ls->layout);
    Layout *lt = layout_read_from_xml(LOADING_SCREEN_LT_PATH);
    layout_reparent(lt, main_win->layout);
    ls->layout = lt;

    Layout *title_lt = lt->children[0];
    Layout *subtitle_lt = lt->children[1];
    Layout *progress_bar_lt = lt->children[2];
   
    ls->title_tb = textbox_create_from_str(
	ls->title,
	title_lt,
	main_win->mono_bold_font,
	16,
	main_win);
    textbox_set_text_color(ls->title_tb, (SDL_Color *)&txt_clr);
    textbox_set_background_color(ls->title_tb, NULL);

    ls->subtitle_tb = textbox_create_from_str(
	ls->subtitle,
	subtitle_lt,
	main_win->mono_font,
	14,
	main_win);
    textbox_set_text_color(ls->subtitle_tb, (SDL_Color *)&txt_clr);
    textbox_set_background_color(ls->subtitle_tb, NULL);

    /* layout_force_reset(lt); */
    layout_center_agnostic(lt, true, true);
    layout_force_reset(lt);

    ls->progress_bar_rect = &progress_bar_lt->children[0]->rect;
    /* textbox_reset_full(ls->subtitle_tb); */
    /* textbox_reset_full(ls->title_tb); */
    
}


void session_set_loading_screen(
    const char *title,
    const char *subtitle,
    bool draw_progress_bar)
{
    Session *session = session_get();
    LoadingScreen *ls = &session->loading_screen;
    loading_screen_init(ls, title, subtitle, draw_progress_bar);

    window_start_draw(main_win, NULL);
    SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 100);
    SDL_RenderFillRect(main_win->rend, &main_win->layout->rect);
    window_end_draw(main_win);
}

static void loading_screen_draw(LoadingScreen *ls)
{
    window_start_draw(main_win, NULL);

    static const SDL_Color bckgrnd = {10, 10, 10, 255};

    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(bckgrnd));
    geom_fill_rounded_rect(main_win->rend, &ls->layout->rect, STD_CORNER_RAD);

    SDL_Rect border = ls->layout->rect;
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(menu_std_clr_outer_border));
    geom_draw_rounded_rect(main_win->rend, &border, STD_CORNER_RAD);
    border.x++;
    border.y++;
    border.w -= 2;
    border.h -= 2;
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(menu_std_clr_inner_border));
    geom_draw_rounded_rect(main_win->rend, &border, STD_CORNER_RAD - 1);

    textbox_draw(ls->title_tb);
    textbox_draw(ls->subtitle_tb);
    /* layout_draw(main_win, ls->layout); */

    if (ls->draw_progress_bar) {
	SDL_Rect progress = *ls->progress_bar_rect;
	progress.w = ls->progress * ls->progress_bar_rect->w;
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(txt_clr));
	SDL_RenderFillRect(main_win->rend, &progress);
	
	SDL_SetRenderDrawColor(main_win->rend, 100, 100, 100, 255);
	geom_draw_rect_thick(main_win->rend, ls->progress_bar_rect, 2, main_win->dpi_scale_factor);
    }

    window_end_draw(main_win);
}


/* Return 1 to abort operation */
int session_loading_screen_update(
    const char *subtitle,
    float progress)
{
    /* return 0; */
    Session *session = session_get();
    LoadingScreen *ls = &session->loading_screen;
    ls->progress = progress;
    if (subtitle) {
	strncpy(ls->subtitle, subtitle, MAX_LOADSTR_LEN);
	textbox_reset_full(ls->subtitle_tb);
    }
    
    loading_screen_draw(ls);
    
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
	switch (e.type) {
	case SDL_QUIT:
	    SDL_PushEvent(&e); /* Push quit event to be handled later */
	    return 1;
	case SDL_KEYDOWN:
	    switch (e.key.keysym.scancode) {
	    case SDL_SCANCODE_ESCAPE:
		return 1;
	    default:
		break;
	    }
	}
    }
    return 0;
}
