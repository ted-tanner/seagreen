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

async uint64_t fibonacci_recursive(void *p) {
    recursive_args *args = (recursive_args *)p;
    int n = args->depth;
    
    if (n <= 1) {
        return n;
    }
    
    async_yield();
    
    recursive_args left_args = {n - 1, args->max_depth, args->current_depth + 1, 0};
    recursive_args right_args = {n - 2, args->max_depth, args->current_depth + 1, 0};
    
    CGNThreadHandle left_handle = async_run(fibonacci_recursive, &left_args);
    CGNThreadHandle right_handle = async_run(fibonacci_recursive, &right_args);
    
    uint64_t left_result = await(left_handle);
    uint64_t right_result = await(right_handle);
    
    return left_result + right_result;
}

uint64_t expected_fibonacci(int n) {
    if (n <= 1) return n;
    return expected_fibonacci(n - 1) + expected_fibonacci(n - 2);
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing multiple concurrent recursive calls...\n");
    
    CGNThreadHandle concurrent_handles[5];
    recursive_args concurrent_args[5];
    
    for (int i = 0; i < 5; i++) {
        concurrent_args[i] = (recursive_args){i + 3, 0, 0, 0};
        concurrent_handles[i] = async_run(fibonacci_recursive, &concurrent_args[i]);
    }
    
    for (int i = 0; i < 5; i++) {
        uint64_t result = await(concurrent_handles[i]);
        uint64_t expected = expected_fibonacci(i + 3);
        printf("Concurrent Fibonacci(%d) = %llu (expected %llu)\n", i + 3, result, expected);
        assert(result == expected);
    }
    
    seagreen_free_rt();
    
    return 0;
}
