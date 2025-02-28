/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    trie.c

    * simple trie implementation for function lookup
    * all lowercase
 *****************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"

#define MAX_COMPLETION_LEN 64

void trie_init(TrieNode *head)
{
    memset(head, '\0', sizeof(TrieNode));
}

static char lower_validate_char(char c)
{
    if (c >= 'A' && c <= 'Z') {
	return c - 'A' + 'a';
    } else if (c < 'a' || c > 'z') {
	#ifdef TESTBUILD
	fprintf(stderr, "Error in trie: alpha characters only. Pased '%c' (%d)\n", c, c);
	#endif
	return '\0';
    }
    return c;
}

void trie_insert_word(TrieNode *trie, char *word, void *ex_obj)
{
    char c;
    TrieNode **node = &trie;
    while ((c = *word) != '\0') {
	c = lower_validate_char(c);
	if (!c) return;
	node = (*node)->children + c - 'a';
	if (!*node) {
	    *node = calloc(1, sizeof(TrieNode));
	    /* (*node)->c = c; */
	}
	word++;
    }
    if (*node) {
	(*node)->ex_obj = ex_obj;
    }
}

TrieNode *trie_lookup_word_leaf(TrieNode *trie, const char *word)
{
    char c;
    TrieNode **node = &trie;
    while ((c = *word) != '\0') {
	c = lower_validate_char(c);
	if (!c) return NULL;
	node = (*node)->children + c - 'a';
	if (!*node) {
	    return NULL;
	}
	word++;
    }
    return *node;
}

void *trie_lookup_word(TrieNode *trie, char *word)
{
    TrieNode *leaf = trie_lookup_word_leaf(trie, word);
    if (leaf) return leaf->ex_obj;
    return NULL;
}


static void trie_completion_recursive(TrieNode *trie, char *completion, int completion_i, int completion_buflen, void do_for_each(char *completion, TrieNode *leaf, void *xarg1, void *xarg2), void *xarg1, void *xarg2)
{
    if (completion_i == completion_buflen - 1) {
	fprintf(stderr, "ERROR: reached end of completion buf\n");
	return;
    }
    if (trie->ex_obj) {
	completion[completion_i] = '\0';
	do_for_each(completion, trie, xarg1, xarg2);
    }
    for (int i=0; i<26; i++) {
	TrieNode *child;
	if ((child = trie->children[i])) {
	    completion[completion_i] = 'a' + i;
	    trie_completion_recursive(child, completion, completion_i + 1, completion_buflen, do_for_each, xarg1, xarg2);
	}
    }
}

static void foreach_add_to_obj_list(char *completion, TrieNode *leaf, void *xarg1, void *xarg2)
{
    void ***obj_list_p = (void ***)xarg1;
    int *dst_lens = (int *)xarg2;
    if (dst_lens[0] == dst_lens[1]) {
	fprintf(stderr, "Error: reached end of obj array\n");
	return;
    }
    (*obj_list_p)[0] = leaf->ex_obj;
    (*obj_list_p)++;
    (*dst_lens)++;
}

int trie_gather_completion_objs(TrieNode *node, const char *word, void **dst, int dst_max_len)
{
    char completion_buf[255];
    int len = strlen(word);
    memcpy(completion_buf, word, len);
    
    node = trie_lookup_word_leaf(node, word);
    if (!node) return 0;
    int dst_lens[] = {0, dst_max_len};
    trie_completion_recursive(node, completion_buf, len, 255, foreach_add_to_obj_list, &dst, dst_lens);
    return dst_lens[0];
}

/* If root node not heap-allocated, second arg should be false */
void trie_destroy(TrieNode *node, bool free_current_node, bool free_obj)
{
    for (int i=0; i<26; i++) {
	TrieNode *child;
	if ((child = node->children[i])) {
	    trie_destroy(node->children[i], true, free_obj);
	    if (!free_current_node) {
		node->children[i] = NULL;
	    }
	}
    }
    if (free_current_node) {
	if (free_obj && node->ex_obj) {
	    free(node->ex_obj);
	}
	free(node);
    }
}


TrieNode glob_trie;

static void lookup_print(char *word)
{
    void *ptr = trie_lookup_word(&glob_trie, word);
    fprintf(stderr, "%s: %lu\n", word, (long int)ptr);
}

static void to_do(char *completion, TrieNode *leaf, void *xarg1, void *xarg2)
{
    fprintf(stderr, "Completion: %s, xob: %p, xargs? %p %p\n", completion, leaf->ex_obj, xarg1, xarg2);
}

void trie_tests()
{
    trie_init(&glob_trie);
    trie_insert_word(&glob_trie, "hello", (void*)1);
    trie_insert_word(&glob_trie, "thEre", (void*)2);
    trie_insert_word(&glob_trie, "you", (void *)3);
    trie_insert_word(&glob_trie, "silly", (void *)4);
    trie_insert_word(&glob_trie, "goose", (void *)5);
    trie_insert_word(&glob_trie, "hell", (void *)6);
    trie_insert_word(&glob_trie, "go", (void *)7);
    trie_insert_word(&glob_trie, "theretilly", (void *)8);
    trie_insert_word(&glob_trie, "hhhhhhherewego", (void *)9);

    lookup_print("ok");
    lookup_print("whatttt");
    lookup_print("you");
    lookup_print("hello");
    lookup_print("hell");
    lookup_print("theretilly");
    lookup_print("there");
    lookup_print("tHere");

    
    char buf[255];
    memset(buf, '\0', 255);
    buf[0] = 'h';

    
    
    trie_completion_recursive(glob_trie.children['h' - 'a'], buf, 1, 255, to_do, (void *)1, (void *)2);


    void *completion_objs[1];
    int num = trie_gather_completion_objs(&glob_trie, "he", completion_objs, 1);
    for (int i=0; i<num; i++) {
	fprintf(stderr, "Completion %d: %p\n", i, completion_objs[i]);
    }

}





