#ifndef CXL_MEMORY_MANAGER_H
#define CXL_MEMORY_MANAGER_H

#include <cstddef>
#include <cstdint>

class CXLMemoryManager {
public:
    CXLMemoryManager();
    ~CXLMemoryManager();
    
    bool initialize(const char* device_path, size_t size);
    void cleanup();
    
    bool write_data(size_t offset, const void* data, size_t length);
    bool read_data(size_t offset, void* buffer, size_t length);
    void* get_direct_pointer(size_t offset);
    size_t get_region_size() const;

private:
    int fd;                  // File descriptor for the CXL device
    void* mapped_region;     // Pointer to the mapped memory region
    size_t region_size;      // Size of the mapped region
    bool initialized;        // Status flag
};

#endif // CXL_MEMORY_MANAGER_H