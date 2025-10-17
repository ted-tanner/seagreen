#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "seagreen.h"

// Global variables to store thread handles for cross-layer communication
static CGNThreadHandle g_deep_spawned_thread = 0;
static CGNThreadHandle g_shallow_spawned_thread = 0;
static volatile int g_thread_ready = 0;

// Test data structures
typedef struct {
    int level;
    int value;
    int expected_result;
} cross_layer_args;

// Simple async function that can be spawned and awaited
async uint64_t simple_worker_func(void *p) {
    cross_layer_args *args = (cross_layer_args *)p;
    
    printf("  Worker func called with value %d at level %d\n", args->value, args->level);
    
    // Yield to allow other threads to run
    async_yield();
    
    return args->value * 3;
}

// Level 1 async function (shallow nesting)
async uint64_t level1_async_func(void *p) {
    cross_layer_args *args = (cross_layer_args *)p;
    
    printf("Level 1 async func with value %d\n", args->value);
    
    if (args->level == 1) {
        // This is the shallow level - spawn a thread that will be awaited by deep level
        cross_layer_args worker_args = {args->level, args->value + 10, 0};
        g_shallow_spawned_thread = async_run(simple_worker_func, &worker_args);
        g_thread_ready = 1;
        
        printf("Level 1 spawned thread %llu\n", g_shallow_spawned_thread);
        
        // Yield to allow the deep level to run and await our thread
        async_yield();
        async_yield();
        async_yield();
        
        return args->value;
    } else {
        // This is called from deeper levels - await the thread spawned by deep level
        printf("Level 1 awaiting deep-spawned thread %llu\n", g_deep_spawned_thread);
        
        // Wait for deep thread to be spawned
        while (g_deep_spawned_thread == 0) {
            async_yield();
        }
        
        uint64_t result = await(g_deep_spawned_thread);
        printf("Level 1 got result %llu from deep-spawned thread\n", result);
        
        return result + args->value;
    }
}

// Level 2 async function (medium nesting)
async uint64_t level2_async_func(void *p) {
    cross_layer_args *args = (cross_layer_args *)p;
    
    printf("Level 2 async func with value %d\n", args->value);
    
    async_yield();
    
    // Call level 1 function
    cross_layer_args level1_args = {args->level, args->value + 1, 0};
    CGNThreadHandle level1_handle = async_run(level1_async_func, &level1_args);
    uint64_t level1_result = await(level1_handle);
    
    printf("Level 2 got result %llu from level 1\n", level1_result);
    
    return level1_result + args->value;
}

// Level 3 async function (deep nesting)
async uint64_t level3_async_func(void *p) {
    cross_layer_args *args = (cross_layer_args *)p;
    
    printf("Level 3 async func with value %d\n", args->value);
    
    if (args->level == 3) {
        // This is the deep level - spawn a thread that will be awaited by shallow level
        cross_layer_args worker_args = {args->level, args->value + 20, 0};
        g_deep_spawned_thread = async_run(simple_worker_func, &worker_args);
        
        printf("Level 3 spawned thread %llu\n", g_deep_spawned_thread);
        
        // Yield to allow shallow level to run and await our thread
        async_yield();
        async_yield();
        
        return args->value;
    } else {
        // This is called from shallower levels - await the thread spawned by shallow level
        printf("Level 3 awaiting shallow-spawned thread %llu\n", g_shallow_spawned_thread);
        
        // Wait for shallow thread to be spawned
        while (g_thread_ready == 0) {
            async_yield();
        }
        
        uint64_t result = await(g_shallow_spawned_thread);
        printf("Level 3 got result %llu from shallow-spawned thread\n", result);
        
        return result + args->value;
    }
}

// Test function for deep spawn -> shallow await
async uint64_t test_deep_spawn_shallow_await(void *p) {
    cross_layer_args *args = (cross_layer_args *)p;
    
    printf("Testing deep spawn -> shallow await with value %d\n", args->value);
    
    // Reset global state
    g_deep_spawned_thread = 0;
    g_shallow_spawned_thread = 0;
    g_thread_ready = 0;
    
    // Start deep level (level 3) which will spawn a thread
    cross_layer_args deep_args = {3, args->value, 0};
    CGNThreadHandle deep_handle = async_run(level3_async_func, &deep_args);
    
    // Yield to let deep level spawn its thread
    async_yield();
    
    // Start shallow level (level 1) which will await the deep-spawned thread
    cross_layer_args shallow_args = {0, args->value + 5, 0}; // Use level 0 to trigger await behavior
    CGNThreadHandle shallow_handle = async_run(level1_async_func, &shallow_args);
    
    // Wait for both to complete
    uint64_t deep_result = await(deep_handle);
    uint64_t shallow_result = await(shallow_handle);
    
    printf("Deep spawn result: %llu, Shallow await result: %llu\n", deep_result, shallow_result);
    
    // The shallow level should have awaited the deep-spawned thread
    // Deep spawns thread with value+20, worker returns (value+20)*3
    // Shallow awaits that thread and adds its own value+5
    uint64_t expected = (args->value + 20) * 3 + (args->value + 5);
    
    printf("Expected shallow result: %llu\n", expected);
    assert(shallow_result == expected);
    return shallow_result;
}

// Test function for shallow spawn -> deep await
async uint64_t test_shallow_spawn_deep_await(void *p) {
    cross_layer_args *args = (cross_layer_args *)p;
    
    printf("Testing shallow spawn -> deep await with value %d\n", args->value);
    
    // Reset global state
    g_deep_spawned_thread = 0;
    g_shallow_spawned_thread = 0;
    g_thread_ready = 0;
    
    // Start shallow level (level 1) which will spawn a thread
    cross_layer_args shallow_args = {1, args->value, 0};
    CGNThreadHandle shallow_handle = async_run(level1_async_func, &shallow_args);
    
    // Yield to let shallow level spawn its thread
    async_yield();
    
    // Start deep level (level 3) which will await the shallow-spawned thread
    cross_layer_args deep_args = {0, args->value + 10, 0}; // Use level 0 to trigger await behavior
    CGNThreadHandle deep_handle = async_run(level3_async_func, &deep_args);
    
    // Wait for both to complete
    uint64_t shallow_result = await(shallow_handle);
    uint64_t deep_result = await(deep_handle);
    
    printf("Shallow spawn result: %llu, Deep await result: %llu\n", shallow_result, deep_result);
    
    // Expected: shallow spawns thread with value+10, worker returns (value+10)*3
    // Deep awaits that thread and adds its own value+10
    uint64_t expected = (args->value + 10) * 3 + (args->value + 10);
    
    assert(deep_result == expected);
    return deep_result;
}

// Test function for complex cross-layer scenario
async uint64_t test_complex_cross_layer(void *p) {
    cross_layer_args *args = (cross_layer_args *)p;
    
    printf("Testing complex cross-layer scenario with value %d\n", args->value);
    
    // Reset global state
    g_deep_spawned_thread = 0;
    g_shallow_spawned_thread = 0;
    g_thread_ready = 0;
    
    // Start multiple levels simultaneously
    cross_layer_args level1_args = {1, args->value, 0};
    cross_layer_args level2_args = {2, args->value + 1, 0};
    cross_layer_args level3_args = {3, args->value + 2, 0};
    
    CGNThreadHandle handle1 = async_run(level1_async_func, &level1_args);
    CGNThreadHandle handle2 = async_run(level2_async_func, &level2_args);
    CGNThreadHandle handle3 = async_run(level3_async_func, &level3_args);
    
    // Yield to let all levels start
    async_yield();
    async_yield();
    
    // Wait for all to complete
    uint64_t result1 = await(handle1);
    uint64_t result2 = await(handle2);
    uint64_t result3 = await(handle3);
    
    printf("Complex cross-layer results: %llu, %llu, %llu\n", result1, result2, result3);
    
    // All should be positive (basic sanity check)
    assert(result1 > 0);
    assert(result2 > 0);
    assert(result3 > 0);
    
    return result1 + result2 + result3;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing cross-layer async thread spawning and awaiting...\n");
    
    // Test 1: Deep spawn -> Shallow await
    printf("\n=== Test 1: Deep spawn -> Shallow await ===\n");
    cross_layer_args args1 = {0, 10, 0};
    CGNThreadHandle handle1 = async_run(test_deep_spawn_shallow_await, &args1);
    uint64_t result1 = await(handle1);
    printf("Deep spawn -> Shallow await result: %llu\n", result1);
    
    // Test 2: Shallow spawn -> Deep await
    printf("\n=== Test 2: Shallow spawn -> Deep await ===\n");
    cross_layer_args args2 = {0, 15, 0};
    CGNThreadHandle handle2 = async_run(test_shallow_spawn_deep_await, &args2);
    uint64_t result2 = await(handle2);
    printf("Shallow spawn -> Deep await result: %llu\n", result2);
    
    // Test 3: Complex cross-layer scenario
    printf("\n=== Test 3: Complex cross-layer scenario ===\n");
    cross_layer_args args3 = {0, 20, 0};
    CGNThreadHandle handle3 = async_run(test_complex_cross_layer, &args3);
    uint64_t result3 = await(handle3);
    printf("Complex cross-layer result: %llu\n", result3);
    
    // Test 4: Sequential cross-layer tests (to avoid global variable conflicts)
    printf("\n=== Test 4: Sequential cross-layer tests ===\n");
    
    cross_layer_args args4a = {0, 30, 0};
    CGNThreadHandle handle4a = async_run(test_deep_spawn_shallow_await, &args4a);
    uint64_t result4a = await(handle4a);
    printf("Sequential test 4a result: %llu\n", result4a);
    
    cross_layer_args args4b = {0, 35, 0};
    CGNThreadHandle handle4b = async_run(test_shallow_spawn_deep_await, &args4b);
    uint64_t result4b = await(handle4b);
    printf("Sequential test 4b result: %llu\n", result4b);
    
    cross_layer_args args4c = {0, 40, 0};
    CGNThreadHandle handle4c = async_run(test_deep_spawn_shallow_await, &args4c);
    uint64_t result4c = await(handle4c);
    printf("Sequential test 4c result: %llu\n", result4c);
    
    cross_layer_args args4d = {0, 45, 0};
    CGNThreadHandle handle4d = async_run(test_shallow_spawn_deep_await, &args4d);
    uint64_t result4d = await(handle4d);
    printf("Sequential test 4d result: %llu\n", result4d);
    
    seagreen_free_rt();
    
    return 0;
}
