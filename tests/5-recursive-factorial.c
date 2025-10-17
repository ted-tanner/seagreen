#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "seagreen.h"

typedef struct {
    int depth;
    int max_depth;
    int current_depth;
    uint64_t expected_result;
} recursive_args;

async uint64_t factorial_recursive(void *p) {
    recursive_args *args = (recursive_args *)p;
    int n = args->depth;
    
    if (n <= 1) {
        return 1;
    }
    
    async_yield();
    
    recursive_args next_args = {n - 1, args->max_depth, args->current_depth + 1, 0};
    CGNThreadHandle next_handle = async_run(factorial_recursive, &next_args);
    
    uint64_t next_result = await(next_handle);
    return n * next_result;
}

uint64_t expected_factorial(int n) {
    if (n <= 1) return 1;
    return n * expected_factorial(n - 1);
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing recursive factorial function...\n");
    
    int factorial_tests[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    int num_fact_tests = sizeof(factorial_tests) / sizeof(factorial_tests[0]);
    
    for (int i = 0; i < num_fact_tests; i++) {
        int n = factorial_tests[i];
        uint64_t expected = expected_factorial(n);
        
        recursive_args args = {n, 0, 0, expected};
        CGNThreadHandle handle = async_run(factorial_recursive, &args);
        uint64_t result = await(handle);
        
        printf("Factorial(%d) = %llu (expected %llu)\n", n, result, expected);
        assert(result == expected);
    }
    
    seagreen_free_rt();
    
    return 0;
}
