#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "seagreen.h"

// Test data structures
typedef struct {
    int value;
    int expected_result;
    int level;
} sync_async_args;

// Simple async worker function
async uint64_t simple_worker_func(void *p) {
    sync_async_args *args = (sync_async_args *)p;
    
    printf("  Worker func called with value %d at level %d\n", args->value, args->level);
    
    // Yield to allow other threads to run
    async_yield();
    
    return args->value * 2;
}

// Regular (non-async) function that calls async functions
uint64_t sync_function_calling_async(void *p) {
    sync_async_args *args = (sync_async_args *)p;
    
    printf("Sync function calling async with value %d\n", args->value);
    
    // Call async function from within sync function
    sync_async_args worker_args = {args->value + 10, 0, args->level + 1};
    CGNThreadHandle handle = async_run(simple_worker_func, &worker_args);
    uint64_t result = await(handle);
    
    printf("Sync function got result %llu from async call\n", result);
    
    return result + args->value;
}

// Another regular function that calls multiple async functions
uint64_t sync_function_calling_multiple_async(void *p) {
    sync_async_args *args = (sync_async_args *)p;
    
    printf("Sync function calling multiple async with value %d\n", args->value);
    
    // Call multiple async functions
    CGNThreadHandle handles[3];
    sync_async_args worker_args[3];
    
    for (int i = 0; i < 3; i++) {
        worker_args[i] = (sync_async_args){args->value + i, 0, args->level + 1};
        handles[i] = async_run(simple_worker_func, &worker_args[i]);
    }
    
    // Wait for all results
    uint64_t total = 0;
    for (int i = 0; i < 3; i++) {
        uint64_t result = await(handles[i]);
        total += result;
    }
    
    printf("Sync function got total result %llu from multiple async calls\n", total);
    
    return total + args->value;
}

// Regular function that calls async functions conditionally
uint64_t sync_function_conditional_async(void *p) {
    sync_async_args *args = (sync_async_args *)p;
    
    printf("Sync function conditional async with value %d\n", args->value);
    
    if (args->value % 2 == 0) {
        // Even value - call one async function
        sync_async_args worker_args = {args->value, 0, args->level + 1};
        CGNThreadHandle handle = async_run(simple_worker_func, &worker_args);
        uint64_t result = await(handle);
        return result;
    } else {
        // Odd value - call two async functions
        sync_async_args worker_args1 = {args->value, 0, args->level + 1};
        sync_async_args worker_args2 = {args->value + 1, 0, args->level + 1};
        
        CGNThreadHandle handle1 = async_run(simple_worker_func, &worker_args1);
        CGNThreadHandle handle2 = async_run(simple_worker_func, &worker_args2);
        
        uint64_t result1 = await(handle1);
        uint64_t result2 = await(handle2);
        
        return result1 + result2;
    }
}

// Async function that calls sync functions
async uint64_t async_function_calling_sync(void *p) {
    sync_async_args *args = (sync_async_args *)p;
    
    printf("Async function calling sync with value %d\n", args->value);
    
    // Yield before calling sync function
    async_yield();
    
    // Call sync function from within async function
    uint64_t sync_result = sync_function_calling_async(args);
    
    printf("Async function got result %llu from sync call\n", sync_result);
    
    // Yield after calling sync function
    async_yield();
    
    return sync_result + args->value;
}

// Async function that calls sync function which calls multiple async functions
async uint64_t async_function_calling_sync_multiple_async(void *p) {
    sync_async_args *args = (sync_async_args *)p;
    
    printf("Async function calling sync->multiple async with value %d\n", args->value);
    
    async_yield();
    
    // Call sync function that calls multiple async functions
    uint64_t sync_result = sync_function_calling_multiple_async(args);
    
    printf("Async function got result %llu from sync->multiple async call\n", sync_result);
    
    async_yield();
    
    return sync_result + args->value;
}

// Async function that calls sync function which calls async functions conditionally
async uint64_t async_function_calling_sync_conditional_async(void *p) {
    sync_async_args *args = (sync_async_args *)p;
    
    printf("Async function calling sync->conditional async with value %d\n", args->value);
    
    async_yield();
    
    // Call sync function that calls async functions conditionally
    uint64_t sync_result = sync_function_conditional_async(args);
    
    printf("Async function got result %llu from sync->conditional async call\n", sync_result);
    
    async_yield();
    
    return sync_result + args->value;
}

// Deep nesting: async -> sync -> async -> sync -> async
uint64_t deep_sync_function(void *p) {
    sync_async_args *args = (sync_async_args *)p;
    
    printf("Deep sync function with value %d\n", args->value);
    
    // Call async function from deep sync function
    sync_async_args worker_args = {args->value + 5, 0, args->level + 1};
    CGNThreadHandle handle = async_run(simple_worker_func, &worker_args);
    uint64_t result = await(handle);
    
    return result + args->value;
}

async uint64_t deep_async_function(void *p) {
    sync_async_args *args = (sync_async_args *)p;
    
    printf("Deep async function with value %d\n", args->value);
    
    async_yield();
    
    // Call sync function from async function
    uint64_t sync_result = deep_sync_function(args);
    
    async_yield();
    
    return sync_result + args->value;
}

async uint64_t top_level_async_function(void *p) {
    sync_async_args *args = (sync_async_args *)p;
    
    printf("Top level async function with value %d\n", args->value);
    
    async_yield();
    
    // Call deep async function
    sync_async_args deep_args = {args->value + 1, 0, args->level + 1};
    CGNThreadHandle deep_handle = async_run(deep_async_function, &deep_args);
    uint64_t deep_result = await(deep_handle);
    
    async_yield();
    
    return deep_result + args->value;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing sync functions calling async functions...\n");
    
    // Test 1: Sync function calling single async function
    printf("\n=== Test 1: Sync function calling single async function ===\n");
    sync_async_args args1 = {10, 0, 0};
    uint64_t result1 = sync_function_calling_async(&args1);
    printf("Sync->async result: %llu (expected: 50)\n", result1);
    // Expected: (10+10)*2 + 10 = 50
    assert(result1 == 50);
    
    // Test 2: Sync function calling multiple async functions
    printf("\n=== Test 2: Sync function calling multiple async functions ===\n");
    sync_async_args args2 = {5, 0, 0};
    uint64_t result2 = sync_function_calling_multiple_async(&args2);
    printf("Sync->multiple async result: %llu (expected: 41)\n", result2);
    // Expected: (5*2) + (6*2) + (7*2) + 5 = 10 + 12 + 14 + 5 = 41
    assert(result2 == 41);
    
    // Test 3: Sync function calling async functions conditionally
    printf("\n=== Test 3: Sync function calling async functions conditionally ===\n");
    for (int i = 0; i < 6; i++) {
        sync_async_args args3 = {i, 0, 0};
        uint64_t result3 = sync_function_conditional_async(&args3);
        printf("Sync->conditional async result for %d: %llu\n", i, result3);
        
        if (i % 2 == 0) {
            // Even - should be i * 2
            assert(result3 == (uint64_t)(i * 2));
        } else {
            // Odd - should be i*2 + (i+1)*2
            assert(result3 == (uint64_t)(i * 2 + (i + 1) * 2));
        }
    }
    
    printf("\nTesting async functions calling sync functions...\n");
    
    // Test 4: Async function calling sync function
    printf("\n=== Test 4: Async function calling sync function ===\n");
    sync_async_args args4 = {15, 0, 0};
    CGNThreadHandle handle4 = async_run(async_function_calling_sync, &args4);
    uint64_t result4 = await(handle4);
    printf("Async->sync->async result: %llu (expected: 80)\n", result4);
    // Expected: ((15+10)*2 + 15) + 15 = 80
    assert(result4 == 80);
    
    // Test 5: Async function calling sync function that calls multiple async functions
    printf("\n=== Test 5: Async function calling sync->multiple async ===\n");
    sync_async_args args5 = {20, 0, 0};
    CGNThreadHandle handle5 = async_run(async_function_calling_sync_multiple_async, &args5);
    uint64_t result5 = await(handle5);
    printf("Async->sync->multiple async result: %llu (expected: 166)\n", result5);
    // Expected: ((20*2) + (21*2) + (22*2) + 20) + 20 = 166
    assert(result5 == 166);
    
    // Test 6: Async function calling sync function that calls async functions conditionally
    printf("\n=== Test 6: Async function calling sync->conditional async ===\n");
    for (int i = 0; i < 4; i++) {
        sync_async_args args6 = {i + 25, 0, 0};
        CGNThreadHandle handle6 = async_run(async_function_calling_sync_conditional_async, &args6);
        uint64_t result6 = await(handle6);
        printf("Async->sync->conditional async result for %d: %llu\n", i + 25, result6);
        
        int val = i + 25;
        uint64_t expected;
        if (val % 2 == 0) {
            expected = (uint64_t)(val * 2 + val);
        } else {
            expected = (uint64_t)(val * 2 + (val + 1) * 2 + val);
        }
        assert(result6 == expected);
    }
    
    // Test 7: Deep nesting: async -> sync -> async -> sync -> async
    printf("\n=== Test 7: Deep nesting async->sync->async->sync->async ===\n");
    sync_async_args args7 = {30, 0, 0};
    CGNThreadHandle handle7 = async_run(top_level_async_function, &args7);
    uint64_t result7 = await(handle7);
    printf("Deep nesting result: %llu\n", result7);
    // Expected: ((30+1+5)*2 + (30+1)) + (30+1) + 30 = 72 + 31 + 31 + 30 = 164
    assert(result7 == 164);
    
    // Test 8: Multiple concurrent async->sync->async calls
    printf("\n=== Test 8: Multiple concurrent async->sync->async calls ===\n");
    CGNThreadHandle concurrent_handles[5];
    sync_async_args concurrent_args[5];
    
    for (int i = 0; i < 5; i++) {
        concurrent_args[i] = (sync_async_args){i + 40, 0, 0};
        concurrent_handles[i] = async_run(async_function_calling_sync, &concurrent_args[i]);
    }
    
    // Wait for all to complete
    for (int i = 0; i < 5; i++) {
        uint64_t result = await(concurrent_handles[i]);
        uint64_t expected = (uint64_t)(((i + 40) + 10) * 2 + (i + 40)) + (i + 40);
        printf("Concurrent async->sync->async %d result: %llu (expected: %llu)\n", i, result, expected);
        assert(result == expected);
    }
    
    seagreen_free_rt();
    
    return 0;
}
