// CXL Shared Memory System
// A prototype implementation for efficient CPU-device communication
// Using CXL for memory expansion and acceleration


#include "cxl_common.h"
#include "cxl_memory_manager.h"
#include "cxl_api.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/cxl.h>
#include <string.h>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>

// CXL device paths (these would be actual device paths in a real implementation)
#define CXL_DEVICE_PATH "/dev/cxl/cxl0"
#define CXL_MEM_REGION_SIZE (1UL << 30)  // 1GB shared memory region

class CXLMemoryManager {
private:
    int fd;                       // File descriptor for the CXL device
    void* mapped_region;          // Pointer to the mapped memory region
    size_t region_size;           // Size of the mapped region
    std::atomic<bool> initialized;// Status flag

public:
    CXLMemoryManager() : fd(-1), mapped_region(nullptr), region_size(0), initialized(false) {}
    
    ~CXLMemoryManager() {
        cleanup();
    }
    
    bool initialize(const char* device_path, size_t size) {
        // Open the CXL device
        fd = open(device_path, O_RDWR);
        if (fd < 0) {
            std::cerr << "Failed to open CXL device at " << device_path << std::endl;
            return false;
        }
        
        // Map the memory region
        mapped_region = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (mapped_region == MAP_FAILED) {
            std::cerr << "Failed to map memory region" << std::endl;
            close(fd);
            fd = -1;
            return false;
        }
        
        region_size = size;
        initialized.store(true, std::memory_order_release);
        std::cout << "CXL memory region initialized: " << size / (1024*1024) << " MB" << std::endl;
        return true;
    }
    
    void cleanup() {
        if (initialized.load(std::memory_order_acquire)) {
            if (mapped_region && mapped_region != MAP_FAILED) {
                munmap(mapped_region, region_size);
                mapped_region = nullptr;
            }
            
            if (fd >= 0) {
                close(fd);
                fd = -1;
            }
            
            initialized.store(false, std::memory_order_release);
            std::cout << "CXL memory region cleaned up" << std::endl;
        }
    }
    
    // Write data to the shared memory region
    bool write_data(size_t offset, const void* data, size_t length) {
        if (!initialized.load(std::memory_order_acquire)) {
            std::cerr << "Memory manager not initialized" << std::endl;
            return false;
        }
        
        if (offset + length > region_size) {
            std::cerr << "Write operation exceeds memory region bounds" << std::endl;
            return false;
        }
        
        char* dest = static_cast<char*>(mapped_region) + offset;
        memcpy(dest, data, length);
        return true;
    }
    
    // Read data from the shared memory region
    bool read_data(size_t offset, void* buffer, size_t length) {
        if (!initialized.load(std::memory_order_acquire)) {
            std::cerr << "Memory manager not initialized" << std::endl;
            return false;
        }
        
        if (offset + length > region_size) {
            std::cerr << "Read operation exceeds memory region bounds" << std::endl;
            return false;
        }
        
        const char* src = static_cast<const char*>(mapped_region) + offset;
        memcpy(buffer, src, length);
        return true;
    }
    
    // Get direct pointer to memory region (for zero-copy operations)
    void* get_direct_pointer(size_t offset) {
        if (!initialized.load(std::memory_order_acquire) || offset >= region_size) {
            return nullptr;
        }
        return static_cast<char*>(mapped_region) + offset;
    }
    
    // Get total size of memory region
    size_t get_region_size() const {
        return region_size;
    }
};

// Memory allocation management for CXL memory pool
class CXLAllocator {
private:
    CXLMemoryManager& memory_manager;
    std::vector<std::pair<size_t, size_t>> free_blocks;  // (offset, size) pairs
    std::vector<std::pair<size_t, size_t>> allocated_blocks;
    std::mutex allocation_mutex;

public:
    CXLAllocator(CXLMemoryManager& mgr) : memory_manager(mgr) {
        // Initialize with one large free block
        free_blocks.push_back(std::make_pair(0, memory_manager.get_region_size()));
    }
    
    // Allocate a block of memory from the pool
    void* allocate(size_t size, size_t alignment = 64) {
        std::lock_guard<std::mutex> lock(allocation_mutex);
        
        // Align size to the specified boundary
        size = (size + alignment - 1) & ~(alignment - 1);
        
        // Find a suitable free block using first-fit strategy
        for (auto it = free_blocks.begin(); it != free_blocks.end(); ++it) {
            size_t block_offset = it->first;
            size_t block_size = it->second;
            
            // Align the block offset
            size_t aligned_offset = (block_offset + alignment - 1) & ~(alignment - 1);
            size_t alignment_waste = aligned_offset - block_offset;
            
            if (block_size >= size + alignment_waste) {
                // We found a suitable block
                
                // Remove the current block from free list
                free_blocks.erase(it);
                
                // If there's space before the aligned address, add it as a free block
                if (alignment_waste > 0) {
                    free_blocks.push_back(std::make_pair(block_offset, alignment_waste));
                }
                
                // If there's space after the allocation, add it as a free block
                if (block_size > size + alignment_waste) {
                    free_blocks.push_back(std::make_pair(aligned_offset + size, 
                                                       block_size - size - alignment_waste));
                }
                
                // Add to allocated blocks
                allocated_blocks.push_back(std::make_pair(aligned_offset, size));
                
                // Return a pointer to the allocated memory
                return memory_manager.get_direct_pointer(aligned_offset);
            }
        }
        
        // No suitable block found
        std::cerr << "Failed to allocate " << size << " bytes from CXL memory pool" << std::endl;
        return nullptr;
    }
    
    // Free a previously allocated block
    bool free(void* ptr) {
        std::lock_guard<std::mutex> lock(allocation_mutex);
        
        // Calculate the offset from the base of the mapped region
        char* base_ptr = static_cast<char*>(memory_manager.get_direct_pointer(0));
        char* free_ptr = static_cast<char*>(ptr);
        size_t offset = free_ptr - base_ptr;
        
        // Find the allocation
        for (auto it = allocated_blocks.begin(); it != allocated_blocks.end(); ++it) {
            if (it->first == offset) {
                size_t size = it->second;
                
                // Remove from allocated blocks
                allocated_blocks.erase(it);
                
                // Add to free blocks
                free_blocks.push_back(std::make_pair(offset, size));
                
                // Coalesce adjacent free blocks (simple implementation)
                std::sort(free_blocks.begin(), free_blocks.end());
                
                for (size_t i = 0; i < free_blocks.size() - 1; ) {
                    if (free_blocks[i].first + free_blocks[i].second == free_blocks[i+1].first) {
                        // Merge blocks
                        free_blocks[i].second += free_blocks[i+1].second;
                        free_blocks.erase(free_blocks.begin() + i + 1);
                    } else {
                        i++;
                    }
                }
                
                return true;
            }
        }
        
        std::cerr << "Failed to free pointer: not found in allocated blocks" << std::endl;
        return false;
    }
    
    // Get statistics about memory usage
    void get_stats(size_t& total, size_t& used, size_t& free) {
        std::lock_guard<std::mutex> lock(allocation_mutex);
        
        total = memory_manager.get_region_size();
        
        used = 0;
        for (const auto& block : allocated_blocks) {
            used += block.second;
        }
        
        free = 0;
        for (const auto& block : free_blocks) {
            free += block.second;
        }
    }
};

// Driver for interfacing with FPGA over CXL
class CXLDeviceDriver {
private:
    int device_fd;
    struct cxl_mem_command cmd;

public:
    CXLDeviceDriver() : device_fd(-1) {
        memset(&cmd, 0, sizeof(cmd));
    }
    
    ~CXLDeviceDriver() {
        if (device_fd >= 0) {
            close(device_fd);
        }
    }
    
    bool initialize(const char* device_path) {
        device_fd = open(device_path, O_RDWR);
        if (device_fd < 0) {
            std::cerr << "Failed to open CXL device for command interface" << std::endl;
            return false;
        }
        return true;
    }
    
    // Send a command to the CXL device (FPGA)
    bool send_command(uint32_t opcode, uint64_t address, uint64_t data) {
        if (device_fd < 0) {
            std::cerr << "Device driver not initialized" << std::endl;
            return false;
        }
        
        // Set up command structure
        cmd.id = 0;
        cmd.opcode = opcode;
        cmd.address = address;
        cmd.data = data;
        
        // Use ioctl to send the command
        if (ioctl(device_fd, CXL_MEM_SEND_COMMAND, &cmd) < 0) {
            std::cerr << "Failed to send command to CXL device" << std::endl;
            return false;
        }
        
        return true;
    }
    
    // Wait for a response from the device
    bool wait_for_response(uint32_t& status, uint64_t& result) {
        if (device_fd < 0) {
            std::cerr << "Device driver not initialized" << std::endl;
            return false;
        }
        
        // Poll for command completion
        struct cxl_mem_query_cmd query;
        query.id = cmd.id;
        
        do {
            if (ioctl(device_fd, CXL_MEM_QUERY_CMD, &query) < 0) {
                std::cerr << "Failed to query command status" << std::endl;
                return false;
            }
            
            if (query.status != CXL_CMD_STATUS_ACTIVE) {
                break;
            }
            
            // Small delay before polling again
            usleep(10);
        } while (true);
        
        status = query.status;
        result = query.result;
        
        return (status == CXL_CMD_STATUS_COMPLETED);
    }
};

// Performance testing utilities
class CXLPerformanceTester {
private:
    CXLMemoryManager& memory_manager;
    size_t buffer_size;
    void* host_buffer;
    
public:
    CXLPerformanceTester(CXLMemoryManager& mgr, size_t size) 
        : memory_manager(mgr), buffer_size(size) {
        // Allocate host buffer for testing
        host_buffer = aligned_alloc(4096, buffer_size);
        if (!host_buffer) {
            std::cerr << "Failed to allocate host buffer for testing" << std::endl;
            throw std::bad_alloc();
        }
        
        // Initialize buffer with test data
        for (size_t i = 0; i < buffer_size / sizeof(uint32_t); i++) {
            ((uint32_t*)host_buffer)[i] = i;
        }
    }
    
    ~CXLPerformanceTester() {
        if (host_buffer) {
            free(host_buffer);
        }
    }
    
    // Test write performance
    double test_write_performance(size_t iterations) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < iterations; i++) {
            size_t offset = (i * buffer_size) % (memory_manager.get_region_size() - buffer_size);
            memory_manager.write_data(offset, host_buffer, buffer_size);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        
        double bandwidth = (buffer_size * iterations) / (elapsed.count() * 1024 * 1024 * 1024);
        return bandwidth;  // GB/s
    }
    
    // Test read performance
    double test_read_performance(size_t iterations) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < iterations; i++) {
            size_t offset = (i * buffer_size) % (memory_manager.get_region_size() - buffer_size);
            memory_manager.read_data(offset, host_buffer, buffer_size);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        
        double bandwidth = (buffer_size * iterations) / (elapsed.count() * 1024 * 1024 * 1024);
        return bandwidth;  // GB/s
    }
    
    // Compare CXL performance with standard memory access
    void compare_with_standard_memory(size_t iterations) {
        // Allocate a standard memory buffer of the same size as the CXL region
        void* std_buffer = aligned_alloc(4096, memory_manager.get_region_size());
        if (!std_buffer) {
            std::cerr << "Failed to allocate standard memory buffer for comparison" << std::endl;
            return;
        }
        
        // Test write to standard memory
        auto start_std_write = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < iterations; i++) {
            size_t offset = (i * buffer_size) % (memory_manager.get_region_size() - buffer_size);
            memcpy((char*)std_buffer + offset, host_buffer, buffer_size);
        }
        auto end_std_write = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_std_write = end_std_write - start_std_write;
        
        // Test read from standard memory
        auto start_std_read = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < iterations; i++) {
            size_t offset = (i * buffer_size) % (memory_manager.get_region_size() - buffer_size);
            memcpy(host_buffer, (char*)std_buffer + offset, buffer_size);
        }
        auto end_std_read = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_std_read = end_std_read - start_std_read;
        
        // Test write to CXL memory
        double cxl_write_bw = test_write_performance(iterations);
        
        // Test read from CXL memory
        double cxl_read_bw = test_read_performance(iterations);
        
        // Calculate standard memory bandwidth
        double std_write_bw = (buffer_size * iterations) / (elapsed_std_write.count() * 1024 * 1024 * 1024);
        double std_read_bw = (buffer_size * iterations) / (elapsed_std_read.count() * 1024 * 1024 * 1024);
        
        // Print results
        std::cout << "=== Performance Comparison: Standard Memory vs CXL ===" << std::endl;
        std::cout << "Standard Memory Write: " << std_write_bw << " GB/s" << std::endl;
        std::cout << "CXL Memory Write:      " << cxl_write_bw << " GB/s" << std::endl;
        std::cout << "Write Ratio (CXL/Std): " << (cxl_write_bw / std_write_bw) << std::endl;
        std::cout << std::endl;
        std::cout << "Standard Memory Read:  " << std_read_bw << " GB/s" << std::endl;
        std::cout << "CXL Memory Read:       " << cxl_read_bw << " GB/s" << std::endl;
        std::cout << "Read Ratio (CXL/Std):  " << (cxl_read_bw / std_read_bw) << std::endl;
        
        free(std_buffer);
    }
};

// Main application
int main(int argc, char* argv[]) {
    std::cout << "CXL Shared Memory System - Prototype" << std::endl;
    std::cout << "-----------------------------------" << std::endl;
    
    // Initialize the CXL memory manager
    CXLMemoryManager memory_manager;
    if (!memory_manager.initialize(CXL_DEVICE_PATH, CXL_MEM_REGION_SIZE)) {
        std::cerr << "Failed to initialize CXL memory manager" << std::endl;
        return 1;
    }
    
    // Initialize the allocator
    CXLAllocator allocator(memory_manager);
    
    // Initialize the device driver
    CXLDeviceDriver driver;
    if (!driver.initialize(CXL_DEVICE_PATH)) {
        std::cerr << "Failed to initialize CXL device driver" << std::endl;
        return 1;
    }
    
    // Run some basic allocator tests
    std::cout << "Testing memory allocation..." << std::endl;
    void* block1 = allocator.allocate(1024 * 1024);  // 1 MB
    void* block2 = allocator.allocate(16 * 1024 * 1024);  // 16 MB
    void* block3 = allocator.allocate(64 * 1024 * 1024);  // 64 MB
    
    size_t total, used, free;
    allocator.get_stats(total, used, free);
    std::cout << "Memory stats after allocation:" << std::endl;
    std::cout << "Total: " << (total / (1024*1024)) << " MB" << std::endl;
    std::cout << "Used:  " << (used / (1024*1024)) << " MB" << std::endl;
    std::cout << "Free:  " << (free / (1024*1024)) << " MB" << std::endl;
    
    // Free some memory
    allocator.free(block2);
    allocator.get_stats(total, used, free);
    std::cout << "Memory stats after freeing 16 MB block:" << std::endl;
    std::cout << "Total: " << (total / (1024*1024)) << " MB" << std::endl;
    std::cout << "Used:  " << (used / (1024*1024)) << " MB" << std::endl;
    std::cout << "Free:  " << (free / (1024*1024)) << " MB" << std::endl;
    
    // Test device commands
    std::cout << "Testing device commands..." << std::endl;
    uint64_t device_memory_offset = reinterpret_cast<uint64_t>(block1) - 
                                    reinterpret_cast<uint64_t>(memory_manager.get_direct_pointer(0));
    
    driver.send_command(0x01, device_memory_offset, 1024*1024);  // Example command
    
    uint32_t status;
    uint64_t result;
    
    if (driver.wait_for_response(status, result)) {
        std::cout << "Command completed successfully" << std::endl;
        std::cout << "Result: " << result << std::endl;
    } else {
        std::cout << "Command failed with status: " << status << std::endl;
    }
    
    // Run performance tests
    std::cout << "Running performance tests..." << std::endl;
    CXLPerformanceTester tester(memory_manager, 1024 * 1024);  // 1 MB buffer
    
    std::cout << "CXL write bandwidth: " << tester.test_write_performance(1000) << " GB/s" << std::endl;
    std::cout << "CXL read bandwidth:  " << tester.test_read_performance(1000) << " GB/s" << std::endl;
    
    // Compare with standard memory
    tester.compare_with_standard_memory(1000);
    
    // Clean up
    allocator.free(block1);
    allocator.free(block3);
    
    std::cout << "CXL Shared Memory System - Test completed" << std::endl;
    
    return 0;
}