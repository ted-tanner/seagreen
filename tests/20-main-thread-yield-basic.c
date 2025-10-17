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

async uint64_t quick_worker_func(void *p) {
    main_yield_args *args = (main_yield_args *)p;
    
    printf("Quick worker %d starting\n", args->thread_id);
    
    async_yield();
    async_yield();
    
    printf("Quick worker %d completed\n", args->thread_id);
    return args->expected_result;
}

async uint64_t slow_worker_func(void *p) {
    main_yield_args *args = (main_yield_args *)p;
    
    printf("Slow worker %d starting\n", args->thread_id);
    
    for (int i = 0; i < 10; i++) {
        printf("Slow worker %d working... step %d/10\n", args->thread_id, i + 1);
        async_yield();
    }
    
    printf("Slow worker %d completed\n", args->thread_id);
    return args->expected_result;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing async_yield() from main thread...\n");
    
    printf("\n=== Test: Basic main thread yield ===\n");
    
    main_yield_args args[5];
    CGNThreadHandle handles[5];
    
    args[0] = (main_yield_args){0, 2, 100};
    handles[0] = async_run(quick_worker_func, &args[0]);
    
    args[1] = (main_yield_args){1, 5, 200};
    handles[1] = async_run(worker_func, &args[1]);
    
    args[2] = (main_yield_args){2, 10, 300};
    handles[2] = async_run(slow_worker_func, &args[2]);
    
    args[3] = (main_yield_args){3, 2, 400};
    handles[3] = async_run(quick_worker_func, &args[3]);
    
    args[4] = (main_yield_args){4, 5, 500};
    handles[4] = async_run(worker_func, &args[4]);
    
    printf("All threads started, main thread yielding...\n");
    
    for (int i = 0; i < 8; i++) {
        printf("Main thread yielding... (%d/8)\n", i + 1);
        main_thread_yield_count++;
        async_yield();
        main_thread_execution_count++;
    }
    
    printf("Main thread finished yielding, now awaiting threads...\n");
    
    uint64_t results[5];
    for (int i = 0; i < 5; i++) {
        results[i] = await(handles[i]);
        printf("Thread %d result: %llu (expected: %d)\n", i, results[i], args[i].expected_result);
        assert(results[i] == (uint64_t)args[i].expected_result);
    }
    
    printf("Main thread yield count: %d\n", main_thread_yield_count);
    printf("Main thread execution count: %d\n", main_thread_execution_count);
    
    seagreen_free_rt();
    
    return 0;
}
