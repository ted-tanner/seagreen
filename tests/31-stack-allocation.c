#define TEST_FAILURE "stack-allocation"
#if defined(_WIN32)
#define TEST_DIAGNOSTIC "stack allocation failed"
#else
#define TEST_DIAGNOSTIC "stack mapping failed"
#endif
#include "support/failure-paths.h"

static void run_failure(void) {
    seagreen_init_rt();
    seagreen_free_rt();
}
