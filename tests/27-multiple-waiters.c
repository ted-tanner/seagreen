#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <inttypes.h>
#include "seagreen.h"
#include <stdio.h>
static CGNThreadHandle target, replacement;
static unsigned consumers;
static uint64_t child(void *p) { (void)p; return 42; }
static uint64_t waiter(void *p) {
    (void)p;
    uint64_t result = await(target);
    assert(result == 42);
    if (++consumers == 2) {
        // No other slots have been reclaimed yet: this reuses the target.
        replacement = async_run(child, 0);
        assert((uint32_t)replacement == (uint32_t)target);
        assert((replacement >> 32) == (target >> 32) + 1);
    }
    return result;
}
static uint64_t spinner(void *p) {
    (void)p;
    for (unsigned i = 0; i < 100; ++i) async_yield();
    return 1;
}
int main(void) {
    seagreen_init_rt();
    CGNThreadHandle w1 = async_run(waiter, 0), w2 = async_run(waiter, 0);
    CGNThreadHandle spin[32];
    for (unsigned i = 0; i < 32; ++i) spin[i] = async_run(spinner, 0);
    target = async_run(child, 0);
    uint64_t a = await(w1), b = await(w2);
    printf("waiter results: %" PRIu64 ", %" PRIu64 "; expected 42, 42\n", a, b);
    assert(a == 42 && b == 42);
    // Reuse the reclaimed target slot and verify the replacement still runs.
    assert(consumers == 2);
    assert(await(replacement) == 42);
    for (unsigned i = 0; i < 32; ++i) assert(await(spin[i]) == 1);
    seagreen_free_rt();
    return a != 42 || b != 42;
}
