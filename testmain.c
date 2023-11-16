#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "seagreen.h"

async int foo(int a, int b) {
    struct timespec ts = {
	.tv_sec = 0,
	.tv_nsec = 250000000,
    };

    printf("foo() - 1\n");
    nanosleep(&ts, NULL);

    async_yield();

    printf("foo() - 2\n");
    nanosleep(&ts, NULL);

    async_yield();

    printf("foo() - 3\n");
    nanosleep(&ts, NULL);

    async_yield();

    printf("foo() - 4\n");
    nanosleep(&ts, NULL);

    return a + b;
}

async int bar(int a) {
    struct timespec ts = {
	.tv_sec = 0,
	.tv_nsec = 250000000,
    };

    printf("bar() - 1\n");
    nanosleep(&ts, NULL);

    async_yield();

    printf("bar() - 2\n");
    nanosleep(&ts, NULL);

    async_yield();

    printf("bar() - 3\n");
    nanosleep(&ts, NULL);

    async_yield();

    printf("bar() - 4\n");
    nanosleep(&ts, NULL);

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

    int bar_res = await(t2);
    printf("bar() returned %d\n", bar_res);

    seagreen_free_rt();

    return 0;
}

// TODO: Test calling async within async
// TODO: Test calling async from sync function called from within within async
// TODO: Test with ints
// TODO: Test with floats
// TODO: Test with void
// TODO: Test on Linux
// TODO: Test on Risc V
// TODO: Test on x86 MacOS
// TODO: Test on x86 Windows
