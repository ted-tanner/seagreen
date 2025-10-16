#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "seagreen.h"

#define THREAD_COUNT 5

static _Bool running_func[THREAD_COUNT] = {0};

typedef struct { int id; } foo_args;
async uint64_t foo(void *p) {
    foo_args *args = (foo_args *)p;
    int id = args->id;
    
    printf("foo() - 1\n"); 
    running_func[id] = 1; 
    async_yield();
    
    printf("foo() - 2\n"); 
    async_yield();
    
    printf("foo() - 3\n"); 
    async_yield();

    printf("foo() - 4\n"); 
    async_yield();
    
    printf("foo() - 5\n"); 
    async_yield();

    return 5;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();

    CGNThreadHandle handles[THREAD_COUNT];

    printf("Starting foo()...\n");
    foo_args args_array[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; ++i) {
        args_array[i] = (foo_args){i};
        handles[i] = async_run_fn(foo, &args_array[i]);
    }

    printf("Awaiting...\n");
    for (int i = 0; i < THREAD_COUNT; ++i) {
        int foo_res = await(handles[i]);
        assert(foo_res == 5);
    }

    for (int i = 0; i < THREAD_COUNT; ++i) {
        assert(running_func[i]);
    }

    seagreen_free_rt();

    return 0;
}
