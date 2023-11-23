#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "seagreen.h"

#define THREAD_COUNT 10000

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

    // TO
    await(handles[0]);
    await(handles[0]);
    await(handles[0]);

    printf("Awaiting...\n");
    for (int i = 1; i < THREAD_COUNT; ++i) {
	int foo_res = await(handles[i]);
	assert(foo_res == 5);

	// Suppress unused variable warning in release builds
	(void) foo_res;
    }

    // TODO: This breaks (reusing the same threads gives wrong return value)
    // printf("Starting %d threads...\n", THREAD_COUNT);
    // for (int i = 0; i < THREAD_COUNT; ++i) {
    // 	handles[i] = async_run(foo());
    // }

    // printf("Awaiting...\n");
    // for (int i = 0; i < THREAD_COUNT; ++i) {
    // 	int foo_res = await(handles[i]);
    // 	assert(foo_res == 5);
    // }

    seagreen_free_rt();

    return 0;
}
