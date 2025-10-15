#if defined(_WIN64) || defined(__APPLE__)
#define __CGN_SAVECTX_SYMBOL ___cgn_savectx
#define __CGN_SAVENEWCTX_SYMBOL ___cgn_savenewctx
#else
#define __CGN_SAVECTX_SYMBOL __cgn_savectx
#define __CGN_SAVENEWCTX_SYMBOL __cgn_savenewctx
#endif

.globl __CGN_SAVECTX_SYMBOL
.globl __CGN_SAVENEWCTX_SYMBOL

#if defined(__x86_64__) && (defined(__unix__) || defined(__APPLE__))

.balign 4
__CGN_SAVECTX_SYMBOL:
    movq %rbp, 0(%rsi)
    movq %rsp, 8(%rsi)
    movq (%rsp), %rax
    movq %rax, 16(%rsi)

    movq %r12, 24(%rsi)
    movq %r13, 32(%rsi)
    movq %r14, 40(%rsi)
    movq %r15, 48(%rsi)
    movq %rbx, 56(%rsi)

    movq $1, %rax
    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    movq $0, 0(%rsi)

    movq %rdx, %rcx
    leaq -8(%rcx), %rcx
    movq %rcx, 8(%rsi)

    leaq 0f(%rip), %r8
    movq %r8, 16(%rsi)
    movq %r8, (%rcx)

    movq $0, 24(%rsi)
    movq $0, 32(%rsi)
    movq $0, 40(%rsi)
    movq $0, 48(%rsi)
    movq $0, 56(%rsi)

    movq $1, %rax

    ret

#elif defined(__x86_64__) && defined(_WIN64)

.balign 4
__CGN_SAVECTX_SYMBOL:
    movq %rbp, 0(%rdx)
    movq %rsp, 8(%rdx)
    movq (%rsp), %rax
    movq %rax, 16(%rdx)

    movq %r12, 24(%rdx)
    movq %r13, 32(%rdx)
    movq %r14, 40(%rdx)
    movq %r15, 48(%rdx)
    movq %rbx, 56(%rdx)
    movq %rdi, 64(%rdx)
    movq %rsi, 72(%rdx)

    movdqu %xmm6, 80(%rdx)
    movdqu %xmm7, 96(%rdx)
    movdqu %xmm8, 112(%rdx)
    movdqu %xmm9, 128(%rdx)
    movdqu %xmm10, 144(%rdx)
    movdqu %xmm11, 160(%rdx)
    movdqu %xmm12, 176(%rdx)
    movdqu %xmm13, 192(%rdx)
    movdqu %xmm14, 208(%rdx)
    movdqu %xmm15, 224(%rdx)

    stmxcsr 240(%rdx)

    movq $1, %rax
    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    movq $0, 0(%rdx)

    movq %r8, %r9
    leaq -8(%r9), %r9
    movq %r9, 8(%rdx)

    leaq 0f(%rip), %r10
    movq %r10, 16(%rdx)
    movq %r10, (%r9)

    movq $0, 24(%rdx)
    movq $0, 32(%rdx)
    movq $0, 40(%rdx)
    movq $0, 48(%rdx)
    movq $0, 56(%rdx)
    movq $0, 64(%rdx)
    movq $0, 72(%rdx)

    pxor %xmm6, %xmm6
    movdqu %xmm6, 80(%rdx)
    movdqu %xmm6, 96(%rdx)
    movdqu %xmm6, 112(%rdx)
    movdqu %xmm6, 128(%rdx)
    movdqu %xmm6, 144(%rdx)
    movdqu %xmm6, 160(%rdx)
    movdqu %xmm6, 176(%rdx)
    movdqu %xmm6, 192(%rdx)
    movdqu %xmm6, 208(%rdx)
    movdqu %xmm6, 224(%rdx)

    movq $1, %rax

    stmxcsr 240(%rdx)

    ret

#elif defined(__aarch64__)

.balign 4
__CGN_SAVECTX_SYMBOL:
    mov x9, sp
    stp lr, x9, [x1, 0]

    stp x19, x20, [x1, 16]
    stp x21, x22, [x1, 32]
    stp x23, x24, [x1, 48]
    stp x25, x26, [x1, 64]
    stp x27, x28, [x1, 80]
    str x29, [x1, 96]

    mrs x9, FPCR
    str w9, [x1, 104]
    mrs x9, FPSR
    str w9, [x1, 108]

    stp q8, q9, [x1, 112]
    stp q10, q11, [x1, 144]
    stp q12, q13, [x1, 176]
    stp q14, q15, [x1, 208]

    mov w0, #1
    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    stp lr, x2, [x1, 0]

    stp x19, x20, [x1, 16]
    stp x21, x22, [x1, 32]
    stp x23, x24, [x1, 48]
    stp x25, x26, [x1, 64]
    stp x27, x28, [x1, 80]
    str x29, [x1, 96]

    mrs x9, FPCR
    str w9, [x1, 104]
    mrs x9, FPSR
    str w9, [x1, 108]

    movi v8.16b, #0
    movi v9.16b, #0
    movi v10.16b, #0
    movi v11.16b, #0
    movi v12.16b, #0
    movi v13.16b, #0
    movi v14.16b, #0
    movi v15.16b, #0
    stp q8, q9, [x1, 112]
    stp q10, q11, [x1, 144]
    stp q12, q13, [x1, 176]
    stp q14, q15, [x1, 208]

    mov w0, #1
    ret

#elif defined(__riscv__)

.balign 4
__CGN_SAVECTX_SYMBOL:
    sd ra, 0(a1)
    sd sp, 8(a1)

    sd s0, 16(a1)
    sd s1, 24(a1)
    sd s2, 32(a1)
    sd s3, 40(a1)
    sd s4, 48(a1)
    sd s5, 56(a1)
    sd s6, 64(a1)
    sd s7, 72(a1)
    sd s8, 80(a1)
    sd s9, 88(a1)
    sd s10, 96(a1)
    sd s11, 104(a1)

    li a0, 1
    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    sd ra, 0(a1)
    sd a2, 8(a1)

    sd s0, 16(a1)
    sd s1, 24(a1)
    sd s2, 32(a1)
    sd s3, 40(a1)
    sd s4, 48(a1)
    sd s5, 56(a1)
    sd s6, 64(a1)
    sd s7, 72(a1)
    sd s8, 80(a1)
    sd s9, 88(a1)
    sd s10, 96(a1)
    sd s11, 104(a1)

    li a0, 1
    ret

#endif
