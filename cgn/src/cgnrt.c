#include "cgnrt.h"

#include <stdio.h>
#include <sys/_types/_size_t.h>

__CGNThreadList __cgn_threadl = {0};

static __CGNThreadBlock *add_block(void) {
    __CGNThreadBlock *block = malloc(sizeof(__CGNThreadBlock));
    __CGN_CHECK_MALLOC(block);

    block->next = 0;
    block->prev = 0;
    block->unused_threads = UINT64_MAX;

    __cgn_threadl.tail->next = block;
    block->prev = __cgn_threadl.tail;
    __cgn_threadl.tail = block;

    ++__cgn_threadl.block_count;

    return block;
}

static void add_thread(void *data, size_t data_size) {
    __CGNThreadBlock *block = __cgn_threadl.tail;

    size_t block_pos = __cgn_threadl.block_count - 1;
    while (!block->unused_threads) {
	if (!block->prev) {
	    block = add_block();
	    break;
	} else {
	    block = block->prev;
	    --block_pos;
	}
    }

    size_t pos = 0;
    // Treat the unused_threads int as an array of bits and find the index
    // of the most significant bit
    for (; !((block->unused_threads << pos) & (1ULL << 63)); ++pos);

    // TODO: Need to find the next thread

    // Mark thread as used
    block->unused_threads &= ~(1ULL << (63 - pos));

    block->threads[pos].data = data;
    block->threads[pos].data_size = data_size;
    block->threads[pos].pos = 64 * block_pos + pos;

    // TODO: Insert thread into linked thread list

    ++__cgn_threadl.thread_count;
}

static void remove_thread(size_t pos) {
    __CGNThreadBlock *block;

    size_t block_pos = pos / 64;

    if (block_pos > __cgn_threadl.block_count / 2) {
	block = __cgn_threadl.tail;
	for (size_t i = __cgn_threadl.block_count - 1; i > block_pos; --i, block = block->prev);
    } else {
	block = __cgn_threadl.head;
	for (size_t i = 0; i < block_pos; ++i, block = block->next);
    }

    block->unused_threads |= 1ULL << (63 - (pos % 64));
    --__cgn_threadl.thread_count;

    // TODO: Remove thread from linked thread list
}

void cgn_init_rt(void) {
    if (__cgn_threadl.head) {
	// No need to initialize if already initialized
	// TODO: Perhaps this should return an error code
	return;
    }

    __CGNThreadBlock *block = malloc(sizeof(__CGNThreadBlock));
    __CGN_CHECK_MALLOC(block);

    block->next = 0;
    block->prev = 0;
    block->unused_threads = UINT64_MAX;

    __cgn_threadl.head = block;
    __cgn_threadl.tail = block;
    __cgn_threadl.block_count = 1;
    __cgn_threadl.thread_count = 0;
}


#ifdef CGN_TEST

#include "cgntest/test.h"

static TEST_RESULT test_add_thread(void) {
    // TODO
    cgn_init_rt();
    add_thread(0, 0);
    return TEST_PASS;
}

static TEST_RESULT test_remove_thread(void) {
    // TODO
    cgn_init_rt();
    remove_thread(0);
    return TEST_PASS;
}

void register_cgnrt_tests() {
    CgnTestSet *set = new_test_set(__FILE__);

    register_test(set, test_add_thread);
    register_test(set, test_remove_thread);
}

#endif
