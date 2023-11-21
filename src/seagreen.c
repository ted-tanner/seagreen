#include "seagreen.h"

int pagesize = 0;

_Thread_local __CGNThreadList threadlist = {0};

_Thread_local __CGNThread *curr_thread = 0;
_Thread_local __CGNThread *main_thread = 0;

_Thread_local __CGNThreadBlock *sched_block = 0;
_Thread_local uint64_t sched_block_pos = 0;
_Thread_local uint64_t sched_thread_pos = 0;

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
    for (__CGNThreadBlock *block = threadlist.head; block; block = block->next, ++i) {
        for (uint64_t pos = 0; pos < __CGN_THREAD_BLOCK_SIZE; ++pos) {
            uint64_t id = i * __CGN_THREAD_BLOCK_SIZE + pos;
            __CGNThread *thread = &block->threads[pos];

            if (thread->in_use) {
                __CGNThread *thread = &block->threads[pos];
                printf("thread %llu:\n\tstate: %s\n\tawaiting: %llu\n\tawait count: %llu\n\treturn_val as int: %d\n\tptr: %p\n\n",
                       id,
                       state_to_name(thread->state),
                       thread->awaited_thread_id,
                       thread->awaiting_thread_count,
                       (int) thread->return_val,
                       thread);
            }
        }
    }
    printf("------------------------------------\n\n");
}

#endif

// TODO: Still need to handle
static __CGNThreadBlock *add_block(void) {
    uint64_t stack_plus_guard_size = __CGN_STACK_SIZE + pagesize;
    uint64_t alloc_size = stack_plus_guard_size * __CGN_THREAD_BLOCK_SIZE;

#if !defined(_WIN32)
    // TODO: Get real page size

    // Map stack + guard page together to assure that the guard page can be mapped at the
    // end of the stack. Will change PROT_NONE to PROT_READ | PROT_WRITE with mprotect().
    void *stacks = mmap(0, alloc_size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    __cgn_check_malloc(stacks);

    for (uint64_t i = 0; i < __CGN_THREAD_BLOCK_SIZE; ++i) {
        // Add pagesize to offset to put guard page at end of stack, with the stack
        // growing downward
        uint64_t offset = (i * stack_plus_guard_size) + pagesize;
        mprotect(stacks + offset, __CGN_STACK_SIZE, PROT_READ | PROT_WRITE);
    }
#else
    void *stacks = VirtualAlloc(0, alloc_size, MEM_RESERVE | MEM_COMMIT, PAGE_GUARD);
    __cgn_check_malloc(stacks);

    for (uint64_t i = 0; i < __CGN_THREAD_BLOCK_SIZE; ++i) {
        uint64_t offset = (i * stack_plus_guard_size) + pagesize;

        void *_oldprot;
        VirtualProtect(stacks + offset, __CGN_STACK_SIZE, PAGE_READWRITE, _oldprot);
    }
#endif

    __CGNThreadBlock *block = (__CGNThreadBlock *)calloc(1, sizeof(__CGNThreadBlock));
    __cgn_check_malloc(block);

    if (!threadlist.tail) {
        threadlist.head = block;
    } else {
        threadlist.tail->next = block;
        block->prev = threadlist.tail;
    }

    block->stacks = stacks;

    threadlist.tail = block;
    ++threadlist.block_count;

    return block;
}

void seagreen_init_rt(void) {
    if (threadlist.head) {
        // No need to initialize if already initialized
        return;
    }

    if (!pagesize) {
#if !defined(_WIN32)
        pagesize = getpagesize();
#else
        LPSYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);

        pagesize = sysinfo->dwPageSize;
#endif
    }

    __CGNThreadBlock *block = add_block();

    main_thread = &threadlist.head->threads[0];
    main_thread->state = __CGN_THREAD_STATE_RUNNING;
    main_thread->run_toggle = 0;
    main_thread->yield_toggle = 1;

    curr_thread = main_thread;

    sched_block = block;
    sched_block_pos = 0;
    sched_thread_pos = 0;

    main_thread->in_use = 1;
    block->used_thread_count = 1;
}

void seagreen_free_rt(void) {
    if (!threadlist.head) {
        // No need to deinitialize if not already initialized
        return;
    }

    if (curr_thread != main_thread) {
        // Can only free from the main thread
        return;
    }

    uint64_t stack_plus_guard_size = __CGN_STACK_SIZE + pagesize;
    uint64_t block_alloc_size = stack_plus_guard_size * __CGN_THREAD_BLOCK_SIZE;

    __CGNThreadBlock *block = threadlist.head;

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

    threadlist.head = 0;

    curr_thread = 0;
    main_thread = 0;

    sched_block = 0;
    sched_block_pos = 0;
    sched_thread_pos = 0;
}

void async_yield(void) {
    __CGNThread *t = __cgn_get_curr_thread();
    __cgn_savectx(&t->ctx, t);

    _Bool temp_yield_toggle = t->yield_toggle;
    t->yield_toggle = !t->yield_toggle;

    if (temp_yield_toggle) {
        __cgn_scheduler();
    }
}

void __cgn_scheduler(void) {
    while (1) {
        for (; sched_block; sched_block = sched_block->next, ++sched_block_pos) {
            if (!sched_block->used_thread_count) {
                continue;
            }

            for (; sched_thread_pos < __CGN_THREAD_BLOCK_SIZE; ++sched_thread_pos) {
                __CGNThread *staged_thread = &sched_block->threads[sched_thread_pos];

                if (!staged_thread->in_use) {
                    continue;
                }

                if (staged_thread->state == __CGN_THREAD_STATE_WAITING) {
                    __CGNThread *awaited_thread = __cgn_get_thread(staged_thread->awaited_thread_id);
                    if (awaited_thread->state == __CGN_THREAD_STATE_DONE) {
                        awaited_thread->awaiting_thread_count--;
                        staged_thread->state = __CGN_THREAD_STATE_READY;
                    } else {
                        continue;
                    }
                }

                if (staged_thread->state != __CGN_THREAD_STATE_READY) {
                    continue;
                }

                __CGNThread *running_thread = curr_thread;
                curr_thread = staged_thread;

                if (running_thread->state == __CGN_THREAD_STATE_RUNNING) {
                    running_thread->state = __CGN_THREAD_STATE_READY;
                }

                staged_thread->state = __CGN_THREAD_STATE_RUNNING;

                // Won't loop after loading new ctx; increment thread position for next access
                // to scheduler
                ++sched_thread_pos;

                __cgn_loadctx(&staged_thread->ctx, staged_thread);
            }

            sched_thread_pos = 0;
        }

        sched_block = threadlist.head;
        sched_block_pos = 0;
    }
}

inline __CGNThreadBlock *__cgn_get_block(uint64_t id) {
    __CGNThreadBlock *block;

    uint64_t block_pos = id / __CGN_THREAD_BLOCK_SIZE;
    if (block_pos > threadlist.block_count / 2) {
        block = threadlist.tail;
        for (uint64_t i = threadlist.block_count - 1; i > block_pos; --i, block = block->prev);
    } else {
        block = threadlist.head;
        for (uint64_t i = 0; i < block_pos; ++i, block = block->next);
    }

    return block;
}

inline __CGNThread *__cgn_get_thread(uint64_t id) {
    __CGNThreadBlock *block = __cgn_get_block(id);
    return &block->threads[id % __CGN_THREAD_BLOCK_SIZE];
}

__CGNThread *__cgn_add_thread(uint64_t *id, void **stack) {
    __CGNThreadBlock *block = threadlist.tail;

    uint64_t block_pos = threadlist.block_count - 1;
    while (block->used_thread_count == __CGN_THREAD_BLOCK_SIZE) {
        if (!block->prev) {
            block = add_block();
            block_pos = threadlist.block_count - 1;
            break;
        } else {
            block = block->prev;
            --block_pos;
        }
    }

    __CGNThread *t;
    uint64_t pos = 0;
    // Treat the unused_threads int as an array of bits and find the index
    // of the most significant bit
    for (t = &block->threads[0]; t->in_use; ++pos, ++t);

    t->in_use = 1;
    t->state = __CGN_THREAD_STATE_READY;

    t->run_toggle = 0;
    t->yield_toggle = 1;

    ++threadlist.thread_count;
    ++block->used_thread_count;

    *id = __CGN_THREAD_BLOCK_SIZE * block_pos + pos;
    *stack = block->stacks + pos * (__CGN_STACK_SIZE + pagesize) + __CGN_STACK_SIZE;

    return t;
}

void __cgn_remove_thread(__CGNThreadBlock *block, uint64_t pos) {
    __CGNThread *t = &block->threads[pos];

    *t = (__CGNThread){0};

    --threadlist.thread_count;
    --block->used_thread_count;
}

inline __CGNThread *__cgn_get_curr_thread(void) {
    return curr_thread;
}

inline __CGNThread *__cgn_get_main_thread(void) {
    return main_thread;
}
