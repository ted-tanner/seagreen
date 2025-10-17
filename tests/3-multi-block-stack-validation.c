#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "seagreen.h"

// Create enough threads to span multiple thread blocks
#define THREAD_COUNT __CGN_THREAD_BLOCK_SIZE * 4
#define STACK_MARKER_SIZE 1024
#define STACK_MARKER_VALUE 0xDEADBEEF

typedef struct {
    int thread_id;
    uint32_t stack_marker[STACK_MARKER_SIZE];
    int yield_count;
    int expected_yields;
} thread_data;

async uint64_t stack_validation_thread(void *p) {
    thread_data *data = (thread_data *)p;
    int thread_id = data->thread_id;

    // Fill the stack with a unique marker pattern
    for (int i = 0; i < STACK_MARKER_SIZE; i++) {
        data->stack_marker[i] = STACK_MARKER_VALUE + thread_id;
    }

    // Perform multiple yields to allow other threads to run and potentially corrupt our stack
    for (int i = 0; i < data->expected_yields; i++) {
        async_yield();
        data->yield_count++;

        // Validate our stack marker is still intact after each yield
        for (int j = 0; j < STACK_MARKER_SIZE; j++) {
            if (data->stack_marker[j] != (STACK_MARKER_VALUE + thread_id)) {
                printf("ERROR: Thread %d stack corruption detected at position %d after %d yields\n", 
                       thread_id, j, i + 1);
                printf("Expected: 0x%08X, Got: 0x%08X\n", 
                       STACK_MARKER_VALUE + thread_id, data->stack_marker[j]);
                return 1; // Return error code
            }
        }
    }

    return 0; // Success
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();

    printf("Creating %d threads to span multiple thread blocks...\n", THREAD_COUNT);

    thread_data *thread_data_array = malloc(THREAD_COUNT * sizeof(thread_data));
    CGNThreadHandle *handles = malloc(THREAD_COUNT * sizeof(CGNThreadHandle));
    
    if (!thread_data_array || !handles) {
        printf("Failed to allocate memory for thread arrays\n");
        seagreen_free_rt();
        return 1;
    }

    // Initialize thread data
    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_data_array[i].thread_id = i;
        thread_data_array[i].yield_count = 0;
        thread_data_array[i].expected_yields = 5 + (i % 10); // Vary yield counts
        memset(thread_data_array[i].stack_marker, 0, sizeof(thread_data_array[i].stack_marker));
    }

    // Start all threads
    for (int i = 0; i < THREAD_COUNT; i++) {
        handles[i] = async_run(stack_validation_thread, &thread_data_array[i]);
    }

    printf("All threads started, awaiting completion...\n");

    // Wait for all threads to complete and check results
    int error_count = 0;
    for (int i = 0; i < THREAD_COUNT; i++) {
        uint64_t result = await(handles[i]);
        if (result != 0) {
            error_count++;
        }

        // Final validation of stack markers
        for (int j = 0; j < STACK_MARKER_SIZE; j++) {
            if (thread_data_array[i].stack_marker[j] != (STACK_MARKER_VALUE + i)) {
                printf("ERROR: Thread %d final stack corruption detected at position %d\n", i, j);
                printf("Expected: 0x%08X, Got: 0x%08X\n", 
                       STACK_MARKER_VALUE + i, thread_data_array[i].stack_marker[j]);
                error_count++;
                break;
            }
        }

        // Verify yield count matches expected
        if (thread_data_array[i].yield_count != thread_data_array[i].expected_yields) {
            printf("ERROR: Thread %d yield count mismatch. Expected: %d, Got: %d\n",
                   i, thread_data_array[i].expected_yields, thread_data_array[i].yield_count);
            error_count++;
        }
    }

    printf("Test completed. Errors detected: %d\n", error_count);

    seagreen_free_rt();

    free(thread_data_array);
    free(handles);

    assert(error_count == 0);
    return 0;
}
