#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "seagreen.h"

static CGNThreadHandle g_deep_spawned_thread = 0;
static CGNThreadHandle g_shallow_spawned_thread = 0;
static volatile int g_thread_ready = 0;

typedef struct {
    int level;
    int value;
    int expected_result;
} cross_layer_args;

async uint64_t simple_worker_func(void *p) {
    cross_layer_args *args = (cross_layer_args *)p;
    
    printf("  Worker func called with value %d at level %d\n", args->value, args->level);
    
    async_yield();
    
    return args->value * 3;
}

async uint64_t level1_async_func(void *p) {
    cross_layer_args *args = (cross_layer_args *)p;
    
    printf("Level 1 async func with value %d\n", args->value);
    
    if (args->level == 1) {
        cross_layer_args worker_args = {args->level, args->value + 10, 0};
        g_shallow_spawned_thread = async_run(simple_worker_func, &worker_args);
        g_thread_ready = 1;
        
        printf("Level 1 spawned thread %llu\n", g_shallow_spawned_thread);
        
        async_yield();
        async_yield();
        async_yield();
        
        return args->value;
    } else {
        printf("Level 1 awaiting deep-spawned thread %llu\n", g_deep_spawned_thread);
        
        while (g_deep_spawned_thread == 0) {
            async_yield();
        }
        
        uint64_t result = await(g_deep_spawned_thread);
        printf("Level 1 got result %llu from deep-spawned thread\n", result);
        
        return result + args->value;
    }
}

async uint64_t level3_async_func(void *p) {
    cross_layer_args *args = (cross_layer_args *)p;
    
    printf("Level 3 async func with value %d\n", args->value);
    
    if (args->level == 3) {
        cross_layer_args worker_args = {args->level, args->value + 20, 0};
        g_deep_spawned_thread = async_run(simple_worker_func, &worker_args);
        
        printf("Level 3 spawned thread %llu\n", g_deep_spawned_thread);
        
        async_yield();
        async_yield();
        
        return args->value;
    } else {
        printf("Level 3 awaiting shallow-spawned thread %llu\n", g_shallow_spawned_thread);
        
        while (g_thread_ready == 0) {
            async_yield();
        }
        
        uint64_t result = await(g_shallow_spawned_thread);
        printf("Level 3 got result %llu from shallow-spawned thread\n", result);
        
        return result + args->value;
    }
}

async uint64_t test_shallow_spawn_deep_await(void *p) {
    cross_layer_args *args = (cross_layer_args *)p;
    
    printf("Testing shallow spawn -> deep await with value %d\n", args->value);
    
    g_deep_spawned_thread = 0;
    g_shallow_spawned_thread = 0;
    g_thread_ready = 0;
    
    cross_layer_args shallow_args = {1, args->value, 0};
    CGNThreadHandle shallow_handle = async_run(level1_async_func, &shallow_args);
    
    async_yield();
    
    cross_layer_args deep_args = {0, args->value + 10, 0};
    CGNThreadHandle deep_handle = async_run(level3_async_func, &deep_args);
    
    uint64_t shallow_result = await(shallow_handle);
    uint64_t deep_result = await(deep_handle);
    
    printf("Shallow spawn result: %llu, Deep await result: %llu\n", shallow_result, deep_result);
    
    uint64_t expected = (args->value + 10) * 3 + (args->value + 10);
    
    assert(deep_result == expected);
    return deep_result;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing cross-layer async thread spawning and awaiting...\n");
    
    printf("\n=== Test: Shallow spawn -> Deep await ===\n");
    cross_layer_args args2 = {0, 15, 0};
    CGNThreadHandle handle2 = async_run(test_shallow_spawn_deep_await, &args2);
    uint64_t result2 = await(handle2);
    printf("Shallow spawn -> Deep await result: %llu\n", result2);
    
    seagreen_free_rt();
    
    return 0;
}
