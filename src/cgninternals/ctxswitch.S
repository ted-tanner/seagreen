// TODO: Do floating point registers need to be saved?

#if defined(__WIN64__) || defined(__APPLE__)
#define __CGN_CTXSWITCH_NAME _ctxswitch
#elif
#define __CGN_CTXSWITCH_NAME ctxswitch
#endif

.globl __CGN_CTXSWITCH_NAME

#if defined(__x86_64__) && (defined(__unix__) || defined(__APPLE__))

.balign 4
__CGN_CTXSWITCH_NAME:
        movq %rsp, (%rdi)
        movq %rbp, 8(%rdi)

        movq %r12, 16(%rdi)
        movq %r13, 24(%rdi)
        movq %r14, 32(%rdi)
        movq %r15, 40(%rdi)
        movq %rbx, 48(%rdi)

        movq %rsp, (%rsi)
        movq %rbp, 8(%rsi)

        movq %r12, 16(%rsi)
        movq %r13, 24(%rsi)
        movq %r14, 32(%rsi)
        movq %r15, 40(%rsi)
        movq %rbx, 48(%rsi)

        ret

#elif defined(__x86_64__) && defined(__WIN64__)

.balign 4
__CGN_CTXSWITCH_NAME:
        movq %rsp, (%rcx)
        movq %rbp, 8(%rcx)

        movq %r12, 16(%rcx)
        movq %r13, 24(%rcx)
        movq %r14, 32(%rcx)
        movq %r15, 40(%rcx)
        movq %rbx, 48(%rcx)
        movq %rdi, 56(%rcx)

        movq %rsp, (%rdx)
        movq %rbp, 8(%rdx)

        movq %r12, 16(%rdx)
        movq %r13, 24(%rdx)
        movq %r14, 32(%rdx)
        movq %r15, 40(%rdx)
        movq %rbx, 48(%rdx)
        movq %rdi, 56(%rdx)

        ret
        
#elif defined(__aarch64__)

.balign 4
__CGN_CTXSWITCH_NAME:
        str lr, [x0, 0]
        mov x9, sp
        str x9, [x0, 8] 

        str x19, [x0, 16]
        str x20, [x0, 24]
        str x21, [x0, 32]
        str x22, [x0, 40]
        str x23, [x0, 48]
        str x24, [x0, 56]
        str x25, [x0, 64]
        str x26, [x0, 72]
        str x27, [x0, 80]
        str x28, [x0, 88]
        str x29, [x0, 96]

        ldr lr, [x1, 0]
        ldr x9, [x1, 8]
        mov sp, x9

        ldr x19, [x1, 16]
        ldr x20, [x1, 24]
        ldr x21, [x1, 32]
        ldr x22, [x1, 40]
        ldr x23, [x1, 48]
        ldr x24, [x1, 56]
        ldr x25, [x1, 64]
        ldr x26, [x1, 72]
        ldr x27, [x1, 80]
        ldr x28, [x1, 88]
        ldr x29, [x1, 96]

        ret

#elif defined(__riscv__)

.balign 4
__CGN_CTXSWITCH_NAME:
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
