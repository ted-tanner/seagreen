#if defined(__WIN64__) || defined(__APPLE__)
#define __CGN_STORERETVAL_NAME ___cgn_storeretval
#define __CGN_LOADRETVAL_NAME ___cgn_loadretval
#else
#define __CGN_STORERETVAL_NAME __cgn_storeretval
#define __CGN_LOADRETVAL_NAME __cgn_loadretval
#endif

.globl __CGN_STORERETVAL_NAME
.globl __CGN_LOADRETVAL_NAME

#if defined(__x86_64__) && (defined(__unix__) || defined(__APPLE__))

    // TODO 

.balign 4
__CGN_STORERETVAL_NAME:
    ret

.balign 4
__CGN_LOADRETVAL_NAME:
    ret

#elif defined(__x86_64__) && defined(__WIN64__)

    // TODO

.balign 4
__CGN_STORERETVAL_NAME:
    ret

.balign 4
__CGN_LOADRETVAL_NAME:
    ret

#elif defined(__aarch64__)

.balign 4
__CGN_STORERETVAL_NAME:
    str x0, [sp, 0]
    ret

.balign 4
__CGN_LOADRETVAL_NAME:
    ldr x0, [sp, 0]
    ret

#elif defined(__riscv__)

    // TODO

.balign 4
__CGN_STORERETVAL_NAME:
    ret

.balign 4
__CGN_LOADRETVAL_NAME:
    ret

#endif
