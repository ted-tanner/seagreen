![SeaGreen Pirate Ship Icon](/seagreen-pirate-ship-icon.svg)

# SeaGreen (seagreenlib)

An easy-to-use green threading library ~~for Sea~~ for C.

## How does SeaGreen work? Why use SeaGreen?

SeaGreen uses [coroutines](https://en.wikipedia.org/wiki/Coroutine) to change program flow in an intuitive way that allows blocking tasks (such as disk or network IO) to be performed asynchronously on a single OS thread. A simple and efficient scheduler manages "green threads"--lightweight subroutines that execute concurrently on a single OS thread. The performance and memory cost of managing and switching between green threads is orders of magnitude smaller than the penalty that is paid to have the OS manage those threads.

Some niceties of SeaGreen:

* No [function coloring](https://journal.stuffwithstuff.com/2015/02/01/what-color-is-your-function/) difficulties. Green threads may be launched from anywhere in your program, making it super easy to integrate seagreenlib into existing codebases.
* SeaGreen is intuitive to use and won't turn your existing code into spaghetti.

## The SeaGreen Pirate's Code (invariants/rules for using the library)

* If ye call `async_yield()` in a function, ye mus' mark tha' function as `async` and call it only using `async_run()` or ye shall walk the plank.
* If ye call `async_run()`, `await()`, or `async_yield()` without firs' callin' `seagreen_init_rt()` or after callin’ `seagreen_free_rt()`, ye shall walk the plank.
* Functions called with `async_run()` may only return values that are 8 bytes (or more wee).
* If ye call `seagreen_free_rt()` from inside a green thread (not on the main thread where `seagreen_init_rt()` was called), ye shall be ignored.
* If ye use SeaGreen expectin' it to not to allocate memory, ye shall walk the plank.

If ye don' heed these warnin's, ye may be squawked at by Seggie the SegFault parrot or worse, cause undefined behavior on yon C.

## TODO

* Can we get rid of the thread's `yield_toggle` and instead have `savectx()` return something different when loading the ctx in than when continuing on?
* Thoughts on current segfault problem in test #2
  - The segfault is happening in async_yield() right after we loadctx and return program flow back to async_yield() and then try to assign to a stack variable. The segfault is a stack problem.
  - The stack we are restoring to is the main thread stack that was saved in `await()`. However, `loadctx()` is taking us to `async_yield()` (where there is another call to `savectx()`). The stack for the main thread will not have the vars needed for `async_yield()`
* Add a test that checks `await()`ing a thread from inside another thread being `await()`ed
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
