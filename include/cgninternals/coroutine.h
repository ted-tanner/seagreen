#ifndef CGN_COROUTINE_H

#include <stdint.h>

#if defined(__x86_64__)

typedef struct __CGNThreadCtx_ {
    uint64_t ret;
    uint64_t rsp;

    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
} __CGNThreadCtx;

#elif defined(__i386__)

typedef struct __CGNThreadCtx_ {
    uint32_t ret;
    uint32_t esp;

    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
} __CGNThreadCtx;

#elif defined(__aarch64__)

typedef struct __CGNThreadCtx_ {
    uint64_t lr; // Link register
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
} __CGNThxeadCtx;

#elif defined(__riscv__)
// TODO: Save the frame pointer
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

#error "Unsupported architecture"

#endif

void ctxswitch(__CGNThreadCtx *oldctx, __CGNThreadCtx *newctx);

#define CGN_COROUTINE_H
#endif
