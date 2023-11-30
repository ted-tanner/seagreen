#ifndef CGN_COROUTINE_H

#include "cgninternals/cgntypes.h"

// Two of these functions return the thread pointer such that it is available when
// the thread is resumed and the stack may have changed.
extern __attribute__((noinline)) __CGNThread *__cgn_loadctx(__CGNThreadCtx *ctx, __CGNThread *t);
extern __attribute__((noinline)) void __cgn_savectx(__CGNThreadCtx *ctx);
extern __attribute__((noinline)) __CGNThread *__cgn_savenewctx(__CGNThreadCtx *ctx, void *stack, __CGNThread *t);

#define CGN_COROUTINE_H
#endif
