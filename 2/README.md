# Roll number: MT25088
# PA02: Analysis of Network I/O Primitives using "perf"

## 1. Project Overview

This project experimentally studies the cost of data movement in network I/O by implementing and comparing three different socket communication strategies. The goal is to analyze micro-architectural effects (CPU cycles, cache behavior) and network metrics (throughput, latency) across:

1. **Two-Copy (Baseline):** Standard `send()`/`recv()` primitives.
2. **One-Copy:** Optimized `sendmsg()` using `struct iovec` buffers to eliminate user-space assembly.
3. **Zero-Copy:** Advanced `sendmsg()` using the `MSG_ZEROCOPY` flag to eliminate kernel-user copying.

---

## 2. Prerequisites & Requirements

To run this project, the following environment is required:

* **Operating System:** Linux (Kernel 4.14+ required for `MSG_ZEROCOPY` support).
* **Compiler:** GCC with `pthread` support.
* **Tools:** `make`, `perf` (linux-tools), `bash`.
* **Python:** Python 3 with `matplotlib` for generating plots.

---

## 3. Project Structure

The project adheres to the strict file naming convention `MT25xxx_Part_<Section>_<File>`:

### Source Code

* **Baseline:** `MT25xxx_Part_A1_Server.c`, `MT25xxx_Part_A1_Client.c`
* **One-Copy:** `MT25xxx_Part_A2_Server.c`, `MT25xxx_Part_A2_Client.c`
* **Zero-Copy:** `MT25xxx_Part_A3_Server.c`, `MT25xxx_Part_A3_Client.c`

### Automation & Analysis

* **Runner Script:** `MT25xxx_Part_C_Runner.sh` - Bash script to compile, run experiments, and gather `perf` data.
* **Plotting:** `MT25xxx_Part_D_Plots.py` - Python script using `matplotlib` with hardcoded data values.
* **Data:** `MT25xxx_Part_B_Results.csv` - Raw profiling data collected from experiments.

---

## 4. Compilation

A `Makefile` is provided to compile all implementations.

**Clean previous builds:**
```bash
make clean
```

**Compile all executables:**
```bash
make all
```

This will generate the following binaries:

* `server_two_copy`, `client_two_copy`
* `server_one_copy`, `client_one_copy`
* `server_zero_copy`, `client_zero_copy`

---

## 5. Usage & Execution

### A. Automated Experiments (Recommended)

To run the full suite of experiments (Message Sizes x Thread Counts) and collect profiling data automatically:

```bash
sudo ./MT25xxx_Part_C_Runner.sh
```

**Note:** `sudo` is required for `perf stat` to access hardware counters like cache misses and cycles.

### B. Manual Execution

You can run individual server/client pairs for testing or debugging.

**1. Start the Server:**
```bash
./server_two_copy <port>
# Example: ./server_two_copy 8080
```

**2. Start the Client:**
```bash
./client_two_copy -s <msg_size> -t <threads> -i <server_ip> -p <server_port>
# Example: ./client_two_copy -s 4096 -t 4 -i 127.0.0.1 -p 8080
```

**Parameters:**
* `-s`: Message size in bytes (e.g., 4096).
* `-t`: Number of threads (e.g., 4).
* `-i`: Server IP address (e.g., 127.0.0.1).
* `-p`: Server Port (e.g., 8080).

---

## 6. Implementation Details

### Part A1: Two-Copy (Standard)

* **Mechanism:** Uses standard `send()` and `recv()` system calls.
* **Data Flow:** Data is copied from User Buffer → Kernel Buffer → NIC.
* **Structure:** The server spawns one thread per client. Messages are constructed dynamically using `malloc` for 8 fields.

### Part A2: One-Copy (Scatter-Gather)

* **Mechanism:** Uses `sendmsg()` with `struct iovec`.
* **Optimization:** Reduces copying by "gathering" data from multiple non-contiguous user buffers directly during the system call, avoiding an intermediate user-space buffer assembly.

### Part A3: Zero-Copy (MSG_ZEROCOPY)

* **Mechanism:** Uses `sendmsg()` with the `MSG_ZEROCOPY` flag.
* **Optimization:** The kernel "pins" user pages in memory and maps them for DMA. Data is not copied to the kernel socket buffer.
* **Constraint:** The application must poll the socket's error queue (`MSG_ERRQUEUE`) to receive a completion notification before it can safely reuse or free the buffer.

---

## 7. Generating Plots

As per assignment constraints, plots are generated using `matplotlib` with values hardcoded in the script.

To generate the graphs (Throughput, Latency, Cache Misses, CPU Cycles):

```bash
python3 MT25xxx_Part_D_Plots.py
```

**Note:** Do not modify the CSV reading logic; the data is embedded in the script as arrays.

---

## 8. Author Information

* **Name:** [Your Name]
* **Roll Number:** MT25xxx
* **Course:** Graduate Systems (CSE638)
