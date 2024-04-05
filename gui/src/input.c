#include <string.h>
#include <stdbool.h>
#include "input.h"
#include "userfn.h"



#define not_whitespace_char(c) (c != ' ' && c != '\n' && c != '\t')
#define is_whitespace_char(c) (c == ' ' || c == '\n' || c == '\t')

Mode *modes[NUM_INPUT_MODES];
KeybNode *input_hash_table[INPUT_HASH_SIZE];

const char *input_mode_str(InputMode im)
{
    switch (im) {
    case GLOBAL:
	return "global";
    case MENU_NAV:
	return "menu_nav";
    case PROJECT:
	return "project";
    default:
	fprintf(stderr, "ERROR: [no mode string for value %d]\n", im);
	return NULL;
    }
}

InputMode input_mode_from_str(char *str)
{
    if (strcmp(str, "global") == 0) {
	return GLOBAL;
    } else if (strcmp(str, "menu_nav") == 0) {
	return MENU_NAV;
    } else if (strcmp(str, "project") == 0) {
	return PROJECT;
    } else {
	return -1;
    }
}

static Mode *mode_create(InputMode im)
{
    Mode *mode = malloc(sizeof(Mode));
    mode->name = input_mode_str(GLOBAL);
    modes[im] = mode;	
    return mode;
}

static ModeSubcat *mode_add_subcat(Mode *mode, const char *name)
{
    ModeSubcat *sc = malloc(sizeof(ModeSubcat));
    mode->subcats[mode->num_subcats] = sc;
    mode->num_subcats++;
    sc->name = name;
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
    const char *fn_annot,
    void (*do_fn) (void))
{
    UserFn *fn = malloc(sizeof(UserFn));
    fn->fn_id = fn_id;
    fn->fn_display_name = fn_display_name;
    fn->annotation = fn_annot;
    fn->do_fn = do_fn;
    return fn;
}

static void mode_load_global()
{
    Mode *mode = mode_create(GLOBAL);
    ModeSubcat *mc = mode_add_subcat(mode, "");

    /* exit(0); */
    
    UserFn *fn;
    fn = create_user_fn(
	"expose_help",
	"Expose help",
	"[annot]",
	user_global_expose_help
	);
    mode_subcat_add_fn(mc, fn);
    
    fn = create_user_fn(
	"quit",
	"Quit",
	"[annot]",
	user_global_quit
	);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"undo",
	"Undo",
	"[annot]",
	user_global_undo
	);

    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"redo",
	"Redo",
	"[annot]",
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
	"[annot]",
	user_menu_nav_next_item
	);
    mode_subcat_add_fn(mc, fn);
    
    fn = create_user_fn(
	"menu_previous_item",
	"Previous item",
	"[annot]",
	user_menu_nav_prev_item
	);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"menu_next_section",
	"Next section",
	"[annot]",
	user_menu_nav_next_sctn
	);

    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"menu_previous_section",
	"Previous section",
	"[annot]",
	user_menu_nav_prev_sctn
	);
    mode_subcat_add_fn(mc, fn);

    
    fn = create_user_fn(
	"menu_choose_item",
	"Choose item",
	"[annot]",
	user_menu_nav_choose_item
	);
    mode_subcat_add_fn(mc, fn);
}

static void mode_load_project()
{
    Mode *mode = mode_create(PROJECT);
    ModeSubcat *mc = mode_add_subcat(mode, "");
}

void input_init_mode_load_all()
{
    mode_load_global();
    mode_load_menu_nav();
    mode_load_project();
}

void input_init_hash_table()
{
    memset(input_hash_table, '\0', INPUT_HASH_SIZE * sizeof(KeybNode*));

}

static int input_hash(uint16_t i_state, SDL_Keycode key)
{
    return (7 * i_state + 13 * key) % INPUT_HASH_SIZE;
}

static int input_fn_hash(char *fn_id)
{
    //TODO:
    return 0;
}

UserFn *input_get(uint16_t i_state, SDL_Keycode keycode, InputMode mode)
{
    int hash = input_hash(i_state, keycode);
    KeybNode *node = input_hash_table[hash];
    if (!node) {
	return NULL;
    }
    while (1) {
	if ((node->kb->mode == mode || node->kb->mode == GLOBAL) && node->kb->i_state == i_state && node->kb->keycode == keycode) {
	    return node->kb->fn;
	} else if (node->next) {
	    node = node->next;
	} else {
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
    }

    char *ret = malloc(strlen(buf));
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
    fprintf(stdout, "SET %d\n", hash);
    KeybNode *keyb_node = input_hash_table[hash];
    KeybNode *last = NULL;
    Keybinding *kb = NULL;
    UserFn *user_fn = NULL;
    if (!keyb_node) {
	keyb_node = malloc(sizeof(KeybNode));
        kb = malloc(sizeof(Keybinding));
	keyb_node->kb = kb;
	kb->mode = mode;
	kb->i_state = i_state;
	kb->keycode = keycode;
	kb->keycmd_str = input_get_keycmd_str(i_state, keycode);
	keyb_node->next = NULL;
	input_hash_table[hash] = keyb_node;
    } else {
	while (keyb_node->kb->mode != mode || keyb_node->kb->i_state != i_state || keyb_node->kb->keycode != keycode) {
	    if (keyb_node->next) {
		last = keyb_node;
		keyb_node = keyb_node->next;
		kb = keyb_node->kb;
	    } else {
		fprintf(stderr, "Error: entry in hash table has no next pointer.\n");
		return;
	    }   
	}
    }
    kb->fn = fn;
    if (last) {
	last->next = keyb_node;
    } 
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
    /* fprintf(stdout, "\t\t->Returning %d\n", ret); */
    return ret;
    
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
	fprintf(stdout, "done input mode from str\n");
	if (mode == -1) {
	    fprintf(stdout, "Error: no mode under name %s\n", buf);
	    exit(1);
	}
	

	bool more_bindings = true;
	fprintf(stdout, "bindings\n");
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
	    /* fprintf(stdout, ", fn %s\n", buf); */
	}

    }
}

