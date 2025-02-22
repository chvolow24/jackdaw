/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    function_lookup.c

    * record all available user function names in a trie
    * lookup functions by name
 *****************************************************************************************************************/


#include "function_lookup.h"
#include "components.h"
#include "input.h"
#include "trie.h"
#include "autocompletion.h"

#define DEFAULT_FN_LIST_LEN 4

static TrieNode FN_TRIE;


static FnList *create_fn_list()
{
    FnList *fnl = calloc(1, sizeof(FnList));
    fnl->fn_arr_len = DEFAULT_FN_LIST_LEN;
    fnl->fns = calloc(fnl->fn_arr_len, sizeof(UserFn *));
    return fnl;
}

static void add_fn_to_list(FnList *fnl, UserFn *fn)
{
    if (fnl->num_fns + 1 == fnl->fn_arr_len) {
	fnl->fn_arr_len *= 2;
	fnl->fns = realloc(fnl->fns, fnl->fn_arr_len * sizeof(UserFn *));
    }
    fnl->fns[fnl->num_fns] = fn;
    fnl->num_fns++;
}

void fn_lookup_index_fn(UserFn *fn)
{
    char *word = strdup(fn->fn_display_name);
    char *cursor = word;
    char c;
    while (1) {
	c = *cursor;
        if (c == ' ' || c == '/' || c == '\0' || c == '.' || c == '(' || c == ')') {
	    *cursor = '\0';
	    void *obj = trie_lookup_word(&FN_TRIE, word);
	    if (obj) {
		FnList *fnl = (FnList *)obj;
		/* fprintf(stderr, "ADDING WORD TO LIST: %s\n", word); */
		add_fn_to_list(fnl, fn);
	    } else {
		FnList *fnl = create_fn_list();
		add_fn_to_list(fnl, fn);
		/* fprintf(stderr, "INSERTING WORD: %s\n", word); */
		trie_insert_word(&FN_TRIE, word, fnl);
	    }
	    *cursor = ' ';
	    word = cursor + 1;
	    if (c == '\0') break;
	}
	cursor++;
    }
}



void TEST_lookup_print_all_matches(const char *word)
{
    void *objs[255];
    int num = trie_gather_completion_objs(&FN_TRIE, word, objs, 255);


    UserFn *fnlist[255];
    int num_fns = 0;
    
    for (int i=0; i<num; i++) {
	fprintf(stderr, "\n");
	FnList *fnl = objs[i];
	for (int i=0; i<fnl->num_fns; i++) {
	    fnlist[num_fns] = fnl->fns[i];
	    num_fns++;
	    fprintf(stderr, "FN: %s\n", fnl->fns[i]->fn_display_name);
	}
    }
}

static int update_records_fn(AutoCompletion *ac, struct autocompletion_item **items_dst)
{

    /* fprintf(stderr, "\n\n\n\nUPDATE RECORDS\n"); */
    char *text = strdup(ac->entry->tb->text->display_value);
    if (strlen(text) == 0) {
	free(text);
	return 0;
    }

    struct autocompletion_item items_loc[255];
    int num_fns = 0;

    char *tok;
    bool first_word = true;
    while ((tok = strsep(&text, " "))) {
	/* fprintf(stderr, "TOK: %s\n", tok); */
	if (strlen(tok) == 0) continue;
	int num_word_matches = 0;    
	void *objs[255];
	if (first_word) {
	    num_word_matches = trie_gather_completion_objs(&FN_TRIE, tok, objs, 255);
	    for (int i=0; i<num_word_matches; i++) {
		FnList *fnl = objs[i];
		for (int i=0; i<fnl->num_fns; i++) {
		    UserFn *fn = fnl->fns[i];
		    struct autocompletion_item item = items_loc[num_fns];
		    item.str = fn->fn_display_name;
		    item.obj = fn;
		    items_loc[num_fns] = item;
		    /* fnlist[num_fns] = fnl->fns[i]; */
		    num_fns++;
		    /* fprintf(stderr, "FN: %s\n", fnl->fns[i]->fn_display_name); */
		}

	    }
	    first_word = false;
	    continue;
	} 
	num_word_matches = trie_gather_completion_objs(&FN_TRIE, tok, objs, 255);
	/* fprintf(stderr, "NUM FNS BEFORE FILTER: %d. Tok: %s. Num words: %d\n", num_fns, tok, num_word_matches); */
	UserFn *tok_fns[255];
	int num_tok_fns = 0;
	for (int i=0; i<num_word_matches; i++) {
	    FnList *fnl = objs[i];
	    for (int i=0; i<fnl->num_fns; i++) {
		tok_fns[num_tok_fns] = fnl->fns[i];
		num_tok_fns++;
	    }
	}
	for (int i=0; i<num_fns; i++) {
	    void *obj1 = items_loc[i].obj;
	    bool found = false;
	    for (int j=0; j<num_tok_fns; j++) {
		void *obj2 = tok_fns[j];
		if (obj1 == obj2) {
		    found = true;
		    break;
		}
	    }
	    if (!found) {
		if (i<num_fns - 1) {
		    memmove(items_loc + i, items_loc + i + 1, (num_fns - i - 1) * sizeof(struct autocompletion));
		}
		num_fns--;
		i--;
	    }
	}
    }
    free(text);

    *items_dst = calloc(num_fns, sizeof(struct autocompletion_item));
    memcpy(*items_dst, items_loc, num_fns * sizeof(struct autocompletion_item));
    return num_fns;
}

/* AutoCompletion GLOBAL_AC; */
extern Window *main_win;
extern void (*user_tl_track_add_automation)(void *);
/* extern void user_global_save_project(void *); */
int fn_lookup_filter(void *current_item, void *x_arg)
{
    struct autocompletion_item *item = (struct autocompletion_item *)current_item;
    UserFn *fn = item->obj;
    if (fn->mode->im == AUTOCOMPLETE_LIST) return 0;
    if (fn->mode->im == TEXT_EDIT) return 0;
    if (input_function_is_accessible(fn, main_win)) {
	return 1;
    }
    return 0;
    /* if (fn->mode->im == GLOBAL) return 1; */
    /* for (int i=main_win->num_modes - 1; i--;) { */
    /* 	InputMode im = main_win->modes[i]; */
    /* 	if (im == fn->mode->im) return 1; */
    /* } */
    /* return 0; */
}

void function_lookup()
{
    Layout *ac_lt = layout_add_child(main_win->layout);
    layout_set_default_dims(ac_lt);
    layout_force_reset(ac_lt);
    autocompletion_init(
	&main_win->ac,
	ac_lt,
	update_records_fn,
	fn_lookup_filter);
    textentry_edit(main_win->ac.entry);
    window_push_mode(main_win, AUTOCOMPLETE_LIST);
    main_win->ac_active = true;

}
/* UserFn *fn_lookup_search_fn() */
/* { */
    
/* } */

