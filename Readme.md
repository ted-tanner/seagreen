![SeaGreen Pirate Ship Icon](/seagreen-pirate-ship-icon.svg)

# SeaGreen (libseagreen) - `async_run`/`await` for C

An easy-to-use green threading library ~~for Sea~~ for C.

## How does SeaGreen work? Why use SeaGreen?

SeaGreen uses stackful [coroutines](https://en.wikipedia.org/wiki/Coroutine) to change program flow in an intuitive way that allows blocking tasks (such as disk or network IO) to be performed asynchronously on a single OS thread. A simple and efficient scheduler manages "green threads"--lightweight subroutines that execute concurrently on a single OS thread. The performance and memory cost of managing and switching between green threads is orders of magnitude smaller than the penalty that is paid to have the OS manage those threads.

Some niceties of SeaGreen:

* No [function coloring](https://journal.stuffwithstuff.com/2015/02/01/what-color-is-your-function/) difficulties. Green threads may be launched from anywhere in your program, making it super easy to integrate libseagreen into existing codebases.
* SeaGreen is intuitive to use and won't turn your existing code into spaghetti.

## Examples

### Context switching on a single OS thread

```c
#include "seagreen.h"
#include <inttypes.h>
#include <stdio.h>

typedef struct { uint64_t a, b; } foo_args;

uint64_t foo(void *arg) {
    foo_args *args = arg;
    printf("foo() started\n");
    async_yield();
    printf("foo() finished\n");
    return args->a + args->b;
}

uint64_t bar(void *arg) {
    uint64_t *a = arg;
    printf("bar() started\n");
    async_yield();
    printf("bar() finished\n");
    return *a + 2;
}

int main(void) {
    seagreen_init_rt();

    foo_args args = {1, 2};
    uint64_t a = 3;
    CGNThreadHandle t1 = async_run(foo, &args);
    CGNThreadHandle t2 = async_run(bar, &a);

    uint64_t foo_result = await(t1);
    uint64_t bar_result = await(t2);
    printf("foo() returned %" PRIu64 "\n", foo_result); // 3
    printf("bar() returned %" PRIu64 "\n", bar_result); // 5

    seagreen_free_rt();
    return 0;
}
```

Output:

```text
foo() started
bar() started
foo() finished
bar() finished
foo() returned 3
bar() returned 5
```

### Handle IO without blocking on a single OS thread

```c
#include "seagreen.h"
#include "your_io.h"
#include <stdio.h>

uint64_t handle_io(void *arg) {
    (void)arg;
    begin_io();
    while (io_result() != IO_DONE) {
        async_yield(); // Let other tasks run while the I/O is pending
    }
    return 0;
}

uint64_t other_work(void *arg) {
    (void)arg;
    printf("Doing other work while I/O is pending\n");
    return 0;
}

int main(void) {
    seagreen_init_rt();

    CGNThreadHandle io = async_run(handle_io, NULL);
    CGNThreadHandle work = async_run(other_work, NULL);
    await(io);
    await(work);
    printf("I/O completed\n");

    seagreen_free_rt();
    return 0;
}
```

### Green threads across multiple OS threads with pthreads

```c
#include "seagreen.h"
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { uint64_t input, result; } worker_args;

uint64_t square(void *arg) {
    uint64_t n = *(uint64_t *)arg;
    async_yield();
    return n * n;
}

void *run_worker(void *arg) {
    worker_args *args = arg;
    seagreen_init_rt();

    uint64_t numbers[] = {args->input, args->input + 1};
    CGNThreadHandle first = async_run(square, &numbers[0]);
    CGNThreadHandle second = async_run(square, &numbers[1]);
    uint64_t a = await(first);
    uint64_t b = await(second);
    args->result = a + b;

    seagreen_free_rt();
    return NULL;
}

int main(void) {
    pthread_t threads[2];
    worker_args args[] = {{3, 0}, {5, 0}};

    for (unsigned i = 0; i < 2; ++i) {
        int error = pthread_create(&threads[i], NULL, run_worker, &args[i]);
        if (error) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned i = 0; i < 2; ++i) {
        int error = pthread_join(threads[i], NULL);
        if (error) {
            fprintf(stderr, "pthread_join: %s\n", strerror(error));
            exit(EXIT_FAILURE);
        }
        printf("Worker %u returned %" PRIu64 "\n", i + 1, args[i].result);
    }
    return 0;
}
```

Output:

```text
Worker 1 returned 25
Worker 2 returned 61
```

## The SeaGreen Pirate's Code (invariants/rules for using the library)

* If ye call `async_run()`, `await()`, or `async_yield()` without firs' callin' `seagreen_init_rt()` or after callin' `seagreen_free_rt()`, ye shall walk the plank.
* Each OS thread mus' initialize its own runtime. Use a handle only on the OS thread and within the runtime instance that created it or ye shall walk the plank.
* Callbacks passed to `async_run()` mus' have the signature `uint64_t function(void *arg)`; do not cast incompatible function pointers. Ye never know what may happen if ye do.
* Never await yer own handle or create circular waits between threads. Awaiting yourself, or reaching a state where no thread can run, will sink yer ship.
* If ye call `seagreen_free_rt()` from inside a green thread (not on the main thread where `seagreen_init_rt()` was called), ye shall be ignored.
* Scheduling be cooperative: a task mus' yield, await, or return for another task to run. Otherwise, ye be foulin' the riggin’.
* Careful with recursion. If ye use deep recursion, ye be stirrin’ up rough seas. If yon stack grow too big, ye may be thrust overboard.
* If ye use SeaGreen expectin' it to not to allocate memory, ye shall walk the plank.
* Be mindful o' the lifetime o' function arguments passed to `async_run()`. If ye `await()` in the same function, stack allocation be fine. But if ye save the handles and `await()` them elsewhere, they should be in a buffer whose lifetime be at least as long as the threads'.

If ye don' heed these warnin's, ye may be squawked at by Seggie the SegFault parrot or worse, cause undefined behavior on yon C.

## Running Tests

Run `./build.sh test` for the native suite, or `./build.sh test release` for
optimized tests. Select a test by number or filename, for example
`./build.sh test 27` or `./build.sh test 27-multiple-waiters.c`.

## Architecture and System Support

The scheduler's test suite has been run on `aarch64-macos` and
`x86_64-macos` in debug and optimized builds.

The assembly and runtime also cross-compile with Zig for `x86_64-linux-gnu`,
`aarch64-linux-gnu`, `x86_64-windows-gnu`, and `aarch64-windows-gnu`.
Execution on those operating systems still needs validation; compilation alone
is not a claim of full platform support.

On x86_64, ARM64 (including macOS), and the experimental RV64 path, saved registers live in each thread's context
storage, so subsequent calls cannot overwrite them. The context routines take
an entire `__CGNThread`, rather than a standalone stack-pointer variable.
x86_64 stack entry follows the platform calling convention, including Windows
shadow space. Context switches preserve the ABI's nonvolatile registers and
floating-point control state.

If you would like to add support for another target, please submit a PR! We'd love to support as many targets as possible. Adding support for a target must not break or affect the performance of an already-supported target.
