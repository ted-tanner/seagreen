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

async uint64_t nested_async_func(void *p) {
    nested_args *args = (nested_args *)p;
    
    printf("Nested async func level %d with value %d\n", args->level, args->value);
    
    if (args->level >= args->max_level) {
        nested_args simple_args = {args->level, args->max_level, args->value, 0};
        CGNThreadHandle handle = async_run(simple_async_func, &simple_args);
        uint64_t result = await(handle);
        return result + args->value;
    }
    
    async_yield();
    
    nested_args next_args = {
        args->level + 1,
        args->max_level,
        args->value + 1,
        args->expected_result
    };
    
    CGNThreadHandle nested_handle = async_run(nested_async_func, &next_args);
    uint64_t nested_result = await(nested_handle);
    
    async_yield();
    
    return nested_result + args->value;
}

async uint64_t conditional_nested_async_func(void *p) {
    nested_args *args = (nested_args *)p;
    
    printf("Conditional-nested async func level %d with value %d\n", args->level, args->value);
    
    async_yield();
    
    if (args->value % 2 == 0) {
        nested_args call_args = {args->level + 1, args->max_level, args->value, 0};
        CGNThreadHandle handle = async_run(simple_async_func, &call_args);
        return await(handle);
    } else {
        nested_args call_args = {args->level + 1, args->max_level, args->value, 0};
        CGNThreadHandle handle = async_run(nested_async_func, &call_args);
        return await(handle);
    }
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing conditional nested async calls...\n");
    
    for (int i = 0; i < 6; i++) {
        nested_args args5 = {0, 2, i, 0};
        CGNThreadHandle handle5 = async_run(conditional_nested_async_func, &args5);
        uint64_t result5 = await(handle5);
        printf("Conditional nested async result for %d: %llu\n", i, result5);
        
        if (i % 2 == 0) {
            assert(result5 == (uint64_t)(i * 2));
        } else {
            assert(result5 > 0);
        }
    }
    
    seagreen_free_rt();
    
    return 0;
}
