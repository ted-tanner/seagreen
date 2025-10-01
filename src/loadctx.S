#if defined(_WIN64) || defined(__APPLE__)
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
    movq 16(%rdi), %rax
    movq 24(%rdi), %r12
    movq 32(%rdi), %r13
    movq 40(%rdi), %r14
    movq 48(%rdi), %r15
    movq 56(%rdi), %rbx

    movdqu 64(%rdi), %xmm6
    movdqu 80(%rdi), %xmm7
    movdqu 96(%rdi), %xmm8
    movdqu 112(%rdi), %xmm9
    movdqu 128(%rdi), %xmm10
    movdqu 144(%rdi), %xmm11
    movdqu 160(%rdi), %xmm12
    movdqu 176(%rdi), %xmm13
    movdqu 192(%rdi), %xmm14
    movdqu 208(%rdi), %xmm15

    movq %rsi, 8(%rsp)
    jmp *%rax

#elif defined(__x86_64__) && defined(_WIN64)

__CGN_LOADCTX_SYMBOL_ret_stub:
    jmp *%rax

.balign 4
__CGN_LOADCTX_SYMBOL:
    movq 0(%rcx), %rbp
    movq 8(%rcx), %rsp
    movq 16(%rcx), %rax

    movq 24(%rcx), %r12
    movq 32(%rcx), %r13
    movq 40(%rcx), %r14
    movq 48(%rcx), %r15
    movq 56(%rcx), %rbx
    movq 64(%rcx), %rdi
    movq 72(%rcx), %rsi

    movdqu 80(%rcx), %xmm6
    movdqu 96(%rcx), %xmm7
    movdqu 112(%rcx), %xmm8
    movdqu 128(%rcx), %xmm9
    movdqu 144(%rcx), %xmm10
    movdqu 160(%rcx), %xmm11
    movdqu 176(%rcx), %xmm12
    movdqu 192(%rcx), %xmm13
    movdqu 208(%rcx), %xmm14
    movdqu 224(%rcx), %xmm15

    movq %rdx, 8(%rsp)
    jmp *%rax

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

    ldp q8, q9, [x0, 112]
    ldp q10, q11, [x0, 144]
    ldp q12, q13, [x0, 176]
    ldp q14, q15, [x0, 208]

    mov x0, x1

    br lr

#elif defined(__riscv__)

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
