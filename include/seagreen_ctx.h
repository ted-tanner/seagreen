#ifndef SEAGREEN_CTX_H
#define SEAGREEN_CTX_H

// Size of the saved context area placed below the saved stack pointer
#if defined(__x86_64__) && (defined(__unix__) || defined(__APPLE__))
#define __CGN_CTX_SAVE_SIZE 56
#elif defined(__x86_64__) && defined(_WIN64)
#define __CGN_CTX_SAVE_SIZE 256
#elif defined(__aarch64__)
#define __CGN_CTX_SAVE_SIZE 240
#elif defined(__riscv__)
#define __CGN_CTX_SAVE_SIZE 104
#endif

#endif // SEAGREEN_CTX_H

