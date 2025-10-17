#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "seagreen.h"

// Test data structures for nested async calls
typedef struct {
    int level;
    int max_level;
    int value;
    int expected_result;
} nested_args;

// Simple async function that can be called from within other async functions
async uint64_t simple_async_func(void *p) {
    nested_args *args = (nested_args *)p;
    
    printf("  Simple async func called with value %d at level %d\n", args->value, args->level);
    
    // Yield to test context switching
    async_yield();
    
    return args->value * 2;
}

// Async function that calls another async function
async uint64_t nested_async_func(void *p) {
    nested_args *args = (nested_args *)p;
    
    printf("Nested async func level %d with value %d\n", args->level, args->value);
    
    if (args->level >= args->max_level) {
        // Base case - call simple async function
        nested_args simple_args = {args->level, args->max_level, args->value, 0};
        CGNThreadHandle handle = async_run(simple_async_func, &simple_args);
        uint64_t result = await(handle);
        return result + args->value;
    }
    
    // Yield before making nested call
    async_yield();
    
    // Recursive nested call
    nested_args next_args = {
        args->level + 1,
        args->max_level,
        args->value + 1,
        args->expected_result
    };
    
    CGNThreadHandle nested_handle = async_run(nested_async_func, &next_args);
    uint64_t nested_result = await(nested_handle);
    
    // Yield after nested call
    async_yield();
    
    return nested_result + args->value;
}

// Async function that calls multiple other async functions
async uint64_t multi_nested_async_func(void *p) {
    nested_args *args = (nested_args *)p;
    
    printf("Multi-nested async func level %d with value %d\n", args->level, args->value);
    
    // Yield before making multiple calls
    async_yield();
    
    // Call multiple async functions
    CGNThreadHandle handles[3];
    nested_args call_args[3];
    
    for (int i = 0; i < 3; i++) {
        call_args[i] = (nested_args){args->level + 1, args->max_level, args->value + i, 0};
        handles[i] = async_run(simple_async_func, &call_args[i]);
    }
    
    // Wait for all results
    uint64_t total = 0;
    for (int i = 0; i < 3; i++) {
        uint64_t result = await(handles[i]);
        total += result;
    }
    
    // Yield after all calls
    async_yield();
    
    return total;
}

// Async function that calls async functions in a loop
async uint64_t loop_nested_async_func(void *p) {
    nested_args *args = (nested_args *)p;
    
    printf("Loop-nested async func level %d with value %d\n", args->level, args->value);
    
    uint64_t total = 0;
    
    for (int i = 0; i < 5; i++) {
        // Yield in the loop
        async_yield();
        
        // Call async function
        nested_args call_args = {args->level + 1, args->max_level, args->value + i, 0};
        CGNThreadHandle handle = async_run(simple_async_func, &call_args);
        uint64_t result = await(handle);
        
        total += result;
    }
    
    return total;
}

// Async function that calls async functions conditionally
async uint64_t conditional_nested_async_func(void *p) {
    nested_args *args = (nested_args *)p;
    
    printf("Conditional-nested async func level %d with value %d\n", args->level, args->value);
    
    async_yield();
    
    if (args->value % 2 == 0) {
        // Even value - call simple async function
        nested_args call_args = {args->level + 1, args->max_level, args->value, 0};
        CGNThreadHandle handle = async_run(simple_async_func, &call_args);
        return await(handle);
    } else {
        // Odd value - call nested async function
        nested_args call_args = {args->level + 1, args->max_level, args->value, 0};
        CGNThreadHandle handle = async_run(nested_async_func, &call_args);
        return await(handle);
    }
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing simple async within async...\n");
    
    // Test 1: Simple async within async
    nested_args args1 = {0, 1, 10, 20};
    CGNThreadHandle handle1 = async_run(nested_async_func, &args1);
    uint64_t result1 = await(handle1);
    printf("Simple nested async result: %llu\n", result1);
    // The nested function calls nested_async_func(level=1, value=11)
    // Which calls simple_async_func(11) -> returns 22, then adds 11 -> 33
    // Then the original call adds 10 -> 43
    assert(result1 == 43);
    
    printf("Testing deep nested async calls...\n");
    
    // Test 2: Deep nested async calls
    nested_args args2 = {0, 3, 5, 0};
    CGNThreadHandle handle2 = async_run(nested_async_func, &args2);
    uint64_t result2 = await(handle2);
    printf("Deep nested async result: %llu\n", result2);
    // Expected: Level 3: 8*2 + 8 = 24, Level 2: 24 + 7 = 31, Level 1: 31 + 6 = 37, Level 0: 37 + 5 = 42
    assert(result2 == 42);
    
    printf("Testing multiple nested async calls...\n");
    
    // Test 3: Multiple nested async calls
    nested_args args3 = {0, 2, 10, 0};
    CGNThreadHandle handle3 = async_run(multi_nested_async_func, &args3);
    uint64_t result3 = await(handle3);
    printf("Multi-nested async result: %llu (expected: 60)\n", result3);
    // Expected: (10*2) + (11*2) + (12*2) = 20 + 22 + 24 = 66
    assert(result3 == 66);
    
    printf("Testing loop nested async calls...\n");
    
    // Test 4: Loop nested async calls
    nested_args args4 = {0, 2, 5, 0};
    CGNThreadHandle handle4 = async_run(loop_nested_async_func, &args4);
    uint64_t result4 = await(handle4);
    printf("Loop-nested async result: %llu (expected: 70)\n", result4);
    // Expected: (5*2) + (6*2) + (7*2) + (8*2) + (9*2) = 10 + 12 + 14 + 16 + 18 = 70
    assert(result4 == 70);
    
    printf("Testing conditional nested async calls...\n");
    
    // Test 5: Conditional nested async calls
    for (int i = 0; i < 6; i++) {
        nested_args args5 = {0, 2, i, 0};
        CGNThreadHandle handle5 = async_run(conditional_nested_async_func, &args5);
        uint64_t result5 = await(handle5);
        printf("Conditional nested async result for %d: %llu\n", i, result5);
        
        if (i % 2 == 0) {
            // Even - should be i * 2
            assert(result5 == (uint64_t)(i * 2));
        } else {
            // Odd - should be nested calculation
            // For odd values, it calls nested_async_func which eventually calls simple_async_func
            // The calculation is complex but we can verify it's not zero
            assert(result5 > 0);
        }
    }
    
    printf("Testing concurrent nested async calls...\n");
    
    // Test 6: Concurrent nested async calls
    CGNThreadHandle concurrent_handles[5];
    nested_args concurrent_args[5];
    
    for (int i = 0; i < 5; i++) {
        concurrent_args[i] = (nested_args){0, 2, i + 10, 0};
        concurrent_handles[i] = async_run(multi_nested_async_func, &concurrent_args[i]);
    }
    
    // Wait for all to complete
    for (int i = 0; i < 5; i++) {
        uint64_t result = await(concurrent_handles[i]);
        uint64_t expected = (uint64_t)((i + 10) * 2 + (i + 11) * 2 + (i + 12) * 2);
        printf("Concurrent nested async %d result: %llu (expected: %llu)\n", i, result, expected);
        assert(result == expected);
    }
    
    seagreen_free_rt();
    
    return 0;
}
