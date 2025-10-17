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

async uint64_t depth_tracker_recursive(void *p) {
    recursive_args *args = (recursive_args *)p;
    
    if (args->current_depth >= args->max_depth) {
        return args->current_depth;
    }
    
    async_yield();
    
    recursive_args next_args = {
        args->depth,
        args->max_depth,
        args->current_depth + 1,
        args->expected_result
    };
    
    CGNThreadHandle next_handle = async_run(depth_tracker_recursive, &next_args);
    uint64_t result = await(next_handle);
    
    assert(result == args->max_depth);
    
    return result;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing recursive depth tracking...\n");
    
    int depth_tests[] = {1, 2, 3, 4, 5, 6, 7, 8};
    int num_depth_tests = sizeof(depth_tests) / sizeof(depth_tests[0]);
    
    for (int i = 0; i < num_depth_tests; i++) {
        int max_depth = depth_tests[i];
        
        recursive_args args = {0, max_depth, 0, max_depth};
        CGNThreadHandle handle = async_run(depth_tracker_recursive, &args);
        uint64_t result = await(handle);
        
        printf("Depth test (max %d) = %llu\n", max_depth, result);
        assert(result == max_depth);
    }
    
    seagreen_free_rt();
    
    return 0;
}
