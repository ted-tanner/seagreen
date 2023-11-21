![SeaGreen Pirate Ship Icon](/seagreen-pirate-ship-icon.svg)

# SeaGreen (seagreenlib)

An easy-to-use green threading library ~~for Sea~~ for C.

## How does SeaGreen work? Why use SeaGreen?

SeaGreen uses [coroutines](https://en.wikipedia.org/wiki/Coroutine) to change program flow in an intuitive way that allows blocking tasks (such as disk or network IO) to be performed asynchronously on a single OS thread. A simple and efficient scheduler manages "green threads"--lightweight subroutines that execute concurrently on a single OS thread. The performance and memory cost of managing and switching between green threads is orders of magnitude smaller than the penalty that is paid to have the OS manage those threads.

SeaGreen can operate in two modes. The default mode guaruntees that green threads are always scheduled on the same OS thread they were launched from, thus eliminating the need to use expensive synchronization primitives such as locks and atomics that complicate programming. The second mode allows green threads to be scheduled across a configurable number of OS threads. In practice, green threads will usualy still run on the same OS thread they were launched from even in the second mode (switching a green thread to a new OS thread can be costly), but this second mode gives SeaGreen the ability to multiplex threads across multiple OS threads if any one OS thread becomes overburdened.

Some additional niceties of SeaGreen:

* No [function coloring](https://journal.stuffwithstuff.com/2015/02/01/what-color-is-your-function/) difficulties. Green threads may be launched from anywhere in your program, making it super easy to integrate seagreenlib into existing codebases.
* SeaGreen is intuitive to use and won't turn your existing code into spaghetti.

## The SeaGreen Pirate's Code (invariants/rules for using the library)

* If ye call `async_yield()` in a function, ye must call that function using `async_run()` or ye shall walk the plank.
* Functions called using `async_run()` must be marked as `async` or ye shall walk the plank.
* If ye call `async_run()`, `await()`, or `async_yield()` without firs' callin' `seagreen_init_rt()`, ye shall walk the plank.
* If ye call `async_run()`, `await()`, or `async_yield()` after callin' `seagreen_free_rt()`, ye shall walk the plank.
* If ye call `seagreen_free_rt()` from inside a green thread (not on the main thread where `seagreen_init_rt()` was called), ye shall be ignored.
* If ye use SeaGreen expectin' it to not to allocate memory, ye shall walk the plank.

If ye don' heed these warnin's, ye may be squacked at by Seggie the SegFault parrot or worse, cause undefined behavior on yon sea.

## TODO

* Get working with GCC
* After same stack is used for a new thread 128 times, mmap and munmap (or VirtualAlloc with MEM_RESET)
* If a thread took longer than 25 microseconds to execute, skip scheduling for the next *n* iterations, where *n* = min(1 + floor(microseconds / 100), 5)
* Use stdint types in macros rather than int/long/short/etc
* Multithreaded scheduler (in a separate header).
  - Linked list for threads that is synchronized using something similar to Linux's RCU.
* Blocking thread pool for making synchronous functions async (sort of)
* Get running on xv6 (will need to implement a simple `malloc()` and `free()`)
* Automated tests
