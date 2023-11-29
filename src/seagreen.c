#include "seagreen.h"
#include <stdint.h>

_Thread_local int __cgn_pagesize = 0;

_Thread_local __CGNThreadList __cgn_threadlist = {0};

_Thread_local __CGNThread *__cgn_curr_thread = 0;
_Thread_local __CGNThread *__cgn_main_thread = 0;

_Thread_local __CGNThreadBlock *__cgn_sched_block = 0;
_Thread_local uint32_t __cgn_sched_block_pos = 0;
_Thread_local uint32_t __cgn_sched_thread_pos = 0;

#ifdef CGN_DEBUG

static char *state_to_name(__CGNThreadState state) {
    switch (state) {
    case __CGN_THREAD_STATE_READY:
        return "ready";
    case __CGN_THREAD_STATE_RUNNING:
        return "running";
    case __CGN_THREAD_STATE_WAITING:
        return "waiting";
    case __CGN_THREAD_STATE_DONE:
        return "done";
    default:
        return "invalid";
    }
}

void print_threads(void) {
    uint64_t i = 0;
    printf("\n------------------------------------\n");
    printf("%u threads:\n\n", __cgn_threadlist.thread_count);
    for (__CGNThreadBlock *block = __cgn_threadlist.head; block;
         block = block->next, ++i) {
        for (uint64_t pos = 0; pos < __CGN_THREAD_BLOCK_SIZE; ++pos) {
            uint32_t id = i * __CGN_THREAD_BLOCK_SIZE + pos;
            __CGNThread *thread = &block->threads[pos];

            if (thread->in_use) {
                __CGNThread *thread = &block->threads[pos];
                printf("thread %u:\n\tstate: %s\n\tawaiting: %u\n\tawait count: "
                       "%u\n\treturn_val as int: %d\n\tptr: %p\n\n",
                       id, state_to_name(thread->state), thread->awaited_thread_id,
                       thread->awaiting_thread_count, (int)thread->return_val, thread);
            }
        }
    }
    printf("------------------------------------\n\n");
}

#endif

static __CGNThreadBlock *add_block(void) {
    uint64_t stack_plus_guard_size = SEAGREEN_MAX_STACK_SIZE + __cgn_pagesize;
    uint64_t alloc_size = stack_plus_guard_size * __CGN_THREAD_BLOCK_SIZE;

#if !defined(_WIN32)
    // Map stack + guard page together to assure that the guard page can be mapped
    // at the end of the stack. Will change PROT_NONE to PROT_READ | PROT_WRITE
    // with mprotect().

    // MAP_STACK doesn't exist on macOS
#ifndef MAP_STACK
#define MAP_STACK 0
#endif

    void *stacks =
        mmap(0, alloc_size, PROT_NONE, MAP_PRIVATE | MAP_ANON | MAP_STACK, -1, 0);
    __cgn_check_malloc(stacks);

    for (uint64_t i = 0; i < __CGN_THREAD_BLOCK_SIZE; ++i) {
        // Add __cgn_pagesize to offset to put guard page at end of stack, with the stack
        // growing downward
        uint64_t offset = (i * stack_plus_guard_size) + __cgn_pagesize;
        mprotect(stacks + offset, SEAGREEN_MAX_STACK_SIZE, PROT_READ | PROT_WRITE);
    }
#else
    void *stacks =
        VirtualAlloc(0, alloc_size, MEM_RESERVE | MEM_COMMIT, PAGE_GUARD);
    __cgn_check_malloc(stacks);

    for (uint64_t i = 0; i < __CGN_THREAD_BLOCK_SIZE; ++i) {
        uint64_t offset = (i * stack_plus_guard_size) + __cgn_pagesize;

        void *_oldprot;
        VirtualProtect(stacks + offset, SEAGREEN_MAX_STACK_SIZE, PAGE_READWRITE, _oldprot);
    }
#endif

    __CGNThreadBlock *block =
        (__CGNThreadBlock *)calloc(1, sizeof(__CGNThreadBlock));
    __cgn_check_malloc(block);

    if (!__cgn_threadlist.tail) {
        __cgn_threadlist.head = block;
    } else {
        __cgn_threadlist.tail->next = block;
        block->prev = __cgn_threadlist.tail;
    }

    block->stacks = stacks;

    __cgn_threadlist.tail = block;
    ++__cgn_threadlist.block_count;

    return block;
}

void seagreen_init_rt(void) {
    if (__cgn_threadlist.head) {
        // No need to initialize if already initialized
        return;
    }

    if (!__cgn_pagesize) {
#if !defined(_WIN32)
        __cgn_pagesize = getpagesize();
#else
        LPSYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);

        __cgn_pagesize = sysinfo->dwPageSize;
#endif
    }

    __CGNThreadBlock *block = add_block();

    __cgn_main_thread = &__cgn_threadlist.head->threads[0];
    __cgn_main_thread->state = __CGN_THREAD_STATE_RUNNING;
    __cgn_main_thread->yield_toggle = 1;

    __cgn_curr_thread = __cgn_main_thread;

    __cgn_sched_block = block;
    __cgn_sched_block_pos = 0;
    __cgn_sched_thread_pos = 0;

    __cgn_main_thread->in_use = 1;
    block->used_thread_count = 1;
}

void seagreen_free_rt(void) {
    if (!__cgn_threadlist.head) {
        // No need to deinitialize if not already initialized
        return;
    }

    if (__cgn_curr_thread != __cgn_main_thread) {
        // Can only free from the main thread
        return;
    }

    uint64_t stack_plus_guard_size = SEAGREEN_MAX_STACK_SIZE + __cgn_pagesize;
    uint64_t block_alloc_size = stack_plus_guard_size * __CGN_THREAD_BLOCK_SIZE;

    __CGNThreadBlock *block = __cgn_threadlist.head;

    while (block) {
        __CGNThreadBlock *next = block->next;

#if !defined(_WIN32)
        munmap(block->stacks, block_alloc_size);
#else
        VirtualFree(block->stacks, block_alloc_size, MEM_RELEASE);
#endif

        free(block);
        block = next;
    }

    __cgn_threadlist.head = 0;

    __cgn_curr_thread = 0;
    __cgn_main_thread = 0;

    __cgn_sched_block = 0;
    __cgn_sched_block_pos = 0;
    __cgn_sched_thread_pos = 0;
}

void async_yield(void) {
    if (__cgn_curr_thread->disable_yield) {
        return;
    }

    __cgn_savectx(&__cgn_curr_thread->ctx);

    _Bool temp_yield_toggle = __cgn_curr_thread->yield_toggle;
    __cgn_curr_thread->yield_toggle = !__cgn_curr_thread->yield_toggle;

    if (temp_yield_toggle) {
        __cgn_scheduler();
    }
}

void __cgn_scheduler(void) {
    while (1) {
        for (; __cgn_sched_block; __cgn_sched_block = __cgn_sched_block->next, ++__cgn_sched_block_pos) {
            if (!__cgn_sched_block->used_thread_count) {
                continue;
            }

            for (; __cgn_sched_thread_pos < __CGN_THREAD_BLOCK_SIZE; ++__cgn_sched_thread_pos) {
                __CGNThread *staged_thread = &__cgn_sched_block->threads[__cgn_sched_thread_pos];

                if (!staged_thread->in_use) {
                    continue;
                }

                if (staged_thread->state == __CGN_THREAD_STATE_WAITING) {
                    __CGNThread *awaited_thread =
                        __cgn_get_thread(staged_thread->awaited_thread_id);
                    if (awaited_thread->state == __CGN_THREAD_STATE_DONE) {
                        awaited_thread->awaiting_thread_count--;
                        staged_thread->state = __CGN_THREAD_STATE_READY;
                    } else {
                        continue;
                    }
                }

                // Including RUNNING here allows a thread to run when it is the only
                // thread that isn't WAITING. This will only happen if the scheduler has
                // checked every other thread and found them to be WAITING.
                _Bool runnable = staged_thread->state == __CGN_THREAD_STATE_READY ||
                    staged_thread->state == __CGN_THREAD_STATE_RUNNING;

                if (!runnable) {
                    continue;
                }

                __CGNThread *running_thread = __cgn_curr_thread;
                __cgn_curr_thread = staged_thread;

                if (running_thread->state == __CGN_THREAD_STATE_RUNNING) {
                    running_thread->state = __CGN_THREAD_STATE_READY;
                }

                staged_thread->state = __CGN_THREAD_STATE_RUNNING;

                // Won't loop after loading new ctx; increment thread position for next
                // access to scheduler
                ++__cgn_sched_thread_pos;

                __cgn_loadctx(&staged_thread->ctx);
            }

            __cgn_sched_thread_pos = 0;
        }

        __cgn_sched_block = __cgn_threadlist.head;
        __cgn_sched_block_pos = 0;
    }
}

inline __CGNThreadBlock *__cgn_get_block(uint32_t id) {
    __CGNThreadBlock *block;

    uint32_t block_pos = id / __CGN_THREAD_BLOCK_SIZE;
    if (block_pos > __cgn_threadlist.block_count / 2) {
        block = __cgn_threadlist.tail;
        for (uint32_t i = __cgn_threadlist.block_count - 1; i > block_pos;
             --i, block = block->prev);
    } else {
        block = __cgn_threadlist.head;
        for (uint32_t i = 0; i < block_pos; ++i, block = block->next);
    }

    return block;
}

inline __CGNThread *__cgn_get_thread(uint32_t id) {
    __CGNThreadBlock *block = __cgn_get_block(id);
    return &block->threads[id % __CGN_THREAD_BLOCK_SIZE];
}

__CGNThread *__cgn_add_thread(void **stack) {
    __CGNThreadBlock *block = __cgn_threadlist.tail;

    uint32_t block_pos = __cgn_threadlist.block_count - 1;
    while (block->used_thread_count == __CGN_THREAD_BLOCK_SIZE) {
        if (!block->prev) {
            block = add_block();
            block_pos = __cgn_threadlist.block_count - 1;
            break;
        } else {
            block = block->prev;
            --block_pos;
        }
    }

    __CGNThread *t;
    uint32_t pos = 0;
    // Treat the unused_threads int as an array of bits and find the index
    // of the most significant bit
    for (t = &block->threads[0]; t->in_use; ++pos, ++t);

    t->in_use = 1;
    t->state = __CGN_THREAD_STATE_READY;

    t->id = __CGN_THREAD_BLOCK_SIZE * block_pos + pos;
    t->yield_toggle = 1;

    ++__cgn_threadlist.thread_count;
    ++block->used_thread_count;

    uint64_t stack_plus_guard_size = SEAGREEN_MAX_STACK_SIZE + __cgn_pagesize;
    *stack = block->stacks + (pos + 1) * stack_plus_guard_size;

    return t;
}

void __cgn_remove_thread(__CGNThreadBlock *block, uint32_t pos) {
    __CGNThread *t = &block->threads[pos];

    *t = (__CGNThread){0};
    t->state = __CGN_THREAD_STATE_READY;

    --__cgn_threadlist.thread_count;
    --block->used_thread_count;
}
