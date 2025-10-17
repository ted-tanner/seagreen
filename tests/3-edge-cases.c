#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "seagreen.h"

async uint64_t foo(void *p) {
    (void)p;
    __asm__ volatile("nop");
    async_yield();
    return 5;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();

    printf("Starting foo()...\n");
    CGNThreadHandle handle = async_run(foo, 0);

    printf("Awaiting...\n");
    await(handle);
    await(handle);
    await(handle);

    seagreen_free_rt();

    return 0;
}
