#include "seagreen.h"

#include <stdatomic.h>
#include <stdio.h>

__CGNThreadList __cgn_threadl = {0};
_Thread_local __CGNThread *__cgn_curr_thread = 0;

static __CGNThreadBlock *add_block(void) {
    __CGNThreadBlock *block = (__CGNThreadBlock *) malloc(sizeof(__CGNThreadBlock));
    __cgn_check_malloc(block);

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

void seagreen_init_rt(void) {
    if (__cgn_threadl.head) {
	// No need to initialize if already initialized
	// TODO: Perhaps this should return an error code
	return;
    }

    __CGNThreadBlock *block = malloc(sizeof(__CGNThreadBlock));
    __cgn_check_malloc(block);

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

void seagreen_free_rt(void) {
    __CGNThreadBlock *block = __cgn_threadl.head;

    while (block) {
	__CGNThreadBlock *next = block->next;
	free(block);
	block = next;
    }

    __cgn_threadl.head = 0;
}

__cgn_define_cgn_scheduler(void);
__cgn_define_cgn_scheduler(char);
__cgn_define_cgn_scheduler(__cgn_signedchar);
__cgn_define_cgn_scheduler(__cgn_unsignedchar);
__cgn_define_cgn_scheduler(short);
__cgn_define_cgn_scheduler(__cgn_unsignedshort);
__cgn_define_cgn_scheduler(int);
__cgn_define_cgn_scheduler(__cgn_unsignedint);
__cgn_define_cgn_scheduler(long);
__cgn_define_cgn_scheduler(__cgn_unsignedlong);
__cgn_define_cgn_scheduler(__cgn_longlong);
__cgn_define_cgn_scheduler(__cgn_unsignedlonglong);
__cgn_define_cgn_scheduler(float);
__cgn_define_cgn_scheduler(double);
__cgn_define_cgn_scheduler(__cgn_longdouble);
__cgn_define_cgn_scheduler(__cgn_voidptr);

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
