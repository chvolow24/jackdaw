#include <stdio.h>
#include "SDL_rect.h"
#include "SDL_render.h"
#include "event_state.h"
#include "draw.h"
#include "layout.h"
#include "layout_xml.h"
#include "menu.h"
#include "screenrecord.h"
#include "text.h"
#include "textbox.h"
#include "window.h"

#include "SDL.h"


Window *main_win;
Menu *main_menu;
Menu *second_menu;
Textbox *tb;
Textbox *tb2;
Layout *some_lt;
TextArea *lorem_ipsum;

SDL_Texture *target;


extern SDL_Color menu_std_clr_highlight;
extern SDL_Color menu_std_clr_tb_bckgrnd;


enum testmode {
    MAIN,
    MENU,
    TEXTEDITING
};


int menu_x_dir = -1;
int menu_y_dir = 1;
int lorem_dir = -5;

SDL_Color bckgrnd_color = (SDL_Color) {200, 200, 200, 255};
void layout_test_draw_main()
{
    menu_translate(main_menu, menu_x_dir, menu_y_dir);

    SDL_Rect *menurect = &main_menu->layout->rect;
    if ((menurect->x <= 0 && menu_x_dir < 0) || (menurect->x + menurect->w > main_win->w && menu_x_dir > 0)) {
	menu_x_dir *= -1;
    }
    if ((menurect->y <= 0 && menu_y_dir < 0)|| (menurect->y + menurect->h > main_win->h && menu_y_dir > 0)) {
	menu_y_dir *= -1;
    }


    if (lorem_dir < 0 && lorem_ipsum->layout->rect.w < 75) {
	lorem_dir *= -1;
    } else if (lorem_dir > 0 && lorem_ipsum->layout->rect.w > 1500) {
	lorem_dir *= -1;
    }
    lorem_ipsum->layout->rect.w += lorem_dir;
    layout_set_values_from_rect(lorem_ipsum->layout);
    txt_area_create_lines(lorem_ipsum);
  

    window_start_draw(main_win, &bckgrnd_color);
    textbox_draw(tb);
    textbox_draw(tb2);
    txt_area_draw(lorem_ipsum);
    if (main_menu) {   
	menu_draw(main_menu);
    } 


    window_end_draw(main_win);
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

void change_clr(Textbox *this, void *target)
{
    SDL_Color *clr = (SDL_Color *)target;
    clr->r = rand() % 255;
    clr->g = rand() % 255;
    clr->b = rand() % 255;
    clr->a = 255;
}

void run_tests()
{

    main_win = window_create(500, 500, "Test window");
    window_assign_std_font(main_win, "../assets/ttf/OpenSans-Regular.ttf");

    SDL_SetRenderTarget(main_win->rend, target);

    some_lt = layout_create_from_window(main_win);
    Layout *child = layout_add_child(some_lt);
    Layout *menu_lt = layout_add_child(some_lt);
    Layout *menu_lt2 = layout_add_child(some_lt);
    

    Layout *textarea_lt = layout_add_child(some_lt);
    layout_set_default_dims(textarea_lt);
    layout_reset(textarea_lt);
    const char *par = "Lorem.\nipsum.\ndolor.\nsit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum";
    lorem_ipsum = txt_area_create(par, textarea_lt, ttf_get_font_at_size(main_win->std_font, 12), (SDL_Color) {0, 0, 0, 0}, main_win);

					
    /* target = SDL_CreateTexture(main_win->rend, 0, SDL_TEXTUREACCESS_TARGET, 500 * main_win->dpi_scale_factor, 500 * main_win->dpi_scale_factor); */
    menu_lt->x.value.intval = 100;
    menu_lt->y.value.intval = 100;
    menu_lt2->x.value.intval = 400;
    menu_lt2->y.value.intval = 20;
    layout_set_default_dims(child);
    layout_fprint(stdout, child);
    
    layout_reset(some_lt);
    child->rect.x = 30;
    child->rect.y = 30;
    child->rect.w = 0;
    child->rect.h = 0;

    layout_set_values_from_rect(child);


    Layout *child2 = layout_add_child(some_lt);
    child2->rect.x = 530;
    child2->rect.y = 30;
    layout_set_values_from_rect(child2);
    
    char hello_world[12];
    snprintf(hello_world, 12, "hello");
    hello_world[5] = '\0';

    char hell_world[12];
    snprintf(hell_world, 11, "world");
    hello_world[5] = '\0';

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
    menu_item_add(a1, "Something!", "C-s", change_clr, &bckgrnd_color);
    menu_item_add(a1, "Something else", "C-e", change_clr, tb2->bckgrnd_clr);
    menu_item_add(a1, "Third thing", "C-t", change_clr, tb->bckgrnd_clr);
    menu_item_add(a2, "Section two item", NULL, NULL, NULL);
    menu_item_add(b1, "Columns two item", NULL, NULL, NULL);

    menu_add_header(main_menu, "Some Title", "This is a description of this menu. Within this description you will find information about how to use this menu, what its things do, how thing do.");

    second_menu = menu_create(menu_lt2, main_win);

    int menu_selector = 0;
    MenuItem *item=NULL;
    while ((item = menu_item_at_index(main_menu, menu_selector))) {
	fprintf(stderr, "Item at %d: %s\n", menu_selector, item->label);
	menu_selector++;
    }

    SDL_StartTextInput();

    bool screenrecord = false;
    int screenshot_index = 0;
    uint16_t e_state = 0;
    while (!(e_state & E_STATE_QUIT)) {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
	    if (e.type == SDL_QUIT) {
	        e_state |= E_STATE_QUIT;
	    } else if (e.type == SDL_WINDOWEVENT) {
		if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
		    window_resize(main_win, e.window.data1, e.window.data2);

		}
		    /* window_auto_resize(main_win); */
	    } else if (e.type == SDL_KEYDOWN) {
	        switch (e.key.keysym.scancode) {
		/* case SDL_SCANCODE_D: */
		/*     if (main_menu) { */
		/* 	menu_destroy(main_menu); */
		/* 	main_menu = NULL; */
		/*     } */
		/*     break; */
		case SDL_SCANCODE_LGUI:
		case SDL_SCANCODE_RGUI:
		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_RCTRL:
		    e_state |= E_STATE_CMDCTRL;
		    break;
		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RSHIFT:
		    e_state |= E_STATE_SHIFT;
		    break;
		case SDL_SCANCODE_E:
		    txt_edit(tb->text, layout_test_draw_main);
		    e_state = 0;
		    textbox_size_to_fit(tb, 5, 5);
		    /* menu_item_add(a1, tb->text->value_handle, "", NULL, NULL); */
		    break;
		case SDL_SCANCODE_R:
		    txt_edit(tb2->text, layout_test_draw_main);
		    e_state = 0;
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
	        case SDL_SCANCODE_Q:
		    screenrecord = !screenrecord;
		    break;
		default:
		    break;
		}
	    } else if (e.type == SDL_KEYUP) {
		switch (e.key.keysym.scancode) {
		case SDL_SCANCODE_LGUI:
		case SDL_SCANCODE_RGUI:
		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_RCTRL:
		    e_state &= ~E_STATE_CMDCTRL;
		    break;
		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RSHIFT:
		    e_state &= ~E_STATE_SHIFT;
		    break;
		default:
		    break;
		}
		
	    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
		switch (e.button.button) {
		case SDL_BUTTON_LEFT:
		    e_state |= E_STATE_MOUSE_L;
		    break;
		case SDL_BUTTON_RIGHT:
		    e_state |= E_STATE_MOUSE_R;
		    break;
		case SDL_BUTTON_MIDDLE:
		    e_state |= E_STATE_MOUSE_M;
		    break;
		default:
		    break;
		}
			
		triage_mouse_menu(main_menu, &main_win->mousep, true);
	        /* textbox_set_fixed_w(tb, 300); */

	    } else if (e.type == SDL_MOUSEBUTTONUP) {
		switch (e.button.button) {
		case SDL_BUTTON_LEFT:
		    e_state &= ~E_STATE_MOUSE_L;
		    break;
		case SDL_BUTTON_RIGHT:
		    e_state &= ~E_STATE_MOUSE_R;
		    break;
		case SDL_BUTTON_MIDDLE:
		    e_state &= ~E_STATE_MOUSE_M;
		    break;
		default:
		    break;
		}
	    } else if (e.type == SDL_MOUSEMOTION) {
		window_set_mouse_point(main_win, e.motion.x, e.motion.y);
		triage_mouse_menu(main_menu, &main_win->mousep, false);
	    }
	    /* } else if (e.type == SDL_MULTIGESTURE) { */
	    /*     window_zoom(main_win, e.mgesture.dDist); */
	    /* } */
		


	}

	if (screenrecord) {
	    screenshot(screenshot_index, main_win->rend);
	    screenshot_index++;
	}
	layout_test_draw_main();

	SDL_Delay(1);
    }
    fprintf(stdout, "EXITING TESTS\n");
}


/* double scale_factor = 1.3; /\* for example * */
/* int scaled_window_dimension = (int) WINDOW_DIMENSION * scale_factor; */
/* int cutoff = (int) round( */
