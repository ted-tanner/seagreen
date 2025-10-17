#if defined(_WIN64) || defined(__APPLE__)
#define __CGN_LOADCTX_SYMBOL ___cgn_loadctx
#define __CGN_JUMPWITHSTACK_SYMBOL ___cgn_jumpwithstack
#else
#define __CGN_LOADCTX_SYMBOL __cgn_loadctx
#define __CGN_JUMPWITHSTACK_SYMBOL __cgn_jumpwithstack
#endif

#include "../include/seagreen_ctx.h"

.globl __CGN_LOADCTX_SYMBOL
.globl __CGN_JUMPWITHSTACK_SYMBOL

#if defined(__x86_64__) && (defined(__unix__) || defined(__APPLE__))

.balign 4
__CGN_LOADCTX_SYMBOL:
    movq 0(%rdi), %rsp

    leaq -__CGN_CTX_SAVE_SIZE(%rsp), %r10

    movq 0(%r10), %rbp
    movq 8(%r10), %r11
    movq 16(%r10), %r12
    movq 24(%r10), %r13
    movq 32(%r10), %r14
    movq 40(%r10), %r15
    movq 48(%r10), %rbx

    movq $1, %rax
    jmp *%r11

.balign 4
__CGN_JUMPWITHSTACK_SYMBOL:
    movq %rsi, %rsp
    jmp *%rdi

#elif defined(__x86_64__) && defined(_WIN64)

__CGN_LOADCTX_SYMBOL_ret_stub:
    jmp *%rax

.balign 4
__CGN_LOADCTX_SYMBOL:
    movq 0(%rcx), %rsp

    leaq -__CGN_CTX_SAVE_SIZE(%rsp), %r10

    movq 0(%r10), %rbp
    movq 8(%r10), %r11
    movq 16(%r10), %r12
    movq 24(%r10), %r13
    movq 32(%r10), %r14
    movq 40(%r10), %r15
    movq 48(%r10), %rbx
    movq 56(%r10), %rdi
    movq 64(%r10), %rsi

    movdqu 72(%r10), %xmm6
    movdqu 88(%r10), %xmm7
    movdqu 104(%r10), %xmm8
    movdqu 120(%r10), %xmm9
    movdqu 136(%r10), %xmm10
    movdqu 152(%r10), %xmm11
    movdqu 168(%r10), %xmm12
    movdqu 184(%r10), %xmm13
    movdqu 200(%r10), %xmm14
    movdqu 216(%r10), %xmm15

    ldmxcsr 232(%r10)

    movq $1, %rax
    jmp *%r11

.balign 4
__CGN_JUMPWITHSTACK_SYMBOL:
    movq %rdx, %rsp
    jmp *%rcx

#elif defined(__aarch64__)

.balign 4
__CGN_LOADCTX_SYMBOL:
    ldr x9, [x0, 0]
    mov sp, x9

    sub x9, sp, #__CGN_CTX_SAVE_SIZE

    ldp lr, x19, [x9, 0]
    ldp x20, x21, [x9, 16]
    ldp x22, x23, [x9, 32]
    ldp x24, x25, [x9, 48]
    ldp x26, x27, [x9, 64]
    ldp x28, x29, [x9, 80]

    ldr w10, [x9, 96]
    msr FPCR, x10
    ldr w10, [x9, 100]
    msr FPSR, x10

    ldp q8, q9, [x9, 112]
    ldp q10, q11, [x9, 144]
    ldp q12, q13, [x9, 176]
    ldp q14, q15, [x9, 208]

    mov w0, #1
    br lr

.balign 4
__CGN_JUMPWITHSTACK_SYMBOL:
    mov sp, x1
    br x0

#elif defined(__riscv__)

.balign 4
__CGN_LOADCTX_SYMBOL:
    ld sp, 0(a0)

    addi t0, sp, -__CGN_CTX_SAVE_SIZE

    ld ra, 0(t0)
    ld s0, 8(t0)
    ld s1, 16(t0)
    ld s2, 24(t0)
    ld s3, 32(t0)
    ld s4, 40(t0)
    ld s5, 48(t0)
    ld s6, 56(t0)
    ld s7, 64(t0)
    ld s8, 72(t0)
    ld s9, 80(t0)
    ld s10, 88(t0)
    ld s11, 96(t0)

    li a0, 1
    ret

.balign 4
__CGN_JUMPWITHSTACK_SYMBOL:
    mv sp, a1
    jr a0

#endif
