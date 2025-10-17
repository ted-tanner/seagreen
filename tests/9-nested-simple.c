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

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing simple async within async...\n");
    
    nested_args args1 = {0, 1, 10, 20};
    CGNThreadHandle handle1 = async_run(nested_async_func, &args1);
    uint64_t result1 = await(handle1);
    printf("Simple nested async result: %llu\n", result1);
    assert(result1 == 43);
    
    printf("Testing deep nested async calls...\n");
    
    nested_args args2 = {0, 3, 5, 0};
    CGNThreadHandle handle2 = async_run(nested_async_func, &args2);
    uint64_t result2 = await(handle2);
    printf("Deep nested async result: %llu\n", result2);
    assert(result2 == 42);
    
    seagreen_free_rt();
    
    return 0;
}
