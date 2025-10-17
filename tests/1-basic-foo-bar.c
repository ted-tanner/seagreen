#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "seagreen.h"

typedef struct { int a; int b; int id; } foo_args;
async uint64_t foo(void *p) {
    foo_args *args = (foo_args *)p;
    int a = args->a;
    int b = args->b;
    
    printf("foo() - 1\n");
    async_yield();

    printf("foo() - 2\n");
    async_yield();

    printf("foo() - 3\n");
    async_yield();

    return a + b;
}

typedef struct { int a; int id; } bar_args;
async uint64_t bar(void *p) {
    bar_args *args = (bar_args *)p;
    int a = args->a;
    
    printf("bar() - 1\n");
    async_yield();

    printf("bar() - 2\n");
    async_yield();

    printf("bar() - 3\n");
    async_yield();

    printf("bar() - 4\n");
    async_yield();

    return a + 2;
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

    seagreen_free_rt();

    return 0;
}
