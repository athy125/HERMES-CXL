#include <iostream>
#include <fstream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define SIM_MEMORY_SIZE (1UL << 30)  // 1GB

int main() {
    std::cout << "Starting CXL simulator..." << std::endl;
    
    // Create a memory mapped file to simulate CXL memory
    void* sim_memory = mmap(NULL, SIM_MEMORY_SIZE, 
                           PROT_READ | PROT_WRITE, 
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (sim_memory == MAP_FAILED) {
        std::cerr << "Failed to allocate simulation memory" << std::endl;
        return 1;
    }
    
    // Create a simulated device file
    std::ofstream device_file("/tmp/cxl_sim/cxl0");
    device_file.close();
    
    std::cout << "CXL simulator running. Press Ctrl+C to stop." << std::endl;
    
    // Keep running until terminated
    while(true) {
        sleep(1);
    }
    
    // Cleanup
    munmap(sim_memory, SIM_MEMORY_SIZE);
    return 0;
}