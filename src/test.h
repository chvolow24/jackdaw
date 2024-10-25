/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/

/*****************************************************************************************************************
    test.h

    * tiny testing framework
    * provides method to declare functions built only in debug build
    * and call those functions
 *****************************************************************************************************************/

#ifndef JDAW_TEST_H
#define JDAW_TEST_H

#ifdef TESTBUILD 
    #define TEST_FN_DEF(name, body, ...) \
        int name(__VA_ARGS__) body
    #define TEST_FN_DECL(name, ...) \
        int name(__VA_ARGS__)
    #define TEST_FN_CALL(name, ...) \
        {int code = name(__VA_ARGS__);	    \
        if (code != 0) {						\
	fprintf(stderr, "\n%s:%d:\ttest \"%s\" failed with error code %d\n", __FILE__, __LINE__, #name, code); \
	    exit(1); \
        }}
#else
    #define TEST_FN_DEF(name, body, ...)
    #define TEST_FN_DECL(name, ...)
    #define TEST_FN_CALL(name, ...)
#endif


#endif
