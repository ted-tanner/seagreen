#ifndef CGNTEST_H

#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TEST_NAME_MAX_LEN 256
#define TEST_SET_NAME_MAX_LEN 512
#define TEST_SET_MAX_TESTS 256
#define TEST_MAX_SET_COUNT 64

#define ASCII_RED "\e[31m"
#define ASCII_GREEN "\e[32m"
#define ASCII_DEFAULT "\e[0m"

typedef enum { TEST_PASS, TEST_FAIL } TEST_RESULT;
typedef TEST_RESULT (*TestFunc)();

typedef struct Test_ {
    char test_name[TEST_NAME_MAX_LEN];
    TestFunc test_func;
} Test;

typedef struct CgnTestSet_ {
    char name[TEST_SET_NAME_MAX_LEN];
    Test tests[TEST_SET_MAX_TESTS];

    int count;
} CgnTestSet;

#define register_test(test_set, test_func) _register_test(test_set, #test_func, test_func)
void _register_test(CgnTestSet *test_set, char *test_name, TestFunc test_func);

CgnTestSet *new_test_set(char *set_name);

CgnTestSet *get_test_sets();
int get_test_set_count();

#define cgnasrt(condition, mcgn) if (condition)                           \
    { ((void)0); } else                                                 \
    { fprintf(stdout, "ASSERTION ERROR: %s\r\n\t: '%s' evaluated to 0\r\n\t: in function '%s'\r\n\t: at '%s' line %d.\r\n", \
              mcgn, #condition, __PRETTY_FUNCTION__, __FILE__, __LINE__); abort(); }

#define CGNTEST_H
#endif
