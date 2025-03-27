#include <iostream>
#include "cxl_api.h"

int main() {
    std::cout << "HERMES-CXL System Simulation" << std::endl;
    
    #ifdef SIMULATION_MODE
    std::cout << "Running in simulation mode" << std::endl;
    
    // Open simulated device
    void* handle = cxl_init("/tmp/cxl_sim/cxl0", 1UL << 30);
    if (!handle) {
        std::cerr << "Failed to initialize CXL memory" << std::endl;
        return 1;
    }
    
    std::cout << "CXL memory initialized successfully" << std::endl;
    std::cout << "Running basic system tests..." << std::endl;
    
    // Cleanup
    cxl_cleanup(handle);
    #else
    std::cout << "Real hardware mode not available in this build" << std::endl;
    #endif
    
    return 0;
}