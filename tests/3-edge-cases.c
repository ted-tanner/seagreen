#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "seagreen.h"

async int foo() {
    __asm__ volatile("nop");
    async_yield();
    return 5;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();

    printf("Starting foo()...\n");
    struct CGNThreadHandle handle = async_run(foo());

    // Awaiting multiple times doesn't cause issues
    printf("Awaiting...\n");
    await(handle);
    await(handle);
    await(handle);

    seagreen_free_rt();

    return 0;
}
