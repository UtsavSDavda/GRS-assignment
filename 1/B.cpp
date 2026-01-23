#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

// --- WORKER FUNCTIONS ---
void cpu_worker() {
    volatile double result = 0.0;
    for (int k = 0; k < 8*300; k++) { 
        for (long i = 0; i < 100000; i++) result += (i * 0.001) + (k * 0.1);
    }
}

void mem_worker() {
    long size = 100 * 1024 * 1024;
    volatile char *arr = (char*)malloc(size);
    if (!arr) return;
    for (int k = 0; k < 8*400; k++) {
        for (long i = 0; i < size; i += 4096) arr[i] = (char)(k % 128);
    }
    free((void*)arr);
}

void io_worker() {
    // Unique filename based on thread ID to avoid collision
    char filename[32];
    sprintf(filename, "io_th_%lu.dat", pthread_self());
    FILE *fp = fopen(filename, "w");
    if (!fp) return;

    char buffer[1024];
    memset(buffer, 'B', 1024);

    for (int i = 0; i < 80000; i++) {
        fwrite(buffer, 1, 1024, fp);
        if (i % 1000 == 0) {
            fflush(fp);
            fsync(fileno(fp));
        }
    }
    fsync(fileno(fp));
    fclose(fp);
    remove(filename);
}

void (*global_worker)() = NULL;

void* thread_wrapper(void* arg) {
    global_worker();
    return NULL;
}

// --- MAIN ---
int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s [cpu|mem|io] [num_threads]\n", argv[0]);
        return 1;
    }

    int num_threads = atoi(argv[2]);
    if (num_threads < 1) num_threads = 1;

    if (strcmp(argv[1], "cpu") == 0) global_worker = cpu_worker;
    else if (strcmp(argv[1], "mem") == 0) global_worker = mem_worker;
    else if (strcmp(argv[1], "io") == 0) global_worker = io_worker;
    else return 1;

    pthread_t *threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads);

    // Create N threads
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, thread_wrapper, NULL);
    }

    // Join N threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    return 0;
}