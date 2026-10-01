#define TEST_FAILURE "reused-slot-handle"
#define TEST_DIAGNOSTIC "invalid or expired thread handle"
#include "support/failure-paths.h"

static uint64_t worker(void *arg) {
    (void)arg;
    return 42;
}

static void run_failure(void) {
    seagreen_init_rt();
    CGNThreadHandle handle = async_run(worker, NULL);
    assert(await(handle) == 42);
    CGNThreadHandle replacement = async_run(worker, NULL);
    assert((uint32_t)replacement == (uint32_t)handle);
    assert(replacement != handle);
    await(handle);
    seagreen_free_rt();
}
