#!/bin/bash
set -e

echo "Setting up HERMES-CXL simulation environment in Codespaces..."

# Create necessary directories
mkdir -p build
mkdir -p /tmp/cxl_sim

# Install dependencies
sudo apt-get update
sudo apt-get install -y build-essential python3-pip
pip3 install numpy matplotlib

# Build the simulation components
echo "Building simulation components..."
g++ -std=c++17 -Wall -o build/cxl_simulator src/simulation/cxl_simulator.cpp

# Build modified libcxl with simulation support
g++ -std=c++17 -Wall -fPIC -shared -o build/libcxl_sim.so src/lib/libcxl.cpp -I./include -DSIMULATION_MODE

# Build system application with simulation support
g++ -std=c++17 -Wall -o build/cxl_system_sim src/app/cxl_system.cpp -I./include -L./build -lcxl_sim -DSIMULATION_MODE -Wl,-rpath,./build

# Setup Python test environment
pip3 install pytest
mkdir -p test/results

# Echo completion message
echo "=============================================================="
echo "HERMES-CXL simulation environment setup complete!"
echo "Run the demo with: ./scripts/run_demo.sh"
echo "=============================================================="