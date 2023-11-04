#include "cgnrt.h"

#include <stdio.h>

__CGNDescriptorList __cgn_descl;

static void add_block(void) {
    __CGNDescriptorBlock *block = malloc(sizeof(__CGNDescriptorBlock));
    __CGN_CHECK_MALLOC(block);

    block->next = 0;
    block->unused_descriptors = UINT64_MAX;

    if (!__cgn_descl.head) {
	__cgn_descl.head = block;
	__cgn_descl.tail = block;
    } else {
	__cgn_descl.tail->next = block;
	__cgn_descl.tail = block;
    }
}

static void add_descriptor(__CGNDescriptor desc) {
    if (!__cgn_descl.head) {
	add_block();
	__cgn_descl.head = __cgn_descl.tail;
    }

    __CGNDescriptorBlock *block = __cgn_descl.head;

    while (!block->unused_descriptors) {
	if (!block->next) {
	    add_block();
	}

        block = block->next;
    }

    size_t pos = 0;
    // Treat the unused_descriptors int as an array of bits and find the index
    // of the most significant bit
    for (; !((block->unused_descriptors << pos) & (1ULL << 63)); ++pos);

    // Mark descriptor as used
    block->unused_descriptors &= ~(1ULL << (63 - pos));

    block->descriptors[pos] = desc;
}

// TODO: Remove descriptor

#ifdef CGN_TEST

#include "cgntest/test.h"

static TEST_RESULT test_add_descriptor(void) {
    cgnasrt(__cgn_descl.head == 0, "Invalid initial state of descriptor list");
    cgnasrt(__cgn_descl.tail == 0, "Invalid initial state of descriptor list");

    __CGNDescriptor desc = {
	.data = (void*) 0xdeadbeef,
	.data_size = 42,
    };

    add_descriptor(desc);

    cgnasrt(__cgn_descl.head > 0, "Descriptor list head should get set when first descriptor is added");
    cgnasrt(__cgn_descl.head == __cgn_descl.head, "Descriptor list tail should equal head when there is only one block");

    cgnasrt(__cgn_descl.head->next == 0, "Descriptor list should only have one block");
    cgnasrt(__cgn_descl.head->unused_descriptors == 0x7fffffffffffffffULL, "Descriptor list should have 63 unused descriptors");
    cgnasrt(__cgn_descl.head->descriptors[0].data == (void*) 0xdeadbeef, "Incorrect descriptor data or position");
    cgnasrt(__cgn_descl.head->descriptors[0].data_size == 42, "Incorrect descriptor data or position");

    desc.data = (void*) 0xcafebabe;
    desc.data_size = 43;

    add_descriptor(desc);

    cgnasrt(__cgn_descl.head->next == 0, "Descriptor list should only have one block");
    cgnasrt(__cgn_descl.head->unused_descriptors == 0x3fffffffffffffffULL, "Descriptor list should have 62 unused descriptors");
    cgnasrt(__cgn_descl.head->descriptors[0].data == (void*) 0xdeadbeef, "Incorrect descriptor data or position");
    cgnasrt(__cgn_descl.head->descriptors[0].data_size == 42, "Incorrect descriptor data or position");
    cgnasrt(__cgn_descl.head->descriptors[1].data == (void*) 0xcafebabe, "Incorrect descriptor data or position");
    cgnasrt(__cgn_descl.head->descriptors[1].data_size == 43, "Incorrect descriptor data or position");

    // TODO: Try freeing a descriptor and see if it gets marked as unused, then try adding one and see if it gets marked as used
    // TODO: Try adding a descriptor to a full block and see if a new block gets allocated

    return TEST_PASS;
}

void register_cgnrt_tests() {
    CgnTestSet *set = new_test_set(__FILE__);

    register_test(set, test_add_descriptor);
}

#endif
