/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    trie.h

    * simple trie implementation for function lookup
    * all lowercase
 *****************************************************************************************************************/


#ifndef JDAW_TRIE_H
#define JDAW_TRIE_H

#include <stdbool.h>

typedef struct trienode TrieNode;
typedef struct trienode {
    /* char c; */
    TrieNode *children[26];
    void *ex_obj; /* External object pointer for word-terminators */
} TrieNode;


void trie_insert_word(TrieNode *trie, char *word, void *ex_obj);
void *trie_lookup_word(TrieNode *trie, char *word);
int trie_gather_completion_objs(TrieNode *node, const char *word, void **dst, int dst_max_len);

/* Root note may not be heap-allocated, so second arg should be false */
void trie_destroy(TrieNode *node, bool free_current_node, bool free_obj);

void trie_tests();

#endif
