// CXL Shared Memory Library
// C interface for the CXL Memory Manager to be used by testing tools

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <chrono>
#include <random>
#include <atomic>
#include <thread>

#define CXL_MEM_REGION_SIZE (1UL << 30)  // 1GB default

class CXLMemoryManager {
private:
    int fd;                     // File descriptor for the CXL device
    void* mapped_region;        // Pointer to the mapped memory region
    size_t region_size;         // Size of the mapped region
    bool initialized;           // Status flag

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
        initialized = true;
        return true;
    }
    
    void cleanup() {
        if (initialized) {
            if (mapped_region && mapped_region != MAP_FAILED) {
                munmap(mapped_region, region_size);
                mapped_region = nullptr;
            }
            
            if (fd >= 0) {
                close(fd);
                fd = -1;
            }
            
            initialized = false;
        }
    }
    
    // Get pointer to shared memory at specified offset
    void* get_pointer(size_t offset = 0) {
        if (!initialized || offset >= region_size) {
            return nullptr;
        }
        return static_cast<char*>(mapped_region) + offset;
    }
    
    // Get size of mapped region
    size_t get_size() const {
        return region_size;
    }
    
    // Test write bandwidth
    double test_write(void* buffer, size_t block_size, int iterations) {
        if (!initialized || block_size > region_size) {
            return 0.0;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; i++) {
            // Calculate a different offset for each iteration to reduce caching effects
            size_t offset = (i * block_size) % (region_size - block_size);
            void* dest = static_cast<char*>(mapped_region) + offset;
            
            // Copy data to CXL memory
            memcpy(dest, buffer, block_size);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        
        // Calculate bandwidth in GB/s
        double bandwidth = (static_cast<double>(block_size) * iterations) / 
                          (elapsed.count() * 1024 * 1024 * 1024);
        
        return bandwidth;
    }
    
    // Test read bandwidth
    double test_read(void* buffer, size_t block_size, int iterations) {
        if (!initialized || block_size > region_size) {
            return 0.0;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; i++) {
            // Calculate a different offset for each iteration to reduce caching effects
            size_t offset = (i * block_size) % (region_size - block_size);
            void* src = static_cast<char*>(mapped_region) + offset;
            
            // Copy data from CXL memory
            memcpy(buffer, src, block_size);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        
        // Calculate bandwidth in GB/s
        double bandwidth = (static_cast<double>(block_size) * iterations) / 
                          (elapsed.count() * 1024 * 1024 * 1024);
        
        return bandwidth;
    }
    
    // Test memory latency
    double test_latency(int iterations) {
        if (!initialized) {
            return 0.0;
        }
        
        // Create a linked list in memory for pointer chasing
        const int list_size = 1024 * 1024;  // 1M nodes
        const size_t node_size = sizeof(size_t);
        
        // Check if we have enough memory
        if (list_size * node_size > region_size) {
            std::cerr << "Region too small for latency test" << std::endl;
            return 0.0;
        }
        
        // Initialize the linked list with pointer indices
        size_t* list = static_cast<size_t*>(mapped_region);
        
        // Create random permutation for the linked list to avoid prefetching
        std::vector<size_t> indices(list_size);
        for (int i = 0; i < list_size; i++) {
            indices[i] = i;
        }
        
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(indices.begin(), indices.end(), g);
        
        // Build the linked list
        for (int i = 0; i < list_size - 1; i++) {
            list[indices[i]] = indices[i + 1];
        }
        list[indices[list_size - 1]] = indices[0]; // Complete the cycle
        
        // Warm up the list to ensure it's in memory
        volatile size_t dummy = 0;
        for (int i = 0; i < list_size; i++) {
            dummy = list[dummy];
        }
        
        // Measure latency by traversing the list
        auto start = std::chrono::high_resolution_clock::now();
        
        volatile size_t index = 0;
        for (int i = 0; i < iterations; i++) {
            // Time how long it takes to walk through a segment of the list
            for (int j = 0; j < 1000; j++) {
                index = list[index];
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::nano> elapsed = end - start;
        
        // Calculate average latency in nanoseconds
        double latency = elapsed.count() / (iterations * 1000);
        
        return latency;
    }
    
    // Simulate FPGA operations
    double test_fpga(int operation, int iterations) {
        if (!initialized) {
            return 0.0;
        }
        
        // For this simulation, we'll just create different memory access patterns
        // based on the operation type
        
        const size_t buffer_size = 1024 * 1024; // 1MB
        
        // Check if we have enough memory
        if (buffer_size > region_size) {
            std::cerr << "Region too small for FPGA test" << std::endl;
            return 0.0;
        }
        
        switch (operation) {
            case 1: { // memcpy
                // Allocate source and destination buffers
                void* src = malloc(buffer_size);
                if (!src) return 0.0;
                
                // Fill source with random data
                std::memset(src, 0xAA, buffer_size);
                
                auto start = std::chrono::high_resolution_clock::now();
                
                for (int i = 0; i < iterations; i++) {
                    // Simulate FPGA memcpy (just do a standard memcpy for now)
                    size_t offset = (i * buffer_size) % (region_size - buffer_size);
                    void* dest = static_cast<char*>(mapped_region) + offset;
                    memcpy(dest, src, buffer_size);
                }
                
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = end - start;
                
                free(src);
                
                // Calculate bandwidth in GB/s
                double bandwidth = (static_cast<double>(buffer_size) * iterations) / 
                                  (elapsed.count() * 1024 * 1024 * 1024);
                
                return bandwidth;
            }
            
            case 2: { // memfill
                auto start = std::chrono::high_resolution_clock::now();
                
                for (int i = 0; i < iterations; i++) {
                    // Simulate FPGA memfill
                    size_t offset = (i * buffer_size) % (region_size - buffer_size);
                    void* dest = static_cast<char*>(mapped_region) + offset;
                    memset(dest, i & 0xFF, buffer_size);
                }
                
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = end - start;
                
                // Calculate bandwidth in GB/s
                double bandwidth = (static_cast<double>(buffer_size) * iterations) / 
                                  (elapsed.count() * 1024 * 1024 * 1024);
                
                return bandwidth;
            }
            
            case 3: { // compute operation (let's simulate FP operations)
                // For this demonstration, we'll use the memory as a floating-point array
                // and perform vector addition operations
                
                const size_t num_elements = buffer_size / sizeof(float);
                float* data = static_cast<float*>(mapped_region);
                
                // Initialize data
                for (size_t i = 0; i < num_elements; i++) {
                    data[i] = static_cast<float>(i);
                }
                
                auto start = std::chrono::high_resolution_clock::now();
                
                // Simulate compute operations
                for (int i = 0; i < iterations; i++) {
                    // Perform vector scaling operation: data[j] = data[j] * scalar
                    float scalar = static_cast<float>(i) * 0.01f;
                    
                    for (size_t j = 0; j < num_elements; j++) {
                        data[j] *= scalar;
                    }
                }
                
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = end - start;
                
                // Calculate performance in GFLOPS (1 multiply per element)
                double gflops = (static_cast<double>(num_elements) * iterations) / 
                               (elapsed.count() * 1e9);
                
                return gflops;
            }
            
            default:
                return 0.0;
        }
    }
};

// C interface for the CXL Memory Manager

extern "C" {

void* cxl_init(const char* device_path, size_t size) {
    CXLMemoryManager* manager = new CXLMemoryManager();
    if (!manager->initialize(device_path, size)) {
        delete manager;
        return nullptr;
    }
    return static_cast<void*>(manager);
}

void cxl_cleanup(void* handle) {
    if (handle) {
        CXLMemoryManager* manager = static_cast<CXLMemoryManager*>(handle);
        manager->cleanup();
        delete manager;
    }
}

double cxl_test_write(void* handle, void* buffer, size_t block_size, int iterations) {
    if (!handle) return 0.0;
    CXLMemoryManager* manager = static_cast<CXLMemoryManager*>(handle);
    return manager->test_write(buffer, block_size, iterations);
}

double cxl_test_read(void* handle, void* buffer, size_t block_size, int iterations) {
    if (!handle) return 0.0;
    CXLMemoryManager* manager = static_cast<CXLMemoryManager*>(handle);
    return manager->test_read(buffer, block_size, iterations);
}

double cxl_test_latency(void* handle, int iterations) {
    if (!handle) return 0.0;
    CXLMemoryManager* manager = static_cast<CXLMemoryManager*>(handle);
    return manager->test_latency(iterations);
}

double cxl_test_fpga(void* handle, int operation, int iterations) {
    if (!handle) return 0.0;
    CXLMemoryManager* manager = static_cast<CXLMemoryManager*>(handle);
    return manager->test_fpga(operation, iterations);
}

} // extern "C"