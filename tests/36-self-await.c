#define TEST_FAILURE "self-await"
#define TEST_DIAGNOSTIC "cannot await the current thread"
#include "support/failure-paths.h"

static CGNThreadHandle handle;
static uint64_t wait_for_self(void *arg) {
    (void)arg;
    return await(handle);
}

static void run_failure(void) {
    seagreen_init_rt();
    handle = async_run(wait_for_self, NULL);
    await(handle);
    seagreen_free_rt();
}
