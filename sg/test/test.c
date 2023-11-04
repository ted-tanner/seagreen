#ifdef SG_TEST

#include "sgtest/test.h"

void _register_test(ModuleTestSet* test_set, char *test_name, TestFunc test_func)
{
    Test test = {
        .test_name = {0},
        .test_func = test_func,
    };
 
    for (int i = 0; i < TEST_NAME_MAX_LEN && test_name[i]; ++i)
        test.test_name[i] = test_name[i];
    
    test_set->tests[test_set->count++] = test;
}

#endif
