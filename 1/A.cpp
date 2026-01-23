#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

// --- WORKER FUNCTIONS ---
void cpu_worker() {
    volatile double result = 0.0;
    // Fixed loop count to ensure consistency
    for (int k = 0; k < 8*300; k++) { 
        for (long i = 0; i < 100000; i++) {
            result += (i * 0.001) + (k * 0.1);
        }
    }
}

void mem_worker() {
    long size = 100 * 1024 * 1024; // 100MB
    volatile char *arr = (char*)malloc(size);
    if (!arr) return;
    for (int k = 0; k < 8*400; k++) {
        for (long i = 0; i < size; i += 4096) arr[i] = (char)(k % 128);
    }
    free((void*)arr);
}

void io_worker() {
    char filename[32];
    sprintf(filename, "io_%d.dat", getpid()); // Unique file per process
    FILE *fp = fopen(filename, "w");
    if (!fp) return;

    char buffer[1024];
    memset(buffer, 'A', 1024);

    // Using your updated loop count logic
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

// --- MAIN ---
int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s [cpu|mem|io] [num_processes]\n", argv[0]);
        return 1;
    }

    int num_procs = atoi(argv[2]);
    if (num_procs < 1) num_procs = 1;

    void (*worker)() = NULL;
    if (strcmp(argv[1], "cpu") == 0) worker = cpu_worker;
    else if (strcmp(argv[1], "mem") == 0) worker = mem_worker;
    else if (strcmp(argv[1], "io") == 0) worker = io_worker;
    else return 1;

    // Create N-1 Children
    for(int i = 0; i < num_procs - 1; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            worker();
            exit(0);
        }
    }

    // Parent also works (Total = N processes)
    worker();

    // Wait for all children
    for(int i = 0; i < num_procs - 1; i++) {
        wait(NULL);
    }

    return 0;
}