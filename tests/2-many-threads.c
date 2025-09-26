#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "seagreen.h"

//#define THREAD_COUNT 10000
#define THREAD_COUNT 5

static _Bool running_func[THREAD_COUNT] = {0};
static int pos = 0;

async int foo() {
    __asm__ volatile("nop");
    printf("foo() - 1\n");
    async_yield();
    __asm__ volatile("nop");
    printf("foo() - 2\n");
    async_yield();
    printf("foo() - 3\n");

    running_func[pos++] = 1;
    
    async_yield();
    __asm__ volatile("nop");
    printf("foo() - 4\n");

    return 5;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();

    CGNThreadHandle *handles =
        (CGNThreadHandle *)malloc(sizeof(CGNThreadHandle) * THREAD_COUNT);

    printf("Starting %d threads...\n", THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; ++i) {
        handles[i] = async_run(foo());
    }

    printf("Awaiting...\n");
    for (int i = 0; i < THREAD_COUNT; ++i) {
        int foo_res = await(handles[i]);
        assert(foo_res == 5);
    }

    for (int i = 0; i < THREAD_COUNT; ++i) {
        assert(running_func[i]);
    }

    printf("\nRepeating test...\n");

    pos = 0;
    for (int i = 0; i < THREAD_COUNT; ++i) {
        running_func[i] = 0;
    }

    printf("Starting %d threads...\n", THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; ++i) {
    	handles[i] = async_run(foo());
    }

    printf("Awaiting...\n");
    for (int i = 0; i < THREAD_COUNT; ++i) {
    	int foo_res = await(handles[i]);
    	assert(foo_res == 5);
    }

    for (int i = 0; i < THREAD_COUNT; ++i) {
        assert(running_func[i]);
    }

    seagreen_free_rt();

    return 0;
}
