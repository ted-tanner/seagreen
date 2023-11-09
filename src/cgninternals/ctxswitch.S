.globl ctxswitch

#if defined(__x86_64__)

#elif defined(__i386__)

#elif defined(__aarch64__)

ctxswitch:
        // Copy parameter registers
        mv x2, x0
        mv x3, x1

        str lr, [x2, 0]
        str sp, [x2, 8] 

        str x19, [x2, 16]
        str x20, [x2, 24]
        str x21, [x2, 32]
        str x22, [x2, 40]
        str x23, [x2, 48]
        str x24, [x2, 56]
        str x25, [x2, 64]
        str x26, [x2, 72]
        str x27, [x2, 80]
        str x28, [x2, 88]
        str x29, [x2, 96]

        ldr lr, [x3, 0]
        ldr sp, [x3, 8]

        ldr x19 [x3, 16]
        ldr x20 [x3, 24]
        ldr x21 [x3, 32]
        ldr x22 [x3, 40]
        ldr x23 [x3, 48]
        ldr x24 [x3, 56]
        ldr x25 [x3, 64]
        ldr x26 [x3, 72]
        ldr x27 [x3, 80]
        ldr x28 [x3, 88]
        ldr x29 [x3, 96]

        ret
        
#elif defined(__riscv)

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
