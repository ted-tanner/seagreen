#include <stdint.h>
#include <stdio.h>

#include "seagreen.h"

async(int) foo(int a, int b) {
    printf("foo(%d, %d)\n", a, b);
    for (int i = 0; i < INT32_MAX; ++i) {
	asm volatile("nop");
    }

    cgn_yield();
    printf("foo(%d, %d)\n", a, b);

    for (int i = 0; i < INT32_MAX; ++i) {
	asm volatile("nop");
    }

    cgn_yield();
    printf("foo(%d, %d)\n", a, b);

    for (int i = 0; i < INT32_MAX; ++i) {
	asm volatile("nop");
    }

    cgn_yield();
    printf("foo(%d, %d)\n", a, b);

    async_return(a + b);
}

int main(void) {
    printf("Initializing runtime...\n");
    cgn_init_rt();

    printf("Starting foo(1, 2)...\n");
    CGNThreadHandle h1 = async_run(foo(1, 2));
    printf("Starting foo(3, 4)...\n");
    CGNThreadHandle h2 = async_run(foo(3, 4));

    printf("Awaiting...\n");

    int foo1_res = await(h1);
    printf("foo(1, 2) returned %d\n", foo1_res);

    int foo2_res = await(h2);
    printf("foo(3, 4) returned %d\n", foo2_res);

    return 0;
}
