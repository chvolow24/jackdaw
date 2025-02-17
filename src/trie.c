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

void trie_init(TrieNode *head)
{
    memset(head, '\0', sizeof(TrieNode));
}

static char lower_validate_char(char c)
{
    if (c >= 'A' && c <= 'Z') {
	return c - 'A' + 'a';
    } else if (c < 'a' || c > 'z') {
	fprintf(stderr, "Error in trie: alpha characters only\n");
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

void *trie_lookup_word(TrieNode *trie, char *word)
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
    if (*node) {
	return (*node)->ex_obj;
    }
    return NULL;
}

/* If root node not heap-allocated, second arg should be false */
void trie_destroy(TrieNode *node, bool free_current_node)
{
    for (int i=0; i<26; i++) {
	TrieNode *child;
	if ((child = node->children[i])) {
	    trie_destroy(node->children[i], true);
	    if (!free_current_node) {
		node->children[i] = NULL;
	    }
	}
    }
    if (free_current_node) free(node);
}


TrieNode glob_trie;

static void lookup_print(char *word)
{
    void *ptr = trie_lookup_word(&glob_trie, word);
    fprintf(stderr, "%s: %lu\n", word, (long int)ptr);
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

    lookup_print("ok");
    lookup_print("whatttt");
    lookup_print("you");
    lookup_print("hello");
    lookup_print("hell");
    lookup_print("theretilly");
    lookup_print("there");
    lookup_print("tHere");
    trie_destroy(&glob_trie, false);

}





