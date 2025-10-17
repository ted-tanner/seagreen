#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "seagreen.h"

typedef struct {
    int value;
    int expected_result;
    int level;
} sync_async_args;

async uint64_t simple_worker_func(void *p) {
    sync_async_args *args = (sync_async_args *)p;
    
    printf("  Worker func called with value %d at level %d\n", args->value, args->level);
    
    async_yield();
    
    return args->value * 2;
}

uint64_t sync_function_conditional_async(void *p) {
    sync_async_args *args = (sync_async_args *)p;
    
    printf("Sync function conditional async with value %d\n", args->value);
    
    if (args->value % 2 == 0) {
        sync_async_args worker_args = {args->value, 0, args->level + 1};
        CGNThreadHandle handle = async_run(simple_worker_func, &worker_args);
        uint64_t result = await(handle);
        return result;
    } else {
        sync_async_args worker_args1 = {args->value, 0, args->level + 1};
        sync_async_args worker_args2 = {args->value + 1, 0, args->level + 1};
        
        CGNThreadHandle handle1 = async_run(simple_worker_func, &worker_args1);
        CGNThreadHandle handle2 = async_run(simple_worker_func, &worker_args2);
        
        uint64_t result1 = await(handle1);
        uint64_t result2 = await(handle2);
        
        return result1 + result2;
    }
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing sync functions calling async functions...\n");
    
    printf("\n=== Test: Sync function calling async functions conditionally ===\n");
    for (int i = 0; i < 6; i++) {
        sync_async_args args3 = {i, 0, 0};
        uint64_t result3 = sync_function_conditional_async(&args3);
        printf("Sync->conditional async result for %d: %llu\n", i, result3);
        
        if (i % 2 == 0) {
            assert(result3 == (uint64_t)(i * 2));
        } else {
            assert(result3 == (uint64_t)(i * 2 + (i + 1) * 2));
        }
    }
    
    seagreen_free_rt();
    
    return 0;
}
