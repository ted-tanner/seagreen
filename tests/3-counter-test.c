#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "seagreen.h"

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

    async_run(increment_counter, 0);
    async_run(increment_counter, 0);
    assert(counter == 0);

    seagreen_free_rt();

    return 0;
}
