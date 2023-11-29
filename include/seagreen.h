#ifndef SEAGREEN_H

#if !defined __GNUC__ && !defined __clang__
#warning "seagreenlib only officially supports the GCC and Clang compilers"
#endif

#include "cgninternals/cgncoroutine.h"
#include "cgninternals/cgntypes.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/mman.h>
#include <unistd.h>
#elif defined(_WIN64)
#include <memoryapi.h>
#include <sysinfoapi.h>
#else
#warning "seagreenlib only supports Unix-like and Windows 64-bit systems"
#include <sys/mman.h>
#include <unistd.h>
#endif

#ifdef CGN_DEBUG
void print_threads(void);
#endif

// mmap returns -1 on error, check ptr < 1 rather than !ptr
#define __cgn_check_malloc(ptr)                         \
    if ((void *)ptr < (void *)1) {                      \
        fprintf(stderr, "memory allocation failure\n"); \
        abort();                                        \
    }

// The stack space allocated for each thread. Many of these pages will remain
// untouched
#define SEAGREEN_MAX_STACK_SIZE 1024 * 1024 * 2 // 2 MB

void seagreen_init_rt(void);
void seagreen_free_rt(void);
void async_yield(void);

void __cgn_scheduler(void);

__CGNThreadBlock *__cgn_get_block(uint32_t id);
__CGNThread *__cgn_get_thread(uint32_t id);
__CGNThread *__cgn_get_thread_by_block(__CGNThreadBlock *block, uint32_t pos);
__CGNThread *__cgn_add_thread(void **stack);
void __cgn_remove_thread(__CGNThreadBlock *block, uint32_t pos);

#define async __attribute__((noinline))

extern _Thread_local int __cgn_pagesize;

extern _Thread_local __CGNThreadList __cgn_threadlist;

extern _Thread_local __CGNThread *__cgn_curr_thread;
extern _Thread_local __CGNThread *__cgn_main_thread;

extern _Thread_local __CGNThreadBlock *__cgn_sched_block;
extern _Thread_local uint32_t __cgn_sched_block_pos;
extern _Thread_local uint32_t __cgn_sched_thread_pos;

#define await(handle)                                                   \
    _Generic((handle),                                                  \
             CGNThreadHandle_void: ({                                   \
                     __cgn_curr_thread->awaited_thread_id = (handle).id; \
                     __cgn_curr_thread->state = __CGN_THREAD_STATE_WAITING; \
                                                                        \
                     uint32_t pos = (handle).id % __CGN_THREAD_BLOCK_SIZE; \
                     __CGNThreadBlock *block = __cgn_get_block((handle).id); \
                                                                        \
                     __CGNThread *t = &block->threads[pos];             \
                     if (t->in_use) {                                   \
                         t->awaiting_thread_count++;                    \
                                                                        \
                         /* Because thread is waiting, the curr thread */ \
                         /* won't be scheduled until awaited thread has */ \
                         /* finished its execution */                   \
                         async_yield();                                 \
                                                                        \
                         if (!t->awaiting_thread_count) {               \
                             __cgn_remove_thread(block, pos);           \
                         }                                              \
                     }                                                  \
                                                                        \
                     (void)0;                                           \
                 }),                                                    \
             default: ({                                                \
                     __cgn_curr_thread->awaited_thread_id = (handle).id; \
                     __cgn_curr_thread->state = __CGN_THREAD_STATE_WAITING; \
                                                                        \
                     uint32_t pos = (handle).id % __CGN_THREAD_BLOCK_SIZE; \
                     __CGNThreadBlock *block = __cgn_get_block((handle).id); \
                                                                        \
                     __CGNThread *t = &block->threads[pos];             \
                     uint64_t return_val = 0;                           \
                     if (t->in_use) {                                   \
                         t->awaiting_thread_count++;                    \
                                                                        \
                         /* Because thread is waiting, the curr thread */ \
                         /* won't be scheduled until awaited thread has */ \
                         /* finished its execution */                   \
                         async_yield();                                 \
                                                                        \
                         return_val = t->return_val;   \
                         if (!t->awaiting_thread_count) {               \
                             __cgn_remove_thread(block, pos);           \
                         }                                              \
                     }                                                  \
                                                                        \
                     return_val;                                        \
                 }))

#define async_run(Fn)                                                   \
    _Generic(                                                           \
        (Fn),                                                           \
        char: (void)0,                                                  \
        signed char: (void)0,                                           \
        unsigned char: (void)0,                                         \
        short: (void)0,                                                 \
        unsigned short: (void)0,                                        \
        int: ({                                                         \
                void *stack;                                            \
                __CGNThread *t = __cgn_add_thread(&stack);              \
                                                                        \
                __cgn_savenewctx(&t->ctx, stack);                       \
                                                                        \
                if (t == __cgn_curr_thread) {                           \
                    t->return_val = (uint64_t) Fn;                      \
                    t->state = __CGN_THREAD_STATE_DONE;                 \
                    __cgn_scheduler();                                  \
                    /* This should never be reached */                  \
                    assert(0);                                          \
                }                                                       \
                                                                        \
                (CGNThreadHandle_int) {                                 \
                    .id = t->id,                                        \
                };                                                      \
            }),                                                         \
        unsigned int: (void)0,                                          \
        long: (void)0,                                                  \
        unsigned long: (void)0,                                         \
        long long: (void)0,                                             \
        unsigned long long: (void)0,                                    \
        float: (void)0,                                                 \
        double: (void)0,                                                \
        void *: (void)0,                                                \
        default: (void)0)

#define SEAGREEN_H
#endif
