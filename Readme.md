![SeaGreen Pirate Ship Icon](/seagreen-pirate-ship-icon.svg)

# SeaGreen (seagreenlib)

An easy-to-use green threading library ~~for Sea~~ for C.

## How does SeaGreen work? Why use SeaGreen?

SeaGreen uses [coroutines](https://en.wikipedia.org/wiki/Coroutine) to change program flow in an intuitive way that allows blocking tasks (such as disk or network IO) to be performed asynchronously on a single OS thread. A simple and efficient scheduler manages "green threads"--lightweight subroutines that execute concurrently on a single OS thread. The performance and memory cost of managing and switching between green threads is orders of magnitude smaller than the penalty that is paid to have the OS manage those threads.

SeaGreen can operate in two modes. The default mode guaruntees that green threads are always scheduled on the same OS thread they were launched from, thus eliminating the need to use expensive synchronization primitives such as locks and atomics that complicate programming. The second mode allows green threads to be scheduled across a configurable number of OS threads. In practice, green threads will usualy still run on the same OS thread they were launched from even in the second mode (switching a green thread to a new OS thread can be costly), but this second mode gives SeaGreen the ability to balance load across multiple OS threads if any one OS thread becomes overburdened.

Some additional niceties of SeaGreen:

* No [function coloring](https://journal.stuffwithstuff.com/2015/02/01/what-color-is-your-function/) difficulties. Green threads may be launched from anywhere in your program, making it super easy to integrate seagreenlib into existing codebases.
* The scheduler is completely lock-free, even in a multithreaded paradigm. Using a technique similar to [Linux's RCU](https://pdos.csail.mit.edu/6.828/2018/readings/rcu-decade-later.pdf), SeaGreen's scheduler for manages to efficiently launch and tear down green threads using just a few CPU cycles regardless of the number of running threads.
* SeaGreen is intuitive to use and won't turn your existing code into spaghetti.
* SeaGreen can push synchronous functions onto a pool of blocking threads, so existing synchronous functions do not need to be modified to be run asynchronously.

## The SeaGreen Pirate's Code (invariants/rules for using the library)

* If ye call `async_run()`, `await()`, `async_yield()`, `async_retval()`, or `async_block_on()` without first calling `seagreen_init_rt()`, ye shall walk the plank.
* If ye call `async_run()`, `await()`, `async_yield()`, `async_retval()`, or `async_block_on()` after calling `seagreen_free_rt()`, ye shall walk the plank.
* If ye call `sync_block_on(fn())`, ye shall be prepared for the arguments passed to `fn()` to be accessed on a different thread or ye shall walk the plank (if ye get caught).
* If ye use SeaGreen expectin' it to not call `malloc()` or `free()`, ye shall walk the plank.
* If ye use SeaGreen's internal functions (prefixed with `__cgn_`), ye shall walk the plank.

If ye don' heed these warnin's, ye may be squacked at by Seggie the SegFault parrot or worse, cause undefined behavior on yon sea.

## TODO

* Stacks of threads are conflicing
* Use stdint types in macros rather than int/long/short/etc
* Multithreaded scheduler (in a separate header).
  - Linked list for threads that is synchronized using something similar to Linux's RCU.
* Blocking thread pool for making synchronous functions async (sort of)
* Get running on xv6 (will need to implement a simple `malloc()` and `free()`)
* Automated tests
