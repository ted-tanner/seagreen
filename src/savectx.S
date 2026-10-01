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

.text

.globl __CGN_SAVECTX_SYMBOL
.globl __CGN_SAVENEWCTX_SYMBOL

#if defined(__x86_64__) && (defined(__unix__) || defined(__APPLE__))

.balign 4
__CGN_SAVECTX_SYMBOL:
    // Resume as if this call had returned, with its return address popped.
    leaq 8(%rsp), %rax
    movq %rax, 0(%rdi)
    leaq __CGN_CTX_STORAGE_OFFSET(%rdi), %r8

    movq 0(%rsp), %rax
    movq %rbp, 0(%r8)
    movq %rax, 8(%r8)
    movq %r12, 16(%r8)
    movq %r13, 24(%r8)
    movq %r14, 32(%r8)
    movq %r15, 40(%r8)
    movq %rbx, 48(%r8)

    stmxcsr 56(%r8)
    fnstcw 60(%r8)

    movq $0, %rax
    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    andq $-16, %rsi
    subq $8, %rsi              // C entry: RSP mod 16 == 8.
    movq $0, 0(%rsi)           // No caller; thread_entry never returns.
    movq %rsi, 0(%rdi)

    leaq __CGN_CTX_STORAGE_OFFSET(%rdi), %r8

    movq $0, 0(%r8)
    leaq __CGN_THREAD_ENTRY_SYMBOL(%rip), %r9
    movq %r9, 8(%r8)
    movq $0, 16(%r8)
    movq $0, 24(%r8)
    movq $0, 32(%r8)
    movq $0, 40(%r8)
    movq $0, 48(%r8)

    stmxcsr 56(%r8)
    fnstcw 60(%r8)
    movq $0, %rax
    ret

#elif defined(__x86_64__) && defined(_WIN64)

.balign 4
__CGN_SAVECTX_SYMBOL:
    leaq 8(%rsp), %rax
    movq %rax, 0(%rcx)
    leaq __CGN_CTX_STORAGE_OFFSET(%rcx), %r10

    movq 0(%rsp), %rax
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
    fnstcw 236(%r10)

    movq $0, %rax
    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    // Reserve a dummy return address and 32 bytes of caller shadow space.
    andq $-16, %rdx
    subq $40, %rdx
    movq $0, 0(%rdx)
    movq %rdx, 0(%rcx)

    leaq __CGN_CTX_STORAGE_OFFSET(%rcx), %r10

    movq $0, 0(%r10)
    leaq __CGN_THREAD_ENTRY_SYMBOL(%rip), %r11
    movq %r11, 8(%r10)
    movq $0, 16(%r10)
    movq $0, 24(%r10)
    movq $0, 32(%r10)
    movq $0, 40(%r10)
    movq $0, 48(%r10)
    movq $0, 56(%r10)
    movq $0, 64(%r10)

    pxor %xmm0, %xmm0
    movdqu %xmm0, 72(%r10)
    movdqu %xmm0, 88(%r10)
    movdqu %xmm0, 104(%r10)
    movdqu %xmm0, 120(%r10)
    movdqu %xmm0, 136(%r10)
    movdqu %xmm0, 152(%r10)
    movdqu %xmm0, 168(%r10)
    movdqu %xmm0, 184(%r10)
    movdqu %xmm0, 200(%r10)
    movdqu %xmm0, 216(%r10)

    movq $0, %rax
    stmxcsr 232(%r10)
    fnstcw 236(%r10)
    ret

#elif defined(__aarch64__)

.balign 4
__CGN_SAVECTX_SYMBOL:
    mov x9, sp
    str x9, [x0, 0]

    add x9, x0, #__CGN_CTX_STORAGE_OFFSET

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

    add x9, x0, #__CGN_CTX_STORAGE_OFFSET

#if defined(__APPLE__)
    adrp x10, ___cgn_thread_entry@PAGE
    add  x10, x10, ___cgn_thread_entry@PAGEOFF
#else
    adrp x10, __cgn_thread_entry
    add  x10, x10, #:lo12:__cgn_thread_entry
#endif
    stp x10, xzr, [x9, 0]
    stp xzr, xzr, [x9, 16]
    stp xzr, xzr, [x9, 32]
    stp xzr, xzr, [x9, 48]
    stp xzr, xzr, [x9, 64]
    stp xzr, xzr, [x9, 80]

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

#elif (defined(__riscv) && __riscv_xlen == 64)

.balign 4
__CGN_SAVECTX_SYMBOL:
    sd sp, 0(a0)
    addi t0, a0, __CGN_CTX_STORAGE_OFFSET

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

#if defined(__riscv_flen)
    frcsr t1
    sw t1, 104(t0)
#if defined(__riscv_float_abi_double)
    fsd fs0, 112(t0)
    fsd fs1, 120(t0)
    fsd fs2, 128(t0)
    fsd fs3, 136(t0)
    fsd fs4, 144(t0)
    fsd fs5, 152(t0)
    fsd fs6, 160(t0)
    fsd fs7, 168(t0)
    fsd fs8, 176(t0)
    fsd fs9, 184(t0)
    fsd fs10, 192(t0)
    fsd fs11, 200(t0)
#elif defined(__riscv_float_abi_single)
    fsw fs0, 112(t0)
    fsw fs1, 116(t0)
    fsw fs2, 120(t0)
    fsw fs3, 124(t0)
    fsw fs4, 128(t0)
    fsw fs5, 132(t0)
    fsw fs6, 136(t0)
    fsw fs7, 140(t0)
    fsw fs8, 144(t0)
    fsw fs9, 148(t0)
    fsw fs10, 152(t0)
    fsw fs11, 156(t0)
#endif
#endif

    li a0, 0
    ret

.balign 4
__CGN_SAVENEWCTX_SYMBOL:
    sd a1, 0(a0)

    addi t0, a0, __CGN_CTX_STORAGE_OFFSET

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

#if defined(__riscv_flen)
    frcsr t1
    sw t1, 104(t0)
#if defined(__riscv_float_abi_double)
    fsd fs0, 112(t0)
    fsd fs1, 120(t0)
    fsd fs2, 128(t0)
    fsd fs3, 136(t0)
    fsd fs4, 144(t0)
    fsd fs5, 152(t0)
    fsd fs6, 160(t0)
    fsd fs7, 168(t0)
    fsd fs8, 176(t0)
    fsd fs9, 184(t0)
    fsd fs10, 192(t0)
    fsd fs11, 200(t0)
#elif defined(__riscv_float_abi_single)
    fsw fs0, 112(t0)
    fsw fs1, 116(t0)
    fsw fs2, 120(t0)
    fsw fs3, 124(t0)
    fsw fs4, 128(t0)
    fsw fs5, 132(t0)
    fsw fs6, 136(t0)
    fsw fs7, 140(t0)
    fsw fs8, 144(t0)
    fsw fs9, 148(t0)
    fsw fs10, 152(t0)
    fsw fs11, 156(t0)
#endif
#endif

    li a0, 0
    ret

#endif

#if defined(__ELF__)
.section .note.GNU-stack,"",@progbits
#endif
