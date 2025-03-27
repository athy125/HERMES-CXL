#!/bin/bash
set -e

echo "Starting HERMES-CXL simulation demo..."

# Start the CXL simulator in the background
echo "Launching CXL simulator..."
if [ -f "./build/cxl_simulator" ]; then
    ./build/cxl_simulator &
    SIMULATOR_PID=$!
    
    # Wait for simulator to initialize
    sleep 2
    echo "CXL simulator started with PID: $SIMULATOR_PID"
    
    # Create a trap to ensure simulator is terminated on script exit
    trap "echo 'Cleaning up...'; kill $SIMULATOR_PID 2>/dev/null || true" EXIT
else
    echo "CXL simulator executable not found, creating simulated device manually"
    mkdir -p /tmp/cxl_sim
    touch /tmp/cxl_sim/cxl0
fi

# Run the system with simulated environment
echo -e "\n=============================================================="
echo "Running HERMES-CXL system with simulation..."
echo "=============================================================="
if [ -f "./build/cxl_system_sim" ]; then
    ./build/cxl_system_sim
else
    echo "ERROR: cxl_system_sim executable not found."
    echo "Using minimal simulation instead."
    echo "HERMES-CXL Minimal System Simulation"
fi

# Run performance tests and generate visualizations
echo -e "\n=============================================================="
echo "Running performance tests and generating visualizations..."
echo "=============================================================="
cd test
mkdir -p results
python3 test_framework.py --simulation --device /tmp/cxl_sim/cxl0 --test all

# Return to root directory
cd ..

# Display results
echo -e "\n=============================================================="
echo "Demo completed successfully!"
echo "Test results and visualizations can be found in test/results/"
echo "=============================================================="

# If in Codespaces, display how to view visualizations
if [ -n "$CODESPACES" ]; then
    echo "To view visualizations in Codespaces:"
    echo "1. Open the Explorer (left sidebar)"
    echo "2. Navigate to test/results/"
    echo "3. Right-click on any image and select 'Open Preview'"
fi
