#if defined(__APPLE__)
#define __CGN_SAVECTX_SYMBOL ___cgn_savectx
#define __CGN_SAVENEWCTX_SYMBOL ___cgn_savenewctx
#define __CGN_THREAD_ENTRY_SYMBOL ___cgn_thread_entry
#else
#define __CGN_SAVECTX_SYMBOL __cgn_savectx
#define __CGN_SAVENEWCTX_SYMBOL __cgn_savenewctx
#define __CGN_THREAD_ENTRY_SYMBOL __cgn_thread_entry
#endif

#include "../include/_seagreen_ctx.h"

.globl __CGN_SAVECTX_SYMBOL
.globl __CGN_SAVENEWCTX_SYMBOL

#if defined(__x86_64__) && (defined(__unix__) || defined(__APPLE__))

.balign 4
__CGN_SAVECTX_SYMBOL:
    movq %rsp, 0(%rdi)

    leaq -__CGN_CTX_SAVE_SIZE-128(%rsp), %r8

    movq 8(%rsp), %rax
    movq %rbp, 0(%r8)
    movq %rax, 8(%r8)
    movq %r12, 16(%r8)
    movq %r13, 24(%r8)
    movq %r14, 32(%r8)
    movq %r15, 40(%r8)
    movq %rbx, 48(%r8)

    movdqu %xmm0, 56(%r8)
    movdqu %xmm1, 72(%r8)
    movdqu %xmm2, 88(%r8)
    movdqu %xmm3, 104(%r8)
    movdqu %xmm4, 120(%r8)
    movdqu %xmm5, 136(%r8)
    movdqu %xmm6, 152(%r8)
    movdqu %xmm7, 168(%r8)

    stmxcsr 184(%r8)

    movq $0, %rax
    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    andq $-16, %rsi
    movq %rsi, 0(%rdi)

    leaq -__CGN_CTX_SAVE_SIZE-128(%rsi), %r8

    movq $0, 0(%r8)
#if defined(__APPLE__)
    leaq ___cgn_thread_entry(%rip), %r9
#else
    leaq __cgn_thread_entry(%rip), %r9
#endif
    movq %r9, 8(%r8)
    movq $0, 16(%r8)
    movq $0, 24(%r8)
    movq $0, 32(%r8)
    movq $0, 40(%r8)
    movq $0, 48(%r8)

    pxor %xmm0, %xmm0
    movdqu %xmm0, 56(%r8)
    movdqu %xmm0, 72(%r8)
    movdqu %xmm0, 88(%r8)
    movdqu %xmm0, 104(%r8)
    movdqu %xmm0, 120(%r8)
    movdqu %xmm0, 136(%r8)
    movdqu %xmm0, 152(%r8)
    movdqu %xmm0, 168(%r8)

    movq $0, %rax
    stmxcsr 184(%r8)
    ret

#elif defined(__x86_64__) && defined(_WIN64)

.balign 4
__CGN_SAVECTX_SYMBOL:
    movq %rsp, 0(%rcx)

    leaq -__CGN_CTX_SAVE_SIZE-128(%rsp), %r10

    movq 8(%rsp), %rax
    movq %rbp, 0(%r10)
    movq %rax, 8(%r10)
    movq %r12, 16(%r10)
    movq %r13, 24(%r10)
    movq %r14, 32(%r10)
    movq %r15, 40(%r10)
    movq %rbx, 48(%r10)
    movq %rdi, 56(%r10)
    movq %rsi, 64(%r10)

    movdqu %xmm6, 72(%r10)
    movdqu %xmm7, 88(%r10)
    movdqu %xmm8, 104(%r10)
    movdqu %xmm9, 120(%r10)
    movdqu %xmm10, 136(%r10)
    movdqu %xmm11, 152(%r10)
    movdqu %xmm12, 168(%r10)
    movdqu %xmm13, 184(%r10)
    movdqu %xmm14, 200(%r10)
    movdqu %xmm15, 216(%r10)

    stmxcsr 232(%r10)

    movq $0, %rax
    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    // RCX = thread*, RDX = stack
    movq %rdx, 0(%rcx)

    leaq -__CGN_CTX_SAVE_SIZE-128(%rdx), %r10

    movq $0, 0(%r10)
    leaq __cgn_thread_entry(%rip), %r11
    movq %r11, 8(%r10)
    movq $0, 16(%r10)
    movq $0, 24(%r10)
    movq $0, 32(%r10)
    movq $0, 40(%r10)
    movq $0, 48(%r10)
    movq $0, 56(%r10)
    movq $0, 64(%r10)

    pxor %xmm6, %xmm6
    movdqu %xmm6, 72(%r10)
    movdqu %xmm6, 88(%r10)
    movdqu %xmm6, 104(%r10)
    movdqu %xmm6, 120(%r10)
    movdqu %xmm6, 136(%r10)
    movdqu %xmm6, 152(%r10)
    movdqu %xmm6, 168(%r10)
    movdqu %xmm6, 184(%r10)
    movdqu %xmm6, 200(%r10)
    movdqu %xmm6, 216(%r10)

    movq $0, %rax
    stmxcsr 232(%r10)
    ret

#elif defined(__aarch64__)

.balign 4
__CGN_SAVECTX_SYMBOL:
    mov x9, sp
    str x9, [x0, 0]

    sub x9, sp, #__CGN_CTX_SAVE_SIZE

    stp lr, x19, [x9, 0]
    stp x20, x21, [x9, 16]
    stp x22, x23, [x9, 32]
    stp x24, x25, [x9, 48]
    stp x26, x27, [x9, 64]
    stp x28, x29, [x9, 80]

    mrs x10, FPCR
    str w10, [x9, 96]
    mrs x10, FPSR
    str w10, [x9, 100]

    stp q8, q9, [x9, 112]
    stp q10, q11, [x9, 144]
    stp q12, q13, [x9, 176]
    stp q14, q15, [x9, 208]

    mov w0, #0
    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    str x1, [x0, 0]

    sub x9, x1, #__CGN_CTX_SAVE_SIZE

#if defined(__APPLE__)
    adrp x10, ___cgn_thread_entry@PAGE
    add  x10, x10, ___cgn_thread_entry@PAGEOFF
#else
    adrp x10, __cgn_thread_entry
    add  x10, x10, #:lo12:__cgn_thread_entry
#endif
    stp  x10, xzr, [x9, 0]
    stp x19, x20, [x9, 16]
    stp x21, x22, [x9, 32]
    stp x23, x24, [x9, 48]
    stp x25, x26, [x9, 64]
    stp x27, x28, [x9, 80]
    str xzr, [x9, 96]

    mrs x10, FPCR
    str w10, [x9, 100]
    mrs x10, FPSR
    str w10, [x9, 104]

    stp q8, q9, [x9, 112]
    stp q10, q11, [x9, 144]
    stp q12, q13, [x9, 176]
    stp q14, q15, [x9, 208]

    mov w0, #0
    ret

#elif defined(__riscv__)

.balign 4
__CGN_SAVECTX_SYMBOL:
    sd sp, 0(a0)
    addi t0, sp, -__CGN_CTX_SAVE_SIZE

    sd ra, 0(t0)
    sd s0, 8(t0)
    sd s1, 16(t0)
    sd s2, 24(t0)
    sd s3, 32(t0)
    sd s4, 40(t0)
    sd s5, 48(t0)
    sd s6, 56(t0)
    sd s7, 64(t0)
    sd s8, 72(t0)
    sd s9, 80(t0)
    sd s10, 88(t0)
    sd s11, 96(t0)

    li a0, 0
    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    sd a1, 0(a0)

    addi t0, a1, -__CGN_CTX_SAVE_SIZE

    la t1, __cgn_thread_entry
    sd t1, 0(t0)
    sd s0, 8(t0)
    sd s1, 16(t0)
    sd s2, 24(t0)
    sd s3, 32(t0)
    sd s4, 40(t0)
    sd s5, 48(t0)
    sd s6, 56(t0)
    sd s7, 64(t0)
    sd s8, 72(t0)
    sd s9, 80(t0)
    sd s10, 88(t0)
    sd s11, 96(t0)

    li a0, 0
    ret

#endif
