#ifndef CGNRT_H

#include <stdint.h>
#include <stdlib.h>

// Threads will be stored in arrays of 256. A linked list will track each
// block
//
// TODO: User of library should be able to pass a function pointer to malloc()
// and free() implementations to use for the thread list

typedef struct __CGNThread_ {
    void *data;
    size_t data_size;

    size_t pos;

    struct __CGNThread_ *next;
} __CGNThread;

typedef struct __CGNThreadBlock_ {
    uint64_t unused_threads;

    struct __CGNThreadBlock_ *next;
    struct __CGNThreadBlock_ *prev;

    __CGNThread threads[64];
} __CGNThreadBlock;

typedef struct __CGNThreadList_ {
    __CGNThreadBlock *head;
    __CGNThreadBlock *tail;

    size_t block_count;
    size_t thread_count;
} __CGNThreadList;

#define __CGN_CHECK_MALLOC(ptr) if (!ptr) { abort(); }

#define async __attribute__((noinline))
typedef __CGNThread *CGNFuture;

void cgn_init_rt(void);


#ifdef CGN_TEST
#include "cgntest/test.h"
void register_cgnrt_tests();
#endif

#define CGNRT_H
#endif
