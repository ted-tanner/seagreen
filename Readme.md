![SeaGreen Pirate Ship Icon](/seagreen-pirate-ship-icon.svg)

# SeaGreen (seagreenlib)

**WARNING: SeaGreen has been tested in a very limited set of environments. There are likely bugs that will cause unsoundness in untested environments.**

An easy-to-use green threading library ~~for Sea~~ for C.

## How does SeaGreen work? Why use SeaGreen?

SeaGreen uses [coroutines](https://en.wikipedia.org/wiki/Coroutine) to change program flow in an intuitive way that allows blocking tasks (such as disk or network IO) to be performed asynchronously on a single OS thread. A simple and efficient scheduler manages "green threads"--lightweight subroutines that execute concurrently on a single OS thread. The performance and memory cost of managing and switching between green threads is orders of magnitude smaller than the penalty that is paid to have the OS manage those threads.

Some niceties of SeaGreen:

* No [function coloring](https://journal.stuffwithstuff.com/2015/02/01/what-color-is-your-function/) difficulties. Green threads may be launched from anywhere in your program, making it super easy to integrate seagreenlib into existing codebases.
* SeaGreen is intuitive to use and won't turn your existing code into spaghetti.

## The SeaGreen Pirate's Code (invariants/rules for using the library)

* If ye call `async_yield()` in a function, ye mus' mark tha' function as `async` and call it only using `async_run()` or `run_as_sync()` or ye shall walk the plank.
* If ye call `async_run()`, `run_as_sync()`, `await()`, or `async_yield()` without firs' callin' `seagreen_init_rt()` or after callin’ `seagreen_free_rt()`, ye shall walk the plank.
* Functions called with `async_run()` may only return values that are 8 bytes (or more wee).
* If ye call `seagreen_free_rt()` from inside a green thread (not on the main thread where `seagreen_init_rt()` was called), ye shall be ignored.
* If ye use SeaGreen expectin' it to not to allocate memory, ye shall walk the plank.

If ye don' heed these warnin's, ye may be squawked at by Seggie the SegFault parrot or worse, cause undefined behavior on yon C.

## TODO

* Fix: 2nd test loops indefinitely. We just changed the async_run implementation to switch to the scheduler thread.

* Documentation
* Can/should we just use setjmp
* Struct of arrays rather than array of structs for the blocks. Makes finding in-use threads quicker (can use uint64_t)
* Rename variables local to macros (is this necessary?)
* After same stack is used for a new thread 128 times, mmap and munmap (or VirtualAlloc with MEM_RESET)
* If a thread took longer than 25 microseconds to execute, skip scheduling for the next *n* iterations, where *n* = min(1 + floor(microseconds / 100), 5)
* Use stdint types in macros rather than int/long/short/etc
* Multithreaded scheduler (in a separate header).
  - Linked list for threads that is synchronized using something similar to Linux's RCU.
  - Blocking thread pool for making synchronous functions async (sort of)


loadctx should return a pointer passed in as an arg
savenewctx should return 0

