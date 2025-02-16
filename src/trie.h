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

typedef struct trienode TrieNode;
typedef struct trienode {
    char c;
    TrieNode *children[26];
    void *ex_obj; /* External object pointer for word-terminators */
} TrieNode;

typedef struct trie {
    TrieNode *head_nodes[26];  
} Trie;

void trie_insert_word(Trie *trie, char *word, void *ex_obj);
void *trie_lookup_word(Trie *trie, char *word);

void trie_tests();

#endif
