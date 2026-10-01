#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
// Exercise the private queue operations without adding a public test API.
#include "../src/seagreen.c"

int main(void) {
    __CGNThread a = {0}, b = {0};
    __cgn_ready_queue_enqueue(&a);
    __cgn_ready_queue_remove(&a);
    assert(__cgn_ready_queue.count == 0);
    assert(__cgn_ready_queue.head == NULL && __cgn_ready_queue.tail == NULL);
    assert(a.ready_next == NULL && a.ready_prev == NULL);
    assert(__cgn_ready_queue_dequeue() == NULL);

    __cgn_ready_queue_enqueue(&a);
    __cgn_ready_queue_enqueue(&b);
    __cgn_ready_queue_remove(&a);
    assert(__cgn_ready_queue.head == &b && __cgn_ready_queue.tail == &b);
    assert(b.ready_next == &b && b.ready_prev == &b);
    assert(__cgn_ready_queue_dequeue() == &b);
    assert(__cgn_ready_queue.count == 0);
    assert(__cgn_ready_queue.head == NULL && __cgn_ready_queue.tail == NULL);
    puts("Ready queue removal and reuse passed");
    return 0;
}
