#!/usr/bin/env python3
# CXL Performance Testing Framework
# Tests shared memory access performance between CPU and CXL devices

import os
import sys
import time
import argparse
import numpy as np
import matplotlib.pyplot as plt
from multiprocessing import Process, Value, Array
import ctypes
import subprocess

# Default settings
DEFAULT_CXL_DEVICE = "/dev/cxl/cxl0"
DEFAULT_MEM_SIZE = 1 * 1024 * 1024 * 1024  # 1 GB
DEFAULT_ITERATIONS = 100
DEFAULT_BLOCK_SIZES = [4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576]

# Shared library for CXL operations (would be built from C++ code)
CXL_LIB_PATH = "./libcxl.so"

class CXLTestHarness:
    def __init__(self, device_path, mem_size):
        self.device_path = device_path
        self.mem_size = mem_size
        self.cxl_lib = None
        self.shared_mem = None
        
    def initialize(self):
        """Initialize the CXL test harness and load the shared library"""
        print(f"Initializing CXL test harness for device: {self.device_path}")
        
        # Check if device exists
        if not os.path.exists(self.device_path):
            print(f"Error: CXL device not found at {self.device_path}")
            return False
            
        try:
            # Load the CXL library
            self.cxl_lib = ctypes.CDLL(CXL_LIB_PATH)
            
            # Initialize memory manager and map memory
            self.cxl_lib.cxl_init.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
            self.cxl_lib.cxl_init.restype = ctypes.c_void_p
            
            self.shared_mem = self.cxl_lib.cxl_init(
                self.device_path.encode('utf-8'), 
                ctypes.c_size_t(self.mem_size)
            )
            
            if not self.shared_mem:
                print("Failed to initialize CXL memory")
                return False
                
            print(f"CXL shared memory initialized: {self.mem_size / (1024*1024)} MB")
            return True
            
        except Exception as e:
            print(f"Error initializing CXL test harness: {e}")
            return False
            
    def cleanup(self):
        """Clean up resources"""
        if self.cxl_lib and self.shared_mem:
            self.cxl_lib.cxl_cleanup(self.shared_mem)
            print("CXL resources cleaned up")
            
    def test_memory_bandwidth(self, block_size, iterations, test_type="write"):
        """Test memory bandwidth for reads or writes"""
        if not self.cxl_lib or not self.shared_mem:
            print("CXL test harness not initialized")
            return 0
            
        # Prepare test buffer
        buffer = np.zeros(block_size, dtype=np.uint8)
        buffer_ptr = buffer.ctypes.data_as(ctypes.c_void_p)
        
        # Set up the test function
        if test_type == "write":
            test_func = self.cxl_lib.cxl_test_write
        elif test_type == "read":
            test_func = self.cxl_lib.cxl_test_read
        else:
            print(f"Unknown test type: {test_type}")
            return 0
            
        test_func.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int]
        test_func.restype = ctypes.c_double
        
        # Run the test
        bandwidth = test_func(
            self.shared_mem,
            buffer_ptr,
            ctypes.c_size_t(block_size),
            ctypes.c_int(iterations)
        )
        
        return bandwidth
        
    def test_memory_latency(self, iterations):
        """Test memory access latency"""
        if not self.cxl_lib or not self.shared_mem:
            print("CXL test harness not initialized")
            return 0
            
        self.cxl_lib.cxl_test_latency.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.cxl_lib.cxl_test_latency.restype = ctypes.c_double
        
        latency = self.cxl_lib.cxl_test_latency(
            self.shared_mem,
            ctypes.c_int(iterations)
        )
        
        return latency
        
    def test_with_multiple_processes(self, num_processes, block_size, iterations, test_type="write"):
        """Test with multiple processes accessing shared memory concurrently"""
        if not self.cxl_lib or not self.shared_mem:
            print("CXL test harness not initialized")
            return []
            
        results = Array('d', num_processes)
        processes = []
        
        def process_func(process_id, block_size, iterations, results):
            # Each process initializes its own CXL connection
            harness = CXLTestHarness(self.device_path, self.mem_size)
            if harness.initialize():
                # Calculate offset for this process to avoid overlapping
                offset = process_id * block_size
                bandwidth = harness.test_memory_bandwidth(block_size, iterations, test_type)
                results[process_id] = bandwidth
                harness.cleanup()
            else:
                results[process_id] = 0
        
        # Create and start processes
        for i in range(num_processes):
            p = Process(target=process_func, args=(i, block_size, iterations, results))
            processes.append(p)
            p.start()
            
        # Wait for all processes to complete
        for p in processes:
            p.join()
            
        return list(results)
        
    def test_fpga_acceleration(self, operation_type, iterations):
        """Test FPGA acceleration for specific operations"""
        if not self.cxl_lib or not self.shared_mem:
            print("CXL test harness not initialized")
            return 0
            
        self.cxl_lib.cxl_test_fpga.argtypes = [
            ctypes.c_void_p, 
            ctypes.c_int, 
            ctypes.c_int
        ]
        self.cxl_lib.cxl_test_fpga.restype = ctypes.c_double
        
        # Map operation type to integer code
        op_code = 0
        if operation_type == "memcpy":
            op_code = 1
        elif operation_type == "memfill":
            op_code = 2
        elif operation_type == "compute":
            op_code = 3
        else:
            print(f"Unknown operation type: {operation_type}")
            return 0
            
        performance = self.cxl_lib.cxl_test_fpga(
            self.shared_mem,
            ctypes.c_int(op_code),
            ctypes.c_int(iterations)
        )
        
        return performance


def run_bandwidth_tests(harness, block_sizes, iterations):
    """Run bandwidth tests for various block sizes"""
    print("\n--- Bandwidth Tests ---")
    
    write_results = []
    read_results = []
    
    for size in block_sizes:
        print(f"Testing with block size: {size / 1024:.1f} KB")
        
        # Test write bandwidth
        write_bw = harness.test_memory_bandwidth(size, iterations, "write")
        write_results.append(write_bw)
        print(f"  Write bandwidth: {write_bw:.2f} GB/s")
        
        # Test read bandwidth
        read_bw = harness.test_memory_bandwidth(size, iterations, "read")
        read_results.append(read_bw)
        print(f"  Read bandwidth: {read_bw:.2f} GB/s")
    
    # Plot results
    plt.figure(figsize=(10, 6))
    plt.plot([size/1024 for size in block_sizes], write_results, 'b-o', label='Write Bandwidth')
    plt.plot([size/1024 for size in block_sizes], read_results, 'r-o', label='Read Bandwidth')
    plt.xlabel('Block Size (KB)')
    plt.ylabel('Bandwidth (GB/s)')
    plt.title('CXL Memory Bandwidth vs. Block Size')
    plt.grid(True)
    plt.legend()
    plt.savefig('cxl_bandwidth.png')
    
    return write_results, read_results


def run_latency_test(harness, iterations):
    """Run memory access latency test"""
    print("\n--- Latency Test ---")
    
    latency = harness.test_memory_latency(iterations)
    print(f"Memory access latency: {latency:.2f} ns")
    return latency


def run_fpga_acceleration_test(harness, iterations):
    """Test FPGA acceleration operations"""
    print("\n--- FPGA Acceleration Test ---")
    
    operations = ["memcpy", "memfill", "compute"]
    results = []
    
    for op in operations:
        print(f"Testing FPGA {op} acceleration...")
        perf = harness.test_fpga_acceleration(op, iterations)
        results.append(perf)
        
        if op == "memcpy" or op == "memfill":
            print(f"  {op} performance: {perf:.2f} GB/s")
        else:
            print(f"  {op} performance: {perf:.2f} GFLOPS")
    
    # Plot results
    plt.figure(figsize=(8, 6))
    plt.bar(operations, results)
    plt.xlabel('Operation Type')
    plt.ylabel('Performance (GB/s or GFLOPS)')
    plt.title('CXL FPGA Acceleration Performance')
    plt.grid(True, axis='y')
    plt.savefig('cxl_fpga_acceleration.png')
    
    return results


def run_concurrency_test(harness, num_processes, block_size, iterations):
    """Run concurrency test with multiple processes"""
    print("\n--- Concurrency Test ---")
    print(f"Testing with {num_processes} concurrent processes")
    
    # Test write bandwidth with multiple processes
    write_results = harness.test_with_multiple_processes(
        num_processes, block_size, iterations, "write"
    )
    
    # Test read bandwidth with multiple processes
    read_results = harness.test_with_multiple_processes(
        num_processes, block_size, iterations, "read"
    )
    
    # Calculate aggregate bandwidth
    total_write_bw = sum(write_results)
    total_read_bw = sum(read_results)
    
    print(f"Aggregate write bandwidth: {total_write_bw:.2f} GB/s")
    print(f"Aggregate read bandwidth: {total_read_bw:.2f} GB/s")
    
    # Plot results
    processes = list(range(1, num_processes + 1))
    
    plt.figure(figsize=(10, 6))
    plt.bar(np.array(processes) - 0.2, write_results, width=0.4, label='Write Bandwidth')
    plt.bar(np.array(processes) + 0.2, read_results, width=0.4, label='Read Bandwidth')
    plt.xlabel('Process ID')
    plt.ylabel('Bandwidth (GB/s)')
    plt.title('CXL Memory Bandwidth per Process')
    plt.grid(True, axis='y')
    plt.legend()
    plt.savefig('cxl_concurrency.png')
    
    return total_write_bw, total_read_bw


def plot_comparison(block_sizes, cxl_write, cxl_read, std_write, std_read):
    """Plot comparison between CXL and standard memory"""
    plt.figure(figsize=(12, 8))
    
    # Plot write bandwidth comparison
    plt.subplot(2, 1, 1)
    plt.plot([size/1024 for size in block_sizes], cxl_write, 'b-o', label='CXL Write')
    plt.plot([size/1024 for size in block_sizes], std_write, 'b--o', label='Standard Write')
    plt.xlabel('Block Size (KB)')
    plt.ylabel('Bandwidth (GB/s)')
    plt.title('Write Bandwidth: CXL vs Standard Memory')
    plt.grid(True)
    plt.legend()
    
    # Plot read bandwidth comparison
    plt.subplot(2, 1, 2)
    plt.plot([size/1024 for size in block_sizes], cxl_read, 'r-o', label='CXL Read')
    plt.plot([size/1024 for size in block_sizes], std_read, 'r--o', label='Standard Read')
    plt.xlabel('Block Size (KB)')
    plt.ylabel('Bandwidth (GB/s)')
    plt.title('Read Bandwidth: CXL vs Standard Memory')
    plt.grid(True)
    plt.legend()
    
    plt.tight_layout()
    plt.savefig('cxl_vs_standard.png')


def compare_with_standard_memory(block_sizes, iterations):
    """Compare CXL memory with standard system memory"""
    print("\n--- Comparing CXL vs Standard Memory ---")
    
    std_write_results = []
    std_read_results = []
    
    # Create temporary buffers for testing
    for size in block_sizes:
        print(f"Testing with block size: {size / 1024:.1f} KB")
        
        # Allocate source and destination buffers
        src_buffer = np.ones(size, dtype=np.uint8)
        dst_buffer = np.zeros(size, dtype=np.uint8)
        
        # Test write bandwidth (memcpy simulates write)
        start_time = time.time()
        for _ in range(iterations):
            np.copyto(dst_buffer, src_buffer)
        end_time = time.time()
        
        elapsed = end_time - start_time
        write_bw = (size * iterations) / (elapsed * 1024 * 1024 * 1024)
        std_write_results.append(write_bw)
        print(f"  Standard memory write bandwidth: {write_bw:.2f} GB/s")
        
        # Test read bandwidth (just reading the buffer multiple times)
        start_time = time.time()
        for _ in range(iterations):
            # Force reads by summing the buffer
            _ = np.sum(src_buffer)
        end_time = time.time()
        
        elapsed = end_time - start_time
        read_bw = (size * iterations) / (elapsed * 1024 * 1024 * 1024)
        std_read_results.append(read_bw)
        print(f"  Standard memory read bandwidth: {read_bw:.2f} GB/s")
    
    return std_write_results, std_read_results


def main():
    """Main function to run the CXL performance test framework"""
    parser = argparse.ArgumentParser(description='CXL Performance Testing Framework')
    parser.add_argument('--device', default=DEFAULT_CXL_DEVICE, help='CXL device path')
    parser.add_argument('--size', type=int, default=DEFAULT_MEM_SIZE, help='Memory size in bytes')
    parser.add_argument('--iterations', type=int, default=DEFAULT_ITERATIONS, help='Number of test iterations')
    parser.add_argument('--processes', type=int, default=4, help='Number of processes for concurrency test')
    parser.add_argument('--test', choices=['all', 'bandwidth', 'latency', 'concurrency', 'fpga', 'compare'],
                        default='all', help='Test to run')
    args = parser.parse_args()
    
    # Initialize the test harness
    harness = CXLTestHarness(args.device, args.size)
    if not harness.initialize():
        print("Failed to initialize test harness. Exiting.")
        return 1
    
    try:
        # Run selected tests
        if args.test == 'all' or args.test == 'bandwidth':
            cxl_write, cxl_read = run_bandwidth_tests(harness, DEFAULT_BLOCK_SIZES, args.iterations)
        
        if args.test == 'all' or args.test == 'latency':
            run_latency_test(harness, args.iterations)
        
        if args.test == 'all' or args.test == 'concurrency':
            run_concurrency_test(harness, args.processes, DEFAULT_BLOCK_SIZES[5], args.iterations)
        
        if args.test == 'all' or args.test == 'fpga':
            run_fpga_acceleration_test(harness, args.iterations)
        
        if args.test == 'all' or args.test == 'compare':
            std_write, std_read = compare_with_standard_memory(DEFAULT_BLOCK_SIZES, args.iterations)
            if args.test == 'all':
                plot_comparison(DEFAULT_BLOCK_SIZES, cxl_write, cxl_read, std_write, std_read)
        
        print("\nTests completed successfully!")
    
    except Exception as e:
        print(f"Error during testing: {e}")
        return 1
    
    finally:
        # Clean up resources
        harness.cleanup()
    
    return 0


if __name__ == "__main__":
    sys.exit(main())