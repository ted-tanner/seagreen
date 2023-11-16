#ifndef SEAGREEN_H

#if !defined __GNUC__ && !defined __clang__
#error "seagreenlib can only be compiled with GCC or Clang"
#endif

#include <stdint.h>
#include <stdlib.h>

#include "cgninternals/coroutine.h"

#ifdef CGN_DEBUG
void print_threads(void);
#endif

#define __cgn_check_malloc(ptr) if (!ptr) abort();

typedef signed char signedchar;
typedef unsigned char unsignedchar;
typedef unsigned short unsignedshort;
typedef unsigned int unsignedint;
typedef unsigned long unsignedlong;
typedef long long longlong;
typedef unsigned long long unsignedlonglong;
typedef long double longdouble;
typedef void *voidptr;

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
__cgn_define_handle_type(longdouble);
__cgn_define_handle_type(voidptr);

typedef enum __CGNThreadState_ {
    __CGN_THREAD_STATE_READY,
    __CGN_THREAD_STATE_RUNNING,
    __CGN_THREAD_STATE_WAITING,
    __CGN_THREAD_STATE_DONE,
} __CGNThreadState;

typedef struct __CGNThread_ {
    __CGNThreadState state;
    __CGNThreadCtx ctx;

    uint64_t awaited_thread_id;
    uint64_t awaiting_thread_count;
    uint64_t return_val;

    void *stack;

    _Bool yield_toggle;
    _Bool run_toggle;
} __CGNThread;

typedef struct __CGNThreadBlock_ {
    struct __CGNThreadBlock_ *next;
    struct __CGNThreadBlock_ *prev;

    uint64_t unused_threads;
    __CGNThread threads[64];
} __CGNThreadBlock;

typedef struct __CGNThreadList_ {
    __CGNThreadBlock *head;
    __CGNThreadBlock *tail;

    uint64_t block_count;
    uint64_t thread_count;
} __CGNThreadList;

void seagreen_init_rt(void);
void seagreen_free_rt(void);

__attribute__((noreturn)) void __cgn_scheduler(void);

__CGNThreadBlock *__cgn_get_block(uint64_t id);
__CGNThread *__cgn_get_thread(uint64_t id);
__CGNThread *__cgn_get_thread_by_block(__CGNThreadBlock *block, uint64_t pos);
__CGNThread *__cgn_add_thread(uint64_t *id);
__CGNThread *__cgn_add_thread_keep_stack(uint64_t *id);
void __cgn_remove_thread(__CGNThreadBlock *block, uint64_t pos);

__CGNThread *__cgn_get_curr_thread(void);

#define async __attribute__((noinline))

#define async_yield() {					\
	__CGNThread *t = __cgn_get_curr_thread();	\
	__cgn_savectx(&t->ctx);				\
							\
	_Bool temp_yield_toggle = t->yield_toggle;	\
	t->yield_toggle = !t->yield_toggle;		\
							\
	if (temp_yield_toggle) {			\
	    __cgn_scheduler();				\
	}						\
    }

#define await(handle)							\
    _Generic((handle),							\
	     CGNThreadHandle_void: ({					\
		     __CGNThread *t_curr = __cgn_get_curr_thread();	\
		     t_curr->awaited_thread_id = (handle).id;		\
		     t_curr->state = __CGN_THREAD_STATE_WAITING;	\
									\
		     uint64_t pos = (handle).id % 64;			\
		     __CGNThreadBlock *block = __cgn_get_block((handle).id); \
		     							\
		     __CGNThread *t = &block->threads[pos];		\
		     t->awaiting_thread_count++;			\
									\
		     /* Because thread is waiting, the curr thread */	\
		     /* won't be scheduled until awaited thread has */	\
		     /* finished its execution */			\
		     async_yield();					\
									\
		     if (!t->awaiting_thread_count) {			\
			 __cgn_remove_thread(block, pos);		\
		     }							\
									\
		     (void)0;						\
		 }),							\
	     default: ({						\
		     __CGNThread *t_curr = __cgn_get_curr_thread();	\
		     t_curr->awaited_thread_id = (handle).id;		\
		     t_curr->state = __CGN_THREAD_STATE_WAITING;	\
									\
		     uint64_t pos = (handle).id % 64;			\
		     __CGNThreadBlock *block = __cgn_get_block((handle).id); \
		     							\
		     __CGNThread *t = &block->threads[pos];		\
		     t->awaiting_thread_count++;			\
									\
		     /* Because thread is waiting, the curr thread */	\
		     /* won't be scheduled until awaited thread has */	\
		     /* finished its execution */			\
		     async_yield();					\
									\
		     uint64_t return_val = block->threads[pos].return_val; \
		     if (!t->awaiting_thread_count) {			\
			 __cgn_remove_thread(block, pos);		\
		     }							\
									\
		     return_val;					\
		 }))

#define async_run(V)							\
    _Generic(								\
	(V),								\
	char: ({							\
		CGNThreadHandle_char handle;                            \
		__CGNThread *t = __cgn_add_thread(&handle.id);		\
		__cgn_savectx(&t->ctx);					\
									\
		_Bool temp_run_toggle = t->run_toggle;			\
		t->run_toggle = !t->run_toggle;				\
									\
		if (temp_run_toggle)  {					\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;       \
		    __cgn_scheduler();                                  \
		}							\
									\
		handle;							\
	    }),                                                         \
	signed char: ({							\
		CGNThreadHandle_signedchar handle;			\
		__CGNThread *t = __cgn_add_thread(&handle.id);		\
		__cgn_savectx(&t->ctx);					\
									\
		_Bool temp_run_toggle = t->run_toggle;			\
		t->run_toggle = !t->run_toggle;				\
									\
		if (temp_run_toggle) {					\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		}							\
									\
		handle;							\
	    }),                                                         \
	unsigned char: ({						\
		CGNThreadHandle_unsignedchar handle;			\
		__CGNThread *t = __cgn_add_thread(&handle.id);		\
		__cgn_savectx(&t->ctx);					\
									\
		_Bool temp_run_toggle = t->run_toggle;			\
		t->run_toggle = !t->run_toggle;				\
									\
		if (temp_run_toggle) {					\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		}							\
									\
		handle;							\
	    }),                                                         \
	short: ({							\
		CGNThreadHandle_short handle;				\
		__CGNThread *t = __cgn_add_thread(&handle.id);		\
		__cgn_savectx(&t->ctx);					\
									\
		_Bool temp_run_toggle = t->run_toggle;			\
		t->run_toggle = !t->run_toggle;				\
									\
		if (temp_run_toggle) {					\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		}							\
									\
		handle;							\
	    }),                                                         \
	unsigned short: ({						\
		CGNThreadHandle_unsignedshort handle;			\
		__CGNThread *t = __cgn_add_thread(&handle.id);		\
		__cgn_savectx(&t->ctx);					\
									\
		_Bool temp_run_toggle = t->run_toggle;			\
		t->run_toggle = !t->run_toggle;				\
									\
		if (temp_run_toggle) {					\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		}							\
									\
		handle;							\
	    }),                                                         \
	int: ({								\
		CGNThreadHandle_int handle;                             \
		__CGNThread *t = __cgn_add_thread(&handle.id);		\
		__cgn_savectx(&t->ctx);					\
		__cgn_set_stack_ptr(&t->ctx, t->stack);			\
									\
		_Bool temp_run_toggle = t->run_toggle;			\
		t->run_toggle = !t->run_toggle;				\
									\
		if (temp_run_toggle) {					\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;       \
		    __cgn_scheduler();                                  \
		}							\
									\
		handle;                                                 \
	    }),                                                         \
	unsigned int: ({						\
		CGNThreadHandle_unsignedint handle;			\
		__CGNThread *t = __cgn_add_thread(&handle.id);		\
		__cgn_savectx(&t->ctx);					\
									\
		_Bool temp_run_toggle = t->run_toggle;			\
		t->run_toggle = !t->run_toggle;				\
									\
		if (temp_run_toggle) {					\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		}							\
									\
		handle;							\
	    }),                                                         \
	long: ({							\
		CGNThreadHandle_long handle;				\
		__CGNThread *t = __cgn_add_thread(&handle.id);		\
		__cgn_savectx(&t->ctx);					\
									\
		_Bool temp_run_toggle = t->run_toggle;			\
		t->run_toggle = !t->run_toggle;				\
									\
		if (temp_run_toggle) {					\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		}							\
									\
		handle;							\
	    }),                                                         \
	unsigned long: ({						\
		CGNThreadHandle_unsignedlong handle;			\
		__CGNThread *t = __cgn_add_thread(&handle.id);		\
		__cgn_savectx(&t->ctx);					\
									\
		_Bool temp_run_toggle = t->run_toggle;			\
		t->run_toggle = !t->run_toggle;				\
									\
		if (temp_run_toggle) {					\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		}							\
									\
		handle;							\
	    }),                                                         \
	long long: ({							\
		CGNThreadHandle_longlong handle;			\
		__CGNThread *t = __cgn_add_thread(&handle.id);		\
		__cgn_savectx(&t->ctx);					\
									\
		_Bool temp_run_toggle = t->run_toggle;			\
		t->run_toggle = !t->run_toggle;				\
									\
		if (temp_run_toggle) {					\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		}							\
									\
		handle;							\
	    }),                                                         \
	unsigned long long: ({						\
		CGNThreadHandle_unsignedlonglong handle;		\
		__CGNThread *t = __cgn_add_thread(&handle.id);		\
		__cgn_savectx(&t->ctx);					\
									\
		_Bool temp_run_toggle = t->run_toggle;			\
		t->run_toggle = !t->run_toggle;				\
									\
		if (temp_run_toggle) {					\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		}							\
									\
		handle;							\
	    }),                                                         \
	float: ({							\
		CGNThreadHandle_float handle;				\
		__CGNThread *t = __cgn_add_thread(&handle.id);		\
		__cgn_savectx(&t->ctx);					\
									\
		_Bool temp_run_toggle = t->run_toggle;			\
		t->run_toggle = !t->run_toggle;				\
									\
		if (temp_run_toggle) {					\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		}							\
									\
		handle;							\
	    }),                                                         \
	double: ({							\
		CGNThreadHandle_double handle;				\
		__CGNThread *t = __cgn_add_thread(&handle.id);		\
		__cgn_savectx(&t->ctx);					\
									\
		_Bool temp_run_toggle = t->run_toggle;			\
		t->run_toggle = !t->run_toggle;				\
									\
		if (temp_run_toggle) {					\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		}							\
									\
		handle;							\
	    }),                                                         \
	long double: ({							\
		CGNThreadHandle_longdouble handle;			\
		__CGNThread *t = __cgn_add_thread(&handle.id);		\
		__cgn_savectx(&t->ctx);					\
									\
		_Bool temp_run_toggle = t->run_toggle;			\
		t->run_toggle = !t->run_toggle;				\
									\
		if (temp_run_toggle) {					\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		}							\
									\
		handle;							\
	    }),                                                         \
	void *: ({							\
		CGNThreadHandle_voidptr handle;				\
		__CGNThread *t = __cgn_add_thread(&handle.id);		\
		__cgn_savectx(&t->ctx);					\
									\
		_Bool temp_run_toggle = t->run_toggle;			\
		t->run_toggle = !t->run_toggle;				\
									\
		if (temp_run_toggle) {					\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		}							\
									\
		handle;							\
	    }),                                                         \
	default: ({							\
		CGNThreadHandle_void handle;				\
		__CGNThread *t = __cgn_add_thread(&handle.id);		\
		__cgn_savectx(&t->ctx);					\
									\
		_Bool temp_run_toggle = t->run_toggle;			\
		t->run_toggle = !t->run_toggle;				\
									\
		if (temp_run_toggle) {					\
		    V;							\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		}							\
									\
		handle;							\
	    }))

#ifdef CGN_TEST
#include "cgntest/test.h"
void register_seagreen_tests();
#endif

#define SEAGREEN_H
#endif
