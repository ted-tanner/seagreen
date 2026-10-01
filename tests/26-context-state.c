// These checks must also run in release builds.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "seagreen.h"

#if defined(__x86_64__)
typedef struct { uint32_t mxcsr; uint16_t x87; } fp_state;
static fp_state read_fp(void) {
    fp_state state;
    __asm__ volatile("stmxcsr %0; fnstcw %1" : "=m"(state.mxcsr), "=m"(state.x87));
    return state;
}
static void write_fp(fp_state state) {
    __asm__ volatile("ldmxcsr %0; fldcw %1" : : "m"(state.mxcsr), "m"(state.x87));
}
static fp_state rounding(fp_state state, unsigned mode) {
    state.mxcsr = (state.mxcsr & ~(3u << 13)) | (mode << 13);
    state.x87 = (state.x87 & ~(3u << 10)) | (mode << 10);
    return state;
}
static void check_fp(fp_state expected) {
    fp_state actual = read_fp();
    assert((actual.mxcsr & (3u << 13)) == (expected.mxcsr & (3u << 13)));
    assert(actual.x87 == expected.x87);
}
#elif defined(__aarch64__)
typedef uint64_t fp_state;
static fp_state read_fp(void) {
    fp_state state;
    __asm__ volatile("mrs %0, fpcr" : "=r"(state));
    return state;
}
static void write_fp(fp_state state) {
    __asm__ volatile("msr fpcr, %0" : : "r"(state));
}
static fp_state rounding(fp_state state, unsigned mode) {
    return (state & ~(3ull << 22)) | ((uint64_t)mode << 22);
}
static void check_fp(fp_state expected) { assert(read_fp() == expected); }
#elif (defined(__riscv) && __riscv_xlen == 64)
typedef uint32_t fp_state;
static fp_state read_fp(void) {
    fp_state state = 0;
#if defined(__riscv_flen)
    __asm__ volatile("frcsr %0" : "=r"(state));
#endif
    return state;
}
static void write_fp(fp_state state) {
#if defined(__riscv_flen)
    __asm__ volatile("fscsr %0" : : "r"(state));
#else
    (void)state;
#endif
}
static fp_state rounding(fp_state state, unsigned mode) {
#if defined(__riscv_flen)
    return (state & ~(7u << 5)) | (mode << 5);
#else
    (void)mode;
    return state;
#endif
}
static void check_fp(fp_state expected) {
    assert((read_fp() & (7u << 5)) == (expected & (7u << 5)));
}
#endif

// Exercise ordinary calls after saving: these must not overwrite the context.
static __attribute__((noinline)) uint64_t overwrite_stack(void) {
    volatile uint64_t words[512];
    uint64_t sum = 0;
    for (unsigned i = 0; i < 512; ++i) words[i] = i;
    for (unsigned i = 0; i < 512; ++i) sum += words[i];
    return sum;
}

static __CGNThread saved;
static void check_saved_context(void) {
    volatile uint64_t canary = UINT64_C(0x123456789abcdef0);
    fp_state original = read_fp();
    fp_state expected = rounding(original, 1);
    write_fp(expected);
    if (!__cgn_savectx(&saved)) {
        assert(overwrite_stack() == 130816);
        write_fp(rounding(original, 2));
        __cgn_loadctx(&saved);
    }
    assert(canary == UINT64_C(0x123456789abcdef0));
    check_fp(expected);
    write_fp(original);
}

#if defined(__x86_64__) && defined(_WIN64)
// New-thread initialization is an ordinary call: it must preserve XMM6-15
// in the creator, even while initializing the child's vector register state.
static void check_new_context_vectors(void) {
    __CGNThread child;
    _Alignas(16) unsigned char stack[512];
    const uint64_t pattern[2] = {UINT64_C(0x123456789abcdef0),
                                 UINT64_C(0xfedcba9876543210)};
    uint64_t original[2], actual[2];
#define CHECK_XMM(reg) do {                                                 \
    __asm__ volatile("movdqu %%" #reg ", %0" : "=m"(original));             \
    __asm__ volatile("movdqu %0, %%" #reg : : "m"(pattern) : #reg);         \
    __cgn_savenewctx(&child, stack + sizeof(stack));                         \
    __asm__ volatile("movdqu %%" #reg ", %0" : "=m"(actual));               \
    __asm__ volatile("movdqu %0, %%" #reg : : "m"(original) : #reg);        \
    assert(actual[0] == pattern[0] && actual[1] == pattern[1]);              \
} while (0)
    CHECK_XMM(xmm6);
    CHECK_XMM(xmm7);
    CHECK_XMM(xmm8);
    CHECK_XMM(xmm9);
    CHECK_XMM(xmm10);
    CHECK_XMM(xmm11);
    CHECK_XMM(xmm12);
    CHECK_XMM(xmm13);
    CHECK_XMM(xmm14);
    CHECK_XMM(xmm15);
#undef CHECK_XMM
}
#endif

static uint64_t worker(void *arg) {
    fp_state expected = *(fp_state *)arg;
    check_fp(expected); // A new context inherits its creator's control state.
    for (unsigned i = 0; i < 100; ++i) {
        assert(overwrite_stack() == 130816);
        async_yield();
        check_fp(expected);
    }
    return 42;
}

int main(void) {
#if defined(__x86_64__) && defined(_WIN64)
    check_new_context_vectors();
#endif
    check_saved_context();
    fp_state original = read_fp();
    seagreen_init_rt();
    fp_state states[3];
    CGNThreadHandle handles[3];
    for (unsigned i = 0; i < 3; ++i) {
        states[i] = rounding(original, i + 1);
        write_fp(states[i]);
        handles[i] = async_run(worker, &states[i]);
    }
    write_fp(original);
    for (unsigned i = 0; i < 3; ++i) {
        assert(await(handles[i]) == 42);
        check_fp(original);
    }
    seagreen_free_rt();
    write_fp(original);
    puts("Context stack and floating-point state preserved");
    return 0;
}
