#include <aio.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "seagreen.h"

#define FILE_COUNT 4096

async int write_file(struct aiocb *aio) {
    aio_write(aio);

    while (aio_error(aio) == EINPROGRESS) {
        async_yield();
    }

    return aio_return(aio);
}

int main(void) {
    printf("Preparing data...\n");

    char file_name[32];
    char *lorem_ipsum =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod \
tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis \
nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis \
aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat \
nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui \
officia deserunt mollit anim id est laborum.";

    size_t lorem_ipsum_len = strlen(lorem_ipsum);
    size_t extra_len = 10;  // 4-digit number + 5 linebreaks + null terminator
    size_t data_len = lorem_ipsum_len * 2 + extra_len;

    char *data_buf = (char *)malloc(data_len * FILE_COUNT);

    struct aiocb *aio_list =
        (struct aiocb *)malloc(FILE_COUNT * sizeof(struct aiocb));

    for (int i = 0; i < FILE_COUNT; ++i) {
        char *pos = data_buf + data_len * i;

        sprintf(pos, "%d\n\n%s\n\n%s\n", i + 1000, lorem_ipsum, lorem_ipsum);
        sprintf(file_name, "./out/file-%d.txt", i + 1000);

        int fd = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);

        if (fd == -1) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }

        aio_list[i] = (struct aiocb) {0};
        aio_list[i] = (struct aiocb) {
            .aio_fildes = fd,
            .aio_offset = 0,
            .aio_buf = pos,
            .aio_nbytes = data_len - 1, // Exclude null terminator
            .aio_reqprio = 0,
            .aio_sigevent.sigev_notify = SIGEV_NONE,
            .aio_lio_opcode = LIO_WRITE,
        };
    }

    struct CGNThreadHandle_int *handles =
        (struct CGNThreadHandle_int *)malloc(FILE_COUNT * sizeof(struct CGNThreadHandle_int));

    printf("Initializing SeaGreen runtime...\n");
    seagreen_init_rt();

    printf("Writing files...\n");
    for (int i = 0; i < FILE_COUNT; ++i) {
        handles[i] = async_run(write_file(&aio_list[i]));
    }

    for (int i = 0; i < FILE_COUNT; ++i) {
        int res = await(handles[i]);

        if (res == -1) {
            perror("Error writing file");
            exit(EXIT_FAILURE);
        }
    }
    printf("Done.\n");

    seagreen_free_rt();

    return 0;
}
