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

async uint64_t multi_nested_async_func(void *p) {
    nested_args *args = (nested_args *)p;
    
    printf("Multi-nested async func level %d with value %d\n", args->level, args->value);
    
    async_yield();
    
    CGNThreadHandle handles[3];
    nested_args call_args[3];
    
    for (int i = 0; i < 3; i++) {
        call_args[i] = (nested_args){args->level + 1, args->max_level, args->value + i, 0};
        handles[i] = async_run(simple_async_func, &call_args[i]);
    }
    
    uint64_t total = 0;
    for (int i = 0; i < 3; i++) {
        uint64_t result = await(handles[i]);
        total += result;
    }
    
    async_yield();
    
    return total;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing concurrent nested async calls...\n");
    
    CGNThreadHandle concurrent_handles[5];
    nested_args concurrent_args[5];
    
    for (int i = 0; i < 5; i++) {
        concurrent_args[i] = (nested_args){0, 2, i + 10, 0};
        concurrent_handles[i] = async_run(multi_nested_async_func, &concurrent_args[i]);
    }
    
    for (int i = 0; i < 5; i++) {
        uint64_t result = await(concurrent_handles[i]);
        uint64_t expected = (uint64_t)((i + 10) * 2 + (i + 11) * 2 + (i + 12) * 2);
        printf("Concurrent nested async %d result: %llu (expected %llu)\n", i, result, expected);
        assert(result == expected);
    }
    
    seagreen_free_rt();
    
    return 0;
}
