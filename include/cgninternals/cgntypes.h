#ifndef CGN_TYPES_H

#include <stdint.h>

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

#else

#error "seagreenlib does not support this architecture"

#endif

// Macros that typedef don't support spaces in type names
typedef signed char signedchar;
typedef unsigned char unsignedchar;
typedef unsigned short unsignedshort;
typedef unsigned int unsignedint;
typedef unsigned long unsignedlong;
typedef long long longlong;
typedef unsigned long long unsignedlonglong;
typedef void *voidptr;

#define __cgn_define_handle_type(T)             \
    typedef struct _CGNThreadHandle_##T {       \
        uint64_t id;                            \
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

// After some testing, it appears that 1024 is the sweet spot. 2048 doesn't
// speed things up under load, and 512 is slower than 1024.
#define __CGN_THREAD_BLOCK_SIZE 1024

typedef struct __CGNThread_ {
    uint64_t id;

    uint64_t awaited_thread_id;
    uint64_t awaiting_thread_count;

    uint64_t return_val;
    uint64_t scratch;
    uint64_t scratch2;

    __CGNThreadCtx ctx;
    __CGNThreadState state;

    _Bool yield_toggle;
    _Bool should_run;

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

#define CGN_TYPES_H
#endif