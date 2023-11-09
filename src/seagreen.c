#include "seagreen.h"

#include <stdatomic.h>
#include <stdio.h>

__CGNThreadList __cgn_threadl = {0};
_Thread_local __CGNThread *__cgn_curr_thread = 0;
_Thread_local _Bool reschedule = 0;

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

static __CGNThread *add_thread() {
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

    // Mark thread as used
    block->unused_threads &= ~(1ULL << (63 - pos));
    block->threads[pos].state = __CGN_THREAD_STATE_READY;

    ++__cgn_threadl.thread_count;

    return &block->threads[pos];
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

    __CGNThread *thread = add_thread();
    __cgn_current_thread = thread;
}

uint64_t __cgn_scheduler(volatile _Bool *reschedule) {
    getctx(&__cgn_curr_thread->ctx);

    if (*reschedule) {
	*reschedule = 0;
	for (__CGNThreadBlock *block = __cgn_threadl.head; block; block = block->next) {
	    if (!block->unused_threads) {
		continue;
	    }

	    for (size_t pos = 0; ((block->unused_threads << pos) & (1ULL << 63)); ++pos) {
		// NOTE: When multicore is implemented, need a lock here and recheck that the thread
		//       is still used
		__CGNThread *thread = block->threads + pos;
		if (thread->state == __CGN_THREAD_STATE_READY) {
		    thread->state = __CGN_THREAD_STATE_RUNNING;
		    ctxswitch(&curr_ctx, &thread->ctx);
		}
	    }
	}
    }
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

void register_seagreen_tests() {
    CgnTestSet *set = new_test_set(__FILE__);

    register_test(set, test_add_thread);
    register_test(set, test_remove_thread);
}

#endif
