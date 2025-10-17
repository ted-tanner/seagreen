#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "seagreen.h"

static volatile int main_thread_yield_count = 0;
static volatile int main_thread_execution_count = 0;

typedef struct {
    int thread_id;
    int work_duration;
    int expected_result;
} main_yield_args;

async uint64_t worker_func(void *p) {
    main_yield_args *args = (main_yield_args *)p;
    
    printf("Worker %d starting (work duration: %d)\n", args->thread_id, args->work_duration);
    
    for (int i = 0; i < args->work_duration; i++) {
        printf("Worker %d working... step %d/%d\n", args->thread_id, i + 1, args->work_duration);
        async_yield();
    }
    
    printf("Worker %d completed\n", args->thread_id);
    return args->expected_result;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing async_yield() from main thread...\n");
    
    printf("\n=== Test: Main thread yield with many threads ===\n");
    
    main_yield_args args[10];
    CGNThreadHandle handles[10];
    
    for (int i = 0; i < 10; i++) {
        args[i] = (main_yield_args){i, 3 + (i % 4), 1000 + i * 10};
        handles[i] = async_run(worker_func, &args[i]);
    }
    
    printf("10 threads started, main thread yielding extensively...\n");
    
    printf("Main thread yielding once...");
    main_thread_yield_count++;
    async_yield();
    main_thread_execution_count++;
    
    printf("Main thread finished extensive yielding, awaiting threads...\n");
    
    for (int i = 0; i < 10; i++) {
        uint64_t result = await(handles[i]);
        printf("Many threads %d result: %llu (expected: %d)\n", i, result, args[i].expected_result);
        assert(result == (uint64_t)args[i].expected_result);
    }
    
    seagreen_free_rt();
    
    return 0;
}
