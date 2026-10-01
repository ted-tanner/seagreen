#define TEST_FAILURE "scheduler-protection"
#define TEST_DIAGNOSTIC "scheduler stack protection failed"
#include "support/failure-paths.h"

static void run_failure(void) {
    seagreen_init_rt();
    seagreen_free_rt();
}
