#define TEST_FAILURE "stack-protection"
#define TEST_DIAGNOSTIC "stack protection failed"
#include "support/failure-paths.h"

static void run_failure(void) {
    seagreen_init_rt();
    seagreen_free_rt();
}
