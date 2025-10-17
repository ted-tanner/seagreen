#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "seagreen.h"

static _Bool running_func_is_bar[8] = {0};

typedef struct { int a; int b; int id; } foo_args;
async uint64_t foo(void *p) {
    foo_args *args = (foo_args *)p;
    int a = args->a;
    int b = args->b;
    int id = args->id;
    
    printf("foo() - 1\n");
    running_func_is_bar[id] = 0;
    async_yield();

    printf("foo() - 2\n");
    running_func_is_bar[id] = 0;
    async_yield();

    printf("foo() - 3\n");
    running_func_is_bar[id] = 0;
    async_yield();

    return a + b;
}

typedef struct { int a; int id; } bar_args;
async uint64_t bar(void *p) {
    bar_args *args = (bar_args *)p;
    int a = args->a;
    int id = args->id;
    
    printf("bar() - 1\n");
    running_func_is_bar[id] = 1;
    async_yield();

    printf("bar() - 2\n");
    running_func_is_bar[id] = 1;
    async_yield();

    printf("bar() - 3\n");
    running_func_is_bar[id] = 1;
    async_yield();

    printf("bar() - 4\n");
    running_func_is_bar[id] = 1;
    async_yield();

    return a + 2;
}

static int counter = 0;
async uint64_t increment_counter(void *p) {
    (void)p;
    async_yield();
    ++counter;
    return 0;
}


int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();

    printf("Starting foo()...\n");
    foo_args fa = {1, 2, 0};
    CGNThreadHandle t1 = async_run(foo, &fa);

    printf("Starting bar()...\n");
    bar_args ba = {3, 1};
    CGNThreadHandle t2 = async_run(bar, &ba);

    printf("Awaiting...\n");
    int foo_res = await(t1);
    printf("foo() returned %d\n", foo_res);
    int bar_res = await(t2);
    printf("bar() returned %d\n", bar_res);

    assert(foo_res == 3);
    assert(bar_res == 5);

    printf("Starting several alternating foo/bar...\n");
    CGNThreadHandle handles[8];
    foo_args foo_args_array[4];
    bar_args bar_args_array[4];
    for (int i = 0; i < 8; ++i) {
        if (i % 2 == 0) {
            foo_args_array[i/2] = (foo_args){1, 2, i};
            handles[i] = async_run(foo, &foo_args_array[i/2]);
        } else {
            bar_args_array[i/2] = (bar_args){3, i};
            handles[i] = async_run(bar, &bar_args_array[i/2]);
        }
    }

    for (int i = 0; i < 8; ++i) {
        await(handles[i]);
    }

    for (int i = 0; i < 8; ++i) {
        assert(running_func_is_bar[i] == (i % 2));
    }

    async_run(increment_counter, 0);
    async_run(increment_counter, 0);
    assert(counter == 0);

    seagreen_free_rt();

    return 0;
}

// TODO: Test that spans multiple thread blocks and validates the stack isn't overwritten by other threads
// TODO: Test with recursive function
// TODO: Test with stack exist=ing before seagreen_init_rt()
// TODO: Test with a bunch of threads
// TODO: Test calling async within async
// TODO: Test calling async from sync function called from within async
// TODO: Test calling func with many args (both async_run and regular call)
// TODO: Test with real IO
// TODO: Test with uint64s
// TODO: Test with floats
// TODO: Test with void
// TODO: Test on Linux
// TODO: Test on Risc V
// TODO: Test on x86 MacOS
// TODO: Test on x86 Windows
