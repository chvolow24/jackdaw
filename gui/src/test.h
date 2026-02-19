#ifndef JDAW_GUI_TEST_H
#define JDAW_GUI_TEST_H

void run_tests();

void breakfn();

#ifdef TESTBUILD
#define TESTBREAK breakfn();
#else
#define TESTBREAK
#endif


#endif
