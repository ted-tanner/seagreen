#ifndef SEAGREEN_H

#if !defined __GNUC__ && !defined __clang__
#warning "seagreenlib only officially supports the GCC and Clang compilers"
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/mman.h>
#elif defined(_WIN64)
#include <memoryapi.h>
#include <sysinfoapi.h>
#else
#warning "seagreenlib only supports Unix-like and Windows 64-bit systems"
#include <unistd.h>
#include <sys/mman.h>
#endif

#include "cgninternals/coroutine.h"

#ifdef CGN_DEBUG
void print_threads(void);
#endif

// mmap returns -1 on error, check ptr < 1 rather than !ptr
#define __cgn_check_malloc(ptr)				\
    if ((void *) ptr < (void *) 1) {			\
        fprintf(stderr, "memory allocation failure\n");	\
        abort();                                        \
    }

// Macros that typedef don't support spaces in type names
typedef signed char signedchar;
typedef unsigned char unsignedchar;
typedef unsigned short unsignedshort;
typedef unsigned int unsignedint;
typedef unsigned long unsignedlong;
typedef long long longlong;
typedef unsigned long long unsignedlonglong;
typedef void *voidptr;

// The stack space allocated for each thread. Many of these pages will remain
// untouched
#define __CGN_STACK_SIZE 1024 * 1024 * 2 // 2 MB

// Must be a power of 2 greater than or equal to 64
#define __CGN_THREAD_BLOCK_SIZE 512
#define __CGN_THREAD_BLOCK_DESCRIPTOR_COUNT __CGN_THREAD_BLOCK_SIZE / 64

#define __cgn_define_handle_type(T)		\
    typedef struct _CGNThreadHandle_##T {	\
        uint64_t id;				\
    } CGNThreadHandle_##T

__cgn_define_handle_type(void);
__cgn_define_handle_type(char);
__cgn_define_handle_type(signedchar);
__cgn_define_handle_type(unsignedchar);
__cgn_define_handle_type(short);
__cgn_define_handle_type(unsignedshort);
__cgn_define_handle_type(int);
__cgn_define_handle_type(unsignedint);
__cgn_define_handle_type(long);
__cgn_define_handle_type(unsignedlong);
__cgn_define_handle_type(longlong);
__cgn_define_handle_type(unsignedlonglong);
__cgn_define_handle_type(float);
__cgn_define_handle_type(double);
__cgn_define_handle_type(voidptr);

typedef enum __CGNThreadState_ {
    __CGN_THREAD_STATE_READY,
    __CGN_THREAD_STATE_RUNNING,
    __CGN_THREAD_STATE_WAITING,
    __CGN_THREAD_STATE_DONE,
} __CGNThreadState;

typedef struct __CGNThread_ {
    __CGNThreadCtx ctx;

    uint64_t awaited_thread_id;
    uint64_t awaiting_thread_count;

    uint64_t return_val;

    __CGNThreadState state;

    _Bool yield_toggle;
    _Bool run_toggle;

    _Bool in_use;
} __CGNThread;

typedef struct __CGNThreadBlock_ {
    struct __CGNThreadBlock_ *next;
    struct __CGNThreadBlock_ *prev;

    __CGNThread threads[__CGN_THREAD_BLOCK_SIZE];
    void *stacks;

    uint64_t used_thread_count;
} __CGNThreadBlock;

typedef struct __CGNThreadList_ {
    __CGNThreadBlock *head;
    __CGNThreadBlock *tail;

    uint64_t block_count;
    uint64_t thread_count;
} __CGNThreadList;

void seagreen_init_rt(void);
void seagreen_free_rt(void);
void async_yield(void);

void __cgn_scheduler(void);

__CGNThreadBlock *__cgn_get_block(uint64_t id);
__CGNThread *__cgn_get_thread(uint64_t id);
__CGNThread *__cgn_get_thread_by_block(__CGNThreadBlock *block, uint64_t pos);
__CGNThread *__cgn_add_thread(uint64_t *id, void **stack);
void __cgn_remove_thread(__CGNThreadBlock *block, uint64_t pos);

__CGNThread *__cgn_get_curr_thread(void);
__CGNThread *__cgn_get_main_thread(void);

#define async __attribute__((noinline))

#define await(handle)							\
    _Generic((handle),							\
             CGNThreadHandle_void: ({					\
                     __CGNThread *t_curr = __cgn_get_curr_thread();	\
                     t_curr->awaited_thread_id = (handle).id;		\
                     t_curr->state = __CGN_THREAD_STATE_WAITING;        \
                                                                        \
                     uint64_t pos = (handle).id % __CGN_THREAD_BLOCK_SIZE; \
                     __CGNThreadBlock *block = __cgn_get_block((handle).id); \
                                                                        \
                     __CGNThread *t = &block->threads[pos];		\
                     t->awaiting_thread_count++;                        \
                                                                        \
                     /* Because thread is waiting, the curr thread */	\
                     /* won't be scheduled until awaited thread has */	\
                     /* finished its execution */			\
                     async_yield();					\
                     t_curr->state = __CGN_THREAD_STATE_RUNNING;        \
                     t_curr->yield_toggle = 0;				\
                                                                        \
                     if (!t->awaiting_thread_count) {			\
                         __cgn_remove_thread(block, pos);		\
                     }							\
                                                                        \
                     (void)0;						\
                 }),							\
             default: ({                                                \
                     __CGNThread *t_curr = __cgn_get_curr_thread();	\
                     t_curr->awaited_thread_id = (handle).id;		\
                     t_curr->state = __CGN_THREAD_STATE_WAITING;        \
                                                                        \
                     uint64_t pos = (handle).id % __CGN_THREAD_BLOCK_SIZE; \
                     __CGNThreadBlock *block = __cgn_get_block((handle).id); \
                                                                        \
                     __CGNThread *t = &block->threads[pos];		\
                     t->awaiting_thread_count++;                        \
                                                                        \
                     /* Because thread is waiting, the curr thread */	\
                     /* won't be scheduled until awaited thread has */	\
                     /* finished its execution */			\
                     async_yield();					\
                     t_curr->state = __CGN_THREAD_STATE_RUNNING;        \
                     t_curr->yield_toggle = 0;				\
                                                                        \
                     uint64_t return_val = block->threads[pos].return_val; \
                     if (!t->awaiting_thread_count) {			\
                         __cgn_remove_thread(block, pos);		\
                     }							\
                                                                        \
                     return_val;                                        \
                 }))

#define async_run(Fn)							\
    _Generic(								\
        (Fn),								\
        char: ({                                                        \
                CGNThreadHandle_char handle;                            \
									\
		void *stack;						\
                __CGNThread *t = __cgn_add_thread(&handle.id, &stack);	\
		/* Must reassign t so it is available with new stack */	\
		/* when execution jumps back */				\
                t = __cgn_savectx(&t->ctx, t);				\
                                                                        \
                _Bool temp_run_toggle = t->run_toggle;			\
                t->run_toggle = !t->run_toggle;				\
                                                                        \
                if (temp_run_toggle)  {					\
                    uint64_t retval = (uint64_t) Fn;			\
                    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
                    curr_thread->return_val = retval;			\
                    curr_thread->state = __CGN_THREAD_STATE_DONE;       \
                    __cgn_scheduler();                                  \
                } else {						\
		    __cgn_set_stack_ptr(&t->ctx, stack);		\
		}							\
                                                                        \
                handle;							\
            }),                                                         \
        signed char: ({							\
                CGNThreadHandle_signedchar handle;			\
									\
		void *stack;						\
                __CGNThread *t = __cgn_add_thread(&handle.id, &stack);	\
		/* Must reassign t so it is available with new stack */	\
		/* when execution jumps back */				\
                t = __cgn_savectx(&t->ctx, t);				\
                                                                        \
                _Bool temp_run_toggle = t->run_toggle;			\
                t->run_toggle = !t->run_toggle;				\
                                                                        \
                if (temp_run_toggle) {					\
                    uint64_t retval = (uint64_t) Fn;			\
                    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
                    curr_thread->return_val = retval;			\
                    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
                    __cgn_scheduler();					\
                } else {						\
		    __cgn_set_stack_ptr(&t->ctx, stack);		\
		}							\
                                                                        \
                handle;							\
            }),                                                         \
        unsigned char: ({						\
                CGNThreadHandle_unsignedchar handle;			\
									\
                void *stack;						\
                __CGNThread *t = __cgn_add_thread(&handle.id, &stack);	\
		/* Must reassign t so it is available with new stack */	\
		/* when execution jumps back */				\
                t = __cgn_savectx(&t->ctx, t);				\
                                                                        \
                _Bool temp_run_toggle = t->run_toggle;			\
                t->run_toggle = !t->run_toggle;				\
                                                                        \
                if (temp_run_toggle) {					\
                    uint64_t retval = (uint64_t) Fn;			\
                    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
                    curr_thread->return_val = retval;			\
                    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
                    __cgn_scheduler();					\
                } else {						\
		    __cgn_set_stack_ptr(&t->ctx, stack);		\
		}							\
                                                                        \
                handle;							\
            }),                                                         \
        short: ({							\
                CGNThreadHandle_short handle;				\
									\
                void *stack;						\
                __CGNThread *t = __cgn_add_thread(&handle.id, &stack);	\
		/* Must reassign t so it is available with new stack */	\
		/* when execution jumps back */				\
                t = __cgn_savectx(&t->ctx, t);				\
                                                                        \
                _Bool temp_run_toggle = t->run_toggle;			\
                t->run_toggle = !t->run_toggle;				\
                                                                        \
                if (temp_run_toggle) {					\
                    uint64_t retval = (uint64_t) Fn;			\
                    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
                    curr_thread->return_val = retval;			\
                    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
                    __cgn_scheduler();					\
                } else {						\
		    __cgn_set_stack_ptr(&t->ctx, stack);		\
		}							\
                                                                        \
                handle;							\
            }),                                                         \
        unsigned short: ({						\
		CGNThreadHandle_unsignedshort handle;			\
									\
		void *stack;						\
                __CGNThread *t = __cgn_add_thread(&handle.id, &stack);	\
		/* Must reassign t so it is available with new stack */	\
		/* when execution jumps back */				\
                t = __cgn_savectx(&t->ctx, t);				\
                                                                        \
                _Bool temp_run_toggle = t->run_toggle;			\
                t->run_toggle = !t->run_toggle;				\
                                                                        \
                if (temp_run_toggle) {					\
                    uint64_t retval = (uint64_t) Fn;			\
                    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
                    curr_thread->return_val = retval;			\
                    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
                    __cgn_scheduler();					\
                } else {						\
		    __cgn_set_stack_ptr(&t->ctx, stack);		\
		}							\
                                                                        \
                handle;							\
            }),                                                         \
        int: ({								\
                CGNThreadHandle_int handle;                             \
									\
                void *stack;						\
                __CGNThread *t = __cgn_add_thread(&handle.id, &stack);	\
		/* Must reassign t so it is available with new stack */	\
		/* when execution jumps back */				\
                t = __cgn_savectx(&t->ctx, t);				\
                                                                        \
                _Bool temp_run_toggle = t->run_toggle;			\
                t->run_toggle = !t->run_toggle;				\
                                                                        \
                if (temp_run_toggle) {					\
                    uint64_t retval = (uint64_t) Fn;			\
                    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
                    curr_thread->return_val = retval;			\
                    curr_thread->state = __CGN_THREAD_STATE_DONE;       \
                    __cgn_scheduler();                                  \
                } else {						\
		    __cgn_set_stack_ptr(&t->ctx, stack);		\
		}							\
                                                                        \
                handle;                                                 \
            }),                                                         \
        unsigned int: ({                                                \
		CGNThreadHandle_unsignedint handle;			\
									\
		void *stack;						\
                __CGNThread *t = __cgn_add_thread(&handle.id, &stack);	\
		/* Must reassign t so it is available with new stack */	\
		/* when execution jumps back */				\
                t = __cgn_savectx(&t->ctx, t);				\
                                                                        \
                _Bool temp_run_toggle = t->run_toggle;			\
                t->run_toggle = !t->run_toggle;				\
                                                                        \
                if (temp_run_toggle) {					\
                    uint64_t retval = (uint64_t) Fn;			\
                    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
                    curr_thread->return_val = retval;			\
                    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
                    __cgn_scheduler();					\
                } else {						\
		    __cgn_set_stack_ptr(&t->ctx, stack);		\
		}							\
                                                                        \
                handle;							\
            }),                                                         \
        long: ({                                                        \
                CGNThreadHandle_long handle;				\
									\
                void *stack;						\
                __CGNThread *t = __cgn_add_thread(&handle.id, &stack);	\
		/* Must reassign t so it is available with new stack */	\
		/* when execution jumps back */				\
                t = __cgn_savectx(&t->ctx, t);				\
                                                                        \
                _Bool temp_run_toggle = t->run_toggle;			\
                t->run_toggle = !t->run_toggle;				\
                                                                        \
                if (temp_run_toggle) {					\
                    uint64_t retval = (uint64_t) Fn;			\
                    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
                    curr_thread->return_val = retval;			\
                    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
                    __cgn_scheduler();					\
                } else {						\
		    __cgn_set_stack_ptr(&t->ctx, stack);		\
		}							\
                                                                        \
                handle;							\
            }),                                                         \
        unsigned long: ({						\
                CGNThreadHandle_unsignedlong handle;			\
									\
                void *stack;						\
                __CGNThread *t = __cgn_add_thread(&handle.id, &stack);	\
		/* Must reassign t so it is available with new stack */	\
		/* when execution jumps back */				\
                t = __cgn_savectx(&t->ctx, t);				\
                                                                        \
                _Bool temp_run_toggle = t->run_toggle;			\
                t->run_toggle = !t->run_toggle;				\
                                                                        \
                if (temp_run_toggle) {					\
                    uint64_t retval = (uint64_t) Fn;			\
                    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
                    curr_thread->return_val = retval;			\
                    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
                    __cgn_scheduler();					\
                } else {						\
		    __cgn_set_stack_ptr(&t->ctx, stack);		\
		}							\
                                                                        \
                handle;							\
            }),                                                         \
        long long: ({							\
                CGNThreadHandle_longlong handle;                        \
									\
                void *stack;						\
                __CGNThread *t = __cgn_add_thread(&handle.id, &stack);	\
		/* Must reassign t so it is available with new stack */	\
		/* when execution jumps back */				\
                t = __cgn_savectx(&t->ctx, t);				\
                                                                        \
                _Bool temp_run_toggle = t->run_toggle;			\
                t->run_toggle = !t->run_toggle;				\
                                                                        \
                if (temp_run_toggle) {					\
                    uint64_t retval = (uint64_t) Fn;			\
                    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
                    curr_thread->return_val = retval;			\
                    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
                    __cgn_scheduler();					\
                } else {						\
		    __cgn_set_stack_ptr(&t->ctx, stack);		\
		}							\
                                                                        \
                handle;							\
            }),                                                         \
        unsigned long long: ({						\
                CGNThreadHandle_unsignedlonglong handle;                \
									\
                void *stack;						\
                __CGNThread *t = __cgn_add_thread(&handle.id, &stack);	\
		/* Must reassign t so it is available with new stack */	\
		/* when execution jumps back */				\
                t = __cgn_savectx(&t->ctx, t);				\
                                                                        \
                _Bool temp_run_toggle = t->run_toggle;			\
                t->run_toggle = !t->run_toggle;				\
                                                                        \
                if (temp_run_toggle) {					\
                    uint64_t retval = (uint64_t) Fn;			\
                    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
                    curr_thread->return_val = retval;			\
                    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
                    __cgn_scheduler();					\
                } else {						\
		    __cgn_set_stack_ptr(&t->ctx, stack);		\
		}							\
                                                                        \
                handle;							\
            }),                                                         \
        float: ({							\
                CGNThreadHandle_float handle;				\
									\
                void *stack;						\
                __CGNThread *t = __cgn_add_thread(&handle.id, &stack);	\
		/* Must reassign t so it is available with new stack */	\
		/* when execution jumps back */				\
                t = __cgn_savectx(&t->ctx, t);				\
                                                                        \
                _Bool temp_run_toggle = t->run_toggle;			\
                t->run_toggle = !t->run_toggle;				\
                                                                        \
                if (temp_run_toggle) {					\
                    uint64_t retval = (uint64_t) Fn;			\
                    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
                    curr_thread->return_val = retval;			\
                    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
                    __cgn_scheduler();					\
                } else {						\
		    __cgn_set_stack_ptr(&t->ctx, stack);		\
		}							\
                                                                        \
                handle;							\
            }),                                                         \
        double: ({							\
                CGNThreadHandle_double handle;				\
									\
                void *stack;						\
                __CGNThread *t = __cgn_add_thread(&handle.id, &stack);	\
		/* Must reassign t so it is available with new stack */	\
		/* when execution jumps back */				\
                t = __cgn_savectx(&t->ctx, t);				\
                                                                        \
                _Bool temp_run_toggle = t->run_toggle;			\
                t->run_toggle = !t->run_toggle;				\
                                                                        \
                if (temp_run_toggle) {					\
                    uint64_t retval = (uint64_t) Fn;			\
                    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
                    curr_thread->return_val = retval;			\
                    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
                    __cgn_scheduler();					\
                } else {						\
		    __cgn_set_stack_ptr(&t->ctx, stack);		\
		}							\
                                                                        \
                handle;							\
            }),                                                         \
        void *: ({							\
		CGNThreadHandle_voidptr handle;				\
									\
		void *stack;						\
                __CGNThread *t = __cgn_add_thread(&handle.id, &stack);	\
		/* Must reassign t so it is available with new stack */	\
		/* when execution jumps back */				\
                t = __cgn_savectx(&t->ctx, t);				\
                                                                        \
                _Bool temp_run_toggle = t->run_toggle;			\
                t->run_toggle = !t->run_toggle;				\
                                                                        \
                if (temp_run_toggle) {					\
                    uint64_t retval = (uint64_t) Fn;			\
                    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
                    curr_thread->return_val = retval;			\
                    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
                    __cgn_scheduler();					\
                } else {						\
		    __cgn_set_stack_ptr(&t->ctx, stack);		\
		}							\
                                                                        \
                handle;							\
            }),                                                         \
        default: ({							\
                CGNThreadHandle_void handle;				\
									\
                void *stack;						\
                __CGNThread *t = __cgn_add_thread(&handle.id, &stack);	\
		/* Must reassign t so it is available with new stack */	\
		/* when execution jumps back */				\
                t = __cgn_savectx(&t->ctx, t);				\
                                                                        \
                _Bool temp_run_toggle = t->run_toggle;			\
                t->run_toggle = !t->run_toggle;				\
                                                                        \
                if (temp_run_toggle) {					\
                    Fn;							\
                    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
                    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
                    __cgn_scheduler();					\
                } else {						\
		    __cgn_set_stack_ptr(&t->ctx, stack);		\
		}							\
                                                                        \
                handle;							\
            }))

#define SEAGREEN_H
#endif
