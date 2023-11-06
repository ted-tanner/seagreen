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

// TODO: This needs extensive testing. EVERY BRANCH needs to be validated
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
    size_t prev_pos = 0;
    // Treat the unused_threads int as an array of bits and find the index
    // of the most significant bit
    for (; !((block->unused_threads << pos) & (1ULL << 63)); ++pos) {
	prev_pos = pos;
    }

    // Mark thread as used
    block->unused_threads &= ~(1ULL << (63 - pos));
    __CGNThread *thread = &block->threads[pos];

    thread->data = data;
    thread->data_size = data_size;
    thread->pos = 64 * block_pos + pos;

    // Still need to set thread->next and insert thread into linked list (by setting
    // thread->next AND modifying the previous thread's next pointer)
    if (prev_pos == pos) {
	// Previous thread is not found in this block, look to previous block
	// (because of previous while loop, we know that there is a used thread in
	// the previous block, if a previous block exists)
	__CGNThreadBlock *prev_block = block->prev;
	if (prev_block) {
	    size_t prev_pos_from_right = 0;
	    for (; prev_block->unused_threads & (1ULL << prev_pos_from_right); ++prev_pos_from_right);
	    prev_pos = 63 - prev_pos_from_right;

	    thread->next = prev_block->threads[prev_pos].next;
	    prev_block->threads[prev_pos].next = thread;
	} else {
	    // No previous block exists, must search forward (and no need to set the next
	    // pointer on a previous thread)
	    size_t next_pos = pos + 1;
	    for (; next_pos < 64 && ((block->unused_threads << next_pos) & (1ULL << 63)); ++next_pos);

	    if (next_pos == 64) {
		// Not found in this block, look to next block until found or no more blocks
		__CGNThreadBlock *next_block = block->next;
		for (; next_pos == 64 && next_block; next_block = next_block->next, next_pos = 0) {
		    for (; next_pos < 64 && ((next_block->unused_threads << next_pos) & (1ULL << 63)); ++next_pos);
		}

		if (next_pos == 64) {
		    // Reached end of list, no threads found.
		    thread->next = 0;
		} else {
		    thread->next = &next_block->threads[next_pos];
		}
	    } else {
		thread->next = &block->threads[next_pos];
	    }
	}
    } else {
	thread->next = block->threads[prev_pos].next;
	block->threads[prev_pos].next = thread;
    }

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

    // TODO: Remove thread from linked thread list. Should be able to copy some code from
    //       add_thread, but no need to search forward for next thread. We just need to find
    //       the previous thread
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
