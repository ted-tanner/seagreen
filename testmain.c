#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "include/seagreen.h"
#include "seagreen.h"

async int foo(int a, int b) {
    struct timespec ts = {
	.tv_sec = 0,
	.tv_nsec = 500000000,
    };

    printf("foo(%d, %d) - 1\n", a, b);
    nanosleep(&ts, NULL);

    async_yield();

    printf("foo(%d, %d) - 2\n", a, b);
    nanosleep(&ts, NULL);

    async_yield();

    printf("foo(%d, %d) - 3\n", a, b);
    nanosleep(&ts, NULL);

    async_yield();

    printf("foo(%d, %d) - 4\n", a, b);
    nanosleep(&ts, NULL);

    return a + b;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();

    printf("Starting foo(1, 2)...\n");
    CGNThreadHandle_int t1 = async_run(foo(1, 2));

    printf("Starting foo(3, 4)...\n");
    CGNThreadHandle_int t2 = async_run(foo(3, 4));

    printf("Awaiting...\n");

    int foo1_res = await(t1);
    printf("foo(1, 2) returned %d\n", foo1_res);

    int foo2_res = await(t2);
    printf("foo(3, 4) returned %d\n", foo2_res);

    seagreen_free_rt();

    return 0;
}

// TODO: Test with ints
// TODO: Test with floats
// TODO: Test with void
// TODO: Test on Linux
// TODO: Test on Risc V
// TODO: Test on x86 MacOS
// TODO: Test on x86 Windows
