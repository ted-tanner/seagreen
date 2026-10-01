#ifndef _SEAGREEN_CTX_H
#define _SEAGREEN_CTX_H

// Keep contexts in the thread: memory below SP is not persistent storage.
#if defined(__x86_64__) || (defined(__riscv) && __riscv_xlen == 64) || defined(__aarch64__)
#define __CGN_CTX_IN_THREAD 1
#define __CGN_CTX_STORAGE_OFFSET 8
#endif

// Size of the saved register area (excluding the stack pointer).
#if defined(__x86_64__) && (defined(__unix__) || defined(__APPLE__))
#define __CGN_CTX_SAVE_SIZE 64
#elif defined(__x86_64__) && defined(_WIN64)
#define __CGN_CTX_SAVE_SIZE 256
#elif defined(__aarch64__)
#define __CGN_CTX_SAVE_SIZE 240
#elif (defined(__riscv) && __riscv_xlen == 64)
#if defined(__riscv_float_abi_double)
#define __CGN_CTX_SAVE_SIZE 208
#elif defined(__riscv_float_abi_single)
#define __CGN_CTX_SAVE_SIZE 160
#elif defined(__riscv_flen)
#define __CGN_CTX_SAVE_SIZE 112
#else
#define __CGN_CTX_SAVE_SIZE 104
#endif
#endif

#endif // _SEAGREEN_CTX_H
