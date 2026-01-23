import pandas as pd
import matplotlib.pyplot as plt
import sys

# 1. Load Data
try:
    df = pd.read_csv("results.csv")
except FileNotFoundError:
    print("Error: 'results.csv' not found. Run bench_d.sh first.")
    sys.exit(1)

df['Count'] = pd.to_numeric(df['Count'])

# 2. General Plotting Function
def create_comparison_plot(metric_col, title, filename, y_label):
    plt.figure(figsize=(10, 6))
    
    styles = {
        'cpu': 'red',
        'mem': 'blue',
        'io':  'green'
    }
    
    for worker in ['cpu', 'mem', 'io']:
        subset = df[df['Function'] == worker]
        
        # Plot ProgA (Solid Line)
        progA = subset[subset['Program'] == 'progA']
        plt.plot(progA['Count'], progA[metric_col], 
                 marker='o', linestyle='-', color=styles[worker], 
                 label=f'{worker} (Proc A)')
        
        # Plot ProgB (Dashed Line)
        progB = subset[subset['Program'] == 'progB']
        plt.plot(progB['Count'], progB[metric_col], 
                 marker='x', linestyle='--', color=styles[worker], 
                 label=f'{worker} (Thread B)')

    plt.title(title)
    plt.xlabel('Concurrency Count (N)')
    plt.ylabel(y_label)
    plt.legend()
    plt.grid(True, which='both', linestyle='--', linewidth=0.5)
    
    # Save
    plt.tight_layout()
    plt.savefig(filename)
    print(f"Generated: {filename}")

# 3. Generate the 4 Graphs

# Graph 1: CPU Usage Comparison
create_comparison_plot('CPU%', 'CPU Usage: Processes (A) vs Threads (B)', 'graph_cpu.png', 'CPU Usage (%)')

# Graph 2: Memory Usage Comparison
create_comparison_plot('Mem%', 'Memory Usage: Processes (A) vs Threads (B)', 'graph_mem.png', 'Memory Usage (%)')

# Graph 3: IO Throughput (The "Interesting" One)
create_comparison_plot('IO(MB/s)', 'Disk Throughput: Processes (A) vs Threads (B)', 'graph_io.png', 'Disk Write Speed (MB/s)')

# Graph 4: Execution Time (Crucial for Performance Analysis)
create_comparison_plot('Time(s)', 'Execution Time: Scalability Analysis', 'graph_time.png', 'Time to Complete (seconds)')

print("All plots generated successfully.")