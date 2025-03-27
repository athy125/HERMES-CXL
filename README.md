# CXL Shared Memory System

A prototype implementation for efficient CPU-device communication using Compute Express Link (CXL) technology.

## Overview

This project implements a shared memory system using CXL technology to enable efficient communication between CPUs and accelerator devices such as FPGAs. CXL is a high-speed interconnect protocol that provides a coherent memory space between the CPU and accelerator devices, allowing for more efficient data transfer and reduced latency.

### Key Components

1. **CXL Memory Manager**: Core C++ class for managing shared memory regions via CXL
2. **FPGA Driver**: Linux kernel module for interfacing with CXL-capable FPGA devices
3. **Memory Allocator**: System for managing memory allocation within the CXL shared memory pool
4. **System Driver**: Interface for communicating with CXL devices and memory regions
5. **Performance Testing Framework**: Tools for measuring and comparing the performance of CXL memory vs. standard memory

## Requirements

- Linux kernel 5.12+ with CXL support enabled
- CXL-capable hardware (CPU and accelerator device)
- GCC/G++ 9.0+ with C++17 support
- Python 3.6+ (for testing framework)
- Matplotlib, NumPy (for visualization)

## Building the Project

To build all components of the project:

```bash
make all
```

This will build:
- `libcxl.so`: Shared library for CXL memory operations
- `cxl_fpga.ko`: Kernel module for CXL FPGA support
- `cxl_system`: Main application for testing the memory system

## Installation

To install the components:

```bash
sudo make install
```

This will:
- Copy the shared library to `/usr/local/lib/`
- Install the kernel module to the appropriate kernel directory
- Copy the main executable to `/usr/local/bin/`

## Usage

### Loading the Kernel Module

```bash
sudo modprobe cxl
sudo insmod cxl_fpga.ko
```

### Running the Main System

```bash
cxl_system
```

This will initialize the CXL memory manager, run memory allocation tests, and measure performance.

### Running Performance Tests

The Python-based testing framework provides comprehensive performance testing:

```bash
./test-framework.py --device /dev/cxl/cxl0 --test all
```

Options:
- `--device`: Path to the CXL device (default: `/dev/cxl/cxl0`)
- `--size`: Memory size to use in bytes (default: 1GB)
- `--iterations`: Number of iterations for tests (default: 100)
- `--processes`: Number of processes for concurrency tests (default: 4)
- `--test`: Specific test to run (choices: all, bandwidth, latency, concurrency, fpga, compare)

## Architecture

### Memory Architecture

The CXL shared memory system uses a memory-pooling architecture where a large region of memory is made available to both the CPU and accelerator devices. The memory is mapped into the CPU's address space using the `mmap` system call, and is also accessible to the accelerator device through CXL memory windows.

```
+-------------+           +----------------+
|             |<--CXL---->|                |
|     CPU     |           |  Accelerator   |
|             |           |  Device (FPGA) |
+-------------+           +----------------+
       ^                          ^
       |                          |
       v                          v
+-------------------------------------+
|                                     |
|      CXL Shared Memory Region       |
|                                     |
+-------------------------------------+
```

### Kernel Driver

The kernel driver (`cxl_fpga.ko`) handles the low-level communication with the CXL-capable device. It:

1. Registers with the CXL subsystem
2. Manages CXL memory windows
3. Provides a character device interface for userspace applications
4. Handles direct memory mapping operations

### Memory Management

The CXL Memory Manager (`CXLMemoryManager` class) provides:

1. Initialization and mapping of CXL memory regions
2. Direct memory access for zero-copy operations
3. Memory allocation tracking and management
4. Performance measurement capabilities

The memory allocator (`CXLAllocator` class) implements:

1. Memory block allocation with customizable alignment
2. Block coalescing for efficient memory usage
3. Thread-safe allocation operations

## Performance Considerations

CXL memory offers several performance advantages over traditional I/O:

1. **Lower Latency**: Direct access to memory without PCIe protocol overhead
2. **Higher Bandwidth**: Leverages CPU memory controller capabilities
3. **Memory Coherence**: Automatic cache coherence between CPU and device
4. **Zero-Copy Operations**: No need to copy data between separate memory spaces

The testing framework measures these aspects and compares with standard system memory.

## Known Limitations

- Requires specific hardware with CXL support
- Current implementation is a prototype and not optimized for production use
- Limited to Linux environments with specific kernel support
- Device hot-plugging is not currently supported

## Future Enhancements

- Support for CXL 2.0/3.0 features
- Integration with fabric-attached memory pools
- Multi-device memory sharing
- Quality of Service (QoS) support
- Memory tiering and paging support
- Dynamic memory allocation and reclamation

## License

This software is provided under the MIT License. See LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.