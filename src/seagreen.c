#include "seagreen.h"
#include <inttypes.h>


_Thread_local int __cgn_pagesize = 0;

_Thread_local __CGNThreadList __cgn_threadlist = {0};
_Thread_local __CGNReadyQueue __cgn_ready_queue = {0};

_Thread_local __CGNThread *volatile __cgn_curr_thread = 0;
_Thread_local __CGNThread *__cgn_main_thread = 0;
_Thread_local void *__cgn_sched_stack_alloc = 0;
_Thread_local void *__cgn_aligned_sched_stack = 0;

_Thread_local __CGNThreadBlock *__cgn_scheduled_block = 0;
_Thread_local uint32_t __cgn_scheduled_thread_pos = 0;
_Thread_local uint64_t __cgn_scheduler_call_count = 0;

#ifdef CGN_DEBUG
#define CGN_DBG(...) printf(__VA_ARGS__)
#else
#define CGN_DBG(...)
#endif

static __attribute__((noreturn)) void __cgn_fatal(const char *message) {
    fprintf(stderr, "seagreen: %s\n", message);
    abort();
}

static inline void __cgn_block_set_in_use(__CGNThreadBlock *block, uint32_t pos) {
    block->in_use_mask[pos / __CGN_IN_USE_CHUNK_SIZE] |= (uint64_t)1 << (pos % __CGN_IN_USE_CHUNK_SIZE);
}

static inline void __cgn_block_clear_in_use(__CGNThreadBlock *block, uint32_t pos) {
    block->in_use_mask[pos / __CGN_IN_USE_CHUNK_SIZE] &= ~((uint64_t)1 << (pos % __CGN_IN_USE_CHUNK_SIZE));
}

static inline _Bool __cgn_block_slot_in_use(const __CGNThreadBlock *block, uint32_t pos) {
    return (block->in_use_mask[pos / __CGN_IN_USE_CHUNK_SIZE] >> (pos % __CGN_IN_USE_CHUNK_SIZE)) & 1U;
}

static inline int64_t __cgn_block_find_free_slot(const __CGNThreadBlock *block) {
    for (uint32_t chunk = 0; chunk < __CGN_THREAD_IN_USE_CHUNK_COUNT; ++chunk) {
        uint64_t mask = block->in_use_mask[chunk] | block->retired_mask[chunk];
        if (mask == UINT64_MAX) {
            continue;
        }

        uint64_t free_mask = ~mask;
        uint32_t bit = (uint32_t)__builtin_ctzll(free_mask);
        return (int64_t)(chunk * __CGN_IN_USE_CHUNK_SIZE + bit);
    }

    return -1;
}

static inline int64_t __cgn_block_next_in_use(const __CGNThreadBlock *block, uint32_t start_pos) {
    if (start_pos >= __CGN_THREAD_BLOCK_SIZE) {
        return -1;
    }

    uint32_t chunk = start_pos / __CGN_IN_USE_CHUNK_SIZE;
    uint32_t offset = start_pos % __CGN_IN_USE_CHUNK_SIZE;

    for (; chunk < __CGN_THREAD_IN_USE_CHUNK_COUNT; ++chunk, offset = 0) {
        uint64_t mask = block->in_use_mask[chunk];
        if (!mask) {
            continue;
        }

        mask &= (~0ULL << offset);
        if (!mask) {
            continue;
        }

        return (int64_t)(chunk * __CGN_IN_USE_CHUNK_SIZE + (uint32_t)__builtin_ctzll(mask));
    }

    return -1;
}

// Ready queue operations
static inline void __cgn_ready_queue_enqueue(__CGNThread *thread) {
    if (thread->ready_next || thread->ready_prev) {
        // Thread already in queue
        return;
    }
    
    if (!__cgn_ready_queue.head) {
        __cgn_ready_queue.head = thread;
        __cgn_ready_queue.tail = thread;
        thread->ready_next = thread;
        thread->ready_prev = thread;
    } else {
        thread->ready_next = __cgn_ready_queue.head;
        thread->ready_prev = __cgn_ready_queue.tail;
        __cgn_ready_queue.tail->ready_next = thread;
        __cgn_ready_queue.head->ready_prev = thread;
        __cgn_ready_queue.tail = thread;
    }
    __cgn_ready_queue.count++;
}

static inline __CGNThread *__cgn_ready_queue_dequeue(void) {
    if (!__cgn_ready_queue.head) {
        return NULL;
    }
    
    __CGNThread *thread = __cgn_ready_queue.head;
    
    if (__cgn_ready_queue.head == __cgn_ready_queue.tail) {
        __cgn_ready_queue.head = NULL;
        __cgn_ready_queue.tail = NULL;
    } else {
        __cgn_ready_queue.head = thread->ready_next;
        __cgn_ready_queue.head->ready_prev = __cgn_ready_queue.tail;
        __cgn_ready_queue.tail->ready_next = __cgn_ready_queue.head;
    }
    
    thread->ready_next = NULL;
    thread->ready_prev = NULL;
    __cgn_ready_queue.count--;
    
    return thread;
}

static inline void __cgn_ready_queue_remove(__CGNThread *thread) {
    if (!thread->ready_next || !thread->ready_prev) {
        // Thread not in queue
        return;
    }
    
    if (thread == __cgn_ready_queue.head && thread == __cgn_ready_queue.tail) {
        __cgn_ready_queue.head = NULL;
        __cgn_ready_queue.tail = NULL;
    } else {
        if (thread == __cgn_ready_queue.head) {
            __cgn_ready_queue.head = thread->ready_next;
        }
        if (thread == __cgn_ready_queue.tail) {
            __cgn_ready_queue.tail = thread->ready_prev;
        }

        thread->ready_prev->ready_next = thread->ready_next;
        thread->ready_next->ready_prev = thread->ready_prev;
    }

    thread->ready_next = NULL;
    thread->ready_prev = NULL;
    __cgn_ready_queue.count--;
}

static __CGNThreadBlock *add_block(void) {
    uint64_t stack_plus_guard_size = SEAGREEN_MAX_STACK_SIZE + __cgn_pagesize;
    uint64_t alloc_size = stack_plus_guard_size * __CGN_THREAD_BLOCK_SIZE;

#if !defined(_WIN32)
    // Map stack + guard page together to assure that the guard page can be mapped
    // at the end of the stack. Will change PROT_NONE to PROT_READ | PROT_WRITE
    // with mprotect().
    void *stacks =
        mmap(0, alloc_size, PROT_NONE, MAP_PRIVATE | MAP_ANON | MAP_STACK | MAP_GROWSDOWN, -1, 0);
    if (stacks == MAP_FAILED) __cgn_fatal("stack mapping failed");

    for (uint64_t i = 0; i < __CGN_THREAD_BLOCK_SIZE; ++i) {
        // Add __cgn_pagesize to offset to put guard page at end of stack, with the stack
        // growing downward
        uint64_t offset = (i * stack_plus_guard_size) + __cgn_pagesize;
        if (mprotect((char *)stacks + offset, SEAGREEN_MAX_STACK_SIZE,
                     PROT_READ | PROT_WRITE) != 0) {
            __cgn_fatal("stack protection failed");
        }
    }
#else
    void *stacks =
        VirtualAlloc(0, alloc_size, MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS);
    if (stacks == NULL) __cgn_fatal("stack allocation failed");

    for (uint64_t i = 0; i < __CGN_THREAD_BLOCK_SIZE; ++i) {
        uint64_t offset = (i * stack_plus_guard_size) + __cgn_pagesize;

        DWORD _oldprot;
        if (!VirtualProtect((char *)stacks + offset, SEAGREEN_MAX_STACK_SIZE,
                            PAGE_READWRITE, &_oldprot)) {
            __cgn_fatal("stack protection failed");
        }
    }
#endif

    __CGNThreadBlock *block =
        (__CGNThreadBlock *)calloc(1, sizeof(__CGNThreadBlock));
    if (block == NULL) __cgn_fatal("thread block allocation failed");

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

static inline __CGNThreadBlock *__cgn_get_block(uint32_t id) {
    __CGNThreadBlock *block;

    uint32_t block_pos = id / __CGN_THREAD_BLOCK_SIZE;
    if (block_pos >= __cgn_threadlist.block_count) return NULL;
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

static inline __CGNThread *__cgn_get_thread(CGNThreadHandle handle) {
    uint32_t id = (uint32_t)handle;
    uint32_t generation = (uint32_t)(handle >> 32);
    __CGNThreadBlock *block = __cgn_get_block(id);
    uint32_t pos = id % __CGN_THREAD_BLOCK_SIZE;
    if (!block || !generation || !__cgn_block_slot_in_use(block, pos) ||
        block->threads[pos].generation != generation) {
        __cgn_fatal("invalid or expired thread handle");
    }
    return &block->threads[pos];
}

static __attribute__((noinline, noreturn)) void __cgn_scheduler(void) {
    __cgn_scheduler_call_count++;
    
    // Check for waiting threads first if it's time for starvation prevention
    _Bool should_check_waiting = (__cgn_scheduler_call_count % __CGN_STARVATION_CHECK_INTERVAL == 0) ||
                                    !__cgn_ready_queue.count;
    
    if (should_check_waiting) {
        // Resume from where we left off in the previous scan
        __CGNThreadBlock *block = __cgn_scheduled_block;
        uint32_t pos = __cgn_scheduled_thread_pos;
        
        // If we don't have a current block, start from the beginning
        if (!block) {
            block = __cgn_threadlist.head;
            pos = 0;
        }

        // Scan through blocks and threads, checking waiting threads
        while (block) {
            if (block->used_thread_count) {
                while (pos < __CGN_THREAD_BLOCK_SIZE) {
                    int64_t next_in_use = __cgn_block_next_in_use(block, pos);
                    if (next_in_use < 0) {
                        break;
                    }

                    size_t thread_index = (size_t)next_in_use;
                    __CGNThread *thread = &block->threads[thread_index];

                    if (thread->state == __CGN_THREAD_STATE_WAITING) {
                        __CGNThread *awaited_thread = __cgn_get_thread(thread->awaited_handle);
                        if (awaited_thread->state == __CGN_THREAD_STATE_DONE) {
                            thread->state = __CGN_THREAD_STATE_READY;
                            thread->awaited_handle = 0;
                            __cgn_ready_queue_enqueue(thread);
                            
                            // Found a thread to make ready
                            goto break_from_outer_waiting_scan_loop;
                        }
                    }

                    pos = thread_index + 1;
                }
            }

            // Move to next block
            block = block->next;
            pos = 0;
        }
    break_from_outer_waiting_scan_loop:
        
        // Reset scan position for next time
        __cgn_scheduled_block = __cgn_threadlist.head;
        __cgn_scheduled_thread_pos = 0;
    }

    // Fast path: try to get a ready thread from the queue
    __CGNThread *staged_thread = __cgn_ready_queue_dequeue();
    if (staged_thread) {
        if (staged_thread->state == __CGN_THREAD_STATE_READY || 
            staged_thread->state == __CGN_THREAD_STATE_RUNNING) {
            
            __CGNThread *running_thread = __cgn_curr_thread;
            __cgn_curr_thread = staged_thread;

            if (running_thread && running_thread->state == __CGN_THREAD_STATE_RUNNING) {
                running_thread->state = __CGN_THREAD_STATE_READY;
                __cgn_ready_queue_enqueue(running_thread);
            }

            staged_thread->state = __CGN_THREAD_STATE_RUNNING;
            atomic_signal_fence(memory_order_seq_cst);
            __cgn_loadctx(staged_thread);
            __builtin_unreachable();
        }
    }

    __cgn_fatal("no runnable threads (deadlock or invalid scheduler state)");
}

static uint64_t *__cgn_get_thread_return_val(__CGNThread *thread) {
    // Return value is stored at the bottom of the stack (only written once when thread completes)
    __CGNThreadBlock *block = __cgn_get_block(thread->id);
    uint32_t pos = thread->id % __CGN_THREAD_BLOCK_SIZE;
    uint64_t stack_plus_guard_size = SEAGREEN_MAX_STACK_SIZE + __cgn_pagesize;
    void *stack_bottom = (char *)block->stacks + pos * stack_plus_guard_size + __cgn_pagesize;
    
    return (uint64_t *)stack_bottom;
}

__attribute__((noreturn)) void __cgn_thread_entry(void) {
    __CGNThread *self = __cgn_curr_thread;
    uint64_t rv = 0;
    if (self && self->fn) {
        rv = self->fn(self->arg);
    }
    *__cgn_get_thread_return_val(self) = rv;
    self->state = __CGN_THREAD_STATE_DONE;
    atomic_signal_fence(memory_order_seq_cst);
    __cgn_jumpwithstack(&__cgn_scheduler, __cgn_aligned_sched_stack);
    __builtin_unreachable();
}

static __CGNThread *__cgn_add_thread(void **stack) {
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

    int64_t free_slot = __cgn_block_find_free_slot(block);
    if (free_slot < 0) {
        block = add_block();
        block_pos = __cgn_threadlist.block_count - 1;
        free_slot = __cgn_block_find_free_slot(block);
    }

    uint32_t pos = (uint32_t)free_slot;
    __CGNThread *t = &block->threads[pos];

    __cgn_block_set_in_use(block, pos);
    t->state = __CGN_THREAD_STATE_READY;
    t->ready_next = NULL;
    t->ready_prev = NULL;

    uint64_t new_id = (uint64_t)__CGN_THREAD_BLOCK_SIZE * block_pos + pos;
    if (new_id >= UINT32_MAX) {
        fprintf(stderr, "seagreen: Maximum thread ID reached (UINT32_MAX)\n");
        abort();
    }
    t->id = (uint32_t)new_id;
    ++t->generation;

    ++__cgn_threadlist.thread_count;
    ++block->used_thread_count;

    uint64_t stack_plus_guard_size = SEAGREEN_MAX_STACK_SIZE + __cgn_pagesize;
    // pos * stack_plus_guard_size gets us to the bottom of the downward-growing stack,
    // + stack_plus_guard_size gets us to the top (where the stack actually begins)
    void *stack_top = (char *)block->stacks + pos * stack_plus_guard_size + stack_plus_guard_size;
    
    // Align stack pointer to 16 bytes for x86_64 ABI compliance
    *stack = (void *)((uintptr_t)stack_top & ~15ULL);

    t->stack_ptr = *stack;

    // Add to ready queue
    __cgn_ready_queue_enqueue(t);

    return t;
}

static void __cgn_remove_thread(__CGNThreadBlock *block, uint32_t pos) {
    __CGNThread *t = &block->threads[pos];

    // Remove from ready queue if present
    __cgn_ready_queue_remove(t);

    uint32_t generation = t->generation;
    *t = (__CGNThread){.generation = generation};
    // Never wrap a generation and make an old handle valid again.
    if (generation == UINT32_MAX) {
        block->retired_mask[pos / __CGN_IN_USE_CHUNK_SIZE] |=
            (uint64_t)1 << (pos % __CGN_IN_USE_CHUNK_SIZE);
    }
    t->state = __CGN_THREAD_STATE_READY;
    t->ready_next = NULL;
    t->ready_prev = NULL;

    __cgn_block_clear_in_use(block, pos);

    --__cgn_threadlist.thread_count;
    --block->used_thread_count;
}

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
    printf("%u thread(s):\n\n", __cgn_threadlist.thread_count);
    for (__CGNThreadBlock *block = __cgn_threadlist.head; block;
         block = block->next, ++i) {
        uint32_t pos = 0;
        while (pos < __CGN_THREAD_BLOCK_SIZE) {
            int64_t next = __cgn_block_next_in_use(block, pos);
            if (next < 0) {
                break;
            }

            pos = (uint32_t)next;
            uint32_t id = i * __CGN_THREAD_BLOCK_SIZE + pos;
            __CGNThread *thread = &block->threads[pos];

            uint64_t stack_ptr = (uint64_t)thread->stack_ptr;

            printf("thread %u:\n\tstate: %s\n\tawaiting: %" PRIu64 "\n\tawait count: "
                   "%u\n\tptr: %p\n\tstack ptr: %p\n\n",
                   id, state_to_name(thread->state), thread->awaited_handle,
                   thread->awaiting_thread_count, (void*)thread, (void*)stack_ptr);

            ++pos;
        }
    }
    printf("------------------------------------\n\n");
}

#endif

__CGN_EXPORT void seagreen_init_rt(void) {
    if (__cgn_threadlist.head) {
        // No need to initialize if already initialized
        return;
    }

    if (!__cgn_pagesize) {
#if !defined(_WIN32)
        __cgn_pagesize = sysconf(_SC_PAGESIZE);
#else
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);

        __cgn_pagesize = sysinfo.dwPageSize;
#endif
    }

    __CGNThreadBlock *block = add_block();

    __cgn_main_thread = &__cgn_threadlist.head->threads[0];
    __cgn_block_set_in_use(block, 0);
    __cgn_main_thread->state = __CGN_THREAD_STATE_RUNNING;
    __cgn_main_thread->ready_next = NULL;
    __cgn_main_thread->ready_prev = NULL;

    block->used_thread_count = 1;

    __cgn_curr_thread = __cgn_main_thread;

    __cgn_scheduled_block = block;
    __cgn_scheduled_thread_pos = 0;

    uint64_t sched_alloc_size = SEAGREEN_MAX_STACK_SIZE + __cgn_pagesize;

#if !defined(_WIN32)
    void *sched_alloc =
        mmap(0, sched_alloc_size, PROT_NONE, MAP_PRIVATE | MAP_ANON | MAP_STACK | MAP_GROWSDOWN, -1, 0);
    if (sched_alloc == MAP_FAILED) __cgn_fatal("scheduler stack mapping failed");

    // Put guard page at start; stack grows downward after it
    if (mprotect((char *)sched_alloc + __cgn_pagesize, SEAGREEN_MAX_STACK_SIZE,
                 PROT_READ | PROT_WRITE) != 0) {
        __cgn_fatal("scheduler stack protection failed");
    }
#else
    void *sched_alloc =
        VirtualAlloc(0, sched_alloc_size, MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS);
    if (sched_alloc == NULL) __cgn_fatal("scheduler stack allocation failed");

    DWORD _oldprot;
    if (!VirtualProtect((char *)sched_alloc + __cgn_pagesize, SEAGREEN_MAX_STACK_SIZE,
                        PAGE_READWRITE, &_oldprot)) {
        __cgn_fatal("scheduler stack protection failed");
    }
#endif

    __cgn_sched_stack_alloc = sched_alloc;

    void *stack_top = (char *)sched_alloc + SEAGREEN_MAX_STACK_SIZE + __cgn_pagesize;
    __cgn_aligned_sched_stack = (void *)((uintptr_t)stack_top & ~15ULL);
}

__CGN_EXPORT CGNThreadHandle async_run(__CGNAsyncFn fn, void *arg) {
    void *stack;
    __CGNThread *t = __cgn_add_thread(&stack);
    t->fn = fn;
    t->arg = arg;
    atomic_signal_fence(memory_order_seq_cst);
    volatile _Bool loaded = __cgn_savenewctx(t, stack);
    atomic_signal_fence(memory_order_seq_cst);
    if (loaded) {
        atomic_signal_fence(memory_order_seq_cst);
        __cgn_jumpwithstack(&__cgn_scheduler, __cgn_aligned_sched_stack);
        __builtin_unreachable();
    }
    return ((CGNThreadHandle)t->generation << 32) | t->id;
}

__CGN_EXPORT void seagreen_free_rt(void) {
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
        VirtualFree(block->stacks, 0, MEM_RELEASE);
#endif

        free(block);
        block = next;
    }

    __cgn_threadlist.head = 0;
    __cgn_threadlist.tail = 0;
    __cgn_threadlist.block_count = 0;
    __cgn_threadlist.thread_count = 0;

    __cgn_ready_queue.head = 0;
    __cgn_ready_queue.tail = 0;
    __cgn_ready_queue.count = 0;

    __cgn_curr_thread = 0;
    __cgn_main_thread = 0;

    __cgn_scheduled_block = 0;
    __cgn_scheduled_thread_pos = 0;
    __cgn_scheduler_call_count = 0;

    uint64_t sched_alloc_size = SEAGREEN_MAX_STACK_SIZE + __cgn_pagesize;
#if !defined(_WIN32)
    if (__cgn_sched_stack_alloc) {
        munmap(__cgn_sched_stack_alloc, sched_alloc_size);
    }
#else
    if (__cgn_sched_stack_alloc) {
        VirtualFree(__cgn_sched_stack_alloc, 0, MEM_RELEASE);
    }
#endif

    __cgn_sched_stack_alloc = 0;
    __cgn_aligned_sched_stack = 0;
}

__CGN_EXPORT void async_yield(void) {
    atomic_signal_fence(memory_order_seq_cst);
    volatile _Bool loaded = __cgn_savectx(__cgn_curr_thread);
    atomic_signal_fence(memory_order_seq_cst);
    if (!loaded) {
        if (__cgn_curr_thread->state == __CGN_THREAD_STATE_RUNNING) {
            __cgn_curr_thread->state = __CGN_THREAD_STATE_READY;
            __cgn_ready_queue_enqueue(__cgn_curr_thread);
        }

        atomic_signal_fence(memory_order_seq_cst);
        __cgn_jumpwithstack(&__cgn_scheduler, __cgn_aligned_sched_stack);
        __builtin_unreachable();
    }
}

__CGN_EXPORT uint64_t await(CGNThreadHandle handle) {
    __CGNThread *t = __cgn_get_thread(handle);
    __cgn_curr_thread->awaited_handle = handle;
    uint32_t pos = t->id % __CGN_THREAD_BLOCK_SIZE;
    __CGNThreadBlock *block = __cgn_get_block(t->id);
    if (t == __cgn_curr_thread) __cgn_fatal("cannot await the current thread");
    uint64_t return_val = 0;
    t->awaiting_thread_count++;

    /* Because thread is waiting, the curr thread */
    /* won't be scheduled until awaited thread has */
    /* finished its execution */

    atomic_signal_fence(memory_order_seq_cst);
    volatile _Bool loaded = __cgn_savectx(__cgn_curr_thread);
    atomic_signal_fence(memory_order_seq_cst);
    if (!loaded) {
        __cgn_curr_thread->state = __CGN_THREAD_STATE_WAITING;
        __cgn_ready_queue_remove(__cgn_curr_thread);

        atomic_signal_fence(memory_order_seq_cst);
        __cgn_jumpwithstack(&__cgn_scheduler, (char *)__cgn_sched_stack_alloc + SEAGREEN_MAX_STACK_SIZE + __cgn_pagesize);
        __builtin_unreachable();
    }

    return_val = *__cgn_get_thread_return_val(t);
    // A ready waiter still owns the result until it has read it.
    if (--t->awaiting_thread_count == 0) {
        __cgn_remove_thread(block, pos);
    }

    return return_val;
}
