#include <assert.h>
#include <stdio.h>

#include "seagreen.h"

static _Bool running_func_is_bar[8] = {0};
static int pos = 0;

static int ran_async = 0;
static int ran_sync = 0;

async int foo(int a, int b) {
    printf("foo() - 1\n");
    running_func_is_bar[pos++] = 0;
    async_yield();

    printf("foo() - 2\n");
    running_func_is_bar[pos++] = 0;
    async_yield();

    printf("foo() - 3\n");
    running_func_is_bar[pos++] = 0;
    async_yield();

    printf("foo() - 4\n");
    running_func_is_bar[pos++] = 0;

    return a + b;
}

async int bar(int a) {
    printf("bar() - 1\n");
    running_func_is_bar[pos++] = 1;
    async_yield();

    printf("bar() - 2\n");
    running_func_is_bar[pos++] = 1;
    async_yield();

    printf("bar() - 3\n");
    running_func_is_bar[pos++] = 1;
    async_yield();

    printf("bar() - 4\n");
    running_func_is_bar[pos++] = 1;

    return a + 5;
}

async int to_run_sync(_Bool as_sync) {
    async_yield();
    async_yield();

    if (as_sync) {
        ran_sync = 1;
    } else {
        ran_async = 1;
    }

    async_yield();

    return 0;
}

int main(void) {
    for (int i = 0; i < 8; ++i) {
        running_func_is_bar[i] = 2;
    }

    printf("Initializing runtime...\n");
    seagreen_init_rt();

    printf("Starting foo()...\n");
    struct CGNThreadHandle_int t1 = async_run(foo(1, 2));

    printf("Starting bar()...\n");
    struct CGNThreadHandle_int t2 = async_run(bar(3));

    printf("Awaiting...\n");

    int foo_res = await(t1);
    printf("foo() returned %d\n", foo_res);

    assert(foo_res == 3);

    int bar_res = await(t2);
    printf("bar() returned %d\n", bar_res);

    assert(bar_res == 8);

    for (int i = 0; i < 8; ++i) {
        assert(running_func_is_bar[i] == i % 2);
    }

    async_run(to_run_sync(0));
    run_as_sync(to_run_sync(1));
    async_run(to_run_sync(0));

    assert(!ran_async);
    assert(ran_sync);

    seagreen_free_rt();

    return 0;
}

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
