#ifndef SEAGREEN_H

#if !defined(__x86_64__) && !defined(__aarch64__) && !defined(__riscv__)
#error "seagreenlib does not support this architecture"
#endif


#if !defined __GNUC__ && !defined __clang__
#warning "seagreenlib only supports the gcc and clang compilers"
#endif

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

#define __CGN_EXPORT __attribute__((visibility("default")))

#if defined(__x86_64__) && (defined(__unix__) || defined(__APPLE__))

typedef struct __CGNThreadCtx_ {
    uint64_t rbp;
    uint64_t rsp;

    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rbx;
} __CGNThreadCtx;

#elif defined(__x86_64__) && defined(_WIN64)

typedef struct __CGNThreadCtx_ {
    uint64_t rbp;
    uint64_t rsp;

    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rbx;
    uint64_t rdi;
} __CGNThreadCtx;

#elif defined(__aarch64__)

typedef struct __CGNThreadCtx_ {
    uint64_t lr;
    uint64_t sp;

    uint64_t x19;
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t x29;
} __CGNThreadCtx;

#elif defined(__riscv__)

typedef struct __CGNThreadCtx_ {
    uint64_t ra;
    uint64_t sp;

    uint64_t s0;
    uint64_t s1;
    uint64_t s2;
    uint64_t s3;
    uint64_t s4;
    uint64_t s5;
    uint64_t s6;
    uint64_t s7;
    uint64_t s8;
    uint64_t s9;
    uint64_t s10;
    uint64_t s11;
} __CGNThreadCtx;

#endif

typedef enum __attribute__ ((__packed__)) __CGNThreadState_ {
    __CGN_THREAD_STATE_READY,
    __CGN_THREAD_STATE_RUNNING,
    __CGN_THREAD_STATE_WAITING,
    __CGN_THREAD_STATE_DONE,
} __CGNThreadState;

#define __CGN_THREAD_BLOCK_SIZE 256

// Careful alignment of these struct fields to avoid padding is important
// for performance here.
typedef struct __CGNThread_ {
    uint32_t id;

    __CGNThreadState state;
    _Bool yield_toggle;
    _Bool disable_yield;
    _Bool in_use;

    uint32_t awaited_thread_id;
    uint32_t awaiting_thread_count;

    __CGNThreadCtx ctx;

    uint64_t return_val;
} __CGNThread;

typedef struct __CGNThreadBlock_ {
    struct __CGNThreadBlock_ *next;
    struct __CGNThreadBlock_ *prev;

    __CGNThread threads[__CGN_THREAD_BLOCK_SIZE];
    void *stacks;

    uint16_t used_thread_count;
} __CGNThreadBlock;

typedef struct __CGNThreadList_ {
    __CGNThreadBlock *head;
    __CGNThreadBlock *tail;

    uint32_t block_count;
    uint32_t thread_count;
} __CGNThreadList;

// Two of these functions return the thread pointer such that it is available when
// the thread is resumed and the stack may have changed.
extern __CGN_EXPORT __attribute__((noinline, noreturn)) void __cgn_loadctx(__CGNThreadCtx *ctx, __CGNThread *t);
extern __CGN_EXPORT __attribute__((noinline, returns_twice)) void __cgn_savectx(__CGNThreadCtx *ctx);
extern __CGN_EXPORT __attribute__((noinline, returns_twice)) __CGNThread *__cgn_savenewctx(__CGNThreadCtx *ctx, void *stack, __CGNThread *t);

// mmap returns -1 on error, check ptr < 1 rather than !ptr
#define __cgn_check_malloc(ptr)                         \
    if ((void *)ptr < (void *)1) {                      \
        fprintf(stderr, "memory allocation failure\n"); \
        abort();                                        \
    }

typedef uint64_t CGNThreadHandle;

// The stack space allocated for each thread. Many of these pages will remain
// untouched
#define SEAGREEN_MAX_STACK_SIZE 1024 * 1024 * 2 // 2 MB

__CGN_EXPORT void seagreen_init_rt(void);
__CGN_EXPORT void seagreen_free_rt(void);
__CGN_EXPORT void async_yield(void);
__CGN_EXPORT uint64_t await(CGNThreadHandle handle);

__CGN_EXPORT void __cgn_scheduler(void);

__CGN_EXPORT __CGNThreadBlock *__cgn_get_block(uint32_t id);
__CGN_EXPORT __CGNThread *__cgn_get_thread(uint32_t id);
__CGN_EXPORT __CGNThread *__cgn_get_thread_by_block(__CGNThreadBlock *block, uint32_t pos);
__CGN_EXPORT __CGNThread *__cgn_add_thread(void **stack);
__CGN_EXPORT void __cgn_remove_thread(__CGNThreadBlock *block, uint32_t pos);

#define async __attribute__((noinline))

extern _Thread_local __CGNThread *__cgn_curr_thread;
extern _Thread_local __CGNThread *__cgn_sched_thread;

#define async_run(Fn) ({                                \
            void *stack;                                \
            __CGNThread *t = __cgn_add_thread(&stack);  \
                                                        \
            t = __cgn_savenewctx(&t->ctx, stack, t);    \
                                                        \
            if (t == __cgn_curr_thread) {               \
                t->return_val = (uint64_t) Fn;          \
                t->state = __CGN_THREAD_STATE_DONE;     \
                __cgn_curr_thread = __cgn_sched_thread; \
                __cgn_sched_thread->yield_toggle = 1;   \
                __asm__ __volatile__("" ::: "memory");  \
                __cgn_loadctx(&__cgn_sched_thread->ctx, __cgn_sched_thread); \
                /* This should never be reached */      \
                abort();                                \
            }                                           \
                                                        \
            (CGNThreadHandle) t->id;                    \
        })

#define run_as_sync(Fn)                         \
    ({                                          \
        __cgn_curr_thread->disable_yield = 1;   \
        uint64_t retval = Fn;                   \
        __cgn_curr_thread->disable_yield = 0;   \
        retval;                                 \
    })

#define SEAGREEN_H
#endif
