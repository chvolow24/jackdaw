/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    test.h

    * tiny testing framework
    * provides method to declare functions built only in debug build
    * and call those functions
 *****************************************************************************************************************/

#ifndef JDAW_TEST_H
#define JDAW_TEST_H

#include <stdbool.h>
#include <stdint.h>

#ifdef TESTBUILD 
    #define TEST_FN_DEF(name, body, ...) \
        int name(__VA_ARGS__) body
    #define TEST_FN_DECL(name, ...) \
        int name(__VA_ARGS__)
    #define TEST_FN_CALL(name, ...) \
        {int code = name(__VA_ARGS__);						\
        if (code != 0) {						\
	    fprintf(stderr, "\n%s:%d:\ttest \"%s\" failed with error code %d\n", __FILE__, __LINE__, #name, code); \
	    exit(1); \
	}}
#else
    #define TEST_FN_DEF(name, body, ...)
    #define TEST_FN_DECL(name, ...)
    #define TEST_FN_CALL(name, ...)
#endif

void breakfn();
void print_backtrace();

TEST_FN_DECL(chaotic_user, bool *run_tests, uint64_t max_num_frames);

#endif
