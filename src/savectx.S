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
    movq %rbp, 0(%rdi)
    movq %rsp, 8(%rdi)
    movq (%rsp), %rax
    movq %rax, 16(%rdi)

    movq %r12, 24(%rdi)
    movq %r13, 32(%rdi)
    movq %r14, 40(%rdi)
    movq %r15, 48(%rdi)
    movq %rbx, 56(%rdi)

    movq $0, %rax
    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    movq $0, 0(%rdi)

    movq %rsi, %rcx
    leaq -8(%rcx), %rcx
    movq %rcx, 8(%rdi)

    leaq 0f(%rip), %r8
    movq %r8, 16(%rdi)
    movq %r8, (%rcx)

    movq $0, 24(%rdi)
    movq $0, 32(%rdi)
    movq $0, 40(%rdi)
    movq $0, 48(%rdi)
    movq $0, 56(%rdi)

    movq $0, %rax
    ret

#elif defined(__x86_64__) && defined(_WIN64)

.balign 4
__CGN_SAVECTX_SYMBOL:
    movq %rbp, 0(%rcx)
    movq %rsp, 8(%rcx)
    movq (%rsp), %rax
    movq %rax, 16(%rcx)

    movq %r12, 24(%rcx)
    movq %r13, 32(%rcx)
    movq %r14, 40(%rcx)
    movq %r15, 48(%rcx)
    movq %rbx, 56(%rcx)
    movq %rdi, 64(%rcx)
    movq %rsi, 72(%rcx)

    movdqu %xmm6, 80(%rcx)
    movdqu %xmm7, 96(%rcx)
    movdqu %xmm8, 112(%rcx)
    movdqu %xmm9, 128(%rcx)
    movdqu %xmm10, 144(%rcx)
    movdqu %xmm11, 160(%rcx)
    movdqu %xmm12, 176(%rcx)
    movdqu %xmm13, 192(%rcx)
    movdqu %xmm14, 208(%rcx)
    movdqu %xmm15, 224(%rcx)

    stmxcsr 240(%rcx)

    movq $0, %rax
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

    movq $0, %rax

    stmxcsr 240(%rdx)

    ret

#elif defined(__aarch64__)

.balign 4
__CGN_SAVECTX_SYMBOL:
    mov x9, sp
    stp lr, x9, [x0, 0]

    stp x19, x20, [x0, 16]
    stp x21, x22, [x0, 32]
    stp x23, x24, [x0, 48]
    stp x25, x26, [x0, 64]
    stp x27, x28, [x0, 80]
    str x29, [x0, 96]

    mrs x9, FPCR
    str w9, [x0, 104]
    mrs x9, FPSR
    str w9, [x0, 108]

    stp q8, q9, [x0, 112]
    stp q10, q11, [x0, 144]
    stp q12, q13, [x0, 176]
    stp q14, q15, [x0, 208]

    mov w0, #0
    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    stp lr, x1, [x0, 0]

    stp x19, x20, [x0, 16]
    stp x21, x22, [x0, 32]
    stp x23, x24, [x0, 48]
    stp x25, x26, [x0, 64]
    stp x27, x28, [x0, 80]
    str x29, [x0, 96]

    mrs x9, FPCR
    str w9, [x0, 104]
    mrs x9, FPSR
    str w9, [x0, 108]

    stp q8, q9, [x0, 112]
    stp q10, q11, [x0, 144]
    stp q12, q13, [x0, 176]
    stp q14, q15, [x0, 208]

    mov w0, #0
    ret

#elif defined(__riscv__)

.balign 4
__CGN_SAVECTX_SYMBOL:
    sd ra, 0(a0)
    sd sp, 8(a0)

    sd s0, 16(a0)
    sd s1, 24(a0)
    sd s2, 32(a0)
    sd s3, 40(a0)
    sd s4, 48(a0)
    sd s5, 56(a0)
    sd s6, 64(a0)
    sd s7, 72(a0)
    sd s8, 80(a0)
    sd s9, 88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)

    li a0, 0
    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    sd ra, 0(a0)
    sd a2, 8(a0)

    sd s0, 16(a0)
    sd s1, 24(a0)
    sd s2, 32(a0)
    sd s3, 40(a0)
    sd s4, 48(a0)
    sd s5, 56(a0)
    sd s6, 64(a0)
    sd s7, 72(a0)
    sd s8, 80(a0)
    sd s9, 88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)

    li a0, 0
    ret

#endif
