#ifndef SEAGREEN_H

#if !defined __GNUC__ && !defined __clang__
#error "seagreenlib can only be compiled with GCC or Clang"
#endif

#include <stdint.h>
#include <stdlib.h>

#include "cgninternals/coroutine.h"

#define __cgn_check_malloc(ptr) if (!ptr) abort();

typedef signed char __cgn_signedchar;
typedef unsigned char __cgn_unsignedchar;
typedef unsigned short __cgn_unsignedshort;
typedef unsigned int __cgn_unsignedint;
typedef unsigned long __cgn_unsignedlong;
typedef long long __cgn_longlong;
typedef unsigned long long __cgn_unsignedlonglong;
typedef long double __cgn_longdouble;
typedef void *__cgn_voidptr;

#define __cgn_define_handle_type(T)		\
    typedef struct _CGNThreadHandle_##T {	\
	uint64_t pos;				\
    } CGNThreadHandle_##T

__cgn_define_handle_type(void);
__cgn_define_handle_type(char);
__cgn_define_handle_type(__cgn_signedchar);
__cgn_define_handle_type(__cgn_unsignedchar);
__cgn_define_handle_type(short);
__cgn_define_handle_type(__cgn_unsignedshort);
__cgn_define_handle_type(int);
__cgn_define_handle_type(__cgn_unsignedint);
__cgn_define_handle_type(long);
__cgn_define_handle_type(__cgn_unsignedlong);
__cgn_define_handle_type(__cgn_longlong);
__cgn_define_handle_type(__cgn_unsignedlonglong);
__cgn_define_handle_type(float);
__cgn_define_handle_type(double);
__cgn_define_handle_type(__cgn_longdouble);
__cgn_define_handle_type(__cgn_voidptr);

typedef enum __CGNThreadState_ {
    __CGN_THREAD_STATE_READY,
    __CGN_THREAD_STATE_RUNNING,
    __CGN_THREAD_STATE_WAITING,
    __CGN_THREAD_STATE_DONE,
} __CGNThreadState;

typedef struct __CGNThread_ {
    __CGNThreadState state;
    __CGNThreadCtx ctx;

    uint64_t awaited_thread_pos;
    uint64_t return_val;
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

__CGNThreadBlock *__cgn_get_block(uint64_t pos);
__CGNThread *__cgn_get_thread(uint64_t pos);
__CGNThread *__cgn_add_thread(uint64_t *pos);
void __cgn_remove_thread(__CGNThreadBlock *block, uint64_t pos);

__CGNThread *__cgn_get_curr_thread(void);

#define async __attribute__((noinline))
#define async_yield()						\
    __cgn_get_curr_thread()->state = __CGN_THREAD_STATE_READY;	\
    __cgn_scheduler()

#define await(handle)							\
    _Generic((handle),							\
	     CGNThreadHandle_void: ({					\
		     __cgn_get_curr_thread()->awaited_thread_pos = (handle).pos; \
		     __cgn_get_curr_thread()->state = __CGN_THREAD_STATE_WAITING; \
		     async_yield();					\
		     uint64_t pos = (handle).pos % 64;			\
		     __CGNThreadBlock *block = __cgn_get_block((handle).pos); \
		     __cgn_remove_thread(block, pos);			\
		     (void)0;						\
		 }),							\
	     default: ({						\
		     __cgn_get_curr_thread()->awaited_thread_pos = (handle).pos; \
		     __cgn_get_curr_thread()->state = __CGN_THREAD_STATE_WAITING; \
		     async_yield();					\
		     uint64_t pos = (handle).pos % 64;			\
		     __CGNThreadBlock *block = __cgn_get_block((handle).pos); \
		     uint64_t return_val = block->threads[pos].return_val; \
		     __cgn_remove_thread(block, pos);			\
		     return_val;					\
		 }))

#define async_run(V)							\
    _Generic(								\
	(V),								\
	char: ({							\
		CGNThreadHandle_char handle;                            \
		__CGNThread *t = __cgn_add_thread(&handle.pos);         \
		__cgn_getctx(&t->ctx);                                  \
		static _Bool should_run_##__FILE__##__LINE__ = 0;       \
		if (should_run_##__FILE__##__LINE__)  {			\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;       \
		    __cgn_scheduler();                                  \
		} else {						\
		    should_run_##__FILE__##__LINE__ = 1;                \
		}                                                       \
		handle;                                                 \
	    }),                                                         \
	signed char: ({							\
		CGNThreadHandle___cgn_signedchar handle;		\
		__CGNThread *t = __cgn_add_thread(&handle.pos);		\
		__cgn_getctx(&t->ctx);					\
		static _Bool should_run_##__FILE__##__LINE__ = 0;	\
		if (should_run_##__FILE__##__LINE__) {			\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		} else {						\
		    should_run_##__FILE__##__LINE__ = 1;		\
		}							\
		handle;							\
	    }),                                                         \
	unsigned char: ({						\
		CGNThreadHandle___cgn_unsignedchar handle;		\
		__CGNThread *t = __cgn_add_thread(&handle.pos);		\
		__cgn_getctx(&t->ctx);					\
		static _Bool should_run_##__FILE__##__LINE__ = 0;	\
		if (should_run_##__FILE__##__LINE__) {			\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		} else {						\
		    should_run_##__FILE__##__LINE__ = 1;		\
		}							\
		handle;							\
	    }),                                                         \
	short: ({							\
		CGNThreadHandle_short handle;				\
		__CGNThread *t = __cgn_add_thread(&handle.pos);		\
		__cgn_getctx(&t->ctx);					\
		static _Bool should_run_##__FILE__##__LINE__ = 0;	\
		if (should_run_##__FILE__##__LINE__) {			\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		} else {						\
		    should_run_##__FILE__##__LINE__ = 1;		\
		}							\
		handle;							\
	    }),                                                         \
	unsigned short: ({						\
		CGNThreadHandle___cgn_unsignedshort handle;		\
		__CGNThread *t = __cgn_add_thread(&handle.pos);		\
		__cgn_getctx(&t->ctx);					\
		static _Bool should_run_##__FILE__##__LINE__ = 0;	\
		if (should_run_##__FILE__##__LINE__) {			\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		} else {						\
		    should_run_##__FILE__##__LINE__ = 1;		\
		}							\
		handle;							\
	    }),                                                         \
	int: ({								\
		CGNThreadHandle_int handle;                             \
		__CGNThread *t = __cgn_add_thread(&handle.pos);         \
		__cgn_getctx(&t->ctx);                                  \
		static _Bool should_run_##__FILE__##__LINE__ = 0;       \
		if (should_run_##__FILE__##__LINE__)                    \
		{                                                       \
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;       \
		    __cgn_scheduler();                                  \
		}                                                       \
		else                                                    \
		{                                                       \
		    should_run_##__FILE__##__LINE__ = 1;                \
		}                                                       \
		handle;                                                 \
	    }),                                                         \
	unsigned int: ({						\
		CGNThreadHandle___cgn_unsignedint handle;		\
		__CGNThread *t = __cgn_add_thread(&handle.pos);		\
		__cgn_getctx(&t->ctx);					\
		static _Bool should_run_##__FILE__##__LINE__ = 0;	\
		if (should_run_##__FILE__##__LINE__) {			\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		} else {						\
		    should_run_##__FILE__##__LINE__ = 1;		\
		}							\
		handle;							\
	    }),                                                         \
	long: ({							\
		CGNThreadHandle_long handle;				\
		__CGNThread *t = __cgn_add_thread(&handle.pos);		\
		__cgn_getctx(&t->ctx);					\
		static _Bool should_run_##__FILE__##__LINE__ = 0;	\
		if (should_run_##__FILE__##__LINE__) {			\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		} else {						\
		    should_run_##__FILE__##__LINE__ = 1;		\
		}							\
		handle;							\
	    }),                                                         \
	unsigned long: ({						\
		CGNThreadHandle___cgn_unsignedlong handle;		\
		__CGNThread *t = __cgn_add_thread(&handle.pos);		\
		__cgn_getctx(&t->ctx);					\
		static _Bool should_run_##__FILE__##__LINE__ = 0;	\
		if (should_run_##__FILE__##__LINE__) {			\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		} else {						\
		    should_run_##__FILE__##__LINE__ = 1;		\
		}							\
		handle;							\
	    }),                                                         \
	long long: ({							\
		CGNThreadHandle___cgn_longlong handle;			\
		__CGNThread *t = __cgn_add_thread(&handle.pos);		\
		__cgn_getctx(&t->ctx);					\
		static _Bool should_run_##__FILE__##__LINE__ = 0;	\
		if (should_run_##__FILE__##__LINE__) {			\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		} else {						\
		    should_run_##__FILE__##__LINE__ = 1;		\
		}							\
		handle;							\
	    }),                                                         \
	unsigned long long: ({						\
		CGNThreadHandle___cgn_unsignedlonglong handle;		\
		__CGNThread *t = __cgn_add_thread(&handle.pos);		\
		__cgn_getctx(&t->ctx);					\
		static _Bool should_run_##__FILE__##__LINE__ = 0;	\
		if (should_run_##__FILE__##__LINE__) {			\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		} else {						\
		    should_run_##__FILE__##__LINE__ = 1;		\
		}							\
		handle;							\
	    }),                                                         \
	float: ({							\
		CGNThreadHandle_float handle;				\
		__CGNThread *t = __cgn_add_thread(&handle.pos);		\
		__cgn_getctx(&t->ctx);					\
		static _Bool should_run_##__FILE__##__LINE__ = 0;	\
		if (should_run_##__FILE__##__LINE__) {			\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		} else {						\
		    should_run_##__FILE__##__LINE__ = 1;		\
		}							\
		handle;							\
	    }),                                                         \
	double: ({							\
		CGNThreadHandle_double handle;				\
		__CGNThread *t = __cgn_add_thread(&handle.pos);		\
		__cgn_getctx(&t->ctx);					\
		static _Bool should_run_##__FILE__##__LINE__ = 0;	\
		if (should_run_##__FILE__##__LINE__) {			\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		} else {						\
		    should_run_##__FILE__##__LINE__ = 1;		\
		}							\
		handle;							\
	    }),                                                         \
	long double: ({							\
		CGNThreadHandle___cgn_longdouble handle;		\
		__CGNThread *t = __cgn_add_thread(&handle.pos);		\
		__cgn_getctx(&t->ctx);					\
		static _Bool should_run_##__FILE__##__LINE__ = 0;	\
		if (should_run_##__FILE__##__LINE__) {			\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		} else {						\
		    should_run_##__FILE__##__LINE__ = 1;		\
		}							\
		handle;							\
	    }),                                                         \
	void *: ({							\
		CGNThreadHandle___cgn_voidptr handle;			\
		__CGNThread *t = __cgn_add_thread(&handle.pos);		\
		__cgn_getctx(&t->ctx);					\
		static _Bool should_run_##__FILE__##__LINE__ = 0;	\
		if (should_run_##__FILE__##__LINE__) {			\
		    uint64_t retval = (uint64_t) V;			\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->return_val = retval;			\
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		} else {						\
		    should_run_##__FILE__##__LINE__ = 1;		\
		}							\
		handle;							\
	    }),                                                         \
	default: ({							\
		CGNThreadHandle_void handle;				\
		__CGNThread *t = __cgn_add_thread(&handle.pos);		\
		__cgn_getctx(&t->ctx);					\
		static _Bool should_run_##__FILE__##__LINE__ = 0;	\
		if (should_run_##__FILE__##__LINE__) {			\
		    V;							\
		    __CGNThread *curr_thread = __cgn_get_curr_thread(); \
		    curr_thread->state = __CGN_THREAD_STATE_DONE;	\
		    __cgn_scheduler();					\
		} else {						\
		    should_run_##__FILE__##__LINE__ = 1;		\
		}							\
		handle;							\
	    }))

// TODO: Once multithreading is supported, create a way to pool blocking threads
// that can pass off waiting to another thread, then return to the scheduler
// (blocking thread will mark the thread as completed)

#ifdef CGN_TEST
#include "cgntest/test.h"
void register_seagreen_tests();
#endif

#define SEAGREEN_H
#endif
