#!/bin/bash
set -e

echo "Starting HERMES-CXL simulation demo..."

# Start the CXL simulator in the background
echo "Launching CXL simulator..."
./build/cxl_simulator &
SIMULATOR_PID=$!

# Wait for simulator to initialize
sleep 2
echo "CXL simulator started with PID: $SIMULATOR_PID"

# Create a trap to ensure simulator is terminated on script exit
trap "echo 'Cleaning up...'; kill $SIMULATOR_PID 2>/dev/null || true" EXIT

# Run the system with simulated environment
echo -e "\n=============================================================="
echo "Running HERMES-CXL system with simulation..."
echo "=============================================================="
./build/cxl_system_sim

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
    echo -e "\nOr run this command to see all visualizations:"
    echo "code-server --open-browser test/results/"
fi