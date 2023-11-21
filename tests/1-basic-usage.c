#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "seagreen.h"

async int foo(int a, int b) {
    printf("foo() - 1\n");
    async_yield();

    printf("foo() - 2\n");
    async_yield();

    printf("foo() - 3\n");
    async_yield();

    printf("foo() - 4\n");

    return a + b;
}

async int bar(int a) {
    printf("bar() - 1\n");
    async_yield();

    printf("bar() - 2\n");
    async_yield();

    printf("bar() - 3\n");
    async_yield();

    printf("bar() - 4\n");

    return a + 5;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();

    printf("Starting foo()...\n");
    CGNThreadHandle_int t1 = async_run(foo(1, 2));

    printf("Starting bar()...\n");
    CGNThreadHandle_int t2 = async_run(bar(3));

    printf("Awaiting...\n");

    int foo_res = await(t1);
    printf("foo() returned %d\n", foo_res);

    assert(foo_res == 3);

    int bar_res = await(t2);
    printf("bar() returned %d\n", bar_res);

    assert(bar_res == 8);

    seagreen_free_rt();

    return 0;
}

// TODO: Test with a bunch of threads
// TODO: Test calling async within async
// TODO: Test calling async from sync function called from within async
// TODO: Test with real IO
// TODO: Test with uint64s
// TODO: Test with floats
// TODO: Test with void
// TODO: Test on Linux
// TODO: Test on Risc V
// TODO: Test on x86 MacOS
// TODO: Test on x86 Windows
