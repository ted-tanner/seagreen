#ifndef SGRT_H

#include <stdint.h>
#include <stdlib.h>

// This should stay at 64. This allows a 64-bit integer to track unused
// descriptors in a block
#define SG_DESC_BLOCK_SIZE 64

// Descriptors will be stored in arrays of 256. A linked list will track each
// block
//
// TODO: User of library should be able to pass a function pointer to malloc()
// and free() implementations to use for the descriptor list

typedef struct _SGDescriptor_ {
    void *data;
    size_t data_size;
} __SGDescriptor;

typedef struct __SGDescriptorBlock_ {
    uint64_t unused_descriptors;

    struct __SGDescriptorBlock_ *next;
    __SGDescriptor descriptors[SG_DESC_BLOCK_SIZE];
} __SGDescriptorBlock;

typedef struct __SGDescriptorList_ {
    __SGDescriptorBlock *head;
    __SGDescriptorBlock *tail;

    size_t block_count;
    size_t descriptor_count;
} __SGDescriptorList;

#define __SG_CHECK_MALLOC(ptr) if (!ptr) { abort(); }

#define async __attribute__((noinline))
typedef __SGDescriptor *SGFuture;

#ifdef SG_TEST
#include "sgtest/test.h"
ModuleTestSet register_sgrt_tests();
#endif

#define SGRT_H
#endif
