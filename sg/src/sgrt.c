#include "sgrt.h"

#include <stdio.h>

__SGDescriptorList __sg_descl;

static void add_block(void) {
    __SGDescriptorBlock *block = malloc(sizeof(__SGDescriptorBlock));
    __SG_CHECK_MALLOC(block);

    block->next = 0;
    block->unused_descriptors = UINT64_MAX;

    if (!__sg_descl.head) {
	__sg_descl.head = block;
	__sg_descl.tail = block;
    } else {
	__sg_descl.tail->next = block;
	__sg_descl.tail = block;
    }
}

static void add_descriptor(__SGDescriptor desc) {
    if (!__sg_descl.head) {
	add_block();
	__sg_descl.head = __sg_descl.tail;
    }

    __SGDescriptorBlock *block = __sg_descl.head;

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

#ifdef SG_TEST

#include "sgtest/test.h"

static TEST_RESULT test_add_descriptor(void) {
    sgasrt(__sg_descl.head == 0, "Invalid initial state of descriptor list");
    sgasrt(__sg_descl.tail == 0, "Invalid initial state of descriptor list");

    __SGDescriptor desc = {
	.data = (void*) 0xdeadbeef,
	.data_size = 42,
    };
    add_descriptor(desc);

    sgasrt(__sg_descl.head > 0, "Descriptor list head should get set when first descriptor is added");
    sgasrt(__sg_descl.head == __sg_descl.head, "Descriptor list tail should equal head when there is only one block");

    sgasrt(__sg_descl.head->next == 0, "Descriptor list should only have one block");
    sgasrt(__sg_descl.head->unused_descriptors == 0x7fffffffffffffffULL, "Descriptor list should have 63 unused descriptors");

    // TODO: Test desc is in the correct position in the block, then change desc.data_size for the next desc to be added

    add_descriptor(desc);
    sgasrt(__sg_descl.head->next == 0, "Descriptor list should only have one block");
    sgasrt(__sg_descl.head->unused_descriptors == 0x3fffffffffffffffULL, "Descriptor list should have 62 unused descriptors");

    add_descriptor(desc);
    sgasrt(__sg_descl.head->next == 0, "Descriptor list should only have one block");
    sgasrt(__sg_descl.head->unused_descriptors == 0x1fffffffffffffffULL, "Descriptor list should have 61 unused descriptors");

    // TODO: Try freeing a descriptor and see if it gets marked as unused, then try adding one and see if it gets marked as used
    // TODO: Try adding a descriptor to a full block and see if a new block gets allocated

    return TEST_PASS;
}

ModuleTestSet register_sgrt_tests() {
    ModuleTestSet set = {
        .module_name = __FILE__,
        .tests = {0},
        .count = 0,
    };

    register_test(&set, test_add_descriptor);
    return set;
}

#endif
