#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "seagreen.h"

typedef struct {
    int thread_id;
    int num_async_threads;
    int work_duration;
} pthread_test_args;

typedef struct {
    int value;
    int thread_id;
    int expected_result;
} async_worker_args;

async uint64_t async_worker_func(void *p) {
    async_worker_args *args = (async_worker_args *)p;
    
    printf("PThread %d: Async worker starting with value %d\n", args->thread_id, args->value);
    
    for (int i = 0; i < 3; i++) {
        printf("PThread %d: Async worker %d working... step %d/3\n", args->thread_id, args->value, i + 1);
        async_yield();
    }
    
    printf("PThread %d: Async worker %d completed\n", args->thread_id, args->value);
    return args->expected_result;
}

async uint64_t recursive_worker_func(void *p) {
    async_worker_args *args = (async_worker_args *)p;
    
    printf("PThread %d: Recursive worker starting with value %d\n", args->thread_id, args->value);
    
    if (args->value <= 1) {
        return args->value;
    }
    
    async_yield();
    
    async_worker_args left_args = {args->value - 1, args->thread_id, 0};
    async_worker_args right_args = {args->value - 2, args->thread_id, 0};
    
    CGNThreadHandle left_handle = async_run(recursive_worker_func, &left_args);
    CGNThreadHandle right_handle = async_run(recursive_worker_func, &right_args);
    
    uint64_t left_result = await(left_handle);
    uint64_t right_result = await(right_handle);
    
    printf("PThread %d: Recursive worker %d completed with result %llu\n", args->thread_id, args->value, left_result + right_result);
    
    return left_result + right_result;
}

void* pthread_worker(void* arg) {
    pthread_test_args *args = (pthread_test_args *)arg;
    
    printf("PThread %d: Starting pthread worker\n", args->thread_id);
    
    // Initialize the seagreen runtime in this pthread
    printf("PThread %d: Initializing seagreen runtime\n", args->thread_id);
    seagreen_init_rt();
    
    // Test 1: Basic async operations
    printf("PThread %d: Test 1 - Basic async operations\n", args->thread_id);
    
    CGNThreadHandle handles[args->num_async_threads];
    async_worker_args worker_args[args->num_async_threads];
    
    for (int i = 0; i < args->num_async_threads; i++) {
        worker_args[i] = (async_worker_args){i, args->thread_id, 100 + i * 10};
        handles[i] = async_run(async_worker_func, &worker_args[i]);
    }
    
    for (int i = 0; i < args->num_async_threads; i++) {
        uint64_t result = await(handles[i]);
        printf("PThread %d: Basic async worker %d result: %llu (expected: %d)\n", 
               args->thread_id, i, result, worker_args[i].expected_result);
        assert(result == (uint64_t)worker_args[i].expected_result);
    }
    
    // Test 2: Recursive async operations
    printf("PThread %d: Test 2 - Recursive async operations\n", args->thread_id);
    
    int fibonacci_tests[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    int num_fib_tests = sizeof(fibonacci_tests) / sizeof(fibonacci_tests[0]);
    
    for (int i = 0; i < num_fib_tests; i++) {
        int n = fibonacci_tests[i];
        uint64_t expected = (n <= 1) ? n : 0; // We'll calculate expected properly
        
        // Calculate expected Fibonacci value
        if (n <= 1) {
            expected = n;
        } else {
            uint64_t a = 0, b = 1;
            for (int j = 2; j <= n; j++) {
                uint64_t temp = a + b;
                a = b;
                b = temp;
            }
            expected = b;
        }
        
        async_worker_args fib_args = {n, args->thread_id, 0};
        CGNThreadHandle fib_handle = async_run(recursive_worker_func, &fib_args);
        uint64_t result = await(fib_handle);
        
        printf("PThread %d: Fibonacci(%d) = %llu (expected %llu)\n", args->thread_id, n, result, expected);
        assert(result == expected);
    }
    
    // Test 3: Concurrent operations with different work loads
    printf("PThread %d: Test 3 - Concurrent operations with different work loads\n", args->thread_id);
    
    CGNThreadHandle mixed_handles[5];
    async_worker_args mixed_args[5];
    
    for (int i = 0; i < 5; i++) {
        mixed_args[i] = (async_worker_args){i * 2, args->thread_id, 200 + i * 20};
        mixed_handles[i] = async_run(async_worker_func, &mixed_args[i]);
    }
    
    for (int i = 0; i < 5; i++) {
        uint64_t result = await(mixed_handles[i]);
        printf("PThread %d: Mixed worker %d result: %llu (expected: %d)\n", 
               args->thread_id, i, result, mixed_args[i].expected_result);
        assert(result == (uint64_t)mixed_args[i].expected_result);
    }
    
    // Test 4: Stack validation with many threads
    printf("PThread %d: Test 4 - Stack validation with many threads\n", args->thread_id);
    
    int stack_test_threads = 50;
    CGNThreadHandle stack_handles[stack_test_threads];
    async_worker_args stack_args[stack_test_threads];
    
    for (int i = 0; i < stack_test_threads; i++) {
        stack_args[i] = (async_worker_args){i, args->thread_id, 300 + i};
        stack_handles[i] = async_run(async_worker_func, &stack_args[i]);
    }
    
    for (int i = 0; i < stack_test_threads; i++) {
        uint64_t result = await(stack_handles[i]);
        printf("PThread %d: Stack test worker %d result: %llu (expected: %d)\n", 
               args->thread_id, i, result, stack_args[i].expected_result);
        assert(result == (uint64_t)stack_args[i].expected_result);
    }
    
    // Free the seagreen runtime in this pthread
    printf("PThread %d: Freeing seagreen runtime\n", args->thread_id);
    seagreen_free_rt();
    
    printf("PThread %d: Completed all tests successfully\n", args->thread_id);
    
    return NULL;
}

int main(void) {
    printf("Testing seagreen runtime across multiple pthreads...\n");
    
    const int num_pthreads = 4;
    pthread_t threads[num_pthreads];
    pthread_test_args args[num_pthreads];
    
    // Initialize arguments for each pthread
    for (int i = 0; i < num_pthreads; i++) {
        args[i] = (pthread_test_args){i, 10, 5}; // thread_id, num_async_threads, work_duration
    }
    
    printf("Creating %d pthreads...\n", num_pthreads);
    
    // Create pthreads
    for (int i = 0; i < num_pthreads; i++) {
        int result = pthread_create(&threads[i], NULL, pthread_worker, &args[i]);
        if (result != 0) {
            printf("Error creating pthread %d: %d\n", i, result);
            return 1;
        }
        printf("Created pthread %d\n", i);
    }
    
    printf("Waiting for all pthreads to complete...\n");
    
    // Wait for all pthreads to complete
    for (int i = 0; i < num_pthreads; i++) {
        int result = pthread_join(threads[i], NULL);
        if (result != 0) {
            printf("Error joining pthread %d: %d\n", i, result);
            return 1;
        }
        printf("PThread %d joined successfully\n", i);
    }
    
    return 0;
}
