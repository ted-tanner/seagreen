![SeaGreen Pirate Ship Icon](/seagreen-pirate-ship-icon.svg)

# SeaGreen (libseagreen) - `async_run`/`await` for C

An easy-to-use green threading library ~~for Sea~~ for C.

## How does SeaGreen work? Why use SeaGreen?

SeaGreen uses [coroutines](https://en.wikipedia.org/wiki/Coroutine) to change program flow in an intuitive way that allows blocking tasks (such as disk or network IO) to be performed asynchronously on a single OS thread. A simple and efficient scheduler manages "green threads"--lightweight subroutines that execute concurrently on a single OS thread. The performance and memory cost of managing and switching between green threads is orders of magnitude smaller than the penalty that is paid to have the OS manage those threads.

Some niceties of SeaGreen:

* No [function coloring](https://journal.stuffwithstuff.com/2015/02/01/what-color-is-your-function/) difficulties. Green threads may be launched from anywhere in your program, making it super easy to integrate libseagreen into existing codebases.
* SeaGreen is intuitive to use and won't turn your existing code into spaghetti.

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

## TODO

* Improve readme with examples upfront. Focus on marketability upfront and then documentation later on.
* Add a section on building
* Thoughts on current segfault problem in test #2
  - The segfault is happening in async_yield() right after we loadctx and return program flow back to async_yield() and then try to assign to a stack variable. The segfault is a stack problem.
  - The stack we are restoring to is the main thread stack that was saved in `await()`. However, `loadctx()` is taking us to `async_yield()` (where there is another call to `savectx()`). The stack for the main thread will not have the vars needed for `async_yield()`
* Add a test that checks `await()`ing a thread from inside another thread being `await()`ed
* In `1-basic-usage.c`, there is a list of things that should be tested
* See if AI can think of more tests that should be added
* Documentation
* Ready/waiting state bits and the yield toggle into a single 64-bit word with bitfields
* Can we make it so the functions can return data of arbitrary size (rather than the current 8-byte return values)?
* After same stack is used for a new thread 128 times, mmap and munmap (or VirtualAlloc with MEM_RESET)
* If a thread took longer than 25 microseconds to execute, skip scheduling for the next *n* iterations, where *n* = min(1 + floor(microseconds / 64), 5)
* Use stdint types in macros rather than int/long/short/etc
* Multithreaded scheduler (in a separate header).
  - Linked list for threads that is synchronized using something similar to Linux's RCU.
  - Blocking thread pool for making synchronous functions async (sort of)
* Test on multiple different architectures and operating systems
