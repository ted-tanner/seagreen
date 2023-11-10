#ifndef SEAGREEN_H

#if !defined __GNUC__ && !defined __clang__
#error "seagreenlib can only be compiled with GCC or Clang"
#endif

#include <stdint.h>
#include <stdlib.h>

#include "cgninternals/coroutine.h"

enum __CGNThreadState {
    __CGN_THREAD_STATE_READY,
    __CGN_THREAD_STATE_RUNNING,
    __CGN_THREAD_STATE_WAITING,
    __CGN_THREAD_STATE_DONE,
};

typedef struct __CGNThread_ {
    __CGNThreadState state;
    __CGNThreadCtx ctx;
} __CGNThread;

typedef struct __CGNThreadBlock_ {
    uint64_t unused_threads;

    struct __CGNThreadBlock_ *next;
    struct __CGNThreadBlock_ *prev;

    __CGNThread threads[64];
} __CGNThreadBlock;

typedef struct __CGNThreadList_ {
    __CGNThreadBlock *head;
    __CGNThreadBlock *tail;

    size_t block_count;
    size_t thread_count;
} __CGNThreadList;

#define __cgn_check_malloc(ptr) if (!ptr) abort();

void seagreen_init_rt(void);
// TODO: seagreen_free_rt()

typedef struct __cgn_async_value_ {
    uintmax_t value;
} __cgn_async_value;

typedef __cgn_async_value __cgn_async_void;

typedef __cgn_async_value __cgn_async_char;
typedef __cgn_async_value __cgn_async_unsigned_char;
typedef __cgn_async_value __cgn_async_short;
typedef __cgn_async_value __cgn_async_unsigned_short;
typedef __cgn_async_value __cgn_async_int;
typedef __cgn_async_value __cgn_async_unsigned_int;
typedef __cgn_async_value __cgn_async_long;
typedef __cgn_async_value __cgn_async_unsigned_long;
typedef __cgn_async_value __cgn_async_long_long;
typedef __cgn_async_value __cgn_async_unsigned_long_long;
typedef __cgn_async_value __cgn_async_float;
typedef __cgn_async_value __cgn_async_double;
typedef __cgn_async_value __cgn_async_long_double;

typedef __cgn_async_value __cgn_async_ptr;

typedef __cgn_async_value __cgn_async_int8_t;
typedef __cgn_async_value __cgn_async_uint8_t;
typedef __cgn_async_value __cgn_async_int16_t;
typedef __cgn_async_value __cgn_async_uint16_t;
typedef __cgn_async_value __cgn_async_int32_t;
typedef __cgn_async_value __cgn_async_uint32_t;
typedef __cgn_async_value __cgn_async_int64_t;
typedef __cgn_async_value __cgn_async_uint64_t;

typedef __cgn_async_value __cgn_async_int_least8_t;
typedef __cgn_async_value __cgn_async_uint_least8_t;
typedef __cgn_async_value __cgn_async_int_least16_t;
typedef __cgn_async_value __cgn_async_uint_least16_t;
typedef __cgn_async_value __cgn_async_int_least32_t;
typedef __cgn_async_value __cgn_async_uint_least32_t;
typedef __cgn_async_value __cgn_async_int_least64_t;
typedef __cgn_async_value __cgn_async_uint_least64_t;

typedef __cgn_async_value __cgn_async_int_fast8_t;
typedef __cgn_async_value __cgn_async_uint_fast8_t;
typedef __cgn_async_value __cgn_async_int_fast16_t;
typedef __cgn_async_value __cgn_async_uint_fast16_t;
typedef __cgn_async_value __cgn_async_int_fast32_t;
typedef __cgn_async_value __cgn_async_uint_fast32_t;
typedef __cgn_async_value __cgn_async_int_fast64_t;
typedef __cgn_async_value __cgn_async_uint_fast64_t;

typedef __cgn_async_value __cgn_async_intmax_t;
typedef __cgn_async_value __cgn_async_uintmax_t;

typedef __cgn_async_value __cgn_async_size_t;

void __cgn_scheduler(void) __attribute__((noreturn));

// Need as_async(f) to push work to blocking thread pool and check if
// function is done (then call __cgn_scheduler() if not done) each
// time green thread is scheduled
// #define as_async(f)

// TODO: Need async_ret(V) macro that saves return data and jumps to the
// scheduler
#define async_return(V) return _Generic(				\
	(T),								\
	void: ((__cgn_async_void) __cgn_async_value { .value = (uintmax_t) 0 }), \
	char: ((__cgn_async_char) __cgn_async_value { .value = (uintmax_t) (V) }), \
	unsigned char: ((__cgn_async_unsigned_char) __cgn_async_value { .value = (uintmax_t) (V) }), \
	short: ((__cgn_async_short) __cgn_async_value { .value = (uintmax_t) (V) }), \
	unsigned short: ((__cgn_async_unsigned_short) __cgn_async_value { .value = (uintmax_t) (V) }), \
	int: ((__cgn_async_int) __cgn_async_value { .value = (uintmax_t) (V) }), \
	unsigned int: ((__cgn_async_unsigned_int) __cgn_async_value { .value = (uintmax_t) (V) }), \
	long: ((__cgn_async_long) __cgn_async_value { .value = (uintmax_t) (V) }), \
	unsigned long: ((__cgn_async_unsigned_long) __cgn_async_value { .value = (uintmax_t) (V) }), \
	long long: ((__cgn_async_long_long) __cgn_async_value { .value = (uintmax_t) (V) }), \
	unsigned long long: ((__cgn_async_unsigned_long_long) __cgn_async_value { .value = (uintmax_t) (V) }), \
	float: ((__cgn_async_float) __cgn_async_value { .value = (uintmax_t) (V) }), \
	double: ((__cgn_async_double) __cgn_async_value { .value = (uintmax_t) (V) }), \
	long double: ((__cgn_async_long_double) __cgn_async_value { .value = (uintmax_t) (V) }), \
	void *: ((__cgn_async_ptr) __cgn_async_value { .value = (uintmax_t) (V) }), \
	int8_t: ((__cgn_async_int8_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	uint8_t: ((__cgn_async_uint8_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	int16_t: ((__cgn_async_int16_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	uint16_t: ((__cgn_async_uint16_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	int32_t: ((__cgn_async_int32_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	uint32_t: ((__cgn_async_uint32_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	int64_t: ((__cgn_async_int64_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	uint64_t: ((__cgn_async_uint64_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	int_least8_t: ((__cgn_async_int_least8_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	uint_least8_t: ((__cgn_async_uint_least8_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	int_least16_t: ((__cgn_async_int_least16_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	uint_least16_t: ((__cgn_async_uint_least16_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	int_least32_t: ((__cgn_async_int_least32_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	uint_least32_t: ((__cgn_async_uint_least32_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	int_least64_t: ((__cgn_async_int_least64_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	uint_least64_t: ((__cgn_async_uint_least64_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	int_fast8_t: ((__cgn_async_int_fast8_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	uint_fast8_t: ((__cgn_async_uint_fast8_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	int_fast16_t: ((__cgn_async_int_fast16_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	uint_fast16_t: ((__cgn_async_uint_fast16_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	int_fast32_t: ((__cgn_async_int_fast32_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	uint_fast32_t: ((__cgn_async_uint_fast32_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	int_fast64_t: ((__cgn_async_int_fast64_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	uint_fast64_t: ((__cgn_async_uint_fast64_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	intmax_t: ((__cgn_async_intmax_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	uintmax_t: ((__cgn_async_uintmax_t) __cgn_async_value { .value = (uintmax_t) (V) }), \
	size_t: ((__cgn_async_size_t) __cgn_async_value { .value = (uintmax_t) (V) }) \
	)

// TODO: Limit these to a subset that encompass compatible types. Separate
// typedefs will be needed for each type.
#define async(T) __attribute__((noinline)) _Generic(			\
	(T),								\
	void: __cgn_async_void,						\
	char: __cgn_async_char,						\
	signed char: __cgn_async_signed_char,			\
	unsigned char: __cgn_async_unsigned_char,			\
	short: __cgn_async_short,					\
	unsigned short: __cgn_async_unsigned_short,			\
	int: __cgn_async_int,						\
	unsigned int: __cgn_async_unsigned_int,				\
	long: __cgn_async_long,						\
	unsigned long: __cgn_async_unsigned_long,			\
	long long: __cgn_async_long_long,				\
	unsigned long long: __cgn_async_unsigned_long_long,		\
	float: __cgn_async_float,					\
	double: __cgn_async_double,					\
	long double: __cgn_async_long_double,				\
	void *: __cgn_async_ptr,					\
	int8_t: __cgn_async_int8_t,					\
	uint8_t: __cgn_async_uint8_t,					\
	int16_t: __cgn_async_int16_t,					\
	uint16_t: __cgn_async_uint16_t,					\
	int32_t: __cgn_async_int32_t,					\
	uint32_t: __cgn_async_uint32_t,					\
	int64_t: __cgn_async_int64_t,					\
	uint64_t: __cgn_async_uint64_t,					\
	int_least8_t: __cgn_async_int_least8_t,				\
	uint_least8_t: __cgn_async_uint_least8_t,			\
	int_least16_t: __cgn_async_int_least16_t,			\
	uint_least16_t: __cgn_async_uint_least16_t,			\
	int_least32_t: __cgn_async_int_least32_t,			\
	uint_least32_t: __cgn_async_uint_least32_t,			\
	int_least64_t: __cgn_async_int_least64_t,			\
	uint_least64_t: __cgn_async_uint_least64_t,			\
	int_fast8_t: __cgn_async_int_fast8_t,				\
	uint_fast8_t: __cgn_async_uint_fast8_t,				\
	int_fast16_t: __cgn_async_int_fast16_t,				\
	uint_fast16_t: __cgn_async_uint_fast16_t,			\
	int_fast32_t: __cgn_async_int_fast32_t,				\
	uint_fast32_t: __cgn_async_uint_fast32_t,			\
	int_fast64_t: __cgn_async_int_fast64_t,				\
	uint_fast64_t: __cgn_async_uint_fast64_t,			\
	intmax_t: __cgn_async_intmax_t,					\
	uintmax_t: __cgn_async_uintmax_t,				\
	size_t: __cgn_async_size_t					\
	)

#define async_yield() __cgn_scheduler()

// TODO: Once multithreading is supported, create a way to pool blocking threads
// that can pass off waiting to another thread, then return to the scheduler
// (blocking thread will mark the thread as completed)

#ifdef CGN_TEST
#include "cgntest/test.h"
void register_seagreen_tests();
#endif

#define SEAGREEN_H
#endif
