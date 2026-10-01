#define TEST_FAILURE "cycle"
#define TEST_DIAGNOSTIC "no runnable threads (deadlock or invalid scheduler state)"
#include "support/failure-paths.h"

static CGNThreadHandle handles[2];
static uint64_t wait_for_other(void *arg) {
    return await(handles[*(unsigned *)arg]);
}

static void run_failure(void) {
    seagreen_init_rt();
    unsigned indices[2] = {1, 0};
    handles[0] = async_run(wait_for_other, &indices[0]);
    handles[1] = async_run(wait_for_other, &indices[1]);
    await(handles[0]);
    seagreen_free_rt();
}
