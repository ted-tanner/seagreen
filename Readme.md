# SeaGreen (seagreenlib)

A green threading library ~~for Sea~~ for C.

![SeaGreen Pirate Ship Icon](/seagreen-pirate-ship-icon.svg)

## The SeaGreen Pirate's Code (invariants/rules for using the library)

* If ye call `async_run()` or `async_block_on()` without first calling `cgn_init_rt()` or `cgn_init_rt_single_os_thread()`, ye shall walk the plank.
* If ye call `async_run()` or `async_block_on()` after calling `cgn_free_rt()`, ye shall walk the plank.
* If ye use SeaGreen expectin' it to not call `malloc()` or `free()`, ye shall walk the plank.
* If ye use `cgn_init_rt_single_os_thread()` and call `async_run()` or `async_block()` on more than one OS thread, ye shall walk the plank (if ye get caught).

If ye don' heed these warnin's, ye may be swallowed by Seggie the SegFault monster or worse, cause undefined behavior on yon sea.

## TODO

* All __cgn_threadl accesses should be made safe for multithreading
* Need to synchronize adding/removing threads, or linked-list may become disjointed
* All double-underscore functions and typedefs can be moved to a "private" header
* Don't need to remove a block. If it was needed at some point of the program, it will likely be needed again.
