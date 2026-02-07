// MT25XXX_PartA3_Server.c - Replace XXX with your roll number
#include "common.h"
#include <sys/uio.h>
#include <linux/errqueue.h>
#include <fcntl.h>

#ifndef MSG_ZEROCOPY
#define MSG_ZEROCOPY 0x4000000
#endif

#ifndef SO_ZEROCOPY
#define SO_ZEROCOPY 60
#endif

// Helper to drain zerocopy notifications (non-blocking)
int drain_zerocopy_notifications(int fd, int max_drain) {
    struct msghdr msg = {0};
    char control[100];
    int drained = 0;
    
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);

    // Non-blocking drain
    while (drained < max_drain) {
        int ret = recvmsg(fd, &msg, MSG_ERRQUEUE | MSG_DONTWAIT);
        if (ret == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No more notifications available
                break;
            }
            // Other error - stop draining
            break;
        }
        drained++;
    }
    
    return drained;
}

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    // 1. Enable Zero-Copy on the socket
    int opt = 1;
    if (setsockopt(client_fd, SOL_SOCKET, SO_ZEROCOPY, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_ZEROCOPY failed (kernel might not support it)");
        // Fallback: close and exit
        close(client_fd);
        return NULL;
    }

    // 2. Receive message size from client
    size_t total_payload_size;
    ssize_t received = recv(client_fd, &total_payload_size, sizeof(total_payload_size), 0);
    if (received != sizeof(total_payload_size)) {
        perror("Failed to receive message size");
        close(client_fd);
        return NULL;
    }

    // Validate message size
    if (total_payload_size < MIN_MSG_SIZE || total_payload_size > MAX_MSG_SIZE) {
        fprintf(stderr, "Invalid message size received: %zu\n", total_payload_size);
        close(client_fd);
        return NULL;
    }

    // 3. Setup Data
    // For zero-copy to be most effective, we use ONE large contiguous buffer
    // instead of 8 scattered buffers. This reduces page-pinning overhead.
    // However, to match the assignment's struct requirement, we keep the struct
    // but acknowledge this tradeoff in the report.
    
    MessageStruct msg;
    allocate_message(&msg, total_payload_size);

    struct msghdr msg_header = {0};
    struct iovec iov[NUM_FIELDS];

    for (int i = 0; i < NUM_FIELDS; i++) {
        iov[i].iov_base = msg.fields[i];
        iov[i].iov_len = msg.field_sizes[i];
    }
    msg_header.msg_iov = iov;
    msg_header.msg_iovlen = NUM_FIELDS;

    // Set socket options
    set_socket_options(client_fd);

    // Track pending notifications
    int pending_notifications = 0;
    const int MAX_PENDING = 16;  // Don't let too many pile up

    while (1) {
        // --- ZERO COPY SEND ---
        // The kernel pins the pages and transmits directly from user memory
        // No copy to kernel socket buffer
        ssize_t sent = sendmsg(client_fd, &msg_header, MSG_ZEROCOPY);
        
        if (sent <= 0) {
            if (errno == ENOBUFS) { 
                // Socket buffer full, drain some notifications and retry
                pending_notifications -= drain_zerocopy_notifications(client_fd, MAX_PENDING);
                if (pending_notifications < 0) pending_notifications = 0;
                usleep(100); // Brief backoff
                continue; 
            }
            break; // Connection closed or error
        }

        pending_notifications++;

        // Periodically drain notifications to avoid memory buildup
        // But don't block on every send - that defeats the purpose!
        if (pending_notifications >= MAX_PENDING) {
            int drained = drain_zerocopy_notifications(client_fd, MAX_PENDING / 2);
            pending_notifications -= drained;
            if (pending_notifications < 0) pending_notifications = 0;
        }
    }

    // Final drain before cleanup
    drain_zerocopy_notifications(client_fd, 1000);

    free_message(&msg);
    close(client_fd);
    return NULL;
}

int main() {
    int server_fd, *new_sock;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server (A3 Zero-Copy) listening on port %d...\n", PORT);

    while (1) {
        new_sock = malloc(sizeof(int));
        if (!new_sock) {
            perror("Malloc failed");
            continue;
        }
        
        *new_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (*new_sock < 0) {
            perror("Accept failed");
            free(new_sock);
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)new_sock) < 0) {
            perror("Thread creation failed");
            close(*new_sock);
            free(new_sock);
            continue;
        }
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}
