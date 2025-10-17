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

uint64_t sync_function_calling_async(void *p) {
    sync_async_args *args = (sync_async_args *)p;
    
    printf("Sync function calling async with value %d\n", args->value);
    
    sync_async_args worker_args = {args->value + 10, 0, args->level + 1};
    CGNThreadHandle handle = async_run(simple_worker_func, &worker_args);
    uint64_t result = await(handle);
    
    printf("Sync function got result %llu from async call\n", result);
    
    return result + args->value;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing sync functions calling async functions...\n");
    
    printf("\n=== Test: Sync function calling single async function ===\n");
    sync_async_args args1 = {10, 0, 0};
    uint64_t result1 = sync_function_calling_async(&args1);
    printf("Sync->async result: %llu (expected: 50)\n", result1);
    assert(result1 == 50);
    
    seagreen_free_rt();
    
    return 0;
}
