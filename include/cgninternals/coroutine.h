#ifndef CGN_COROUTINE_H

#include <stdint.h>

#if defined(__x86_64__) && (defined(__unix__) || defined(__APPLE__))

typedef struct __CGNThreadCtx_ {
    uint64_t rsp;
    uint64_t rbp;

    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rbx;
} __CGNThreadCtx;

#elif defined(__x86_64__) && defined(_WIN64)

typedef struct __CGNThreadCtx_ {
    uint64_t rsp;
    uint64_t rbp;

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

extern void __cgn_ctxswitch(__CGNThreadCtx *oldctx, __CGNThreadCtx *newctx);
extern void __cgn_getctx(__CGNThreadCtx *ctx);

#define CGN_COROUTINE_H
#endif
