#ifndef CGNRT_H

#include <stdint.h>
#include <stdlib.h>

// This should stay at 64. This allows a 64-bit integer to track unused
// descriptors in a block
#define CGN_DESC_BLOCK_SIZE 64

// Descriptors will be stored in arrays of 256. A linked list will track each
// block
//
// TODO: User of library should be able to pass a function pointer to malloc()
// and free() implementations to use for the descriptor list

typedef struct _CGNDescriptor_ {
    void *data;
    size_t data_size;
} __CGNDescriptor;

typedef struct __CGNDescriptorBlock_ {
    uint64_t unused_descriptors;

    struct __CGNDescriptorBlock_ *next;
    __CGNDescriptor descriptors[CGN_DESC_BLOCK_SIZE];
} __CGNDescriptorBlock;

typedef struct __CGNDescriptorList_ {
    __CGNDescriptorBlock *head;
    __CGNDescriptorBlock *tail;

    size_t block_count;
    size_t descriptor_count;
} __CGNDescriptorList;

#define __CGN_CHECK_MALLOC(ptr) if (!ptr) { abort(); }

#define async __attribute__((noinline))
typedef __CGNDescriptor *CGNFuture;

#ifdef CGN_TEST
#include "cgntest/test.h"
void register_cgnrt_tests();
#endif

#define CGNRT_H
#endif
