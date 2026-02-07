# MT25xxx_Part_D_Plotting.py
import matplotlib.pyplot as plt
import platform
import numpy as np

# --- CONFIGURATION ---
SYSTEM_INFO = f"System: {platform.node()} ({platform.system()} {platform.release()})"
DURATION_SECONDS = 5

# Set style (standard matplotlib only)
plt.rcParams.update({
    'font.size': 10, 
    'figure.autolayout': True,
    'axes.grid': True,
    'grid.alpha': 0.3
})

def save_plot(filename):
    """Save plot with high DPI"""
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    print(f"Saved {filename}")

# ==========================================
# HARDCODED DATA (Extracted from final_results_v3.csv)
# ==========================================
# Organized by Implementation for easier matplotlib plotting
# ==========================================
# HARDCODED DATA (New Data from User)
# ==========================================
data = {
    'Two-Copy': {
        'Threads': [1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 4, 8, 8, 8, 8],
        'MsgSize': [4096, 16384, 65536, 524288, 4096, 16384, 65536, 524288, 4096, 16384, 65536, 524288, 4096, 16384, 65536, 524288],
        'Throughput': [2.49, 8.37, 14.23, 9.31, 4.95, 21.96, 28.85, 18.07, 11.09, 30.51, 55.84, 32.33, 14.02, 44.39, 59.92, 44.5],
        'Latency': [13.14, 15.66, 36.85, 450.29, 13.23, 11.94, 36.35, 464.27, 11.82, 17.18, 37.55, 519.01, 18.7, 23.62, 70.0, 754.01],
        'CacheMisses': [43395331, 57589020, 85640182, 79124045, 76960100, 148986355, 209111175, 169957940, 191437155, 245088547, 469638856, 301699078, 182790949, 344722664, 533015455, 437873084],
        'Cycles': [6730878861, 6880595195, 5444368670, 8233223668, 13161788923, 17263535691, 11633033764, 16555769374, 33335559090, 28201742461, 25058427814, 33240203877, 38145699326, 39261310017, 30650371712, 47339698408]
    },
    'One-Copy': {
        'Threads': [1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 4, 8, 8, 8, 8],
        'MsgSize': [4096, 16384, 65536, 524288, 4096, 16384, 65536, 524288, 4096, 16384, 65536, 524288, 4096, 16384, 65536, 524288],
        'Throughput': [2.51, 8.82, 17.07, 12.49, 4.95, 16.68, 45.3, 31.59, 9.61, 31.47, 61.28, 50.39, 13.34, 45.71, 85.23, 87.39],
        'Latency': [13.04, 14.85, 30.72, 335.8, 13.23, 15.71, 23.15, 265.55, 13.64, 16.66, 34.22, 332.92, 19.65, 22.94, 49.21, 383.97],
        'CacheMisses': [44672593, 57034096, 90653046, 88154538, 81227539, 116155095, 243354757, 251431880, 159878759, 215623167, 431209382, 386789303, 171036023, 315008746, 584519798, 687999948],
        'Cycles': [6999570428, 7095145474, 6068909265, 9564894518, 13602304595, 13220086472, 15208863455, 20797467145, 29611997340, 28723530263, 25521970707, 36356900924, 39493190153, 40911132701, 37087420884, 65086750468]
    },
    'Zero-Copy': {
        'Threads': [1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 4, 8, 8, 8, 8],
        'MsgSize': [4096, 16384, 65536, 524288, 4096, 16384, 65536, 524288, 4096, 16384, 65536, 524288, 4096, 16384, 65536, 524288],
        'Throughput': [1.62, 5.5, 11.88, 12.89, 3.2, 10.05, 23.26, 25.96, 5.56, 21.58, 40.88, 44.83, 7.15, 25.4, 54.55, 64.41],
        'Latency': [20.25, 23.83, 44.13, 325.48, 20.5, 26.08, 45.09, 323.17, 23.57, 24.3, 51.3, 374.26, 36.65, 41.28, 76.9, 520.92],
        'CacheMisses': [34303646, 53062018, 86535286, 144704303, 69686165, 95658762, 197815944, 318842392, 144503310, 247510025, 403387485, 681108196, 207533509, 363746740, 687086544, 854314832],
        'Cycles': [5424441820, 5988326994, 4853279337, 5388762352, 10806771797, 10871351314, 9826800423, 11037000118, 23323180759, 26517904753, 20931937052, 26273679659, 36329884150, 36796977024, 33028512609, 43547038342]
    },
}

IMPLS = ['Two-Copy', 'One-Copy', 'Zero-Copy']
THREADS = [1, 2, 4, 8]
MSG_SIZES = [4096, 16384, 65536, 524288]
MSG_LABELS = ['4KB', '16KB', '64KB', '512KB']
COLORS = {'Two-Copy': 'blue', 'One-Copy': 'green', 'Zero-Copy': 'red'}
MARKERS = {'Two-Copy': 'o', 'One-Copy': '^', 'Zero-Copy': 's'}

# Helper to get specific data point
def get_data(impl, thread, metric_key):
    try:
        indices = [i for i, x in enumerate(data[impl]['Threads']) if x == thread]
        # Sort by message size to ensure line plots are correct
        pairs = sorted([(data[impl]['MsgSize'][i], data[impl][metric_key][i]) for i in indices])
        return [p[0] for p in pairs], [p[1] for p in pairs]
    except:
        return [], []

# Helper for Cycles per Byte calculation
def get_cycles_per_byte(impl, thread):
    sizes, cycles = get_data(impl, thread, 'Cycles')
    _, throughputs = get_data(impl, thread, 'Throughput')
    cpb = []
    for i in range(len(sizes)):
        total_bytes = throughputs[i] * 125_000_000 * DURATION_SECONDS
        if total_bytes > 0:
            cpb.append(cycles[i] / total_bytes)
        else:
            cpb.append(0)
    return sizes, cpb

# ==========================================
# PLOT 1: Throughput vs Message Size (Quadrant)
# ==========================================
print("Generating Plot 1: Throughput...")
fig, axs = plt.subplots(2, 2, figsize=(12, 10))
fig.suptitle(f'Throughput vs Message Size\n{SYSTEM_INFO}')

for i, t in enumerate(THREADS):
    row, col = divmod(i, 2)
    ax = axs[row, col]
    
    for impl in IMPLS:
        x, y = get_data(impl, t, 'Throughput')
        if x:
            ax.plot(x, y, label=impl, marker=MARKERS[impl], color=COLORS[impl])
            
    ax.set_title(f'Threads: {t}')
    ax.set_xscale('log')
    ax.set_xticks(MSG_SIZES)
    ax.set_xticklabels(MSG_LABELS)
    ax.set_ylabel('Throughput (Gbps)')
    ax.set_xlabel('Message Size')
    
    # Only legend on first plot
    if i == 0:
        ax.legend()

plt.tight_layout()
save_plot('quadrant_throughput.png')
plt.close()

# ==========================================
# PLOT 2: Latency vs Thread Count (Single Plot, 4KB)
# ==========================================
print("Generating Plot 2: Latency...")
plt.figure(figsize=(10, 6))

for impl in IMPLS:
    latencies = []
    for t in THREADS:
        sizes, lats = get_data(impl, t, 'Latency')
        # Find 4KB data (index 0 usually)
        try:
            idx = sizes.index(4096)
            latencies.append(lats[idx])
        except ValueError:
            latencies.append(0)
            
    plt.plot(THREADS, latencies, label=impl, marker=MARKERS[impl], color=COLORS[impl], linewidth=2)

plt.title(f'Latency vs Thread Count (4KB Message)\n{SYSTEM_INFO}')
plt.xlabel('Thread Count')
plt.ylabel('Latency (µs)')
plt.xticks(THREADS)
plt.legend()
plt.tight_layout()
save_plot('plot_latency_vs_threads.png')
plt.close()

# ==========================================
# PLOT 3: Cache Misses (Quadrant Bar Chart)
# ==========================================
print("Generating Plot 3: Cache Misses...")
fig, axs = plt.subplots(2, 2, figsize=(14, 10))
fig.suptitle(f'Cache Misses vs Message Size\n{SYSTEM_INFO}')

bar_width = 0.25
x_indices = np.arange(len(MSG_SIZES))

for i, t in enumerate(THREADS):
    row, col = divmod(i, 2)
    ax = axs[row, col]
    
    for j, impl in enumerate(IMPLS):
        sizes, misses = get_data(impl, t, 'CacheMisses')
        # Ensure data is aligned with MSG_SIZES
        aligned_misses = []
        for size in MSG_SIZES:
            if size in sizes:
                aligned_misses.append(misses[sizes.index(size)])
            else:
                aligned_misses.append(0)
                
        # Calculate offset
        offset = (j - 1) * bar_width
        ax.bar(x_indices + offset, aligned_misses, width=bar_width, label=impl, color=COLORS[impl], alpha=0.8)

    ax.set_title(f'Threads: {t}')
    ax.set_xticks(x_indices)
    ax.set_xticklabels(MSG_LABELS)
    ax.set_ylabel('Cache Misses')
    ax.ticklabel_format(style='sci', axis='y', scilimits=(0,0))
    
    if i == 0:
        ax.legend()

plt.tight_layout()
save_plot('quadrant_cache_misses.png')
plt.close()

# ==========================================
# PLOT 4: CPU Efficiency (Quadrant Bar Chart)
# ==========================================
print("Generating Plot 4: CPU Efficiency...")
fig, axs = plt.subplots(2, 2, figsize=(14, 10))
fig.suptitle(f'CPU Cycles Per Byte Transferred\n{SYSTEM_INFO}')

for i, t in enumerate(THREADS):
    row, col = divmod(i, 2)
    ax = axs[row, col]
    
    for j, impl in enumerate(IMPLS):
        sizes, cpb = get_cycles_per_byte(impl, t)
        
        aligned_cpb = []
        for size in MSG_SIZES:
            if size in sizes:
                aligned_cpb.append(cpb[sizes.index(size)])
            else:
                aligned_cpb.append(0)
                
        offset = (j - 1) * bar_width
        ax.bar(x_indices + offset, aligned_cpb, width=bar_width, label=impl, color=COLORS[impl], alpha=0.8)

    ax.set_title(f'Threads: {t}')
    ax.set_xticks(x_indices)
    ax.set_xticklabels(MSG_LABELS)
    ax.set_ylabel('Cycles / Byte')
    
    if i == 0:
        ax.legend()

plt.tight_layout()
save_plot('quadrant_cpu_efficiency.png')
plt.close()

print("\nAll plots generated successfully using matplotlib only.")