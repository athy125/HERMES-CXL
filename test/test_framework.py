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
    print(f"Memory access latency