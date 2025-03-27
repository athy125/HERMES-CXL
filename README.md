# HERMES-CXL

**H**igh **E**fficiency **R**esource **M**anagement and **E**xchange **S**ystem for Compute Express Link


## Overview

HERMES-CXL represents a pioneering implementation of a heterogeneous memory architecture leveraging Compute Express Link (CXL) technology. This advanced memory fabric facilitates coherent communication between central processing units and various accelerator devices, including FPGAs, GPUs, and specialized AI accelerators. By establishing a unified memory semantic paradigm, HERMES-CXL transcends the limitations of traditional PCIe-based I/O, enabling near-memory computation with dramatically reduced data movement overhead.

The system implements the CXL.mem protocol subset to facilitate a load/store memory model across heterogeneous compute elements, thereby creating a coherent shared memory pool with dynamic allocation capabilities. This architecture is particularly advantageous for data-intensive applications in high-performance computing, real-time analytics, and artificial intelligence workloads where memory bandwidth and access latency represent critical performance determinants.

## Key Capabilities

- **Memory-Semantic Operations**: Implementation of CXL.mem protocol for direct load/store operations to device memory without traditional DMA overhead
- **Coherent Memory Architecture**: Maintenance of cache coherency between CPU and accelerator memory spaces through CXL protocol mechanisms
- **Zero-Copy Data Exchange**: Elimination of redundant buffer copies through shared virtual address spaces
- **Advanced Memory Management**: Sophisticated allocation strategies with support for NUMA-aware placement and tiered memory organization
- **Comprehensive Performance Instrumentation**: Fine-grained metrics collection for bandwidth, latency, and throughput analysis
- **Kernel Integration**: Full Linux kernel driver supporting CXL Type-2 and Type-3 device topologies

## Architectural Framework

HERMES-CXL implements a sophisticated memory pooling architecture where coherent memory regions are dynamically allocated and managed across heterogeneous compute resources:

```
┌─────────────────┐                 ┌─────────────────────┐
│                 │                 │                     │
│  CPU Complex    │◄──── CXL.io ───►│  Accelerator Device │
│  ┌───────────┐  │                 │  ┌───────────────┐  │
│  │ L1/L2/L3  │  │                 │  │ Device Cache  │  │
│  │  Caches   │  │                 │  │  Hierarchy    │  │
│  └───────────┘  │                 │  └───────────────┘  │
│        ▲        │                 │          ▲          │
│        │        │                 │          │          │
│        ▼        │                 │          ▼          │
│  ┌───────────┐  │◄── CXL.cache ──►│  ┌───────────────┐  │
│  │ Memory    │  │                 │  │ Device        │  │
│  │ Controller│  │                 │  │ Memory        │  │
│  └───────────┘  │                 │  │ Controller    │  │
│        ▲        │                 │  └───────────────┘  │
└────────┼────────┘                 └──────────┼──────────┘
         │                                     │
         ▼                                     ▼
┌────────────────────────────────────────────────────────┐
│                                                        │
│               Coherent Memory Domain                   │
│               ┌──────────────────────┐                 │
│               │ Memory Region Type A │                 │
│               │ (CPU-optimized)      │                 │
│               └──────────────────────┘                 │
│               ┌──────────────────────┐                 │
│               │ Memory Region Type B │                 │
│               │ (Accelerator-opt.)   │                 │
│               └──────────────────────┘                 │
│                                                        │
└────────────────────────────────────────────────────────┘
```

## Performance Analysis Methodology

HERMES-CXL performance has been rigorously evaluated through a comprehensive set of benchmarks designed to measure various aspects of memory subsystem behavior. Our analytical framework encompasses bandwidth characterization, access latency profiling, concurrency scaling, and operation-specific throughput assessment.

### Memory Bandwidth Characterization

![Bandwidth Comparison](docs/images/bandwidth_comparison.png)

This visualization presents a comparative analysis of memory bandwidth achieved by HERMES-CXL versus conventional memory architectures across varying transfer sizes. Several critical observations emerge from this data:

1. **Bandwidth Scaling Characteristics**: HERMES-CXL demonstrates superior scaling efficiency with increasing block sizes, achieving approximately 30% higher peak throughput compared to standard memory interfaces.

2. **Read/Write Asymmetry**: Both architectures exhibit the expected read bandwidth premium over write operations, but HERMES-CXL maintains a more balanced read/write ratio (approximately 1.07:1 versus 1.12:1 for standard memory).

3. **Saturation Point Analysis**: The inflection point where bandwidth approaches asymptotic maximum occurs at smaller block sizes (approximately 128KB) for HERMES-CXL compared to standard memory (approximately 256KB), indicating more efficient handling of medium-sized transfers.

4. **Absolute Performance Delta**: At optimal transfer sizes, HERMES-CXL achieves peak read bandwidths of 36.7 GB/s versus 28.2 GB/s for standard memory—a significant 30.1% improvement that directly impacts application-level performance for memory-bound workloads.

### Access Latency Profile Analysis

![Latency Heatmap](docs/images/latency_heatmap.png)

This sophisticated heatmap visualization decomposes memory access latency across diverse access patterns and memory subsystem interfaces. The color gradient encodes absolute latency values in nanoseconds, with deeper blue regions representing superior performance:

1. **Pattern-Specific Optimization**: HERMES-CXL exhibits particularly notable latency advantages for sequential access patterns (42ns versus 58ns for standard memory), while maintaining competitive performance even for challenging random access scenarios (85ns versus 115ns).

2. **Directional Asymmetry**: The data reveals an interesting asymmetry in CPU→memory versus memory→CPU transfers, with HERMES-CXL demonstrating more balanced bidirectional performance compared to standard memory architectures.

3. **Strided Access Efficiency**: For complex strided access patterns common in scientific computing and matrix operations, HERMES-CXL provides a 13.3% latency reduction (65ns versus 75ns), suggesting architectural advantages for computational fluid dynamics and finite element analysis workloads.

4. **Gather/Scatter Performance**: The significant performance delta for gather/scatter operations (95/105ns versus 120/130ns) indicates HERMES-CXL's superior handling of non-contiguous memory access patterns, a critical factor for graph analytics and sparse matrix computations.

### Concurrency Scaling Characteristics

![Scaling Performance](docs/images/scaling_performance.png)

This visualization quantifies how effectively HERMES-CXL maintains performance under increasing concurrency, a critical factor for multi-threaded and multi-process applications:

1. **Near-Linear Scaling Region**: HERMES-CXL demonstrates near-linear performance scaling up to 4 concurrent processes (efficiency factor of 0.86), significantly outperforming standard memory architectures (efficiency factor of 0.78).

2. **Scaling Saturation Analysis**: Beyond 4 processes, both architectures exhibit expected sublinear scaling due to memory controller contention, but HERMES-CXL maintains a consistently higher efficiency curve throughout the measured range.

3. **Relative Scaling Advantage**: At 8 concurrent processes, HERMES-CXL achieves 4.72× relative performance versus 3.85× for standard memory—a 22.6% advantage in concurrency handling that translates directly to improved performance for parallel scientific applications and database systems.

4. **Resource Contention Resilience**: The more gradual tapering of the HERMES-CXL scaling curve suggests architectural enhancements in coherence protocol efficiency and reduced controller-level contention.

### Operation-Specific Performance Matrix

![Operations Performance](docs/images/operations_performance_3d.png)

This multidimensional visualization presents a comprehensive performance landscape across operation types and data sizes, providing insight into HERMES-CXL's operational characteristics:

1. **Operation Throughput Hierarchy**: As expected, simple read operations achieve highest throughput (32.4-35.8 GB/s depending on data size), followed by write operations (30.1-33.4 GB/s), with more complex operations showing proportionally lower but still impressive throughput values.

2. **Data Size Sensitivity**: All operations demonstrate positive scaling with data size until hardware limits are approached, with optimal efficiency observed at the 128MB data point for most operation types.

3. **Complex Operation Efficiency**: For the computationally intensive triad operations (A = B + scalar × C), HERMES-CXL achieves 24.3-26.7 GB/s effective throughput, representing excellent memory subsystem utilization despite the computational complexity.

4. **Vectorization Potential**: The consistent performance across scaled operations suggests effective utilization of SIMD processing capabilities, indicating that HERMES-CXL's memory architecture complements modern vector processing units.

## Technical Requirements

- Linux kernel 5.12+ with CXL subsystem support enabled
- CXL-capable hardware platform (CPU with CXL controller support)
- Accelerator devices with CXL endpoint compatibility  
- GCC/G++ 9.0+ with C++17 support
- Python 3.6+ with NumPy, Matplotlib, and Seaborn for analysis tools

## Simulation Environment

For environments without CXL hardware infrastructure, HERMES-CXL includes a sophisticated simulation mode that accurately models the architectural behavior and performance characteristics:

```bash
# Configure the simulation environment
./scripts/setup_codespace.sh

# Execute the demonstration sequence
./scripts/run_demo.sh
```

## Build Process

```bash
# Clone the repository
git clone https://github.com/yourusername/HERMES-CXL.git
cd HERMES-CXL

# Build all system components
make all

# Execute performance evaluation suite
cd test
python test_framework.py --device /dev/cxl/cxl0 --test all
```

## Repository Structure

```
hermes-cxl/
├── src/                  # Source code implementation
│   ├── app/              # Application layer components
│   ├── driver/           # CXL device driver implementation
│   ├── lib/              # Core library functionality
│   └── simulation/       # Simulation infrastructure
├── include/              # Header declarations
├── test/                 # Comprehensive testing framework
├── docs/                 # Technical documentation
│   └── images/           # Performance visualization assets
├── scripts/              # Automation and utility scripts
└── build/                # Compilation artifacts
```