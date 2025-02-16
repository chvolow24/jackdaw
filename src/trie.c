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

void trie_init(Trie *trie)
{
    memset(trie, '\0', sizeof(Trie));
}

static char lower_validate_char(char c)
{
    if (c >= 'A' && c <= 'Z') {
	return c - 'A' + 'a';
    } else if (c < 'a' || c > 'z') {
	fprintf(stderr, "Error in trie: alpha characters only");
	return '\0';
    }
    return c;
}

void trie_insert_word(Trie *trie, char *word, void *ex_obj)
{
    char c = word[0];
    c = lower_validate_char(c);
    if (!c) return;
    TrieNode **node = trie->head_nodes + (c - 'a');
    if (!*node) {
	*node = calloc(1, sizeof(TrieNode));
	(*node)->c = c;
    }
    word++;
    while ((c = *word) != '\0') {
	c = lower_validate_char(c);
	node = (*node)->children + c - 'a';
	if (!*node) {
	    *node = calloc(1, sizeof(TrieNode));
	    (*node)->c = c;
	}
	word++;
    }
    if (*node) {
	(*node)->ex_obj = ex_obj;
    }
}

void *trie_lookup_word(Trie *trie, char *word)
{
    char c = word[0];
    c = lower_validate_char(c);
    if (!c) return NULL;
    
    TrieNode **node = trie->head_nodes + (c - 'a');
    if (!*node) {
	return NULL;
    }
    word++;
    while ((c = *word) != '\0') {
	c = lower_validate_char(c);
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


Trie glob_trie;

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
}





