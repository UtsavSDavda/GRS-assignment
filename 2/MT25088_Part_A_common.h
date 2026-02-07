// MT25XXX - Replace XXX with your roll number
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <stdatomic.h>

#define PORT 8080
#define NUM_FIELDS 8
#define MIN_MSG_SIZE 1024
#define MAX_MSG_SIZE (10 * 1024 * 1024)  // 10MB

// The structure with 8 dynamically allocated string fields
typedef struct {
    char *fields[NUM_FIELDS];
    size_t field_sizes[NUM_FIELDS];
} MessageStruct;

// Helper to fill the struct with random data
void allocate_message(MessageStruct *msg, size_t total_size) {
    if (total_size < MIN_MSG_SIZE || total_size > MAX_MSG_SIZE) {
        fprintf(stderr, "Invalid message size: %zu (must be between %d and %d)\n", 
                total_size, MIN_MSG_SIZE, MAX_MSG_SIZE);
        exit(1);
    }
    
    size_t chunk_size = total_size / NUM_FIELDS;
    for (int i = 0; i < NUM_FIELDS; i++) {
        msg->field_sizes[i] = chunk_size;
        msg->fields[i] = (char *)malloc(chunk_size);
        if (!msg->fields[i]) {
            perror("Malloc failed");
            exit(1);
        }
        memset(msg->fields[i], 'A' + i, chunk_size); // Fill with data
    }
}

// Helper to free the struct
void free_message(MessageStruct *msg) {
    for (int i = 0; i < NUM_FIELDS; i++) {
        if (msg->fields[i]) {
            free(msg->fields[i]);
            msg->fields[i] = NULL;
        }
    }
}

// Helper to set TCP socket options for better performance
void set_socket_options(int sock) {
    int flag = 1;
    // Disable Nagle's algorithm for lower latency
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0) {
        perror("Warning: TCP_NODELAY failed");
    }
    
    // Increase send buffer
    int sndbuf = 1024 * 1024;  // 1MB
    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)) < 0) {
        perror("Warning: SO_SNDBUF failed");
    }
    
    // Increase receive buffer
    int rcvbuf = 1024 * 1024;  // 1MB
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf)) < 0) {
        perror("Warning: SO_RCVBUF failed");
    }
}

#endif
