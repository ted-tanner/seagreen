#define TEST_FAILURE "zero-generation"
#define TEST_DIAGNOSTIC "invalid or expired thread handle"
#include "support/failure-paths.h"

static uint64_t worker(void *arg) {
    (void)arg;
    return 42;
}

static void run_failure(void) {
    seagreen_init_rt();
    CGNThreadHandle handle = async_run(worker, NULL);
    await((uint32_t)handle);
    seagreen_free_rt();
}
