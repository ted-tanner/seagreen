#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "sgrt.h"
#include "sgtest/test.h"

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
    
    static ModuleTestSet test_sets[TEST_MAX_SET_COUNT] = {0};
    
    int test_set_count = 0;
    test_sets[test_set_count++] = register_sgrt_tests();

    printf("Running tests...\n");

    int passed = 0;
    int failed = 0;

    signal(SIGABRT, abort_handler);
    
    for (int i = 0; i < test_set_count; ++i) {
        ModuleTestSet *set = test_sets + i;

        printf("\n%s\n", set->module_name);
        
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
// #include "sgrt.h"
// #include "sgfut.h"

// typedef struct _MyOwnType {
//     float a;
//     float b;
// } MyOwnType;

// // If necessary
// SGDeclare(MyOwnType);
// SGDeclare(int);
// SGDeclare(void);

// SGFut(MyOwnType) fake_io() {
//     MyOwnType data = {
// 	.a = 2.5f,
// 	.b = 7.5f,
//     };

//     return sg_fut_from(data);
// }

// SGFut(int) io_func() {
//     SGFut(int) res = sg_block_on(do_io());
//     return res;
// }

// int cpu_func(SGFut(void) io_res) {
//     int res = sg_await(io_func());
//     float a = sg_await(fake_io());
//     return res + a;
// }

// int sg_main() {
//     int res = cpu_func();
//     return 0;
// }

// int main() {
//     int sgrt_err;
//     int sg_main_return = sg_start_rt(&sg_main, &sgrt_err);

//     if (sgrt_err != 0) {
// 	// Handle error from the runtime
//     }

//     return sg_main_return;
// }
