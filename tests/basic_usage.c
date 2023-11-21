#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "seagreen.h"

async int foo(int a, int b) {
    struct timespec ts = {
        .tv_sec = 0,
        .tv_nsec = 250000000,
    };

    printf("foo() - 1\n");
    nanosleep(&ts, NULL);

    async_yield();

    printf("foo() - 2\n");
    nanosleep(&ts, NULL);

    async_yield();

    printf("foo() - 3\n");
    nanosleep(&ts, NULL);

    async_yield();

    printf("foo() - 4\n");
    nanosleep(&ts, NULL);

    return a + b;
}

async int bar(int a) {
    struct timespec ts = {
        .tv_sec = 0,
        .tv_nsec = 250000000,
    };

    printf("bar() - 1\n");
    nanosleep(&ts, NULL);

    async_yield();

    printf("bar() - 2\n");
    nanosleep(&ts, NULL);

    async_yield();

    printf("bar() - 3\n");
    nanosleep(&ts, NULL);

    async_yield();

    printf("bar() - 4\n");
    nanosleep(&ts, NULL);

    return a + 5;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();

    printf("Starting foo()...\n");
    CGNThreadHandle_int t1 = async_run(foo(1, 2));

    printf("Starting bar()...\n");
    CGNThreadHandle_int t2 = async_run(bar(3));

    printf("Awaiting...\n");

    int foo_res = await(t1);
    printf("foo() returned %d\n", foo_res);

    int bar_res = await(t2);
    printf("bar() returned %d\n", bar_res);

    seagreen_free_rt();

    return 0;
}

// TODO: Test with a bunch of threads
// TODO: Test calling async within async
// TODO: Test calling async from sync function called from within async
// TODO: Test with real IO
// TODO: Test with uint64s
// TODO: Test with floats
// TODO: Test with void
// TODO: Test on Linux
// TODO: Test on Risc V
// TODO: Test on x86 MacOS
// TODO: Test on x86 Windows

// int main(void) {
//     printf("Initializing runtime...\n");
//     seagreen_init_rt();

//     printf("Starting foo()...\n");
//     CGNThreadHandle_int t1 = ({						
// 	    CGNThreadHandle_int handle;

//             void *stack;
// 	    __CGNThread *t = __cgn_add_thread(&handle.id, &stack);
// 	    __cgn_savectx(&t->ctx);

// 	    _Bool temp_run_toggle = t->run_toggle;			
// 	    t->run_toggle = !t->run_toggle;				
                                                                        
// 	    if (temp_run_toggle) {
// 		uint64_t retval = (uint64_t) foo(1, 2);			
// 		__CGNThread *curr_thread = __cgn_get_curr_thread(); 
// 		curr_thread->return_val = retval;			
// 		curr_thread->state = __CGN_THREAD_STATE_DONE;       
// 		__cgn_scheduler();                                  
// 	    } else {
// 		__cgn_set_stack_ptr(&t->ctx, stack);
// 	    }

                                                                        
// 	    handle;                                                 
// 	});

//     printf("Starting bar()...\n");
//     CGNThreadHandle_int t2 = ({						
// 	    CGNThreadHandle_int handle;

//             void *stack;
// 	    __CGNThread *t = __cgn_add_thread(&handle.id, &stack);
// 	    __cgn_savectx(&t->ctx);
                                                                        
// 	    _Bool temp_run_toggle = t->run_toggle;			
// 	    t->run_toggle = !t->run_toggle;				
                                                                        
// 	    if (temp_run_toggle) {
// 		uint64_t retval = (uint64_t) bar(3);			
// 		__CGNThread *curr_thread = __cgn_get_curr_thread(); 
// 		curr_thread->return_val = retval;			
// 		curr_thread->state = __CGN_THREAD_STATE_DONE;       
// 		__cgn_scheduler();                                  
// 	    } else {
// 		__cgn_set_stack_ptr(&t->ctx, stack);
// 	    }
                                                                        
// 	    handle;                                                 
// 	});

//     printf("Awaiting...\n");

//     int foo_res = ({                                               
// 	    __CGNThread *t_curr = __cgn_get_curr_thread();	
// 	    t_curr->awaited_thread_id = (t1).id;		
// 	    t_curr->state = __CGN_THREAD_STATE_WAITING;       
                                                                        
// 	    uint64_t pos = (t1).id % __CGN_THREAD_BLOCK_SIZE;
// 	    __CGNThreadBlock *block = __cgn_get_block((t1).id);
                                                                        
// 	    __CGNThread *t = &block->threads[pos];		
// 	    t->awaiting_thread_count++;                       
                                                                        
// 	    /* Because thread is waiting, the curr thread */	
// 	    /* won't be scheduled until awaited thread has */	
// 	    /* finished its execution */			
// 	    async_yield();					
// 	    t_curr->state = __CGN_THREAD_STATE_RUNNING;       
// 	    t_curr->yield_toggle = 0;				
                                                                        
// 	    uint64_t return_val = block->threads[pos].return_val;
// 	    if (!t->awaiting_thread_count) {			
// 		__cgn_remove_thread(block, pos);		
// 	    }							
                                                                        
// 	    return_val;                                       
//  	});
//     printf("foo() returned %d\n", foo_res);

//     int bar_res = ({                                               
// 	    __CGNThread *t_curr = __cgn_get_curr_thread();	
// 	    t_curr->awaited_thread_id = (t2).id;		
// 	    t_curr->state = __CGN_THREAD_STATE_WAITING;       
                                                                       
// 	    uint64_t pos = (t2).id % __CGN_THREAD_BLOCK_SIZE;
// 	    __CGNThreadBlock *block = __cgn_get_block((t2).id);
                                                                       
// 	    __CGNThread *t = &block->threads[pos];		
// 	    t->awaiting_thread_count++;                       
                                                                       
// 	    /* Because thread is waiting, the curr thread */	
// 	    /* won't be scheduled until awaited thread has */	
// 	    /* finished its execution */			
// 	    async_yield();					
// 	    t_curr->state = __CGN_THREAD_STATE_RUNNING;       
// 	    t_curr->yield_toggle = 0;				
                                                                        
// 	    uint64_t return_val = block->threads[pos].return_val;
// 	    if (!t->awaiting_thread_count) {			
// 		__cgn_remove_thread(block, pos);		
// 	    }							
                                                                        
// 	    return_val;                                       
//  	});
//     printf("bar() returned %d\n", bar_res);

//     seagreen_free_rt();

//     return 0;
// }
