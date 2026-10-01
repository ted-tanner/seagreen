#define TEST_FAILURE "out-of-range-handle"
#define TEST_DIAGNOSTIC "invalid or expired thread handle"
#include "support/failure-paths.h"

static void run_failure(void) {
    seagreen_init_rt();
    await((UINT64_C(1) << 32) | UINT32_MAX);
    seagreen_free_rt();
}
