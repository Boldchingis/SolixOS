#!/bin/bash

# SolixOS Build Script
# Automated build system for SolixOS

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check prerequisites
check_prereqs() {
    print_status "Checking prerequisites..."
    
    # Check for required tools
    local tools=("gcc" "nasm" "ld" "make" "qemu-system-i386" "mkisofs")
    local missing=()
    
    for tool in "${tools[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            missing+=("$tool")
        fi
    done
    
    if [ ${#missing[@]} -ne 0 ]; then
        print_error "Missing required tools: ${missing[*]}"
        print_status "Please install the missing tools and try again."
        exit 1
    fi
    
    print_success "All prerequisites found"
}

# Clean previous builds
clean_build() {
    print_status "Cleaning previous builds..."
    make clean
    print_success "Build directory cleaned"
}

# Setup build environment
setup_build() {
    print_status "Setting up build environment..."
    make setup
    print_success "Build environment ready"
}

# Build bootloader
build_bootloader() {
    print_status "Building bootloader..."
    make bootloader
    print_success "Bootloader built successfully"
}

# Build kernel
build_kernel() {
    print_status "Building kernel components..."
    
    # Build each component separately for better error reporting
    print_status "Building kernel core..."
    make -C kernel CC=gcc CFLAGS="-m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -c -Iinclude -Ikernel -Idrivers -Ifs -Ishell -Inet -Iapps/browser -Iapps/pkg -Iapps/net"
    
    print_status "Building drivers..."
    make -C drivers CC=gcc CFLAGS="-m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -c -Iinclude -Ikernel -Idrivers -Ifs -Ishell -Inet -Iapps/browser -Iapps/pkg -Iapps/net"
    
    print_status "Building filesystem..."
    make -C fs CC=gcc CFLAGS="-m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -c -Iinclude -Ikernel -Idrivers -Ifs -Ishell -Inet -Iapps/browser -Iapps/pkg -Iapps/net"
    
    print_status "Building shell..."
    make -C shell CC=gcc CFLAGS="-m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -c -Iinclude -Ikernel -Idrivers -Ifs -Ishell -Inet -Iapps/browser -Iapps/pkg -Iapps/net"
    
    print_status "Building network stack..."
    make -C net CC=gcc CFLAGS="-m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -c -Iinclude -Ikernel -Idrivers -Ifs -Ishell -Inet -Iapps/browser -Iapps/pkg -Iapps/net"
    
    print_status "Building applications..."
    make -C apps/browser CC=gcc CFLAGS="-m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -c -Iinclude -Ikernel -Idrivers -Ifs -Ishell -Inet -Iapps/browser -Iapps/pkg -Iapps/net"
    make -C apps/pkg CC=gcc CFLAGS="-m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -c -Iinclude -Ikernel -Idrivers -Ifs -Ishell -Inet -Iapps/browser -Iapps/pkg -Iapps/net"
    make -C apps/net CC=gcc CFLAGS="-m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -c -Iinclude -Ikernel -Idrivers -Ifs -Ishell -Inet -Iapps/browser -Iapps/pkg -Iapps/net"
    
    print_status "Assembling interrupt handlers..."
    nasm -f elf32 -o build/isr.o kernel/isr.asm
    
    print_status "Linking kernel..."
    ld -melf_i386 -Tlinker.ld -o build/kernel.bin \
        kernel/*.o drivers/*.o fs/*.o shell/*.o net/*.o \
        apps/browser/*.o apps/pkg/*.o apps/net/*.o \
        build/isr.o
    
    print_success "Kernel built successfully"
}

# Create ISO image
create_iso() {
    print_status "Creating ISO image..."
    make iso
    print_success "ISO image created: kernel.iso"
}

# Run tests
run_tests() {
    print_status "Running basic tests..."
    
    # Check if kernel binary exists
    if [ ! -f "build/kernel.bin" ]; then
        print_error "Kernel binary not found"
        exit 1
    fi
    
    # Check if ISO exists
    if [ ! -f "kernel.iso" ]; then
        print_error "ISO image not found"
        exit 1
    fi
    
    # Check kernel size (should be reasonable)
    local kernel_size=$(stat -c%s "build/kernel.bin")
    if [ $kernel_size -gt 1048576 ]; then  # 1MB
        print_warning "Kernel size is large: ${kernel_size} bytes"
    fi
    
    print_success "Basic tests passed"
}

# Run in QEMU
run_qemu() {
    print_status "Starting QEMU..."
    if command -v qemu-system-i386 &> /dev/null; then
        make run
    else
        print_error "QEMU not found"
        exit 1
    fi
}

# Main build function
main_build() {
    print_status "Starting SolixOS build process..."
    
    check_prereqs
    clean_build
    setup_build
    build_bootloader
    build_kernel
    create_iso
    run_tests
    
    print_success "Build completed successfully!"
    print_status "Run './build.sh run' to start SolixOS in QEMU"
    print_status "Run './build.sh debug' to start with GDB support"
}

# Debug build
debug_build() {
    print_status "Starting debug build..."
    main_build
    print_status "Starting QEMU with GDB support..."
    make debug
}

# Clean build
clean_all() {
    print_status "Cleaning all build artifacts..."
    make clean
    print_success "All artifacts cleaned"
}

# Show help
show_help() {
    echo "SolixOS Build Script"
    echo "Usage: $0 [COMMAND]"
    echo ""
    echo "Commands:"
    echo "  build    - Build complete system (default)"
    echo "  clean    - Clean all build artifacts"
    echo "  run      - Build and run in QEMU"
    echo "  debug    - Build and run with GDB support"
    echo "  help     - Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0           # Build complete system"
    echo "  $0 run       # Build and run"
    echo "  $0 clean     # Clean build directory"
    echo "  $0 debug     # Debug build"
}

# Parse command line arguments
case "${1:-build}" in
    "build")
        main_build
        ;;
    "clean")
        clean_all
        ;;
    "run")
        main_build
        run_qemu
        ;;
    "debug")
        debug_build
        ;;
    "help"|"-h"|"--help")
        show_help
        ;;
    *)
        print_error "Unknown command: $1"
        show_help
        exit 1
        ;;
esac
