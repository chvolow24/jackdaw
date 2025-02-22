/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    function_lookup.h

    * variable len struct for functions by word
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

/* void lookup_print_all_matches(const char *word); */

void TEST_lookup_print_all_matches(const char *word);

void function_lookup();

#endif
