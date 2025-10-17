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
    
    printf("Testing multiple nested async calls...\n");
    
    nested_args args3 = {0, 2, 10, 0};
    CGNThreadHandle handle3 = async_run(multi_nested_async_func, &args3);
    uint64_t result3 = await(handle3);
    printf("Multi-nested async result: %llu (expected: 66)\n", result3);
    assert(result3 == 66);
    
    seagreen_free_rt();
    
    return 0;
}
