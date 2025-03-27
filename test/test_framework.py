# Add these imports at the top
import os 
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import cm
import seaborn as sns
from matplotlib.colors import LinearSegmentedColormap

# Set visualization style
plt.style.use('ggplot')
sns.set(style="darkgrid")
custom_cmap = LinearSegmentedColormap.from_list("hermes_blue", 
                                               ["#08306b", "#4292c6", "#c6dbef"], N=256)

def generate_visualizations(results_dir="./results"):
    """Generate various visualizations for HERMES-CXL performance"""
    # Ensure results directory exists
    os.makedirs(results_dir, exist_ok=True)
    
    # Generate simulated performance data (replace with actual measurements in real system)
    block_sizes = [4, 8, 16, 32, 64, 128, 256, 512, 1024]  # KB
    
    # Bandwidth comparison data (simulated)
    cxl_read = [8.2, 14.6, 23.8, 29.4, 33.2, 35.8, 36.2, 36.5, 36.7]  # GB/s
    cxl_write = [7.4, 13.1, 21.5, 27.2, 31.1, 33.6, 34.1, 34.3, 34.5]  # GB/s
    std_read = [6.1, 11.2, 18.4, 23.1, 25.8, 27.4, 27.9, 28.1, 28.2]   # GB/s
    std_write = [5.6, 10.3, 17.1, 21.5, 24.2, 25.9, 26.3, 26.4, 26.5]  # GB/s
    
    # 1. Bandwidth comparison chart
    plt.figure(figsize=(12, 7))
    width = 0.2
    x = np.arange(len(block_sizes))
    
    plt.bar(x - width*1.5, cxl_read, width, label='CXL Read', color='#1f77b4')
    plt.bar(x - width/2, cxl_write, width, label='CXL Write', color='#2ca02c')
    plt.bar(x + width/2, std_read, width, label='Standard Read', color='#ff7f0e')
    plt.bar(x + width*1.5, std_write, width, label='Standard Write', color='#d62728')
    
    plt.xlabel('Block Size (KB)', fontsize=14)
    plt.ylabel('Bandwidth (GB/s)', fontsize=14)
    plt.title('HERMES-CXL vs Standard Memory Performance', fontsize=16)
    plt.xticks(x, block_sizes)
    plt.legend(fontsize=12)
    plt.grid(axis='y', alpha=0.3)
    plt.savefig(f"{results_dir}/bandwidth_comparison.png", dpi=300, bbox_inches='tight')
    
    # 2. Latency heatmap
    access_patterns = ['Random', 'Sequential', 'Strided', 'Gather', 'Scatter']
    devices = ['CPU→CXL', 'CXL→CPU', 'CPU→DRAM', 'DRAM→CPU']
    
    # Simulated latency data (nanoseconds)
    latency_data = np.array([
        [85, 62, 115, 145],  # Random
        [42, 38, 58, 65],    # Sequential
        [65, 55, 75, 85],    # Strided
        [95, 85, 120, 130],  # Gather
        [105, 95, 130, 140]  # Scatter
    ])
    
    plt.figure(figsize=(10, 7))
    sns.heatmap(latency_data, annot=True, fmt=".1f", 
                xticklabels=devices, yticklabels=access_patterns,
                cmap=custom_cmap, linewidths=0.5)
    plt.title('Memory Access Latency (ns) by Pattern and Device', fontsize=16)
    plt.tight_layout()
    plt.savefig(f"{results_dir}/latency_heatmap.png", dpi=300, bbox_inches='tight')
    
    # 3. Scaling performance with concurrent processes
    processes = np.arange(1, 9)
    cxl_scaling = [1.0, 1.92, 2.76, 3.45, 3.95, 4.32, 4.56, 4.72]  # Efficiency scaling
    std_scaling = [1.0, 1.85, 2.55, 3.10, 3.42, 3.65, 3.78, 3.85]  # Efficiency scaling
    
    plt.figure(figsize=(10, 6))
    plt.plot(processes, cxl_scaling, 'o-', linewidth=2, markersize=10, label='HERMES-CXL', color='#1f77b4')
    plt.plot(processes, std_scaling, 's-', linewidth=2, markersize=10, label='Standard Memory', color='#ff7f0e')
    plt.plot(processes, processes, '--', linewidth=1, label='Ideal Linear', color='#2ca02c', alpha=0.7)
    
    plt.xlabel('Number of Concurrent Processes', fontsize=14)
    plt.ylabel('Relative Performance', fontsize=14)
    plt.title('Scaling Efficiency with Concurrent Processes', fontsize=16)
    plt.grid(True, alpha=0.3)
    plt.legend(fontsize=12)
    plt.savefig(f"{results_dir}/scaling_performance.png", dpi=300, bbox_inches='tight')
    
    # 4. 3D visualization of memory throughput by operation type and data size
    fig = plt.figure(figsize=(12, 8))
    ax = fig.add_subplot(111, projection='3d')
    
    operations = ['Read', 'Write', 'Copy', 'Scale', 'Add', 'Triad']
    data_sizes = np.array([8, 16, 32, 64, 128])  # MB
    
    x, y = np.meshgrid(np.arange(len(operations)), np.arange(len(data_sizes)))
    x = x.flatten()
    y = y.flatten()
    
    # Simulated throughput data
    z = np.array([
        # Read throughput for each size
        [32.4, 33.8, 34.7, 35.2, 35.8],
        # Write throughput
        [30.1, 31.5, 32.3, 33.0, 33.4],
        # Copy throughput
        [28.5, 29.8, 30.6, 31.2, 31.5],
        # Scale throughput
        [27.2, 28.4, 29.1, 29.6, 30.0],
        # Add throughput
        [25.8, 26.9, 27.5, 28.0, 28.3],
        # Triad throughput
        [24.3, 25.4, 26.0, 26.4, 26.7]
    ])
    
    # Convert to flattened representation for 3D bar
    z = z.T.flatten()
    
    # Set color gradient based on height
    colors = cm.viridis(z / max(z))
    
    # Plot 3D bars
    dx = 0.7
    dy = 0.7
    ax.bar3d(x, y, np.zeros_like(z), dx, dy, z, color=colors, shade=True, alpha=0.8)
    
    # Set labels and ticks
    ax.set_xticks(np.arange(len(operations)))
    ax.set_yticks(np.arange(len(data_sizes)))
    ax.set_xticklabels(operations)
    ax.set_yticklabels(data_sizes)
    ax.set_zlabel('Throughput (GB/s)')
    ax.set_ylabel('Data Size (MB)')
    
    # Set a nice viewing angle
    ax.view_init(30, 225)
    plt.title('HERMES-CXL Memory Operations Performance', fontsize=16)
    plt.tight_layout()
    plt.savefig(f"{results_dir}/operations_performance_3d.png", dpi=300, bbox_inches='tight')
    
    print(f"Generated visualizations saved to {results_dir}/")

# Call this function at the end of your tests
generate_visualizations()