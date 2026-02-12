/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <string.h>
#include <stdbool.h>

#include "assets.h"
#include "input.h"
#include "input_mode.h"
#include "layout.h"
#include "log.h"
#include "menu.h"

#define not_whitespace_char(c) (c != ' ' && c != '\n' && c != '\t')
#define is_whitespace_char(c) ((c) == ' ' || (c) == '\n' || (c) == '\t')

#define FN_REFERENCE_FILENAME "fn_reference.md"

extern Mode *MODES[];
static KeybNode *INPUT_HASH_TABLE[INPUT_HASH_SIZE];
/* KeybNode *input_keyup_hash_table[INPUT_KEYUP_HASH_SIZE]; */
extern Window *main_win;



void input_init_mode_load_all()
{
    mode_load_all();
}


void input_init_hash_table()
{
    memset(INPUT_HASH_TABLE, '\0', INPUT_HASH_SIZE * sizeof(KeybNode*));
    /* memset(input_keyup_hash_table, '\0', INPUT_KEYUP_HASH_SIZE * sizeof(KeybNode*)); */
}

static int input_hash(uint16_t i_state, SDL_Keycode key)
{
    return (7 * i_state + 13 * key) % INPUT_HASH_SIZE;
}

char *input_get_keycmd_str(uint16_t i_state, SDL_Keycode keycode);

UserFn *input_get(uint16_t i_state, SDL_Keycode keycode)
{
    int hash = input_hash(i_state, keycode);
    int win_mode_i = main_win->num_modes - 1;
    /* fprintf(stderr, "Main win num modes = %d (starting %d)\n", main_win->num_modes, win_mode_i); */
    while (win_mode_i >= -1) {
	InputMode mode = main_win->modes[win_mode_i];
	/* fprintf(stderr, "\tCHECKING INPUT mode %d (%s)\n", mode, input_mode_str(mode)); */
	KeybNode *init_node = INPUT_HASH_TABLE[hash];
	KeybNode *node = init_node;
	if (!node) {
	    return NULL;
	}
	while (1) {
	    /* if ((node->kb->mode == mode || node->kb->mode == GLOBAL) && node->kb->i_state == i_state && node->kb->keycode == keycode) { */
	    if ((node->kb->mode == mode) && node->kb->i_state == i_state && node->kb->keycode == keycode) {
		if (node->kb->mode == MODE_GLOBAL && mode == MODE_TEXT_EDIT) {
		    txt_stop_editing(main_win->txt_editing);
		}
		return node->kb->fn;
	    } else if (node->next) {
		node = node->next;
	    } else {
		break;
	    }
	}
	if (mode == MODE_TEXT_EDIT) return NULL; /* No "sieve" for text edit mode */
	win_mode_i--;
	if (win_mode_i < 0) {
	    mode = MODE_GLOBAL;
	} else {
	    mode = main_win->modes[win_mode_i];
	}
    }
    return NULL;
}


/* char *input_state_str(uint16_t i_state) */
/* { */
    

/* } */

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
    } else if (strncmp("S-g-", keycmd, 4) == 0) {
	*i_state = I_STATE_SHIFT | I_STATE_G;
	keycmd += 4;
    } else if (strncmp("S-", keycmd, 2) == 0) {
	*i_state =  I_STATE_SHIFT;
	keycmd += 2;
    } else if (strncmp("A-", keycmd, 2) == 0) {
	*i_state =  I_STATE_META;
	keycmd += 2;
    } else if (strncmp("k-", keycmd, 2) == 0) {
	*i_state = I_STATE_K;
	keycmd += 2;
    } else if (strncmp("g-", keycmd, 2) == 0) {
	*i_state = I_STATE_G;
	keycmd += 2;
    } else {
	*i_state =  0;
    }

    if (strcmp(keycmd, "<ret>") == 0) {
	*key = SDLK_RETURN;
    } else if (strcmp(keycmd, "<esc>") == 0) {
	*key = SDLK_ESCAPE;
    } else if (strcmp(keycmd, "<spc>") == 0) {
	*key = SDLK_SPACE;
    } else if (strcmp(keycmd, "<del>") == 0) {
	*key = SDLK_BACKSPACE;
    } else if (strcmp(keycmd, "<tab>") == 0) {
	*key = SDLK_TAB;
    } else if (strcmp(keycmd, "<up>") == 0) {
	*key = SDLK_UP;
    } else if (strcmp(keycmd, "<down>") == 0) {
        *key = SDLK_DOWN;
    } else if (strcmp(keycmd, "<left>") == 0) {
	/* fprintf(stdout, "SETTING KEY TO %d\n", SDLK_LEFT); */
	/* exit(0); */
	*key = SDLK_LEFT;
    } else if (strcmp(keycmd, "<right>") == 0) {
	*key = SDLK_RIGHT;
    } else {
	*key = * (char *) keycmd;
    }
    
}

char *input_get_keycmd_str(uint16_t i_state, SDL_Keycode keycode)
{
    char buf[32];
    memset(buf, '\0', 32);
    /* fprintf(stderr, "GET KEYCMD i state %x, keycode %c\n", i_state, keycode); */
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
    case I_STATE_K:
	mod = "k-";
	break;
    case I_STATE_G:
	mod = "g-";
	break;
    case (I_STATE_SHIFT | I_STATE_G):
	mod = "S-g-";
	break;
    default:
	mod = "";
	break;
    }

    switch (keycode) {
    case SDLK_RETURN:
	snprintf(buf, sizeof(buf),"%s<ret>", mod);
	break;
    case SDLK_SPACE:
	snprintf(buf, sizeof(buf), "%s<spc>", mod);
	break;
    case SDLK_ESCAPE:
	snprintf(buf, sizeof(buf), "%s<esc>", mod);
	break;
    case SDLK_BACKSPACE:
	snprintf(buf, sizeof(buf), "%s<del>", mod);
	break;
    case SDLK_TAB:
	snprintf(buf, sizeof(buf), "%s<tab>", mod);
	break;
    case SDLK_UP:
	snprintf(buf, sizeof(buf), "%s<up>", mod);
	break;
    case SDLK_DOWN:
	snprintf(buf, sizeof(buf), "%s<down>", mod);
	break;
    case SDLK_LEFT:
	snprintf(buf, sizeof(buf), "%s<left>", mod);
	break;
    case SDLK_RIGHT:
	snprintf(buf, sizeof(buf), "%s<right>", mod);
	break;
    default:
	snprintf(buf, sizeof(buf), "%s%c", mod, keycode);
    }

    char *ret = malloc(strlen(buf) + 1);
    strcpy(ret, buf);
    /* fprintf(stdout, "\t->keycmd str %s (%s)\n", ret, buf); */
    return ret;
}


void nullfn(void *arg){}

/* typedef struct user_fn { */
/*     const char *fn_id; */
/*     const char *fn_display_name; */
/*     char annotation[MAX_ANNOT_STRLEN]; */
/*     bool is_toggle; */
/*     void (*do_fn)(void *arg); */
/*     Mode *mode; */

/*     Keybinding *key_bindings[MAX_FN_KEYBS]; */
/*     int num_keybindings; */
/* } UserFn; */

static UserFn null_userfn = {
    "null",
    "null (block)",
    {0},
    false,
    nullfn,
    NULL,
    {0},
    0
};

bool is_null_userfn(UserFn *fn)
{
    return fn == &null_userfn;
}

/* Returns null if no function found by that id */
UserFn *input_get_fn_by_id(char *id, InputMode im)
{
    if (strncmp(id, "null", 4) == 0) {
	return &null_userfn;
    }
    Mode *mode = MODES[im];
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
    /* fn->hashes[fn->num_hashes] = hash; */
    /* fn->i_states[fn->num_hashes] = i_state; */
    /* fn->keycodes[fn->num_hashes] = keycode; */
    /* fn->num_hashes++; */
    /* fprintf(stdout, "Binding input %s in mode %s. Root: %p\n", input_get_keycmd_str(i_state, keycode), input_mode_str(mode), &input_hash_table[hash]); */
    KeybNode *keyb_node = INPUT_HASH_TABLE[hash];
    /* KeybNode *last = NULL; */
    Keybinding *kb = calloc(1, sizeof(Keybinding));
    /* UserFn *user_fn = NULL; */
    if (!keyb_node) {
	/* fprintf(stdout, "\t->first slot empty\n"); */
	keyb_node = calloc(1, sizeof(KeybNode));
	keyb_node->kb = kb;
	keyb_node->next = NULL;
	INPUT_HASH_TABLE[hash] = keyb_node;
    } else {
	while (keyb_node->kb->mode != mode || keyb_node->kb->i_state != i_state || keyb_node->kb->keycode != keycode) {
	    if (keyb_node->next) {
		/* fprintf(stdout, "\t->slot %p taken, next...\n", &keyb_node); */
		/* last = keyb_node; */
		keyb_node = keyb_node->next;
	    } else {
		keyb_node->next = calloc(1, sizeof(KeybNode));
		keyb_node = keyb_node->next;
		/* fprintf(stdout, "\t->inserting at %p\n", &keyb_node); */
		/* kb = malloc(sizeof(Keybinding)); */
		keyb_node->kb = kb;
		keyb_node->next = NULL;
		break;
	    }   
	}
    }
    kb->mode = mode;
    kb->i_state = i_state;
    kb->keycode = keycode;
    kb->hash = hash;
    kb->keycmd_str = input_get_keycmd_str(i_state, keycode);
    if (fn->num_keybindings == MAX_FN_KEYBS) {
	log_tmp(LOG_ERROR, "Max num keybindings reached for %s\n", kb->keycmd_str);
    } else {
	fn->key_bindings[fn->num_keybindings] = kb;
	fn->num_keybindings++;
    }
    if (fn->annotation[0] == '\0') {
	strcat(fn->annotation, kb->keycmd_str);
    } else {
	strcat(fn->annotation, "\t/\t");
	strcat(fn->annotation, kb->keycmd_str);
    }
    /* if (!fn->annotation) { */
    /* 	/\* fprintf(stdout, "binding fn to %s\n", kb->keycmd_str); *\/ */
    /* 	fn->annotation = kb->keycmd_str; */
    /* } else { */
    /* 	strcat((char *)fn->annotation, kb->keycmd_str); */
    /* } */
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
    /* while (is_whitespace_char((c = fgetc(f)))) { */
    while (1) {
	c = fgetc(f);
	if (c == EOF) {
	    return -1;
	}
	if (!is_whitespace_char(c)) {
	    ungetc(c, f);
	    return ret;
	}
	ret++;
    }
    ungetc(c, f);
    
}

static Layout *create_menu_layout()
{
    Layout *menu_lt = layout_add_child(main_win->layout);
    layout_set_default_dims(menu_lt);
    menu_lt->w.value = 1200.0f;
    layout_reset(menu_lt);
    return menu_lt;
}

static void create_menu_from_mode_subcat(void *sc_v)
{
    ModeSubcat *sc = (ModeSubcat *)sc_v;
    Layout *m_layout = create_menu_layout();
    Menu *m = menu_create(m_layout, main_win);
    MenuColumn *c = menu_column_add(m, sc->name);
    MenuSection *sctn = menu_section_add(c, "");
    for (uint8_t i=0; i<sc->num_fns; i++) {
	UserFn *fn = sc->fns[i];
	menu_item_add(sctn, fn->fn_display_name, fn->annotation, fn->do_fn, NULL);
    }
    menu_add_header(m, sc->name, "n  -  next item\np  -  previous item\nh  -  go back (dismiss)\n<ret>  -  select item");
    window_add_menu(main_win, m);
    /* if (main_win->modes[main_win->num_modes - 1] != MENU_NAV) { */
    /* 	window_push_mode(main_win, MENU_NAV); */
    /* } */
}

void input_create_menu_from_mode(InputMode im)
{
    Mode *mode = MODES[im];
    if (!mode) {
	fprintf(stderr, "Error: mode %s not initialized\n", input_mode_str(im));
    }
    if (mode->num_subcats == 1) {
	create_menu_from_mode_subcat(mode->subcats[0]);
    } else {
	Layout *m_layout = create_menu_layout();
	Menu *m = menu_create(m_layout, main_win);
	MenuColumn *c = menu_column_add(m, "");
	MenuSection *sctn = menu_section_add(c, "");
	for (uint8_t i=0; i<mode->num_subcats; i++) {
	    ModeSubcat *sc = mode->subcats[i];
	    menu_item_add(sctn, sc->name, ">", create_menu_from_mode_subcat, sc);
	}
	menu_add_header(m, "", "n  -  next item\np  -  previous item\nh  -  go back (dismiss)\n<ret>  -  select item");
	window_add_menu(main_win, m);
	/* if (main_win->modes[main_win->num_modes - 1] != MENU_NAV) { */
	/*     window_push_mode(main_win, MENU_NAV); */
	/* } */
    }
    /* return NULL; */
}




/* Menu *input_create_menu_from_mode(InputMode im) */
/* { */
/*     Mode *mode = modes[im]; */
/*     if (!mode) { */
/* 	fprintf(stderr, "Error: mode %s not initialized\n", input_mode_str(im)); */
/* 	exit(1); */
/*     } */
/*     Layout *m_layout = create_menu_layout(); */
/*     if (!m_layout) { */
/* 	fprintf(stderr, "Error: Unable to create menu layout\n"); */
/* 	exit(1); */
/*     } */
/*     Menu *m = menu_create(m_layout, main_win); */
/*     for (int i=0; i<mode->num_subcats; i++) { */
/* 	ModeSubcat *sc = mode->subcats[i]; */
/* 	MenuColumn *c = menu_column_add(m, sc->name); */
/* 	MenuSection *sctn = menu_section_add(c, ""); */
/* 	for (int j=0; j<sc->num_fns; j++) { */
/* 	    UserFn *fn = sc->fns[j]; */
/* 	    menu_item_add(sctn, fn->fn_display_name, fn->annotation, fn->do_fn, NULL); */
/* 	} */
/*     } */
/*     menu_add_header(m, mode->name, "Here are functions available to you in aforementioned mode."); */
/*     return m; */
/* } */


Menu *input_create_master_menu()
{
    /* InputMode im = GLOBAL; */

    Layout *m_layout = create_menu_layout();
    if (!m_layout) {
	fprintf(stderr, "Error: Unable to create menu layout\n");
	exit(1);
    }
    Menu *m = menu_create(m_layout, main_win);
    /* while (im < NUM_INPUT_MODES) { */
    for (uint8_t i=0; i<main_win->num_modes + 1; i++) {
	InputMode im;
	if (i == 0) {
	    im = MODE_GLOBAL;
	} else {
	    im = main_win->modes[i - 1];

	}
	if (im == MODE_MENU_NAV) continue;
	Mode *mode = MODES[im];
	if (!mode) {
	    fprintf(stderr, "Error: mode %s not initialized\n", input_mode_str(im));
	    exit(1);
	}
	/* Layout *m_layout = create_menu_layout(); */
	/* if (!m_layout) { */
	/*     fprintf(stderr, "Error: Unable to create menu layout\n"); */
	/*     exit(1); */
	/* } */
	if (mode->num_subcats == 1) {
	    MenuColumn *c = menu_column_add(m, "");
	    MenuSection *sctn = menu_section_add(c, mode->name);
	    for (uint8_t i=0; i<mode->subcats[0]->num_fns; i++) {
		UserFn *fn = mode->subcats[0]->fns[i];
		menu_item_add(sctn, fn->fn_display_name, fn->annotation, fn->do_fn, NULL);
	    }
	} else {
	    MenuColumn *c = menu_column_add(m, "");
	    for (int i=0; i<mode->num_subcats; i++) {
		ModeSubcat *sc = mode->subcats[i];
	    
		MenuSection *sctn = menu_section_add(c, sc->name);
		menu_item_add(sctn, sc->name, ">", create_menu_from_mode_subcat, sc);
	    }
	    /* for (int j=0; j<sc->num_fns; j++) { */
	    /* 	UserFn *fn = sc->fns[j]; */
	    /* 	menu_item_add(sctn, fn->fn_display_name, fn->annotation, fn->do_fn, NULL); */
	    /* } */
	/* } */
	}
    }

    menu_add_header(m, "", "n - next item\np - previous item\nl - column right\nj - column left\n<ret> - select\nm - dismiss");
    layout_center_agnostic(m_layout, true, true);
    menu_reset_layout(m);
    return m;
}

void input_load_keybinding_config(const char *asset_path)
{
    FILE *f = asset_open(asset_path, "r");
    /* FILE *f = fopen(filepath, "r"); */
    if (!f) {
	fprintf(stderr, "Error: failed to open asset at %s\n", asset_path);
	return;
    }
    int c;
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
	    return;
	    /* exit(1); */
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
	    
	    while ((c = fgetc(f)) == ' ' || c == '\t' || c == '\n') {}
	    if (c != '-') {
		fprintf(stderr, "YAML parse error; expected '-' and got '%c'\n", c);
		exit(1);
	    }
	    while ((c = fgetc(f)) == ' ' || c == '\t') {}
	    
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
	    int end_check = fgetc(f);
	    if (end_check == EOF) {
		more_bindings = false;
		more_modes = false;		
	    } else {
		ungetc(end_check, f);
	    }
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
#define MAX_KEYB_BLOCKS 64

bool input_function_is_accessible(UserFn *fn, Window *win)
{
    InputMode keyb_blocks[MAX_KEYB_BLOCKS];
    int num_keyb_blocks = 0;
    for (int i=0; i<fn->num_keybindings; i++) {
	Keybinding *kb = fn->key_bindings[i];
	int hash = kb->hash;
	/* int hash = fn->hashes[i]; */
	KeybNode *kn = INPUT_HASH_TABLE[hash];
	while (kn) {
	    if (kn->kb != kb && kn->kb->i_state == kb->i_state && kn->kb->keycode == kb->keycode) {
		if (num_keyb_blocks == 64) {
		    fprintf(stderr, "ERROR: %d keyb blocks in input_function_is_accessible\n", MAX_KEYB_BLOCKS);
		    return false;
		}

		if (!is_null_userfn(kn->kb->fn)) {
		    keyb_blocks[num_keyb_blocks] = kn->kb->fn->mode->im;
		    num_keyb_blocks++;
		}
	    }
	    kn = kn->next;
	}
    }
    /* Don't check AUTOCOMPLETE LIST, hence -2 not -1 */
    for (int i=win->num_modes-2; i>=-1; i--) {
	InputMode im;
	if (i == -1) im = MODE_GLOBAL;
	else im = win->modes[i];
	if (im == MODE_TEXT_EDIT) continue;
	if (fn->mode->im == im) return true;
	else {
	    for (int i=0; i<num_keyb_blocks; i++) {
		if (keyb_blocks[i] == im) {
		    return false; /* Blocked in sieve */
		}
	    }
	}
    }
    return false;
}


void input_quit()
{
    KeybNode *node = NULL;
    for (int i=0; i<INPUT_HASH_SIZE; i++) {
	node = INPUT_HASH_TABLE[i];
	KeybNode *nodes[64];
	uint8_t num_nodes = 0;
	while (node) {
	    nodes[num_nodes] = node;
	    num_nodes++;
	    node = node->next;
	}
	while (num_nodes > 0) {
	    node = nodes[num_nodes - 1];
	    free(node->kb->keycmd_str);
	    free(node->kb);
	    free(node);
	    num_nodes--;
	}
	
    }
    for (uint8_t i=0; i<NUM_INPUT_MODES; i++) {
	Mode *mode = MODES[i];
	for (uint8_t j=0; j<mode->num_subcats; j++) {
	    ModeSubcat *sc = mode->subcats[j];
	    for (uint8_t k=0; k<sc->num_fns; k++) {
		UserFn *fn = sc->fns[k];
		free(fn);
	    }
	    free(sc);
	}
	free(mode);
    }
}

void input_create_function_reference()
{
    fprintf(stdout, "Creating function reference at %s\n", FN_REFERENCE_FILENAME);
    FILE *f = fopen(FN_REFERENCE_FILENAME, "w");
    if (!f) {
	fprintf(stderr, "Error: unable to open file %s\n", FN_REFERENCE_FILENAME);
    }
    for (uint8_t i=0; i<NUM_INPUT_MODES; i++) {
	Mode *mode = MODES[i];
	fprintf(f, "### %s mode\n", mode->name);
	for (uint8_t j=0; j<mode->num_subcats; j++) {
	    ModeSubcat *sc = mode->subcats[j];
	    if (mode->num_subcats > 1) {
		fprintf(f, "#### %s\n", sc->name);
	    }
	    for (uint8_t k=0; k<sc->num_fns; k++) {
		UserFn *fn = sc->fns[k];
		char buf[255] = {0};
		char *c = fn->annotation;
		int i=0;
		/* bool had_multiple = false; */
		snprintf(buf, 6, "<kbd>");
		i += 5;
		while (*c != '\0') {
		    if (*c == '\t') {
			/* if (had_multiple) { */
			/*     strlcpy(buf + i, ", <kbd>", 5); */
			/*     i+=5; */
			/* } */
			snprintf(buf + i, 14, "</kbd>, <kbd>");
			/* strlcpy(buf + i, "</kbd>, <kbd>", 13); */
			i+=13;
			/* had_multiple = true; */
			c+=3;
		    } else if (*c == '<') {
			snprintf(buf + i, 3, "\\<");
			i+=2;
			c++;
		    } else if (*c == '>') {
			snprintf(buf + i, 3, "\\>");
			i+=2;
			c++;
		    } else {
			snprintf(buf + i, 2, "%s", c);
			c++;
			i++;
		    }
	
		}
		buf[i] = '\0';
		fprintf(f, "- %s : %s</kbd>\n", fn->fn_display_name, buf);
		/* fprintf(f, "     - <kbd>%s</kbd>\n", fn->annotation); */
	    }
	}
    }
    fclose(f);
}
