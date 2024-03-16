#include <stdio.h>
#include "SDL_render.h"
#include "draw.h"
#include "layout.h"
#include "layout_xml.h"
#include "text.h"
#include "textbox.h"
#include "window.h"

#include "SDL.h"


Window *main_win;
void run_tests()
{
    fprintf(stdout, "Running tests\n");

    char *filepath = "jackdaw_main_layout.xml";

    main_win = window_create(500, 500, "Test window");
    window_assign_std_font(main_win, "../assets/ttf/OpenSans-Regular.ttf");

    Layout *some_lt = layout_create_from_window(main_win);
    Layout *child = layout_add_child(some_lt);
    layout_set_default_dims(child);
    layout_fprint(stdout, child);
    
    layout_reset(some_lt);
    child->rect.x = 30;
    child->rect.y = 30;
    child->rect.w = 0;
    child->rect.h = 0;

    layout_set_values_from_rect(child);


    Textbox *tb = textbox_create_from_str(
	"hello world",
	child,
	ttf_get_font_at_size(main_win->std_font, 16),
	main_win->rend
    );
    tb->corner_radius = 50;
    tb->border_thickness = 5;

    textbox_pad(tb, 0);

    /* SDL_Rect *cont = tb->text->container; */
    /* fprintf(stdout, "text rect: %d, %d, %d, %d\n", cont->x, cont->y, cont->w, cont->h); */

    /* exit(0); */

    bool quit = false;
    int i=1;
    while (!quit) {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
	    if (e.type == SDL_QUIT) {
		quit = true;
	    } else if (e.type == SDL_KEYDOWN) {
		i++;
		textbox_pad(tb, i);
		txt_reset_display_value(tb->text);
	    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
		i = 2;
		textbox_pad(tb, i);
	    }
	}
	SDL_SetRenderDrawColor(main_win->rend, 255, 255, 255, 255);
	SDL_RenderClear(main_win->rend);
	draw_layout(main_win, some_lt);

	textbox_draw(tb);
	/* txt_draw(tb->text); */
	/* child->rect.w += 1; */
	/* layout_set_values_from_rect(child); */

	txt_reset_display_value(tb->text);
	layout_set_values_from_rect(tb->layout);

	SDL_RenderPresent(main_win->rend);
	SDL_Delay(1);
    }
    
}
