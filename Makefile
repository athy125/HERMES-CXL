# Makefile for CXL Shared Memory System
# Builds the library, kernel module, and test program

# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O3 -fPIC
LDFLAGS = -shared -pthread

# Kernel module flags
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build

# Targets
all: libcxl.so cxl_fpga.ko cxl_system

# Library
libcxl.so: cxl-lib.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

# Kernel module
cxl_fpga.ko: fpga-driver.c
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

# Main program
cxl_system: cxl-system.cpp libcxl.so
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lcxl -Wl,-rpath,.

# Test framework
test_framework: test-framework.py
	chmod +x test-framework.py

# Clean
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f libcxl.so cxl_system *.o

# Install
install: all
	cp libcxl.so /usr/local/lib/
	cp cxl_fpga.ko /lib/modules/$(shell uname -r)/kernel/drivers/cxl/
	depmod -a
	cp cxl_system /usr/local/bin/
	ldconfig

# Module configuration
obj-m := cxl_fpga.o

# Help
help:
	@echo "Available targets:"
	@echo "  all           - Build everything"
	@echo "  libcxl.so     - Build CXL shared memory library"
	@echo "  cxl_fpga.ko   - Build CXL FPGA kernel module"
	@echo "  cxl_system    - Build the main system program"
	@echo "  test_framework - Set up the testing framework"
	@echo "  clean         - Clean build files"
	@echo "  install       - Install libraries, modules, and executables"
	@echo "  help          - Display this help"

.PHONY: all clean install help test_framework