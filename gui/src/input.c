#include <string.h>
#include <stdbool.h>
#include "input.h"
#include "menu.h"
#include "userfn.h"

#define not_whitespace_char(c) (c != ' ' && c != '\n' && c != '\t')
#define is_whitespace_char(c) (c == ' ' || c == '\n' || c == '\t')

Mode *modes[NUM_INPUT_MODES];
KeybNode *input_hash_table[INPUT_HASH_SIZE];
KeybNode *input_keyup_hash_table[INPUT_KEYUP_HASH_SIZE];
extern Window *main_win;


const char *input_mode_str(InputMode im)
{
    switch (im) {
    case GLOBAL:
	return "global";
    case MENU_NAV:
	return "menu_nav";
    case TIMELINE:
	return "timeline";
    case SOURCE:
	return "source";
    default:
	fprintf(stderr, "ERROR: [no mode string for value %d]\n", im);
	return "[no mode]";
    }
}

InputMode input_mode_from_str(char *str)
{
    if (strcmp(str, "global") == 0) {
	return GLOBAL;
    } else if (strcmp(str, "menu_nav") == 0) {
	return MENU_NAV;
    } else if (strcmp(str, "timeline") == 0) {
	return TIMELINE;
    } else if (strcmp(str, "source") == 0) {
	return SOURCE;
    } else {
	return -1;
    }
}

static Mode *mode_create(InputMode im)
{
    Mode *mode = malloc(sizeof(Mode));
    mode->name = input_mode_str(im);
    mode->num_subcats = 0;
    fprintf(stdout, "Inserting mode %s at index %d\n", mode->name, im);
    modes[im] = mode;	
    return mode;
}

static ModeSubcat *mode_add_subcat(Mode *mode, const char *name)
{
    ModeSubcat *sc = malloc(sizeof(ModeSubcat));
    if (!sc) {
	fprintf(stderr, "Error: failed to allocate memory\n");
	exit(1);
    }
    if (mode->num_subcats == MAX_MODE_SUBCATS) {
	fprintf(stderr, "Mode already has maximum number of subcats (%d)\n", mode->num_subcats);
	return NULL;
    }
    mode->subcats[mode->num_subcats] = sc;
    mode->num_subcats++;
    sc->name = name;
    sc->num_fns = 0;
    return sc;
}

static void mode_subcat_add_fn(ModeSubcat *ms, UserFn *fn)
{
    ms->fns[ms->num_fns] = fn;
    ms->num_fns++;
}

static UserFn *create_user_fn(
    const char *fn_id,
    const char *fn_display_name,
    void (*do_fn) (void))
{
    UserFn *fn = malloc(sizeof(UserFn));
    fn->fn_id = fn_id;
    fn->annotation = NULL;
    fn->fn_display_name = fn_display_name;
    fn->do_fn = do_fn;
    return fn;
}

static void mode_load_global()
{
    Mode *mode = mode_create(GLOBAL);
    ModeSubcat *mc = mode_add_subcat(mode, "");
    if (!mc) {
	return;
    }

    
    /* exit(0); */
    
    UserFn *fn;
    fn = create_user_fn(
	"expose_help",
	"Expose help",
	user_global_expose_help
	);
    mode_subcat_add_fn(mc, fn);
    
    fn = create_user_fn(
	"quit",
	"Quit",
	user_global_quit
	);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"undo",
	"Undo",
	user_global_undo
	);

    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"redo",
	"Redo",
	user_global_redo
	);

    mode_subcat_add_fn(mc, fn);   
}


static void mode_load_menu_nav()
{
    
    Mode *mode = mode_create(MENU_NAV);
    ModeSubcat *mc = mode_add_subcat(mode, "");
    UserFn *fn;
    fn = create_user_fn(
	"menu_next_item",
	"Next item",
	user_menu_nav_next_item
	);
    mode_subcat_add_fn(mc, fn);
    
    fn = create_user_fn(
	"menu_previous_item",
	"Previous item",
	user_menu_nav_prev_item
	);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"menu_next_section",
	"Next section",

	user_menu_nav_next_sctn
	);

    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"menu_previous_section",
	"Previous section",
	user_menu_nav_prev_sctn
	);
    mode_subcat_add_fn(mc, fn);

    
    fn = create_user_fn(
	"menu_choose_item",
	"Choose item",
	user_menu_nav_choose_item
	);
    mode_subcat_add_fn(mc, fn);

        fn = create_user_fn(
	"menu_column_right",
	"Column right",
	user_menu_nav_column_right
	);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"menu_column_left",
	"Column left",
	user_menu_nav_column_left
	);
    mode_subcat_add_fn(mc, fn);


    fn = create_user_fn(
	"menu_translate_up",
	"Move menu up",
	user_menu_translate_up
	);
    mode_subcat_add_fn(mc, fn);

        fn = create_user_fn(
	"menu_translate_left",
	"Move menu left",
	user_menu_translate_left
	);
    mode_subcat_add_fn(mc, fn);

    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"menu_translate_down",
	"Move menu down",
	user_menu_translate_down
	);
    mode_subcat_add_fn(mc, fn);
    
    fn = create_user_fn(
	"menu_translate_right",
	"Move menu right",
	user_menu_translate_right
	);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"menu_dismiss",
	"Dismiss menu",
	user_menu_dismiss
	);
    mode_subcat_add_fn(mc, fn);

}

static void mode_load_timeline()
{
    Mode *mode = mode_create(TIMELINE);
    /* ModeSubcat *sc= mode_add_subcat(mode, "Timeline Navigation"); */
    UserFn *fn;


    ModeSubcat *sc= mode_add_subcat(mode, "Transport");

    
    fn = create_user_fn(
	"tl_play",
	"Play",
	user_tl_play
	);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"tl_pause",
	"Pause",
	user_tl_pause
	);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"tl_rewind",
	"Rewind",
	user_tl_rewind
	);
    mode_subcat_add_fn(sc, fn);

    fn=create_user_fn(
	"tl_play_slow",
	"Play slow",
	user_tl_play_slow
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_rewind_slow",
	"Rewind slow",
	user_tl_rewind_slow
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_move_right",
	"Move right",
	user_tl_move_right
	);
    mode_subcat_add_fn(sc,fn);

    fn = create_user_fn(
	"tl_move_left",
	"Move left",
	user_tl_move_left
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_zoom_out",
	"Move left",
	user_tl_zoom_out
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_zoom_in",
	"Move left",
	user_tl_zoom_in
	);
    mode_subcat_add_fn(sc, fn);

    

    sc = mode_add_subcat(mode, "Marks");
    fn = create_user_fn(
	"tl_set_in_mark",
	"Set In",
	user_tl_set_mark_in
	);
    mode_subcat_add_fn(sc, fn);
    
    fn = create_user_fn(
	"tl_set_out_mark",
	"Set Out",
	user_tl_set_mark_out
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_goto_in_mark",
	"Go to In",
	user_tl_goto_mark_in
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_goto_out_mark",
	"Go to Out",
	user_tl_goto_mark_out
	);
    mode_subcat_add_fn(sc, fn);


    fn = create_user_fn(
	"tl_record",
	"Record (start or stop)",
	user_tl_record
	);
    mode_subcat_add_fn(sc, fn);
    sc= mode_add_subcat(mode, "Tracks");  
    fn = create_user_fn(
	"tl_track_add",
	"Add Track",
        user_tl_add_track
	);
    mode_subcat_add_fn(sc, fn);


    fn = create_user_fn(
	"tl_track_add",
	"Add Track",
        user_tl_add_track
	);
    mode_subcat_add_fn(sc, fn);

        fn = create_user_fn(
	"tl_track_select_1",
	"Select track 1",
        user_tl_track_select_1
	);
    mode_subcat_add_fn(sc, fn);

        fn = create_user_fn(
	"tl_track_select_2",
	"Select track 2",
        user_tl_track_select_2
	);
    mode_subcat_add_fn(sc, fn);

        fn = create_user_fn(
	"tl_track_select_3",
	"Select track 3",
        user_tl_track_select_3
	);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"tl_track_select_4",
	"Select track 4",
        user_tl_track_select_4
	);
    mode_subcat_add_fn(sc, fn);

        fn = create_user_fn(
	"tl_track_select_5",
	"Select track 5",
        user_tl_track_select_5
	);
    mode_subcat_add_fn(sc, fn);

        fn = create_user_fn(
	"tl_track_select_6",
	"Select track 6",
        user_tl_track_select_6
	);
    mode_subcat_add_fn(sc, fn);

        fn = create_user_fn(
	"tl_track_select_7",
	"Select track 7",
        user_tl_track_select_7
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_select_8",
	"Select track 8",
        user_tl_track_select_8
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_select_9",
	"Select track 9",
        user_tl_track_select_9
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_activate_all",
	"Activate or deactivate all tracks",
        user_tl_track_activate_all
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_selector_up",
	"Move track selector up",
        user_tl_track_selector_up
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_selector_down",
	"Move track selector down",
        user_tl_track_selector_down
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_activate_selected",
	"Activate selected track",
        user_tl_track_activate_selected
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_mute",
	"Mute selected_track",
	user_tl_mute
	);
    mode_subcat_add_fn(sc, fn);


    fn = create_user_fn(
	"tl_grab_clips_at_point",
	"Grab clip at point",
	user_tl_clipref_grab_ungrab
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_load_clip_at_point_to_source",
	"Load clip at point to source",
        user_tl_load_clip_at_point_to_src
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_activate_source_mode",
	"Activate Source Mode",
	user_tl_activate_source_mode
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_drop_from_source",
	"Drop from source",
	user_tl_drop_from_source
	);
    mode_subcat_add_fn(sc, fn);

}

static void mode_load_source()
{
    Mode *mode = mode_create(SOURCE);
    ModeSubcat *sc= mode_add_subcat(mode, "");
    UserFn *fn;

    fn = create_user_fn(
	"source_play",
	"Play (source)",
        user_source_play
	);
    mode_subcat_add_fn(sc, fn);


    fn = create_user_fn(
	"source_pause",
	"Pause (source)",
        user_source_pause
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"source_rewind",
	"Rewind (source)",
        user_source_rewind
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"source_play_slow",
	"Play slow (source)",
	user_source_play_slow
	);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"source_rewind_slow",
	"Rewind slow (source",
	user_source_rewind_slow
	);
    mode_subcat_add_fn(sc, fn);
    
    fn = create_user_fn(
	"source_set_in_mark",
	"Set In Mark (source)",
        user_source_set_in_mark
	);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"source_set_out_mark",
	"Set Out Mark (source)",
        user_source_set_out_mark
	);
    mode_subcat_add_fn(sc, fn);

}


void input_init_mode_load_all()
{
    mode_load_global();
    mode_load_menu_nav();
    mode_load_timeline();
    mode_load_source();
}

void input_init_hash_table()
{
    memset(input_hash_table, '\0', INPUT_HASH_SIZE * sizeof(KeybNode*));
    memset(input_keyup_hash_table, '\0', INPUT_KEYUP_HASH_SIZE * sizeof(KeybNode*));
}

static int input_hash(uint16_t i_state, SDL_Keycode key)
{
    return (7 * i_state + 13 * key) % INPUT_HASH_SIZE;
}

static char *input_get_keycmd_str(uint16_t i_state, SDL_Keycode keycode);

UserFn *input_get(uint16_t i_state, SDL_Keycode keycode, InputMode mode)
{
    /* fprintf(stdout, "KEYCMD STR: %s\n", input_get_keycmd_str(i_state, keycode)); */
    int hash = input_hash(i_state, keycode);
    KeybNode *node = input_hash_table[hash];
    if (!node) {
	return NULL;
    }
    while (1) {
	/* fprintf(stdout, "Testing keycode %c against %c\n", keycode, node->kb->keycode); */
	/* fprintf(stdout, "\tmode %s, (found %s); i_state %d, %d\n", input_mode_str(mode), input_mode_str(node->kb->mode), i_state, node->kb->i_state); */
	if ((node->kb->mode == mode || node->kb->mode == GLOBAL) && node->kb->i_state == i_state && node->kb->keycode == keycode) {
	    return node->kb->fn;
	} else if (node->next) {
	    node = node->next;
	} else {
	    /* fprintf(stdout, "NOT FOUND\n"); */
	    return NULL;
	}
    }
}

static void input_read_keycmd(char *keycmd, uint16_t *i_state, SDL_Keycode *key)
{
    
    /* Get prefix */
    if (strncmp("C-S-", keycmd, 4) == 0) {
	*i_state =  I_STATE_CMDCTRL | I_STATE_SHIFT;
	keycmd += 4;
    } else if (strncmp("A-S-", keycmd, 4) == 0) {
	*i_state =  I_STATE_META | I_STATE_SHIFT;
	keycmd += 4;
    } else if (strncmp("C-A-", keycmd, 4) == 0) {
	*i_state =  I_STATE_CMDCTRL | I_STATE_META;
	keycmd += 4;
    } else if (strncmp("C-", keycmd, 2) == 0) {
	*i_state =  I_STATE_CMDCTRL;
	keycmd += 2;
    } else if (strncmp("S-", keycmd, 2) == 0) {
	*i_state =  I_STATE_SHIFT;
	keycmd += 2;
    } else if (strncmp("A-", keycmd, 2) == 0) {
	*i_state =  I_STATE_META;
	keycmd += 2;
    } else if (strncmp("K-", keycmd, 2) == 0) {
	*i_state = I_STATE_K;
	keycmd += 2;
    } else {
	*i_state =  0;
    }

    if (strcmp(keycmd, "<ret>") == 0) {
	*key = SDLK_RETURN;
    } else if (strcmp(keycmd, "<esc>") == 0) {
	*key = SDLK_ESCAPE;
    } else if (strcmp(keycmd, "<del>") == 0) {
	*key = SDLK_BACKSPACE;
    } else if (strcmp(keycmd, "<tab>") == 0) {
	*key = SDLK_TAB;
    } else if (strcmp(keycmd, "<up>") == 0) {
	*key = SDLK_UP;
    } else if (strcmp(keycmd, "<down>") == 0) {
        *key = SDLK_DOWN;
    } else if (strcmp(keycmd, "<left>") == 0) {
	*key = SDLK_LEFT;
    } else if (strcmp(keycmd, "<right>") == 0) {
	*key = SDLK_RIGHT;
    } else {
	*key = * (char *) keycmd;
    }
    
}

static char *input_get_keycmd_str(uint16_t i_state, SDL_Keycode keycode)
{
    char buf[32];
    memset(buf, '\0', 32);
    const char *mod;
    switch (i_state) {
    case (0):
	mod = "";
	break;
    case (I_STATE_CMDCTRL):
	mod = "C-";
	break;
    case (I_STATE_SHIFT):
	mod = "S-";
	break;
    case (I_STATE_CMDCTRL | I_STATE_SHIFT):
	mod = "C-S-";
	break;
    case I_STATE_META:
	mod = "A-";
	break;
    case (I_STATE_META | I_STATE_SHIFT):
	mod = "A-S-";
	break;
    case (I_STATE_META | I_STATE_CMDCTRL):
	mod = "C-A-";
	break;
    case (I_STATE_K):
	mod = "K-";
	break;
    default:
	break;
    }

    switch (keycode) {
    case SDLK_RETURN:
	sprintf(buf, "%s<ret>", mod);
	break;
    case SDLK_ESCAPE:
	sprintf(buf, "%s<esc>", mod);
	break;
    case SDLK_BACKSPACE:
	sprintf(buf, "%s<del>", mod);
	break;
    case SDLK_TAB:
	sprintf(buf, "%s<tab>", mod);
	break;
    case SDLK_UP:
	sprintf(buf, "%s<up>", mod);
	break;
    case SDLK_DOWN:
	sprintf(buf, "%s<down>", mod);
	break;
    case SDLK_LEFT:
	sprintf(buf, "%s<left>", mod);
	break;
    case SDLK_RIGHT:
	sprintf(buf, "%s<right>", mod);
	break;
    default:
	sprintf(buf, "%s%c", mod, keycode);
    }

    char *ret = malloc(strlen(buf) + 1);
    strcpy(ret, buf);
    /* fprintf(stdout, "\t->keycmd str %s (%s)\n", ret, buf); */
    return ret;
}

/* Returns null if no function found by that id */
UserFn *input_get_fn_by_id(char *id, InputMode im)
{
    Mode *mode = modes[im];
    for (uint8_t s=0; s<mode->num_subcats; s++) {
	ModeSubcat *sc = mode->subcats[s];
	for (uint8_t f=0; f<sc->num_fns; f++) {
	    UserFn *fn = sc->fns[f];
	    if (strcmp(id, fn->fn_id) == 0) {
		return fn;
	    }
	}
    }
    return NULL;
}

void input_bind_fn(UserFn *fn, uint16_t i_state, SDL_Keycode keycode, InputMode mode)
{
    int hash = input_hash(i_state, keycode);
    /* fprintf(stdout, "Binding input %s in mode %s. Root: %p\n", input_get_keycmd_str(i_state, keycode), input_mode_str(mode), &input_hash_table[hash]); */
    KeybNode *keyb_node = input_hash_table[hash];
    /* KeybNode *last = NULL; */
    Keybinding *kb = malloc(sizeof(Keybinding));
    /* UserFn *user_fn = NULL; */
    if (!keyb_node) {
	/* fprintf(stdout, "\t->first slot empty\n"); */
	keyb_node = malloc(sizeof(KeybNode));
	keyb_node->kb = kb;
	keyb_node->next = NULL;
	input_hash_table[hash] = keyb_node;
    } else {
	while (keyb_node->kb->mode != mode || keyb_node->kb->i_state != i_state || keyb_node->kb->keycode != keycode) {
	    if (keyb_node->next) {
		/* fprintf(stdout, "\t->slot %p taken, next...\n", &keyb_node); */
		/* last = keyb_node; */
		keyb_node = keyb_node->next;
	    } else {
		keyb_node->next = malloc(sizeof(KeybNode));
		keyb_node = keyb_node->next;
		/* fprintf(stdout, "\t->inserting at %p\n", &keyb_node); */
		kb = malloc(sizeof(Keybinding));
		keyb_node->kb = kb;
		keyb_node->next = NULL;
		break;
	    }   
	}
    }
    kb->mode = mode;
    kb->i_state = i_state;
    kb->keycode = keycode;
    kb->keycmd_str = input_get_keycmd_str(i_state, keycode);
    if (!fn->annotation) {
	/* fprintf(stdout, "binding fn to %s\n", kb->keycmd_str); */
	fn->annotation = kb->keycmd_str;
    }
    kb->fn = fn;
    /* if (last) { */
    /* 	last->next = keyb_node; */
    /* }  */
}


static int check_next_line_indent(FILE *f)
{
    char c;
    int ret = 0;
    while (fgetc(f) != '\n') {}
    while (is_whitespace_char((c = fgetc(f)))) {
	if (c == EOF) {
	    return -1;
	}
	ret++;
    }
    ungetc(c, f);
    return ret;
    
}

static Layout *create_menu_layout()
{
    Layout *menu_lt = layout_add_child(main_win->layout);
    layout_set_default_dims(menu_lt);
    menu_lt->w.value.intval = 1200;
    layout_reset(menu_lt);
    return menu_lt;
}
    

Menu *input_create_menu_from_mode(InputMode im)
{
    Mode *mode = modes[im];
    if (!mode) {
	fprintf(stderr, "Error: mode %s not initialized\n", input_mode_str(im));
	exit(1);
    }
    Layout *m_layout = create_menu_layout();
    if (!m_layout) {
	fprintf(stderr, "Error: Unable to create menu layout\n");
	exit(1);
    }
    Menu *m = menu_create(m_layout, main_win);
    MenuColumn *c = menu_column_add(m, "");
    for (int i=0; i<mode->num_subcats; i++) {
	ModeSubcat *sc = mode->subcats[i];
	MenuSection *sctn = menu_section_add(c, sc->name);
	for (int j=0; j<sc->num_fns; j++) {
	    UserFn *fn = sc->fns[j];
	    menu_item_add(sctn, fn->fn_display_name, fn->annotation, fn->do_fn, NULL);
	}
    }
    menu_add_header(m, mode->name, "Here are functions available to you in aforementioned mode.");
    return m;
}


Menu *input_create_master_menu()
{
    InputMode im = GLOBAL;

    Layout *m_layout = create_menu_layout();
    if (!m_layout) {
	fprintf(stderr, "Error: Unable to create menu layout\n");
	exit(1);
    }
    Menu *m = menu_create(m_layout, main_win);
    while (im < NUM_INPUT_MODES) {
	Mode *mode = modes[im];
	if (!mode) {
	    fprintf(stderr, "Error: mode %s not initialized\n", input_mode_str(im));
	    exit(1);
	}
	/* Layout *m_layout = create_menu_layout(); */
	/* if (!m_layout) { */
	/*     fprintf(stderr, "Error: Unable to create menu layout\n"); */
	/*     exit(1); */
	/* } */
	MenuColumn *c = menu_column_add(m, mode->name);
	for (int i=0; i<mode->num_subcats; i++) {
	    ModeSubcat *sc = mode->subcats[i];
	    MenuSection *sctn = menu_section_add(c, sc->name);
	    for (int j=0; j<sc->num_fns; j++) {
		UserFn *fn = sc->fns[j];
		menu_item_add(sctn, fn->fn_display_name, fn->annotation, fn->do_fn, NULL);
	    }
	}
	im++;
    }

    menu_add_header(m, "ALL", "Here are functions available to you in aforementioned mode.");
    return m;
}

void input_load_keybinding_config(const char *filepath)
{
    FILE *f = fopen(filepath, "r");
    if (!f) {
	fprintf(stderr, "Error: failed to open file at %s\n", filepath);
	return;
    }
    char c;
    int i=0;
    char buf[255];
    
    /* Get through comments */
    bool comment = true;
    while (comment) {
	while ((c = fgetc(f)) != '\n') {}
	if ((c = fgetc(f)) != '#' && c != '\n') {
	    comment = false;
	}
    }
    
    bool more_modes = true;

    while (more_modes) {
	/* Get mode name */

	while ((c = fgetc(f)) == '-' || c == ' ' || c == '\t' || c == '\n') {}
	/* ungetc(c, f); */
	i = 0;
	do {
	    buf[i] = c;
	    i++;
	    c = fgetc(f);
	} while (c != ':');
	buf[i] = '\0';
	
	InputMode mode = input_mode_from_str(buf);
	if (mode == -1) {
	    fprintf(stderr, "Error: no mode under name %s\n", buf);
	    exit(1);
	}
	

	bool more_bindings = true;

	while (more_bindings) {
	    
	    uint16_t i_state = 0;
	    SDL_Keycode key = 0;
	    UserFn *fn = NULL;
	    
	    int next_line_indent;
	    
	    if ((next_line_indent = check_next_line_indent(f)) < 0) {
		more_modes = false;
		break;
	    } else if (next_line_indent == 0) {
		break;
	    }

	    while ((c = fgetc(f)) == '-' || c == ' ' || c == '\t') {}
	    
	    /* Get input pattern */
	    i = 0;
	    do {
		buf[i] = c;
		i++;
		c = fgetc(f);
	    } while (c != ':' && c != ' ' && c != '\t');
	    buf[i] = '\0';
	    input_read_keycmd(buf, &i_state, &key);
	    while ((c = fgetc(f)) == ' ' || c == '\t' || c == ':') {}

	    /* Get fn id*/
	    i = 0;
	    do {
		buf[i] = c;
		i++;
		c = fgetc(f);
	    } while (c != '\n' && c != EOF);

	    if (c == EOF) {
		more_bindings = false;
		more_modes = false;
	    }
	    buf[i] = '\0';
	    ungetc(c, f);
	    fn = input_get_fn_by_id(buf, mode);

	    if (fn) {
		input_bind_fn(fn, i_state, key, mode);
	    } else {
		fprintf(stderr, "Error: no function found with id %s\n", buf);
	    }
	}

    }
}
