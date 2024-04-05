#include <stdio.h>
#include "SDL_rect.h"
#include "SDL_render.h"
#include "input.h"
#include "draw.h"
#include "layout.h"
#include "layout_xml.h"
#include "menu.h"
#include "screenrecord.h"
#include "text.h"
#include "textbox.h"
#include "window.h"

#include "SDL.h"

#define INPUT_HASH_SIZE 1024

Window *main_win;
Menu *main_menu;
Menu *second_menu;
Textbox *tb;
Textbox *tb2;
Layout *some_lt;
TextArea *lorem_ipsum;

SDL_Texture *target;

InputMode active_mode;

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
    fprintf(stdout, "Start draw\n");
    /* fprintf(stdout, "\ttranslate\n"); */
    //  menu_translate(main_menu, menu_x_dir, menu_y_dir);

    SDL_Rect *menurect = &main_menu->layout->rect;
    if ((menurect->x <= 0 && menu_x_dir < 0) || (menurect->x + menurect->w > main_win->w && menu_x_dir > 0)) {
	menu_x_dir *= -1;
    }
    if ((menurect->y <= 0 && menu_y_dir < 0)|| (menurect->y + menurect->h > main_win->h && menu_y_dir > 0)) {
	menu_y_dir *= -1;
    }


    /* if (lorem_dir < 0 && lorem_ipsum->layout->rect.w < 75) { */
    /* 	lorem_dir *= -1; */
    /* } else if (lorem_dir > 0 && lorem_ipsum->layout->rect.w > 1500) { */
    /* 	lorem_dir *= -1; */
    /* } */
    /* lorem_ipsum->layout->rect.w += lorem_dir; */
    /* layout_set_values_from_rect(lorem_ipsum->layout); */
    /* txt_area_create_lines(lorem_ipsum); */
  

    window_start_draw(main_win, &bckgrnd_color);
    textbox_draw(tb);
    textbox_draw(tb2);
    txt_area_draw(lorem_ipsum);
    if (main_menu) {   
	menu_draw(main_menu);
    } 

    fprintf(stdout, "End draw\n");
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

void user_func_change_bckgrnd_color()
{
    change_clr(NULL, &bckgrnd_color);
    fprintf(stdout, "Called user func, change bckgrnd\n");
}

void user_func_change_tb_color()
{
    change_tb_color(NULL, &tb);
    fprintf(stdout, "Called user func, change tb\n");
}

void user_func_print_bullshit()
{
    fprintf(stdout, "Called user func, print some bullshit\n");
}

void user_func_change_mode()
{
    if (active_mode < 2) {
	(active_mode)++;
    } else {
	active_mode = 0;
    }
    fprintf(stdout, "Changed mode to %s...\n", input_mode_str(active_mode));
}


void run_tests()
{
    
    input_init_hash_table();
    input_init_mode_load_all();
    input_load_keybinding_config("../assets/key_bindings/default.yaml");
    /* exit(0); */
    main_win = window_create(500, 500, "Test window");
    window_assign_std_font(main_win, "../assets/ttf/OpenSans-Regular.ttf");

    SDL_SetRenderTarget(main_win->rend, target);

    window_set_layout(main_win, layout_create_from_window(main_win));
    some_lt = main_win->layout;
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

    menu_add_header(main_menu, "Some Title", "This is a description of this menu. Within this description you will find information about how to use this menu, what its things do, how thing do.\n\nYou can also treat this as a text area only, and not add any items.\n\n\tI don't think there's anything wrong with that.");

    second_menu = menu_create(menu_lt2, main_win);

    int menu_selector = 0;
    MenuItem *item=NULL;
    while ((item = menu_item_at_index(main_menu, menu_selector))) {
	fprintf(stderr, "Item at %d: %s\n", menu_selector, item->label);
	menu_selector++;
    }




    /* FUNCTION BINDING */

    /* InputMode mode = GLOBAL; */
    /* input_bind_func(user_func_change_mode, input_state, SDLK_m, mode); */
    /* mode = MENU_NAV; */
    /* input_state |= I_STATE_CMDCTRL; */
    /* input_bind_func(user_func_change_bckgrnd_color, input_state, SDLK_b, mode); */
    /* input_bind_func(user_func_change_tb_color, input_state, SDLK_g, mode); */
    /* input_bind_func(user_func_print_bullshit, input_state, SDLK_b, MENU_NAV); */
    SDL_StartTextInput();


    bool screenrecord = false;
    int screenshot_index = 0;
    uint16_t i_state = 0;
    while (!(i_state & I_STATE_QUIT)) {
	fprintf(stdout, "Start frame\n");
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
	    if (e.type == SDL_QUIT) {
	        i_state |= I_STATE_QUIT;
	    } else if (e.type == SDL_WINDOWEVENT) {
		if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
		    window_resize(main_win, e.window.data1, e.window.data2);

		}
		    /* window_auto_resize(main_win); */
	    } else if (e.type == SDL_KEYDOWN) {


		/* TEST HASHING HERE */
		UserFn *thing_to_do = input_get(i_state, e.key.keysym.sym, active_mode);
		if (thing_to_do) {
		    thing_to_do->do_fn();
		} else {
		    fprintf(stdout, "Unbound!\n");
		}

	        switch (e.key.keysym.scancode) {
		case SDL_SCANCODE_LGUI:
		case SDL_SCANCODE_RGUI:
		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_RCTRL:
		    i_state |= I_STATE_CMDCTRL;
		    break;
		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RSHIFT:
		    i_state |= I_STATE_SHIFT;
		    break;
		case SDL_SCANCODE_RALT:
		case SDL_SCANCODE_LALT:
		    i_state |= I_STATE_META;
		    break;

		case SDL_SCANCODE_M:
		    user_func_change_mode();
		    menu_destroy(main_menu);
		    fprintf(stdout, "Menu destroyed\n");
		    main_menu = input_create_menu_from_mode(active_mode);
		    fprintf(stdout, "Successfully recreated main menu\n");
		    break;
		case SDL_SCANCODE_L:
		    layout_write(stdout, main_menu->layout, 0);
		/* case SDL_SCANCODE_E: */
		/*     txt_edit(tb->text, layout_test_draw_main); */
		/*     i_state = 0; */
		/*     textbox_size_to_fit(tb, 5, 5); */
		/*     /\* menu_item_add(a1, tb->text->value_handle, "", NULL, NULL); *\/ */
		/*     break; */
		/* case SDL_SCANCODE_R: */
		/*     txt_edit(tb2->text, layout_test_draw_main); */
		/*     i_state = 0; */
		/*     textbox_size_to_fit(tb2, 10, 10); */
		/*     menu_item_add(a1, tb->text->value_handle, tb2->text->value_handle, NULL, NULL); */
		/*     break; */
		/* case SDL_SCANCODE_T: */
		/*     textbox_set_trunc(tb, !(tb->text->truncate)); */
		/*     break; */
		/* case SDL_SCANCODE_C: { */
		/*     SDL_Color clr; */
		/*     clr.r = rand() % 255; */
		/*     clr.g = rand() % 255; */
		/*     clr.b = rand() % 255; */
		/*     textbox_set_text_color(tb, &clr); */
		/*  } */
		/*     break; */
	        /* case SDL_SCANCODE_Q: */
		/*     /\* screenrecord = !screenrecord; *\/ */
		/*     break; */
		default:
		    break;
		}
	    } else if (e.type == SDL_KEYUP) {
		switch (e.key.keysym.scancode) {
		case SDL_SCANCODE_LGUI:
		case SDL_SCANCODE_RGUI:
		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_RCTRL:
		    i_state &= ~I_STATE_CMDCTRL;
		    break;
		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RSHIFT:
		    i_state &= ~I_STATE_SHIFT;
		    break;
		case SDL_SCANCODE_RALT:
		case SDL_SCANCODE_LALT:
		    i_state &= ~I_STATE_META;
		    break;
		default:
		    break;
		}
		
	    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
		switch (e.button.button) {
		case SDL_BUTTON_LEFT:
		    i_state |= I_STATE_MOUSE_L;
		    break;
		case SDL_BUTTON_RIGHT:
		    i_state |= I_STATE_MOUSE_R;
		    break;
		case SDL_BUTTON_MIDDLE:
		    i_state |= I_STATE_MOUSE_M;
		    break;
		default:
		    break;
		}
			
		triage_mouse_menu(main_menu, &main_win->mousep, true);
	        /* textbox_set_fixed_w(tb, 300); */

	    } else if (e.type == SDL_MOUSEBUTTONUP) {
		switch (e.button.button) {
		case SDL_BUTTON_LEFT:
		    i_state &= ~I_STATE_MOUSE_L;
		    break;
		case SDL_BUTTON_RIGHT:
		    i_state &= ~I_STATE_MOUSE_R;
		    break;
		case SDL_BUTTON_MIDDLE:
		    i_state &= ~I_STATE_MOUSE_M;
		    break;
		default:
		    break;
		}
	    } else if (e.type == SDL_MOUSEMOTION) {
		fprintf(stdout, "Mousemotion\n");
		window_set_mouse_point(main_win, e.motion.x, e.motion.y);
		fprintf(stdout, "\t->triage mouse menu\n");
		triage_mouse_menu(main_menu, &main_win->mousep, false);
		fprintf(stdout, "\t->DONe triage mmouse mneu\n");
	    }
	    /* } else if (e.type == SDL_MULTIGESTURE) { */
	    /*     window_zoom(main_win, e.mgesture.dDist); */
	    /* } */
		


	}
	fprintf(stdout, "End event handling\n");

	if (screenrecord) {
	    screenshot(screenshot_index, main_win->rend);
	    screenshot_index++;
	}
	/* fprintf(stdout, "INIT DRAW\n"); */
	layout_test_draw_main();

	/* fprintf(stdout, "EXIT DRAW\n"); */
	SDL_Delay(1);
    }
    fprintf(stdout, "EXITING TESTS\n");
}


/* double scale_factor = 1.3; /\* for example * */
/* int scaled_window_dimension = (int) WINDOW_DIMENSION * scale_factor; */
/* int cutoff = (int) round( */
