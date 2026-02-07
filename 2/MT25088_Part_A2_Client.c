// MT25XXX_PartB_Client.c - Replace XXX with your roll number
#include "common.h"
#include <sys/time.h>

// Arguments structure
typedef struct {
    char *server_ip;
    int message_size;
    int duration;
    long long bytes_received;
    long long messages_received;
} thread_args_t;

atomic_int keep_running = 1;

void *client_thread(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;
    int sock = 0;
    struct sockaddr_in serv_addr;
    char *buffer = malloc(args->message_size);
    
    if (!buffer) {
        perror("Buffer malloc failed");
        return NULL;
    }
    
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        free(buffer);
        return NULL;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, args->server_ip, &serv_addr.sin_addr) <= 0) {
        close(sock);
        free(buffer);
        return NULL;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        free(buffer);
        return NULL;
    }

    // Set socket options
    set_socket_options(sock);

    // Send message size to server
    size_t msg_size = args->message_size;
    if (send(sock, &msg_size, sizeof(msg_size), 0) != sizeof(msg_size)) {
        perror("Failed to send message size");
        close(sock);
        free(buffer);
        return NULL;
    }

    // Warmup period - let TCP connection stabilize
    usleep(100000); // 100ms warmup

    // Reset counters after warmup
    args->bytes_received = 0;
    args->messages_received = 0;

    // Receive Loop
    while (atomic_load(&keep_running)) {
        int bytes_in_msg = 0;
        
        // Ensure we receive the FULL message size to count it as 1 message
        while (bytes_in_msg < args->message_size && atomic_load(&keep_running)) {
            ssize_t valread = recv(sock, buffer + bytes_in_msg, 
                                   args->message_size - bytes_in_msg, 0);
            if (valread <= 0) break;
            bytes_in_msg += valread;
        }

        if (bytes_in_msg == args->message_size) {
            args->bytes_received += bytes_in_msg;
            args->messages_received++;
        } else {
            break;
        }
    }

    close(sock);
    free(buffer);
    return NULL;
}

int main(int argc, char const *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <Server IP> <Threads> <Msg Size> <Duration (s)>\n", argv[0]);
        return -1;
    }

    char *server_ip = (char *)argv[1];
    int thread_count = atoi(argv[2]);
    int message_size = atoi(argv[3]);
    int duration = atoi(argv[4]);

    // Validate inputs
    if (thread_count <= 0 || thread_count > 100) {
        fprintf(stderr, "Invalid thread count: %d\n", thread_count);
        return -1;
    }

    if (message_size < MIN_MSG_SIZE || message_size > MAX_MSG_SIZE) {
        fprintf(stderr, "Invalid message size: %d (must be between %d and %d)\n", 
                message_size, MIN_MSG_SIZE, MAX_MSG_SIZE);
        return -1;
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * thread_count);
    thread_args_t *t_args = malloc(sizeof(thread_args_t) * thread_count);

    if (!threads || !t_args) {
        perror("Memory allocation failed");
        return -1;
    }

    // Start all threads
    for (int i = 0; i < thread_count; i++) {
        t_args[i].server_ip = server_ip;
        t_args[i].message_size = message_size;
        t_args[i].duration = duration;
        t_args[i].bytes_received = 0;
        t_args[i].messages_received = 0;
        
        if (pthread_create(&threads[i], NULL, client_thread, &t_args[i]) != 0) {
            perror("Thread creation failed");
            atomic_store(&keep_running, 0);
            // Wait for already created threads
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            free(threads);
            free(t_args);
            return -1;
        }
    }

    // Run for specified duration
    sleep(duration);
    atomic_store(&keep_running, 0);

    // Join threads and collect statistics
    long long total_bytes = 0;
    long long total_messages = 0;
    
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
        total_bytes += t_args[i].bytes_received;
        total_messages += t_args[i].messages_received;
    }

    // Metrics Calculation
    double throughput_mbps = (total_bytes * 8.0) / (duration * 1000000.0);
    
    // Average Latency approximation
    // This is the average time between messages: (Total Time * Threads) / Total Messages
    // Note: This is a throughput-based approximation, not true per-message latency
    double latency_us = 0.0;
    if (total_messages > 0) {
        latency_us = ((double)duration * 1000000.0 * thread_count) / total_messages;
    }

    // PRINT OUTPUT IN CSV FORMAT for the benchmark script to parse
    // Format: DATA,BYTES,MESSAGES,THROUGHPUT_MBPS,LATENCY_US
    printf("DATA,%lld,%lld,%.2f,%.2f\n", total_bytes, total_messages, throughput_mbps, latency_us);

    free(threads);
    free(t_args);
    return 0;
}
