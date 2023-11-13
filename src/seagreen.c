#include "seagreen.h"

#include <stdint.h>
#include <stdio.h>

_Thread_local __CGNThreadList __cgn_threadl = {0};

_Thread_local __CGNThread *__cgn_curr_thread = 0;

_Thread_local __CGNThreadBlock *__cgn_sched_block = 0;
_Thread_local uint64_t __cgn_sched_block_pos = 0;
_Thread_local uint64_t __cgn_sched_thread_pos = 0;

static __CGNThreadBlock *add_block(void) {
    __CGNThreadBlock *block = (__CGNThreadBlock *)malloc(sizeof(__CGNThreadBlock));
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

    uint64_t _pos;
    __CGNThread *thread = __cgn_add_thread(&_pos);
    thread->state = __CGN_THREAD_STATE_RUNNING;
    __cgn_curr_thread = thread;

    __cgn_sched_block = block;
}

void seagreen_free_rt(void) {
    __CGNThreadBlock *block = __cgn_threadl.head;

    while (block) {
        __CGNThreadBlock *next = block->next;
        free(block);
        block = next;
    }

    __cgn_threadl.head = 0;

    __cgn_curr_thread = 0;

    __cgn_sched_block = 0;
    __cgn_sched_block_pos = 0;
    __cgn_sched_thread_pos = 0;
}

__attribute__((noreturn)) void __cgn_scheduler(void) {
    while (1) {
        for (; __cgn_sched_block; __cgn_sched_block = __cgn_sched_block->next, ++__cgn_sched_block_pos) {
            if (__cgn_sched_block->unused_threads == UINT64_MAX) {
                continue;
            }

            for (; __cgn_sched_thread_pos < 64; ++__cgn_sched_thread_pos) {
                _Bool is_thread_unused =
                    (__cgn_sched_block->unused_threads << __cgn_sched_thread_pos) & (1ULL << 63);

                if (!is_thread_unused) {
                    __CGNThread *staged_thread = &__cgn_sched_block->threads[__cgn_sched_thread_pos];

                    if (staged_thread == __cgn_curr_thread) {
                        continue;
                    }

                    if ((staged_thread->state == __CGN_THREAD_STATE_READY) ||
                        (staged_thread->state == __CGN_THREAD_STATE_WAITING &&
                         __cgn_get_thread(staged_thread->awaited_thread_pos)->state == __CGN_THREAD_STATE_DONE))
                    {
                        __CGNThread *running_thread = __cgn_curr_thread;
                        __cgn_curr_thread = staged_thread;

                        running_thread->state = __CGN_THREAD_STATE_READY;
                        staged_thread->state = __CGN_THREAD_STATE_RUNNING;

                        __cgn_ctxswitch(&running_thread->ctx, &staged_thread->ctx);
                    }
                }
            }

            __cgn_sched_thread_pos = 0;
        }

        __cgn_sched_block = __cgn_threadl.head;
        __cgn_sched_block_pos = 0;
    }
}

__CGNThreadBlock *__cgn_get_block(uint64_t pos) {
    __CGNThreadBlock *block;

    uint64_t block_pos = pos / 64;
    if (block_pos > __cgn_threadl.block_count / 2) {
        block = __cgn_threadl.tail;
        for (uint64_t i = __cgn_threadl.block_count - 1; i > block_pos; --i, block = block->prev);
    } else {
        block = __cgn_threadl.head;
        for (uint64_t i = 0; i < block_pos; ++i, block = block->next);
    }

    return block;
}

__CGNThread *__cgn_get_thread(uint64_t pos) {
    __CGNThreadBlock *block = __cgn_get_block(pos);
    return &block->threads[pos % 64];
}

__CGNThread *__cgn_add_thread(uint64_t *placement_pos) {
    __CGNThreadBlock *block = __cgn_threadl.tail;

    uint64_t block_pos = __cgn_threadl.block_count - 1;
    while (!block->unused_threads) {
        if (!block->prev) {
            block = add_block();
            break;
        } else {
            block = block->prev;
            --block_pos;
        }
    }

    uint64_t pos = 0;
    // Treat the unused_threads int as an array of bits and find the index
    // of the most significant bit
    for (; !((block->unused_threads << pos) & (1ULL << 63)); ++pos);

    // Mark thread as used
    block->unused_threads &= ~(1ULL << (63 - pos));
    block->threads[pos].state = __CGN_THREAD_STATE_READY;

    ++__cgn_threadl.thread_count;

    *placement_pos = 64 * block_pos + pos;

    return &block->threads[pos];
}

void __cgn_remove_thread(__CGNThreadBlock *block, uint64_t pos) {
    block->unused_threads |= 1ULL << (63 - (pos % 64));
    --__cgn_threadl.thread_count;
}

inline __CGNThread *__cgn_get_curr_thread(void) {
    return __cgn_curr_thread;
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
