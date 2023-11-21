#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "seagreen.h"

#define THREAD_COUNT 100000

async int foo() {
    __asm__ volatile("nop");
    async_yield();
    __asm__ volatile("nop");
    async_yield();
    __asm__ volatile("nop");
    async_yield();
    __asm__ volatile("nop");

    return 5;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();

    CGNThreadHandle_int *handles = (CGNThreadHandle_int *)
	malloc(sizeof(CGNThreadHandle_int) * THREAD_COUNT);

    printf("Starting %d threads...\n", THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; ++i) {
	handles[i] = async_run(foo());
    }

    printf("Awaiting...\n");
    for (int i = 0; i < THREAD_COUNT; ++i) {
	int foo_res = await(handles[i]);
	assert(foo_res == 5);
    }

    seagreen_free_rt();

    return 0;
}
