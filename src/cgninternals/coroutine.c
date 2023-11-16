#include "cgninternals/coroutine.h"

inline void __cgn_set_stack_ptr(__CGNThreadCtx *ctx, void *ptr) {
#if defined(__x86_64__)
    ctx->rsp = (uint64_t)ptr;
#elif defined(__aarch64__) || defined(__riscv__)
    ctx->sp = (uint64_t)ptr;
#endif
}
