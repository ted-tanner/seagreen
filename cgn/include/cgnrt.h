#include <_types/_uint64_t.h>
#ifndef CGNRT_H

#include <stdint.h>
#include <stdlib.h>

typedef struct __CGNThread_ {
    void *data;
    size_t data_size;

    size_t pos;

    struct __CGNThread_ *next;

    // void *ret_addr;
    // __CGNRegisters saved_registers;
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

#define __CGN_CHECK_MALLOC(ptr) if (!ptr) abort();

void cgn_init_rt(void);

typedef struct __cgn_async_value_ {
    __CGNThread *thread;
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

uint64_t __cgn_scheduler();

// TODO: Need async_ret(V) macro that saves return data and jumps to the scheduler

#define async(T) __attribute__((noinline)) _Generic(			\
	(T),								\
	void: __cgn_async_void,						\
	char: __cgn_async_char,						\
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

#define await(V) _Generic(						\
	(T),								\
	__cgn_async_void: ((void) __cgn_scheduler()),			\
	__cgn_async_char: ((char) __cgn_scheduler()),			\
	__cgn_async_unsigned_char: ((unsigned char) __cgn_scheduler()),	\
	__cgn_async_short: ((short) __cgn_scheduler()),			\
	__cgn_async_unsigned_short: ((unsigned short) __cgn_scheduler()), \
	__cgn_async_int: ((int) __cgn_scheduler()),			\
	__cgn_async_unsigned_int: ((unsigned int) __cgn_scheduler()),	\
	__cgn_async_long: ((long) __cgn_scheduler()),			\
	__cgn_async_unsigned_long: ((unsigned long) __cgn_scheduler()),	\
	__cgn_async_long_long: ((long long) __cgn_scheduler()),		\
	__cgn_async_unsigned_long_long: ((unsigned long long) __cgn_scheduler()), \
	__cgn_async_float: ((float) __cgn_scheduler()),			\
	__cgn_async_double: ((double) __cgn_scheduler()),		\
	__cgn_async_long_double: ((long double) __cgn_scheduler()),	\
	__cgn_async_ptr: ((ptr) __cgn_scheduler()),			\
	__cgn_async_int8_t: ((int8_t) __cgn_scheduler()),		\
	__cgn_async_uint8_t: ((uint8_t) __cgn_scheduler()),		\
	__cgn_async_int16_t: ((int16_t) __cgn_scheduler()),		\
	__cgn_async_uint16_t: ((uint16_t) __cgn_scheduler()),		\
	__cgn_async_int32_t: ((int32_t) __cgn_scheduler()),		\
	__cgn_async_uint32_t: ((uint32_t) __cgn_scheduler()),		\
	__cgn_async_int64_t: ((int64_t) __cgn_scheduler()),		\
	__cgn_async_uint64_t: ((uint64_t) __cgn_scheduler()),		\
	__cgn_async_int_least8_t: ((int_least8_t) __cgn_scheduler()),	\
	__cgn_async_uint_least8_t: ((uint_least8_t) __cgn_scheduler()),	\
	__cgn_async_int_least16_t: ((int_least16_t) __cgn_scheduler()),	\
	__cgn_async_uint_least16_t: ((uint_least16_t) __cgn_scheduler()), \
	__cgn_async_int_least32_t: ((int_least32_t) __cgn_scheduler()),	\
	__cgn_async_uint_least32_t: ((uint_least32_t) __cgn_scheduler()), \
	__cgn_async_int_least64_t: ((int_least64_t) __cgn_scheduler()),	\
	__cgn_async_uint_least64_t: ((uint_least64_t) __cgn_scheduler()), \
	__cgn_async_int_fast8_t: ((int_fast8_t) __cgn_scheduler()),	\
	__cgn_async_uint_fast8_t: ((uint_fast8_t) __cgn_scheduler()),	\
	__cgn_async_int_fast16_t: ((int_fast16_t) __cgn_scheduler()),	\
	__cgn_async_uint_fast16_t: ((uint_fast16_t) __cgn_scheduler()),	\
	__cgn_async_int_fast32_t: ((int_fast32_t) __cgn_scheduler()),	\
	__cgn_async_uint_fast32_t: ((uint_fast32_t) __cgn_scheduler()),	\
	__cgn_async_int_fast64_t: ((int_fast64_t) __cgn_scheduler()),	\
	__cgn_async_uint_fast64_t: ((uint_fast64_t) __cgn_scheduler()),	\
	__cgn_async_intmax_t: ((intmax_t) __cgn_scheduler()),		\
	__cgn_async_uintmax_t: ((uintmax_t) __cgn_scheduler()),		\
	__cgn_async_size_t: ((size_t) __cgn_scheduler())		\
	)

#ifdef CGN_TEST
#include "cgntest/test.h"
void register_cgnrt_tests();
#endif

#define CGNRT_H
#endif
