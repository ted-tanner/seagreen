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

async uint64_t lightweight_recursive(void *p) {
    recursive_args *args = (recursive_args *)p;
    
    if (args->current_depth >= args->max_depth) {
        return args->current_depth;
    }
    
    if (args->current_depth % 3 == 0) {
        async_yield();
    }
    
    recursive_args next_args = {
        args->depth,
        args->max_depth,
        args->current_depth + 1,
        args->expected_result
    };
    
    CGNThreadHandle next_handle = async_run(lightweight_recursive, &next_args);
    uint64_t result = await(next_handle);
    
    return result;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing lightweight recursive function with deep recursion...\n");
    
    int deep_tests[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    int num_deep_tests = sizeof(deep_tests) / sizeof(deep_tests[0]);
    
    for (int i = 0; i < num_deep_tests; i++) {
        int max_depth = deep_tests[i];
        
        recursive_args args = {0, max_depth, 0, max_depth};
        CGNThreadHandle handle = async_run(lightweight_recursive, &args);
        uint64_t result = await(handle);
        
        printf("Lightweight recursive (depth %d) = %llu\n", max_depth, result);
        assert(result == max_depth);
    }
    
    printf("Testing many concurrent lightweight recursive calls...\n");
    
    int num_lightweight_concurrent = 20;
    CGNThreadHandle lightweight_handles[num_lightweight_concurrent];
    recursive_args lightweight_args[num_lightweight_concurrent];
    
    for (int i = 0; i < num_lightweight_concurrent; i++) {
        lightweight_args[i] = (recursive_args){0, 30 + (i % 20), 0, 30 + (i % 20)};
        lightweight_handles[i] = async_run(lightweight_recursive, &lightweight_args[i]);
    }
    
    for (int i = 0; i < num_lightweight_concurrent; i++) {
        uint64_t result = await(lightweight_handles[i]);
        uint64_t expected = 30 + (i % 20);
        printf("Concurrent lightweight recursive %d = %llu (expected %llu)\n", i, result, expected);
        assert(result == expected);
    }
    
    printf("Testing very deep recursion (stress test)...\n");
    
    int stress_depths[] = {200, 300, 400, 500};
    int num_stress_tests = sizeof(stress_depths) / sizeof(stress_depths[0]);
    
    for (int i = 0; i < num_stress_tests; i++) {
        int max_depth = stress_depths[i];
        
        recursive_args args = {0, max_depth, 0, max_depth};
        CGNThreadHandle handle = async_run(lightweight_recursive, &args);
        uint64_t result = await(handle);
        
        printf("Stress test (depth %d) = %llu\n", max_depth, result);
        assert(result == max_depth);
    }
    
    seagreen_free_rt();
    
    return 0;
}
