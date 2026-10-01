#define TEST_FAILURE "scheduler-allocation"
#if defined(_WIN32)
#define TEST_DIAGNOSTIC "scheduler stack allocation failed"
#else
#define TEST_DIAGNOSTIC "scheduler stack mapping failed"
#endif
#include "support/failure-paths.h"

static void run_failure(void) {
    seagreen_init_rt();
    seagreen_free_rt();
}
