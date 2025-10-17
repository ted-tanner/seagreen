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

uint64_t sync_function_calling_multiple_async(void *p) {
    sync_async_args *args = (sync_async_args *)p;
    
    printf("Sync function calling multiple async with value %d\n", args->value);
    
    CGNThreadHandle handles[3];
    sync_async_args worker_args[3];
    
    for (int i = 0; i < 3; i++) {
        worker_args[i] = (sync_async_args){args->value + i, 0, args->level + 1};
        handles[i] = async_run(simple_worker_func, &worker_args[i]);
    }
    
    uint64_t total = 0;
    for (int i = 0; i < 3; i++) {
        uint64_t result = await(handles[i]);
        total += result;
    }
    
    printf("Sync function got total result %llu from multiple async calls\n", total);
    
    return total + args->value;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing sync functions calling async functions...\n");
    
    printf("\n=== Test: Sync function calling multiple async functions ===\n");
    sync_async_args args2 = {5, 0, 0};
    uint64_t result2 = sync_function_calling_multiple_async(&args2);
    printf("Sync->multiple async result: %llu (expected: 41)\n", result2);
    assert(result2 == 41);
    
    seagreen_free_rt();
    
    return 0;
}
