#if defined(__WIN64__) || defined(__APPLE__)
#define __CGN_LOADCTX_SYMBOL ___cgn_loadctx
#else
#define __CGN_LOADCTX_SYMBOL __cgn_loadctx
#endif

.globl __CGN_LOADCTX_SYMBOL

#if defined(__x86_64__) && (defined(__unix__) || defined(__APPLE__))

.balign 4
__CGN_LOADCTX_SYMBOL:
    movq 0(%rdi), %rbp
    movq 8(%rdi), %rsp

    movq 16(%rdi), %r12
    movq 24(%rdi), %r13
    movq 32(%rdi), %r14
    movq 40(%rdi), %r15
    movq 48(%rdi), %rbx

    movq %rsi, %rax

    ret

#elif defined(__x86_64__) && defined(__WIN64__)

.balign 4
__CGN_LOADCTX_SYMBOL:
    movq 0(%rcx), %rbp
    movq 8(%rcx), %rsp

    movq 16(%rcx), %r12
    movq 24(%rcx), %r13
    movq 32(%rcx), %r14
    movq 40(%rcx), %r15
    movq 48(%rcx), %rbx
    movq 56(%rcx), %rdi

    movq %rdx, %rax

    ret

#elif defined(__aarch64__)

.balign 4
__CGN_LOADCTX_SYMBOL:
    ldp lr, x9, [x0, 0]
    mov sp, x9

    ldp x19, x20, [x0, 16]
    ldp x21, x22, [x0, 32]
    ldp x23, x24, [x0, 48]
    ldp x25, x26, [x0, 64]
    ldp x27, x28, [x0, 80]
    ldr x29, [x0, 96]

    mov x0, x1

    br lr

#elif defined(__riscv__)

    // TODO

.balign 4
__CGN_LOADCTX_SYMBOL:
    ld ra, 0(a0)
    ld sp, 8(a0)

    ld s0, 16(a0)
    ld s1, 24(a0)
    ld s2, 32(a0)
    ld s3, 40(a0)
    ld s4, 48(a0)
    ld s5, 56(a0)
    ld s6, 64(a0)
    ld s7, 72(a0)
    ld s8, 80(a0)
    ld s9, 88(a0)
    ld s10, 96(a0)
    ld s11, 104(a0)

    mv a0, a1

    ret

#endif
