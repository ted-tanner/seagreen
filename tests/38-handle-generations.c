#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include "seagreen.h"

static uint64_t worker(void *arg) {
    (void)arg;
    return 42;
}

int main(void) {
    seagreen_init_rt();
    CGNThreadHandle previous = 0;
    for (unsigned i = 0; i < 1000; ++i) {
        CGNThreadHandle handle = async_run(worker, NULL);
        if (i) {
            assert((uint32_t)handle == (uint32_t)previous);
            assert((handle >> 32) == (previous >> 32) + 1);
        } else {
            assert((handle >> 32) == 1);
        }
        // Completion alone does not discard the result.
        async_yield();
        assert(await(handle) == 42);
        previous = handle;
    }
    seagreen_free_rt();
    puts("Handle generations and completed results passed");
    return 0;
}
