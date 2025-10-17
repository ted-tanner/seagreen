#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "seagreen.h"

// Test recursive function with different recursion depths
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
    
    // Yield at each recursive call to test stack integrity
    async_yield();
    
    // Create arguments for recursive calls
    recursive_args left_args = {n - 1, args->max_depth, args->current_depth + 1, 0};
    recursive_args right_args = {n - 2, args->max_depth, args->current_depth + 1, 0};
    
    // Make recursive calls
    CGNThreadHandle left_handle = async_run(fibonacci_recursive, &left_args);
    CGNThreadHandle right_handle = async_run(fibonacci_recursive, &right_args);
    
    // Wait for results
    uint64_t left_result = await(left_handle);
    uint64_t right_result = await(right_handle);
    
    return left_result + right_result;
}

async uint64_t factorial_recursive(void *p) {
    recursive_args *args = (recursive_args *)p;
    int n = args->depth;
    
    if (n <= 1) {
        return 1;
    }
    
    // Yield at each recursive call
    async_yield();
    
    // Recursive call
    recursive_args next_args = {n - 1, args->max_depth, args->current_depth + 1, 0};
    CGNThreadHandle next_handle = async_run(factorial_recursive, &next_args);
    
    uint64_t next_result = await(next_handle);
    return n * next_result;
}

async uint64_t depth_tracker_recursive(void *p) {
    recursive_args *args = (recursive_args *)p;
    
    if (args->current_depth >= args->max_depth) {
        return args->current_depth;
    }
    
    // Yield to test stack integrity
    async_yield();
    
    // Recursive call with increased depth
    recursive_args next_args = {
        args->depth,
        args->max_depth,
        args->current_depth + 1,
        args->expected_result
    };
    
    CGNThreadHandle next_handle = async_run(depth_tracker_recursive, &next_args);
    uint64_t result = await(next_handle);
    
    // Verify we're at the expected depth
    assert(result == args->max_depth);
    
    return result;
}

async uint64_t lightweight_recursive(void *p) {
    recursive_args *args = (recursive_args *)p;
    
    if (args->current_depth >= args->max_depth) {
        return args->current_depth;
    }
    
    // Yield every few levels to test stack integrity without too much overhead
    if (args->current_depth % 3 == 0) {
        async_yield();
    }
    
    // Recursive call with increased depth
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

// Helper function to calculate expected Fibonacci result
uint64_t expected_fibonacci(int n) {
    if (n <= 1) return n;
    return expected_fibonacci(n - 1) + expected_fibonacci(n - 2);
}

// Helper function to calculate expected factorial result
uint64_t expected_factorial(int n) {
    if (n <= 1) return 1;
    return n * expected_factorial(n - 1);
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing recursive Fibonacci function...\n");
    
    // Test Fibonacci with different depths
    int fibonacci_tests[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    int num_fib_tests = sizeof(fibonacci_tests) / sizeof(fibonacci_tests[0]);
    
    for (int i = 0; i < num_fib_tests; i++) {
        int n = fibonacci_tests[i];
        uint64_t expected = expected_fibonacci(n);
        
        recursive_args args = {n, 0, 0, expected};
        CGNThreadHandle handle = async_run(fibonacci_recursive, &args);
        uint64_t result = await(handle);
        
        printf("Fibonacci(%d) = %llu (expected %llu)\n", n, result, expected);
        assert(result == expected);
    }
    
    printf("Testing recursive factorial function...\n");
    
    // Test factorial with different values
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
    
    printf("Testing recursive depth tracking...\n");
    
    // Test depth tracking with different max depths
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
    
    printf("Testing multiple concurrent recursive calls...\n");
    
    // Test multiple recursive calls running concurrently
    CGNThreadHandle concurrent_handles[5];
    recursive_args concurrent_args[5];
    
    for (int i = 0; i < 5; i++) {
        concurrent_args[i] = (recursive_args){i + 3, 0, 0, 0}; // Fibonacci(3) to Fibonacci(7)
        concurrent_handles[i] = async_run(fibonacci_recursive, &concurrent_args[i]);
    }
    
    // Wait for all to complete
    for (int i = 0; i < 5; i++) {
        uint64_t result = await(concurrent_handles[i]);
        uint64_t expected = expected_fibonacci(i + 3);
        printf("Concurrent Fibonacci(%d) = %llu (expected %llu)\n", i + 3, result, expected);
        assert(result == expected);
    }
    
    printf("Testing lightweight recursive function with deep recursion...\n");
    
    // Test deep recursion with the lightweight function
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
    
    // Test many concurrent lightweight recursive calls
    int num_lightweight_concurrent = 20;
    CGNThreadHandle lightweight_handles[num_lightweight_concurrent];
    recursive_args lightweight_args[num_lightweight_concurrent];
    
    for (int i = 0; i < num_lightweight_concurrent; i++) {
        lightweight_args[i] = (recursive_args){0, 30 + (i % 20), 0, 30 + (i % 20)};
        lightweight_handles[i] = async_run(lightweight_recursive, &lightweight_args[i]);
    }
    
    // Wait for all to complete
    for (int i = 0; i < num_lightweight_concurrent; i++) {
        uint64_t result = await(lightweight_handles[i]);
        uint64_t expected = 30 + (i % 20);
        printf("Concurrent lightweight recursive %d = %llu (expected %llu)\n", i, result, expected);
        assert(result == expected);
    }
    
    printf("Testing very deep recursion (stress test)...\n");
    
    // Stress test with very deep recursion
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
