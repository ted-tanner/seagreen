#ifdef CGN_TEST

#include "cgntest/test.h"

int set_count = 0;
CgnTestSet test_sets[TEST_MAX_SET_COUNT];

void _register_test(CgnTestSet* test_set, char *test_name, TestFunc test_func) {
    Test test = {
        .test_name = {0},
        .test_func = test_func,
    };

    strncpy(test.test_name, test_name, TEST_NAME_MAX_LEN);
    
    test_set->tests[test_set->count++] = test;
}

CgnTestSet *new_test_set(char *set_name) {
    CgnTestSet *set = test_sets + set_count++;

    cgnasrt(set_count < TEST_MAX_SET_COUNT, "TEST_MAX_SET_COUNT exceeded");

    strncpy(set->name, set_name, TEST_SET_NAME_MAX_LEN);

    return set;
}

CgnTestSet *get_test_sets() {
    return test_sets;
}

int get_test_set_count() {
    return set_count;
}

#endif
