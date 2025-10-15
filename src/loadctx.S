#if defined(_WIN64) || defined(__APPLE__)
#define __CGN_LOADCTX_SYMBOL ___cgn_loadctx
#else
#define __CGN_LOADCTX_SYMBOL __cgn_loadctx
#endif

.globl __CGN_LOADCTX_SYMBOL

#if defined(__x86_64__) && (defined(__unix__) || defined(__APPLE__))

.balign 4
__CGN_LOADCTX_SYMBOL:
    movq 0(%rsi), %rbp
    movq 8(%rsi), %rsp
    movq 16(%rsi), %rax
    movq 24(%rsi), %r12
    movq 32(%rsi), %r13
    movq 40(%rsi), %r14
    movq 48(%rsi), %r15
    movq 56(%rsi), %rbx

    movq %rax, %r11
    movq %rdi, 8(%rsp)
    movq %rdi, %rax
    jmp *%r11

#elif defined(__x86_64__) && defined(_WIN64)

__CGN_LOADCTX_SYMBOL_ret_stub:
    jmp *%rax

.balign 4
__CGN_LOADCTX_SYMBOL:
    movq 0(%rdx), %rbp
    movq 8(%rdx), %rsp
    movq 16(%rdx), %rax

    movq 24(%rdx), %r12
    movq 32(%rdx), %r13
    movq 40(%rdx), %r14
    movq 48(%rdx), %r15
    movq 56(%rdx), %rbx
    movq 64(%rdx), %rdi
    movq 72(%rdx), %rsi

    movdqu 80(%rdx), %xmm6
    movdqu 96(%rdx), %xmm7
    movdqu 112(%rdx), %xmm8
    movdqu 128(%rdx), %xmm9
    movdqu 144(%rdx), %xmm10
    movdqu 160(%rdx), %xmm11
    movdqu 176(%rdx), %xmm12
    movdqu 192(%rdx), %xmm13
    movdqu 208(%rdx), %xmm14
    movdqu 224(%rdx), %xmm15

    ldmxcsr 240(%rdx)

    movq %rax, %r11
    movq %rcx, 8(%rsp)
    movq %rcx, %rax
    jmp *%r11

#elif defined(__aarch64__)

.balign 4
__CGN_LOADCTX_SYMBOL:
    ldp lr, x9, [x1, 0]
    mov sp, x9

    ldp x19, x20, [x1, 16]
    ldp x21, x22, [x1, 32]
    ldp x23, x24, [x1, 48]
    ldp x25, x26, [x1, 64]
    ldp x27, x28, [x1, 80]
    ldr x29, [x1, 96]

    ldr w9, [x1, 104]
    msr FPCR, x9
    ldr w9, [x1, 108]
    msr FPSR, x9

    ldp q8, q9, [x1, 112]
    ldp q10, q11, [x1, 144]
    ldp q12, q13, [x1, 176]
    ldp q14, q15, [x1, 208]

    br lr

#elif defined(__riscv__)

.balign 4
__CGN_LOADCTX_SYMBOL:
    ld ra, 0(a1)
    ld sp, 8(a1)

    ld s0, 16(a1)
    ld s1, 24(a1)
    ld s2, 32(a1)
    ld s3, 40(a1)
    ld s4, 48(a1)
    ld s5, 56(a1)
    ld s6, 64(a1)
    ld s7, 72(a1)
    ld s8, 80(a1)
    ld s9, 88(a1)
    ld s10, 96(a1)
    ld s11, 104(a1)

    ret

#endif
