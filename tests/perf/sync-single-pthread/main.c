#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define FILE_COUNT 4096

#define LOREM_IPSUM_COUNT 10000

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
    size_t extra_len = 2;  // 2 linebreaks per lorem ipsum
    size_t data_len = lorem_ipsum_len + extra_len;

    size_t data_buf_len = data_len * LOREM_IPSUM_COUNT + 1; // add 1 for null terminator
    char *data_buf = (char *)malloc(data_buf_len);

    for (int i = 0; i < LOREM_IPSUM_COUNT; ++i) {
        sprintf(data_buf + data_len * i, "%s\n\n", lorem_ipsum);
    }

    FILE **file_list =(FILE **)malloc(FILE_COUNT * sizeof(FILE *));

    for (int i = 0; i < FILE_COUNT; ++i) {
        sprintf(file_name, "./out/file-%d.txt", i + 1000);

        FILE *f = fopen(file_name, "w");

        if (!f) {
            printf("Error opening file %d\n", i);
            exit(EXIT_FAILURE);
        }

        file_list[i] = f;
    }

    printf("Writing files...\n");
    clock_t start = clock();
    for (int i = 0; i < FILE_COUNT; ++i) {
        unsigned long res = fwrite(data_buf, data_buf_len, 1, file_list[i]);
        if (res != 1) {
            printf("Error writing file %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    clock_t diff = clock() - start;
    printf("Done.\n");

    int msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("Write took %d.%d seconds\n", msec / 1000, msec % 1000);

    return 0;
}
