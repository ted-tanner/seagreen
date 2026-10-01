#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#define _DARWIN_C_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include "seagreen.h"
#if defined(_WIN32)
#include <io.h>
#include <process.h>
#else
#include <signal.h>
#include <sys/wait.h>
#endif
#include <string.h>

static const char *failure = TEST_FAILURE;
static unsigned allocation_calls, protection_calls;
static int fail_allocation(void) {
    ++allocation_calls;
    return (strcmp(failure, "stack-allocation") == 0 && allocation_calls == 1) ||
           (strcmp(failure, "scheduler-allocation") == 0 && allocation_calls == 2);
}
static int fail_protection(void) {
    ++protection_calls;
    return (strcmp(failure, "stack-protection") == 0 && protection_calls == 1) ||
           (strcmp(failure, "scheduler-protection") == 0 &&
            protection_calls == __CGN_THREAD_BLOCK_SIZE + 1);
}
static void *test_calloc(size_t count, size_t size) {
    return strcmp(failure, "block-allocation") == 0 ? NULL : calloc(count, size);
}
#if defined(_WIN32)
static void *test_VirtualAlloc(void *address, SIZE_T size, DWORD kind, DWORD protection) {
    return fail_allocation() ? NULL : VirtualAlloc(address, size, kind, protection);
}
static BOOL test_VirtualProtect(void *address, SIZE_T size, DWORD protection, DWORD *old) {
    return fail_protection() ? FALSE : VirtualProtect(address, size, protection, old);
}
#define VirtualAlloc test_VirtualAlloc
#define VirtualProtect test_VirtualProtect
#else
static void *test_mmap(void *address, size_t size, int protection, int flags, int fd, off_t offset) {
    return fail_allocation() ? MAP_FAILED : mmap(address, size, protection, flags, fd, offset);
}
static int test_mprotect(void *address, size_t size, int protection) {
    return fail_protection() ? -1 : mprotect(address, size, protection);
}
#define mmap test_mmap
#define mprotect test_mprotect
#endif
#define calloc test_calloc
#include "../../src/seagreen.c"
#undef calloc
#if defined(_WIN32)
#undef VirtualAlloc
#undef VirtualProtect
#else
#undef mmap
#undef mprotect
#endif

static void run_failure(void);

// Shared fixture: require the scenario to abort with the intended diagnostic.
static void check_failure(const char *executable) {
    FILE *errors = tmpfile();
    assert(errors != NULL);
    fflush(NULL);
#if defined(_WIN32)
    int saved_stderr = _dup(2);
    assert(saved_stderr >= 0);
    assert(_dup2(_fileno(errors), 2) == 0);
    const char *arguments[] = {executable, "--child", NULL};
    intptr_t status = _spawnv(_P_WAIT, executable, arguments);
    assert(_dup2(saved_stderr, 2) == 0);
    _close(saved_stderr);
    assert(status != -1 && status != 0);
#else
    (void)executable;
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        alarm(15);
        if (dup2(fileno(errors), STDERR_FILENO) == -1) _Exit(2);
        run_failure();
        _Exit(0);
    }
    int status;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
#endif
    rewind(errors);
    char output[4096];
    size_t length = fread(output, 1, sizeof(output) - 1, errors);
    output[length] = '\0';
    assert(strstr(output, "seagreen: ") != NULL);
    assert(strstr(output, TEST_DIAGNOSTIC) != NULL);
    assert(strstr(output, "runtime error:") == NULL);
    fclose(errors);
    printf("%s: passed\n", TEST_FAILURE);
}

int main(int argc, char **argv) {
    if (argc == 2) {
        assert(strcmp(argv[1], "--child") == 0);
        run_failure();
        return 0;
    }
    assert(argc == 1);
    check_failure(argv[0]);
    return 0;
}
