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
#include "input.h"
#include "trie.h"

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
        if (c == ' ' || c == '\0') {
	    *cursor = '\0';
	    void *obj = trie_lookup_word(&FN_TRIE, word);
	    if (obj) {
		FnList *fnl = (FnList *)obj;
		fprintf(stderr, "ADDING WORD TO LIST: %s\n", word);
		add_fn_to_list(fnl, fn);
	    } else {
		FnList *fnl = create_fn_list();
		add_fn_to_list(fnl, fn);
		fprintf(stderr, "INSERTING WORD: %s\n", word);
		trie_insert_word(&FN_TRIE, word, fnl);
	    }
	    *cursor = ' ';
	    word = cursor + 1;
	    if (c == '\0') break;
	}
	cursor++;
    }
}

/* UserFn *fn_lookup_search_fn() */
/* { */
    
/* } */

