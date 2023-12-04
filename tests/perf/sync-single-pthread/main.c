#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define FILE_COUNT 4096

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

    FILE **file_list =(FILE **)malloc(FILE_COUNT * sizeof(FILE *));

    for (int i = 0; i < FILE_COUNT; ++i) {
        char *pos = data_buf + data_len * i;

        sprintf(pos, "%d\n\n%s\n\n%s\n", i + 1000, lorem_ipsum, lorem_ipsum);
        sprintf(file_name, "./out/file-%d.txt", i + 1000);

        FILE *f = fopen(file_name, "w");

        if (!f) {
            printf("Error opening file %d\n", i);
            exit(EXIT_FAILURE);
        }

        file_list[i] = f;
    }

    printf("Writing files...\n");
    for (int i = 0; i < FILE_COUNT; ++i) {
        unsigned long res = fwrite(data_buf + data_len * i, data_len, 1, file_list[i]);
        if (res != 1) {
            printf("Error writing file %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    printf("Done.\n");

    return 0;
}
