#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

#include "seagreen.h"
#include "cgntest/test.h"

jmp_buf jump_buf;

static void abort_handler(int signum) {
    (void) signum;
    siglongjmp(jump_buf, 1);
}

int main(int argc, char **argv) {
    _Bool no_capture = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--no-capture") == 0) {
            no_capture = 1;
	}
    }

    register_seagreen_tests();

    CgnTestSet *test_sets = get_test_sets();
    int test_set_count = get_test_set_count();
    
    printf("Running tests...\n");

    int passed = 0;
    int failed = 0;

    signal(SIGABRT, abort_handler);
    
    for (int i = 0; i < test_set_count; ++i) {
        CgnTestSet *set = test_sets + i;

        printf("\n%s\n", set->name);
        
        for (int j = 0; j < set->count; ++j) {
            Test *test = set->tests + j;
            
            printf("\t* %s --> ", test->test_name);
            fflush(stdout);

            // Redirect stdout if --no-capture parameter wasn't specified
            int temp_stdout;
            int pipes[2];

            if (!no_capture) {
                temp_stdout = dup(fileno(stdout));
                pipe(pipes);
                dup2(pipes[1], fileno(stdout));
            }
            
            TEST_RESULT result = TEST_FAIL;

            // Set jmp_return so assertions can jump back here
            int jmp_return = sigsetjmp(jump_buf, 1);
            if (!jmp_return) {
                result = test->test_func();
	    }

            if (!no_capture) {
                // Terminate test output with a zero
                write(pipes[1], "", 1);

                // Restore stdout
                fflush(stdout);
                dup2(temp_stdout, fileno(stdout));
            }

            if (result == TEST_PASS) {
                printf("%s%s%s\n", ASCII_GREEN, "PASS", ASCII_DEFAULT);
                ++passed;
            } else {
                printf("%s%s%s\n", ASCII_RED, "FAIL", ASCII_DEFAULT);
                ++failed;

                if (!no_capture) {
                    // Read output from test
                    _Bool is_first = 1;
                    while (1) {
                        char c;
                        read(pipes[0], &c, 1);

                        if (!c) {
                            if (!is_first) {
                                putc('\n', stdout);
			    }
                        
                            break;
                        }


                        if (c == '\n' || is_first) {
                            printf("\n\t\t");
			}
                    
                        if (c != '\n') {
                            putc(c, stdout);
			}

                        is_first = 0;
                    }
                }
            }
        }
    }

    if (failed == 0) {
        printf("\n%s%s%s - ", ASCII_GREEN, "PASS", ASCII_DEFAULT);
    } else {
        printf("\n%s%s%s - ", ASCII_RED, "FAIL", ASCII_DEFAULT);
    }

    printf("%u test(s) passed, %u test(s) failed\n", passed, failed);
    
    return 0;
}


// Planned interface
// #include "cgnrt.h"
// #include "cgnfut.h"

// typedef struct _MyOwnType {
//     float a;
//     float b;
// } MyOwnType;

// // If necessary
// CGNDeclare(MyOwnType);
// CGNDeclare(int);
// CGNDeclare(void);

// CGNFut(MyOwnType) fake_io() {
//     MyOwnType data = {
// 	.a = 2.5f,
// 	.b = 7.5f,
//     };

//     return cgn_fut_from(data);
// }

// CGNFut(int) io_func() {
//     CGNFut(int) res = cgn_block_on(do_io());
//     return res;
// }

// int cpu_func(CGNFut(void) io_res) {
//     int res = cgn_await(io_func());
//     float a = cgn_await(fake_io());
//     return res + a;
// }

// int cgn_main() {
//     int res = cpu_func();
//     return 0;
// }

// int main() {
//     int cgnrt_err;
//     int cgn_main_return = cgn_start_rt(&cgn_main, &cgnrt_err);

//     if (cgnrt_err != 0) {
// 	// Handle error from the runtime
//     }

//     return cgn_main_return;
// }
