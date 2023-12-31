#if defined(__WIN64__) || defined(__APPLE__)
#define __CGN_LOADCTX_SYMBOL ___cgn_loadctx
#else
#define __CGN_LOADCTX_SYMBOL __cgn_loadctx
#endif

.globl __CGN_LOADCTX_SYMBOL

#if defined(__x86_64__) && (defined(__unix__) || defined(__APPLE__))

    // TODO 

.balign 4
__CGN_LOADCTX_SYMBOL:
    movq %rbp, (%rdi)
    movq %rsp, 8(%rdi)

    movq %r12, 16(%rdi)
    movq %r13, 24(%rdi)
    movq %r14, 32(%rdi)
    movq %r15, 40(%rdi)
    movq %rbx, 48(%rdi)

    movq %rsi, %rdi

    ret

#elif defined(__x86_64__) && defined(__WIN64__)

    // TODO

.balign 4
__CGN_LOADCTX_SYMBOL:
    movq %rbp, (%rcx)
    movq %rsp, 8(%rcx)

    movq %r12, 16(%rcx)
    movq %r13, 24(%rcx)
    movq %r14, 32(%rcx)
    movq %r15, 40(%rcx)
    movq %rbx, 48(%rcx)
    movq %rdi, 56(%rcx)

    movq %rdx, %rcx

    ret

#elif defined(__aarch64__)

.balign 4
__CGN_LOADCTX_SYMBOL:
    ldr lr, [x0, 0]
    ldr x9, [x0, 8]
    mov sp, x9

    ldr x19, [x0, 16]
    ldr x20, [x0, 24]
    ldr x21, [x0, 32]
    ldr x22, [x0, 40]
    ldr x23, [x0, 48]
    ldr x24, [x0, 56]
    ldr x25, [x0, 64]
    ldr x26, [x0, 72]
    ldr x27, [x0, 80]
    ldr x28, [x0, 88]
    ldr x29, [x0, 96]

    mov x0, x1

    ret

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
