#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "seagreen.h"

typedef struct {
    int level;
    int max_level;
    int value;
    int expected_result;
} nested_args;

async uint64_t simple_async_func(void *p) {
    nested_args *args = (nested_args *)p;
    
    printf("  Simple async func called with value %d at level %d\n", args->value, args->level);
    
    async_yield();
    
    return args->value * 2;
}

async uint64_t loop_nested_async_func(void *p) {
    nested_args *args = (nested_args *)p;
    
    printf("Loop-nested async func level %d with value %d\n", args->level, args->value);
    
    uint64_t total = 0;
    
    for (int i = 0; i < 5; i++) {
        async_yield();
        
        nested_args call_args = {args->level + 1, args->max_level, args->value + i, 0};
        CGNThreadHandle handle = async_run(simple_async_func, &call_args);
        uint64_t result = await(handle);
        
        total += result;
    }
    
    return total;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing loop nested async calls...\n");
    
    nested_args args4 = {0, 2, 5, 0};
    CGNThreadHandle handle4 = async_run(loop_nested_async_func, &args4);
    uint64_t result4 = await(handle4);
    printf("Loop-nested async result: %llu (expected: 70)\n", result4);
    assert(result4 == 70);
    
    seagreen_free_rt();
    
    return 0;
}
