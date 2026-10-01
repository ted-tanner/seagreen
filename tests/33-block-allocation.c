#define TEST_FAILURE "block-allocation"
#define TEST_DIAGNOSTIC "thread block allocation failed"
#include "support/failure-paths.h"

static void run_failure(void) {
    seagreen_init_rt();
    seagreen_free_rt();
}
