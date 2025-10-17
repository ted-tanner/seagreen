#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/select.h>

#include "seagreen.h"

#define REQUEST_COUNT 250

typedef struct { int request_id; } http_request_args;

async uint64_t make_http_request_async(void *p) {
    http_request_args *args = (http_request_args *)p;
    int request_id = args->request_id;
    
    struct addrinfo hints, *result;
    int sockfd;
    char request[] = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: close\r\n\r\n";
    char response[4096];
    ssize_t bytes_sent, bytes_received;
    size_t request_len = strlen(request);
    size_t total_sent = 0;
    size_t total_received = 0;
    
    printf("Starting request %d\n", request_id);
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    int status = getaddrinfo("www.example.com", "80", &hints, &result);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }
    
    sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        freeaddrinfo(result);
        return -1;
    }
    
    // Make socket non-blocking
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1 || fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        close(sockfd);
        freeaddrinfo(result);
        return -1;
    }
    
    // Connect asynchronously
    int connect_result = connect(sockfd, result->ai_addr, result->ai_addrlen);
    if (connect_result == -1 && errno != EINPROGRESS) {
        perror("connect");
        close(sockfd);
        freeaddrinfo(result);
        return -1;
    }
    
    freeaddrinfo(result);
    
    // Wait for connection to complete
    if (connect_result == -1) {
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(sockfd, &write_fds);
        
        while (1) {
            int select_result = select(sockfd + 1, NULL, &write_fds, NULL, NULL);
            if (select_result > 0) {
                int error = 0;
                socklen_t error_len = sizeof(error);
                if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &error_len) == 0 && error == 0) {
                    break;
                } else {
                    perror("connection failed");
                    close(sockfd);
                    return -1;
                }
            } else if (select_result == -1) {
                perror("select");
                close(sockfd);
                return -1;
            }
            async_yield();
        }
    }
    
    // Send request asynchronously
    while (total_sent < request_len) {
        bytes_sent = send(sockfd, request + total_sent, request_len - total_sent, 0);
        if (bytes_sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                async_yield();
                continue;
            } else {
                perror("send");
                close(sockfd);
                return -1;
            }
        }
        total_sent += bytes_sent;
    }
    
    // Receive response asynchronously
    while (total_received < sizeof(response) - 1) {
        bytes_received = recv(sockfd, response + total_received, sizeof(response) - 1 - total_received, 0);
        if (bytes_received == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                async_yield();
                continue;
            } else {
                perror("recv");
                close(sockfd);
                return -1;
            }
        } else if (bytes_received == 0) {
            // Connection closed
            break;
        }
        total_received += bytes_received;
    }
    
    response[total_received] = '\0';
    close(sockfd);
    
    printf("Completed request %d\n", request_id);
    return 0;
}

int main(void) {
    printf("Initializing SeaGreen runtime...\n");
    seagreen_init_rt();

    printf("Making %d concurrent HTTP requests to https://www.example.com...\n", REQUEST_COUNT);

    CGNThreadHandle *handles =
        (CGNThreadHandle *)malloc(REQUEST_COUNT * sizeof(CGNThreadHandle));
    http_request_args *args_array =
        (http_request_args *)malloc(REQUEST_COUNT * sizeof(http_request_args));

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    for (int i = 0; i < REQUEST_COUNT; ++i) {
        args_array[i] = (http_request_args){i + 1};
        handles[i] = async_run(make_http_request_async, &args_array[i]);
    }

    for (int i = 0; i < REQUEST_COUNT; ++i) {
        int res = await(handles[i]);

        if (res != 0) {
            printf("Error making request %d\n", i + 1);
            exit(EXIT_FAILURE);
        }
    }

    clock_gettime(CLOCK_REALTIME, &end);

    printf("Done.\n");

    signed long long nsec_diff = end.tv_nsec - start.tv_nsec;
    signed long long sec_diff = end.tv_sec - start.tv_sec;

    if (nsec_diff < 0) {
        nsec_diff += 1000000000;
        sec_diff -= 1;
    }

    printf("All requests took %lld.%lld seconds\n", sec_diff, nsec_diff / 1000000);

    free(args_array);
    free(handles);
    seagreen_free_rt();

    return 0;
}
