#if defined(__WIN64__) || defined(__APPLE__)
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

    movq %r12, 16(%rdi)
    movq %r13, 24(%rdi)
    movq %r14, 32(%rdi)
    movq %r15, 40(%rdi)
    movq %rbx, 48(%rdi)

    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    movq %rbp, 0(%rdi)
    movq %rsi, 8(%rdi)

    movq %r12, 16(%rdi)
    movq %r13, 24(%rdi)
    movq %r14, 32(%rdi)
    movq %r15, 40(%rdi)
    movq %rbx, 48(%rdi)

    movq %rdx, %rax

    ret

#elif defined(__x86_64__) && defined(__WIN64__)

.balign 4
__CGN_SAVECTX_SYMBOL:
    movq %rbp, 0(%rcx)
    movq %rsp, 8(%rcx)

    movq %r12, 16(%rcx)
    movq %r13, 24(%rcx)
    movq %r14, 32(%rcx)
    movq %r15, 40(%rcx)
    movq %rbx, 48(%rcx)
    movq %rdi, 56(%rcx)

    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    movq %rbp, 0(%rcx)
    movq %rdx, 8(%rcx)

    movq %r12, 16(%rcx)
    movq %r13, 24(%rcx)
    movq %r14, 32(%rcx)
    movq %r15, 40(%rcx)
    movq %rbx, 48(%rcx)
    movq %rdi, 56(%rcx)

    movq %r8, %rax

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

    mov x0, x2

    ret

#elif defined(__riscv__)

// TODO

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

    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    sd ra, 0(a0)
    sd a1, 8(a0)

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

    mv a0, a2

    ret

#endif
