#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "seagreen.h"

// Global counter to track main thread execution
static volatile int main_thread_yield_count = 0;
static volatile int main_thread_execution_count = 0;

// Test data structure
typedef struct {
    int thread_id;
    int work_duration; // Number of yields before completion
    int expected_result;
} main_yield_args;

// Async worker function that does some work and yields
async uint64_t worker_func(void *p) {
    main_yield_args *args = (main_yield_args *)p;
    
    printf("Worker %d starting (work duration: %d)\n", args->thread_id, args->work_duration);
    
    // Do work by yielding multiple times
    for (int i = 0; i < args->work_duration; i++) {
        printf("Worker %d working... step %d/%d\n", args->thread_id, i + 1, args->work_duration);
        async_yield();
    }
    
    printf("Worker %d completed\n", args->thread_id);
    return args->expected_result;
}

// Quick worker that finishes fast
async uint64_t quick_worker_func(void *p) {
    main_yield_args *args = (main_yield_args *)p;
    
    printf("Quick worker %d starting\n", args->thread_id);
    
    // Do minimal work
    async_yield();
    async_yield();
    
    printf("Quick worker %d completed\n", args->thread_id);
    return args->expected_result;
}

// Slow worker that takes a long time
async uint64_t slow_worker_func(void *p) {
    main_yield_args *args = (main_yield_args *)p;
    
    printf("Slow worker %d starting\n", args->thread_id);
    
    // Do lots of work
    for (int i = 0; i < 10; i++) {
        printf("Slow worker %d working... step %d/10\n", args->thread_id, i + 1);
        async_yield();
    }
    
    printf("Slow worker %d completed\n", args->thread_id);
    return args->expected_result;
}

// Async function that calls other async functions recursively
async uint64_t recursive_worker_func(void *p) {
    main_yield_args *args = (main_yield_args *)p;
    
    printf("Recursive worker %d starting\n", args->thread_id);
    
    if (args->work_duration > 1) {
        // Create a child async function
        main_yield_args child_args = {args->thread_id + 100, args->work_duration - 1, args->expected_result + 1};
        CGNThreadHandle child_handle = async_run(recursive_worker_func, &child_args);
        uint64_t child_result = await(child_handle);
        
        async_yield();
        return child_result + args->expected_result;
    } else {
        async_yield();
        return args->expected_result;
    }
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing async_yield() from main thread...\n");
    
    // Test 1: Basic main thread yield with mixed work durations
    printf("\n=== Test 1: Basic main thread yield ===\n");
    
    // Create threads with different work durations
    main_yield_args args[5];
    CGNThreadHandle handles[5];
    
    // Thread 0: Quick worker (2 yields)
    args[0] = (main_yield_args){0, 2, 100};
    handles[0] = async_run(quick_worker_func, &args[0]);
    
    // Thread 1: Medium worker (5 yields)
    args[1] = (main_yield_args){1, 5, 200};
    handles[1] = async_run(worker_func, &args[1]);
    
    // Thread 2: Slow worker (10 yields)
    args[2] = (main_yield_args){2, 10, 300};
    handles[2] = async_run(slow_worker_func, &args[2]);
    
    // Thread 3: Another quick worker (2 yields)
    args[3] = (main_yield_args){3, 2, 400};
    handles[3] = async_run(quick_worker_func, &args[3]);
    
    // Thread 4: Medium worker (5 yields)
    args[4] = (main_yield_args){4, 5, 500};
    handles[4] = async_run(worker_func, &args[4]);
    
    printf("All threads started, main thread yielding...\n");
    
    // Main thread yields multiple times to let other threads run
    for (int i = 0; i < 8; i++) {
        printf("Main thread yielding... (%d/8)\n", i + 1);
        main_thread_yield_count++;
        async_yield();
        main_thread_execution_count++;
    }
    
    printf("Main thread finished yielding, now awaiting threads...\n");
    
    // Await all threads (some may already be done)
    uint64_t results[5];
    for (int i = 0; i < 5; i++) {
        results[i] = await(handles[i]);
        printf("Thread %d result: %llu (expected: %d)\n", i, results[i], args[i].expected_result);
        assert(results[i] == (uint64_t)args[i].expected_result);
    }
    
    printf("Main thread yield count: %d\n", main_thread_yield_count);
    printf("Main thread execution count: %d\n", main_thread_execution_count);
    
    // Test 2: Main thread yield with many threads
    printf("\n=== Test 2: Main thread yield with many threads ===\n");
    
    main_yield_args many_args[10];
    CGNThreadHandle many_handles[10];
    
    // Create 10 threads with varying work durations
    for (int i = 0; i < 10; i++) {
        many_args[i] = (main_yield_args){i, 3 + (i % 4), 1000 + i * 10};
        many_handles[i] = async_run(worker_func, &many_args[i]);
    }
    
    printf("10 threads started, main thread yielding extensively...\n");
    
    // Main thread yields many times
    printf("Main thread yielding once...");
    main_thread_yield_count++;
    async_yield();
    main_thread_execution_count++;
    
    printf("Main thread finished extensive yielding, awaiting threads...\n");
    
    // Await all threads
    uint64_t many_results[10];
    for (int i = 0; i < 10; i++) {
        many_results[i] = await(many_handles[i]);
        printf("Many threads %d result: %llu (expected: %d)\n", i, many_results[i], many_args[i].expected_result);
        assert(many_results[i] == (uint64_t)many_args[i].expected_result);
    }
    
    // Test 3: Main thread yield with immediate completion
    printf("\n=== Test 3: Main thread yield with immediate completion ===\n");
    
    // Create some threads that will complete very quickly
    main_yield_args quick_args[3];
    CGNThreadHandle quick_handles[3];
    
    for (int i = 0; i < 3; i++) {
        quick_args[i] = (main_yield_args){i, 1, 2000 + i * 100};
        quick_handles[i] = async_run(quick_worker_func, &quick_args[i]);
    }
    
    printf("Quick threads started, main thread yielding...\n");
    
    // Main thread yields a few times
    for (int i = 0; i < 5; i++) {
        printf("Main thread yielding... (%d/5)\n", i + 1);
        main_thread_yield_count++;
        async_yield();
        main_thread_execution_count++;
    }
    
    printf("Main thread finished yielding, awaiting quick threads...\n");
    
    // Await quick threads (they should already be done)
    uint64_t quick_results[3];
    for (int i = 0; i < 3; i++) {
        quick_results[i] = await(quick_handles[i]);
        printf("Quick thread %d result: %llu (expected: %d)\n", i, quick_results[i], quick_args[i].expected_result);
        assert(quick_results[i] == (uint64_t)quick_args[i].expected_result);
    }
    
    // Test 4: Main thread yield with mixed completion times
    printf("\n=== Test 4: Main thread yield with mixed completion times ===\n");
    
    // Create mix of quick and slow threads
    main_yield_args mixed_args[6];
    CGNThreadHandle mixed_handles[6];
    
    // Quick threads (will finish early)
    for (int i = 0; i < 3; i++) {
        mixed_args[i] = (main_yield_args){i, 1, 3000 + i * 10};
        mixed_handles[i] = async_run(quick_worker_func, &mixed_args[i]);
    }
    
    // Slow threads (will finish later)
    for (int i = 3; i < 6; i++) {
        mixed_args[i] = (main_yield_args){i, 8, 4000 + i * 10};
        mixed_handles[i] = async_run(worker_func, &mixed_args[i]);
    }
    
    printf("Mixed threads started, main thread yielding...\n");
    
    // Main thread yields moderately
    for (int i = 0; i < 6; i++) {
        printf("Main thread yielding... (%d/6)\n", i + 1);
        main_thread_yield_count++;
        async_yield();
        main_thread_execution_count++;
    }
    
    printf("Main thread finished yielding, awaiting mixed threads...\n");
    
    // Await all threads (quick ones should be done, slow ones may still be running)
    uint64_t mixed_results[6];
    for (int i = 0; i < 6; i++) {
        mixed_results[i] = await(mixed_handles[i]);
        printf("Mixed thread %d result: %llu (expected: %d)\n", i, mixed_results[i], mixed_args[i].expected_result);
        assert(mixed_results[i] == (uint64_t)mixed_args[i].expected_result);
    }
    
    // Test 5: Main thread yield with recursive async calls
    printf("\n=== Test 5: Main thread yield with recursive async calls ===\n");
    
    main_yield_args recursive_args[3];
    CGNThreadHandle recursive_handles[3];
    
    for (int i = 0; i < 3; i++) {
        recursive_args[i] = (main_yield_args){i, 3, 5000 + i * 100};
        recursive_handles[i] = async_run(recursive_worker_func, &recursive_args[i]);
    }
    
    printf("Recursive threads started, main thread yielding...\n");
    
    // Main thread yields while recursive threads run
    for (int i = 0; i < 10; i++) {
        printf("Main thread yielding... (%d/10)\n", i + 1);
        main_thread_yield_count++;
        async_yield();
        main_thread_execution_count++;
    }
    
    printf("Main thread finished yielding, awaiting recursive threads...\n");
    
    // Await recursive threads
    uint64_t recursive_results[3];
    for (int i = 0; i < 3; i++) {
        recursive_results[i] = await(recursive_handles[i]);
        printf("Recursive thread %d result: %llu\n", i, recursive_results[i]);
        assert(recursive_results[i] > 0);
    }

    seagreen_free_rt();
    
    return 0;
}
