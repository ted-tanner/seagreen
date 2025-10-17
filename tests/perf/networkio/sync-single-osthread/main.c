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

#define REQUEST_COUNT 250

int make_http_request(void) {
    struct addrinfo hints, *result;
    int sockfd;
    char request[] = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: close\r\n\r\n";
    char response[4096];
    ssize_t bytes_received;
    
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
    
    if (connect(sockfd, result->ai_addr, result->ai_addrlen) == -1) {
        perror("connect");
        close(sockfd);
        freeaddrinfo(result);
        return -1;
    }
    
    freeaddrinfo(result);
    
    if (send(sockfd, request, strlen(request), 0) == -1) {
        perror("send");
        close(sockfd);
        return -1;
    }
    
    bytes_received = recv(sockfd, response, sizeof(response) - 1, 0);
    if (bytes_received == -1) {
        perror("recv");
        close(sockfd);
        return -1;
    }
    
    response[bytes_received] = '\0';
    close(sockfd);
    
    return 0;
}

int main(void) {
    printf("Making %d sequential HTTP requests to https://www.example.com...\n", REQUEST_COUNT);

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    for (int i = 0; i < REQUEST_COUNT; ++i) {
        printf("Request %d/%d\n", i + 1, REQUEST_COUNT);
        
        if (make_http_request() != 0) {
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

    return 0;
}
