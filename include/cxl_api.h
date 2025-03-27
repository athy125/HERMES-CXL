#ifndef CXL_API_H
#define CXL_API_H

#include <cstddef>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize CXL memory
void* cxl_init(const char* device_path, size_t size);

// Clean up CXL memory
void cxl_cleanup(void* handle);

// Test write bandwidth
double cxl_test_write(void* handle, void* buffer, size_t block_size, int iterations);

// Test read bandwidth
double cxl_test_read(void* handle, void* buffer, size_t block_size, int iterations);

// Test memory latency
double cxl_test_latency(void* handle, int iterations);

// Test FPGA operations
double cxl_test_fpga(void* handle, int operation, int iterations);

#ifdef __cplusplus
}
#endif

#endif // CXL_API_H