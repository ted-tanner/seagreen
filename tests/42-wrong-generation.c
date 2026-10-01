#define TEST_FAILURE "wrong-generation"
#define TEST_DIAGNOSTIC "invalid or expired thread handle"
#include "support/failure-paths.h"

static uint64_t worker(void *arg) {
    (void)arg;
    return 42;
}

static void run_failure(void) {
    seagreen_init_rt();
    CGNThreadHandle handle = async_run(worker, NULL);
    await(handle + (UINT64_C(1) << 32));
    seagreen_free_rt();
}
