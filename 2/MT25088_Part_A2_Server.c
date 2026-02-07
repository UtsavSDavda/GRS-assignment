// MT25XXX_PartA2_Server.c - Replace XXX with your roll number
#include "common.h"
#include <sys/uio.h> // Required for struct iovec

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    // 1. Receive message size from client
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

    // 2. Setup Data (Same as A1)
    MessageStruct msg;
    allocate_message(&msg, total_payload_size);

    // 3. Prepare IO Vector (Scatter-Gather)
    // We do NOT malloc a large send_buffer here - this eliminates COPY 1
    struct msghdr msg_header = {0};
    struct iovec iov[NUM_FIELDS];

    // Point the IO vectors to our scattered data
    for (int i = 0; i < NUM_FIELDS; i++) {
        iov[i].iov_base = msg.fields[i];       // Pointer to the data
        iov[i].iov_len = msg.field_sizes[i];   // Length of the data
    }

    msg_header.msg_iov = iov;
    msg_header.msg_iovlen = NUM_FIELDS;

    // Set socket options
    set_socket_options(client_fd);

    // Keep sending
    while (1) {
        // --- COPY 1 ELIMINATED: No memcpy here ---
        // The kernel reads directly from the 8 scattered locations
        
        // --- COPY 2: User -> Kernel Copy ---
        // The kernel reads the 8 locations in 'iov' and copies them 
        // directly into the kernel socket buffer (one copy total)
        ssize_t sent = sendmsg(client_fd, &msg_header, 0);
        
        if (sent <= 0) {
            break;
        }
    }

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

    printf("Server (A2 One-Copy) listening on port %d...\n", PORT);

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
