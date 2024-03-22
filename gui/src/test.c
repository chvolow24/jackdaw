#include <stdio.h>
#include "SDL_rect.h"
#include "SDL_render.h"
#include "draw.h"
#include "layout.h"
#include "layout_xml.h"
#include "menu.h"
#include "text.h"
#include "textbox.h"
#include "window.h"

#include "SDL.h"


Window *main_win;
Menu *main_menu;
Textbox *tb;
Textbox *tb2;
Layout *some_lt;

extern SDL_Color menu_std_clr_highlight;
extern SDL_Color menu_std_clr_tb_bckgrnd;



void layout_test_draw_main()
{
    SDL_SetRenderDrawColor(main_win->rend, 200, 200, 200, 255);
    SDL_RenderClear(main_win->rend);
    
    /* draw_layoutmain_win, somed_lt); */
    if (main_menu) {   
	menu_draw(main_menu);
    }
    textbox_draw(tb);
    textbox_draw(tb2);
    /* layout_set_values_from_rect(tb->layout); */

    SDL_RenderPresent(main_win->rend);
}

void change_tb_color(Textbox *this, void *target)
{
    Textbox *to_change = (Textbox *)target;
    SDL_Color clr;
    clr.r = rand() % 255;
    clr.g = rand() % 255;
    clr.b = rand() % 255;
    clr.a = 255;
    textbox_set_text_color(to_change, &clr);
}

void run_tests()
{
    char *filepath = "jackdaw_main_layout.xml";

    main_win = window_create(500, 500, "Test window");
    window_assign_std_font(main_win, "../assets/ttf/OpenSans-Regular.ttf");

    some_lt = layout_create_from_window(main_win);
    Layout *child = layout_add_child(some_lt);
    Layout *menu_lt = layout_add_child(some_lt);
    menu_lt->x.value.intval = 100;
    menu_lt->y.value.intval = 100;
    layout_set_default_dims(child);
    layout_fprint(stdout, child);
    
    layout_reset(some_lt);
    child->rect.x = 30;
    child->rect.y = 30;
    child->rect.w = 0;
    child->rect.h = 0;

    layout_set_values_from_rect(child);


    Layout *child2 = layout_add_child(some_lt);
    child2->rect.x = 230;
    child2->rect.y = 30;
    layout_set_values_from_rect(child2);
    
    char hello_world[12];
    snprintf(hello_world, 12, "hello world");
    hello_world[11] = '\0';

    char hell_world[12];
    snprintf(hell_world, 11, "hell world");
    hello_world[10] = '\0';

    tb = textbox_create_from_str(
	hello_world,
	child,
	ttf_get_font_at_size(main_win->std_font, 16),
	main_win
    );

    tb2 = textbox_create_from_str(
	hell_world,
	child2,
	ttf_get_font_at_size(main_win->std_font, 16),
	main_win);
    tb2->corner_radius = 10;
	
    tb->corner_radius = 50;
    tb->border_thickness = 1;

    textbox_size_to_fit(tb, 5, 20);
    textbox_size_to_fit(tb2, 5, 5);

    /* SDL_Rect *cont = tb->text->container; */
    /* fprintf(stdout, "text rect: %d, %d, %d, %d\n", cont->x, cont->y, cont->w, cont->h); */

    /* exit(0); */
    
    main_menu = menu_create(menu_lt, main_win);
    fprintf(stderr, "menu win: %p\n", main_menu->window);
    MenuColumn *col_a = menu_column_add(main_menu, "Column A");
    MenuColumn *col_b = menu_column_add(main_menu, "Column B");

    MenuSection *a1 = menu_section_add(col_a, "A1");
    MenuSection *a2 = menu_section_add(col_a, "A2");
    MenuSection *b1 = menu_section_add(col_b, "B1");

    fprintf(stderr, "About to create first item\n");
    menu_item_add(a1, "Something!", "C-s", change_tb_color, tb);
    fprintf(stderr, "Created first item\n");
    menu_item_add(a1, "Something else", "C-e", change_tb_color, tb2);
    menu_item_add(a1, "Third thing", "C-t", NULL, NULL);
    menu_item_add(a2, "Section two item", NULL, NULL, NULL);
    menu_item_add(b1, "Columns two item", NULL, NULL, NULL);

    SDL_Point mousep;
    bool quit = false;
    while (!quit) {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
	    if (e.type == SDL_QUIT) {
		quit = true;
	    } else if (e.type == SDL_KEYDOWN) {
	        switch (e.key.keysym.scancode) {
		/* case SDL_SCANCODE_D: */
		/*     if (main_menu) { */
		/* 	menu_destroy(main_menu); */
		/* 	main_menu = NULL; */
		/*     } */
		/*     break; */
		case SDL_SCANCODE_E:
		    txt_edit(tb->text, layout_test_draw_main);
		    textbox_size_to_fit(tb, 5, 5);
		    /* menu_item_add(a1, tb->text->value_handle, "", NULL, NULL); */
		    break;
		case SDL_SCANCODE_R:
		    txt_edit(tb2->text, layout_test_draw_main);
		    textbox_size_to_fit(tb2, 10, 10);
		    menu_item_add(a1, tb->text->value_handle, tb2->text->value_handle, NULL, NULL);
		    break;
		case SDL_SCANCODE_T:
		    textbox_set_trunc(tb, !(tb->text->truncate));
		    break;
		case SDL_SCANCODE_C: {
		    SDL_Color clr;
		    clr.r = rand() % 255;
		    clr.g = rand() % 255;
		    clr.b = rand() % 255;
		    textbox_set_text_color(tb, &clr);
		 }
		    break;
		default:
		    break;
		}
		
	    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
		fprintf(stderr, "Triaging mouse menu from buttondown\n");
		triage_mouse_menu(main_menu, &mousep, true);
	        /* textbox_set_fixed_w(tb, 300); */
		
	    } else if (e.type == SDL_MOUSEMOTION) {
		mousep.x = e.motion.x * main_win->dpi_scale_factor;
		mousep.y = e.motion.y * main_win->dpi_scale_factor;
		triage_mouse_menu(main_menu, &mousep, false);

	    }
	}


	layout_test_draw_main();

	SDL_Delay(1);
    }
    fprintf(stdout, "EXITING TESTS\n");
}
