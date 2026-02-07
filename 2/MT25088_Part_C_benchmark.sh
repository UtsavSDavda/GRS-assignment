#!/bin/bash
# MT25088 PartC Benchmark (CLIENT-SIDE PERF)

# --- CONFIGURATION ---
declare -A SERVERS
SERVERS=( ["Two-Copy"]="./server_a1"
          ["One-Copy"]="./server_a2"
          ["Zero-Copy"]="./server_a3" )

CLIENT="./client_b"
SERVER_IP="127.0.0.1"
DURATION=5

SIZES=(4096 16384 65536 524288)
THREADS=(1 2 4 8)

OUT_DIR="experiment_data_v4"
CSV_FILE="final_results_v4.csv"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

info() { echo -e "${GREEN}[INFO]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
err()  { echo -e "${RED}[ERR ]${NC} $1"; }

command -v perf >/dev/null || { err "perf not installed"; exit 1; }

mkdir -p "$OUT_DIR"
rm -f "$CSV_FILE"

echo "Implementation,Threads,MsgSize,Throughput_Gbps,Latency_us,Cycles,Instructions,L1_Misses,Cache_Misses,Context_Switches" \
    > "$CSV_FILE"

wait_for_server() {
    for _ in {1..20}; do
        ss -tuln | grep -q ":8080 " && return 0
        sleep 0.25
    done
    return 1
}

cleanup_server() {
    local pid=$1
    kill -TERM "$pid" 2>/dev/null
    wait "$pid" 2>/dev/null
}

parse_perf() {
    grep "$1" "$2" | awk '{print $1}' | tr -d ',' | grep -E '^[0-9]+$'
}

total=$(( ${#SERVERS[@]} * ${#THREADS[@]} * ${#SIZES[@]} ))
count=0

for IMPL in "${!SERVERS[@]}"; do
    SERVER_BIN=${SERVERS[$IMPL]}

    for T in "${THREADS[@]}"; do
        for S in "${SIZES[@]}"; do
            count=$((count + 1))
            info "[$count/$total] $IMPL | Threads=$T | MsgSize=$S"

            sudo fuser -k 8080/tcp >/dev/null 2>&1
            sleep 0.3

            PERF_FILE="$OUT_DIR/perf_client_${IMPL}_t${T}_s${S}.txt"
            CLIENT_FILE="$OUT_DIR/client_${IMPL}_t${T}_s${S}.txt"

            # Start server (NO perf here)
            "$SERVER_BIN" &
            SERVER_PID=$!

            wait_for_server || {
                warn "Server failed to start"
                cleanup_server "$SERVER_PID"
                echo "$IMPL,$T,$S,0,0,,,,," >> "$CSV_FILE"
                continue
            }

            sleep 0.2

            # Run CLIENT under perf
            sudo perf stat \
                -e cycles,instructions,L1-dcache-load-misses,cache-misses,context-switches \
                -o "$PERF_FILE" \
                "$CLIENT" "$SERVER_IP" "$T" "$S" "$DURATION" \
                > "$CLIENT_FILE" 2>&1

            cleanup_server "$SERVER_PID"

            CLIENT_DATA=$(grep "^DATA," "$CLIENT_FILE")
            if [ -z "$CLIENT_DATA" ]; then
                warn "No client output"
                echo "$IMPL,$T,$S,0,0,,,,," >> "$CSV_FILE"
                continue
            fi

            MBPS=$(echo "$CLIENT_DATA" | cut -d',' -f4)
            Gbps=$(awk "BEGIN {printf \"%.2f\", $MBPS/1000}")
            LAT=$(echo "$CLIENT_DATA" | cut -d',' -f5)

            CYCLES=$(parse_perf cycles "$PERF_FILE")
            INSTR=$(parse_perf instructions "$PERF_FILE")
            L1MISS=$(parse_perf L1-dcache-load-misses "$PERF_FILE")
            CMISS=$(parse_perf cache-misses "$PERF_FILE")
            CSW=$(parse_perf context-switches "$PERF_FILE")

            echo "$IMPL,$T,$S,$Gbps,$LAT,${CYCLES},${INSTR},${L1MISS},${CMISS},${CSW}" \
                >> "$CSV_FILE"

            info "  → $Gbps Gbps | $LAT µs"
        done
    done
done

sudo fuser -k 8080/tcp >/dev/null 2>&1
info "All experiments complete"
info "CSV rows: $(($(wc -l < "$CSV_FILE") - 1))"
