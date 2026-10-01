#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include "../src/seagreen.c"

static uint64_t worker(void *arg) {
    (void)arg;
    return 42;
}

int main(void) {
    seagreen_init_rt();
    __CGNThreadBlock *block = __cgn_threadlist.head;
    // Simulate a block with only one reusable slot remaining.
    for (unsigned i = 0; i < __CGN_THREAD_IN_USE_CHUNK_COUNT; ++i) {
        block->retired_mask[i] = UINT64_MAX;
    }
    block->retired_mask[0] &= ~UINT64_C(3);
    block->threads[1].generation = UINT32_MAX - 1;
    CGNThreadHandle last = async_run(worker, NULL);
    assert((uint32_t)last == 1 && (last >> 32) == UINT32_MAX);
    assert(await(last) == 42);
    assert(block->threads[1].generation == UINT32_MAX);
    assert(!__cgn_block_slot_in_use(block, 1));
    assert(__cgn_block_find_free_slot(block) == -1);
    CGNThreadHandle next = async_run(worker, NULL);
    assert((uint32_t)next == __CGN_THREAD_BLOCK_SIZE);
    assert((next >> 32) == 1);
    assert(await(next) == 42);
    seagreen_free_rt();
    puts("Exhausted generations retire slots without wrapping");
    return 0;
}
