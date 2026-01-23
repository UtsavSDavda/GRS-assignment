#!/bin/bash

# CONFIG
DISK_DEVICE="sdd"  
CPU_CORE=0         

# Initialize CSV
echo "Program,Function,Count,CPU%,Mem%,IO(MB/s),Time(s)" > results.csv

run_benchmark() {
    local prog=$1
    local worker=$2
    local count=$3
    
    echo "Running $prog $worker with count $count..."
    
    # Start Program pinned to core 0
    (time taskset -c $CPU_CORE ./$prog $worker $count) > /dev/null 2> temp_time.log &
    PID=$!
    
    # Wait a moment for processes to spawn
    sleep 0.5

    # Reset logs
    > temp_io.log
    > temp_top.log
    
    # Monitoring Loop
    while kill -0 $PID 2> /dev/null; do
        # 1. CPU/MEM (TOP)
        # Grep by Name (-w matches whole word) to catch Parent AND Children
        top -b -n 1 | grep -w "$prog" | awk '{c+=$9; m+=$10} END {print c, m}' >> temp_top.log
        
        # 2. IO (IOSTAT - System Wide)
        # This captures ALL writes to 'sdd', ensuring we see Child processes too.
        # We take 2 samples, keep the last one (tail -1) to avoid the "boot average" error.
        iostat -d -k 1 2 | grep "$DISK_DEVICE" | tail -n 1 | awk '{print $4/1024}' >> temp_io.log
        
        sleep 1
    done

    # Calculate Averages
    avg_cpu=$(awk '{sum+=$1; n++} END {if (n>0) print sum/n; else print 0}' temp_top.log)
    avg_mem=$(awk '{sum+=$2; n++} END {if (n>0) print sum/n; else print 0}' temp_top.log)
    avg_io=$(awk '{sum+=$1; n++} END {if (n>0) print sum/n; else print 0}' temp_io.log)
    
    # Extract Time
    real_time_raw=$(grep "real" temp_time.log | awk '{print $2}')
    min=$(echo $real_time_raw | cut -d'm' -f1)
    sec=$(echo $real_time_raw | cut -d'm' -f2 | cut -d's' -f1)
    total_sec=$(echo "$min * 60 + $sec" | bc)

    # Save to CSV
    echo "$prog,$worker,$count,$avg_cpu,$avg_mem,$avg_io,$total_sec" >> results.csv
}

# --- EXECUTION LOOPS ---

WORKERS=("cpu" "mem" "io")

# 1. Program A (Processes)
for worker in "${WORKERS[@]}"; do
    for count in 2 3 4 5; do
        run_benchmark "progA" "$worker" "$count"
        sleep 1
    done
done

# 2. Program B (Threads)
for worker in "${WORKERS[@]}"; do
    for count in 2 3 4 5 6 7 8; do
        run_benchmark "progB" "$worker" "$count"
        sleep 1
    done
done

echo "Benchmark Complete. Generating Plots..."