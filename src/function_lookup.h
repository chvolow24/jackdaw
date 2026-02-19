/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    function_lookup.h

    * store all user-accessible functions in a trie
    * provide an autocomplete GUI for access
 *****************************************************************************************************************/


#ifndef JDAW_FUNCTION_LOOKUP_H
#define JDAW_FUNCTION_LOOKUP_H

typedef struct user_fn UserFn;

typedef struct fn_list {
    int num_fns;
    int fn_arr_len;
    UserFn **fns;
} FnList;

void fn_lookup_index_fn(UserFn *fn);
void function_lookup();
void function_lookup_deinit();

#endif
