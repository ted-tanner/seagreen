.globl ctxswitch

#if defined(__x86_64__)

#elif defined(__i386__)

#elif defined(__aarch64__)

ctxswitch:
        str lr, [x0, 0]
        str sp, [x0, 8] 

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
        ldr sp, [x1, 8]

        ldr x19 [x1, 16]
        ldr x20 [x1, 24]
        ldr x21 [x1, 32]
        ldr x22 [x1, 40]
        ldr x23 [x1, 48]
        ldr x24 [x1, 56]
        ldr x25 [x1, 64]
        ldr x26 [x1, 72]
        ldr x27 [x1, 80]
        ldr x28 [x1, 88]
        ldr x29 [x1, 96]

        ret
        
#elif defined(__riscv__)

ctxswitch:
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

#else

#error "Unsupported architecture"

#endif
