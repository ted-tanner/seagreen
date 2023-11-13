#include <stdint.h>
#include <stdio.h>

#include "seagreen.h"

async int foo(int a, int b) {
    printf("1 foo(%d, %d)\n", a, b);
    for (int i = 0; i < INT32_MAX; ++i) {
        __asm__ volatile("nop");
    }

    async_yield();

    printf("2 foo(%d, %d)\n", a, b);
    for (int i = 0; i < INT32_MAX; ++i) {
        __asm__ volatile("nop");
    }

    async_yield();

    printf("3 foo(%d, %d)\n", a, b);
    for (int i = 0; i < INT32_MAX; ++i) {
        __asm__ volatile("nop");
    }

    async_yield();

    printf("4 foo(%d, %d)\n", a, b);

    return a + b;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();

    printf("Starting foo(1, 2)...\n");
    CGNThreadHandle_int h1 = async_run(foo(1, 2));

    printf("Starting foo(3, 4)...\n");
    CGNThreadHandle_int h2 = async_run(foo(3, 4));

    printf("Awaiting...\n");

    int foo1_res = await(h1);
    printf("foo(1, 2) returned %d\n", foo1_res);

    int foo2_res = await(h2);
    printf("foo(3, 4) returned %d\n", foo2_res);

    return 0;
}

// TODO: Test with ints
// TODO: Test with floats
// TODO: Test with void//
// TODO: Test on x86
