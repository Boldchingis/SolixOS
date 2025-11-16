# SolixOS Makefile
# Enhanced with better error reporting and diagnostics
# Build system for SolixOS x86 microkernel

# Compiler and tools
CC = gcc
ASM = nasm
LD = ld
OBJCOPY = objcopy
STRIP = strip

# Build configuration
BUILD_TYPE ?= release
DEBUG ?= 0
VERBOSE ?= 0

# Directories
SRC_DIR = src
KERNEL_DIR = kernel
DRIVERS_DIR = drivers
FS_DIR = fs
SHELL_DIR = shell
NET_DIR = net
APPS_DIR = apps
INCLUDE_DIR = include
BUILD_DIR = build
ISO_DIR = iso

# Output files
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
ISO_FILE = $(BUILD_DIR)/solixos.iso

# Compiler flags with enhanced diagnostics
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -nostartfiles -nodefaultlibs -Wall -Wextra -Werror \
         -Wno-unused-function -Wno-unused-parameter \
         -I$(INCLUDE_DIR) -ffreestanding -fno-pic -fno-pie

# Build-specific flags
ifeq ($(BUILD_TYPE),debug)
    CFLAGS += -g -O0 -DDEBUG=1 -fsanitize=address
    ASMFLAGS += -g
else
    CFLAGS += -O2 -DNDEBUG
endif

# Verbose output control
ifeq ($(VERBOSE),0)
    V = @
else
    V =
endif

# Assembly flags
ASMFLAGS = -f elf32

# Linker flags
LDFLAGS = -m elf_i386 -nostdlib -T linker.ld

# Source files
KERNEL_SOURCES = $(wildcard $(KERNEL_DIR)/*.c) $(wildcard $(KERNEL_DIR)/*.S)
DRIVER_SOURCES = $(wildcard $(DRIVERS_DIR)/*.c) $(wildcard $(DRIVERS_DIR)/*.S)
FS_SOURCES = $(wildcard $(FS_DIR)/*.c)
SHELL_SOURCES = $(wildcard $(SHELL_DIR)/*.c)
NET_SOURCES = $(wildcard $(NET_DIR)/*.c)
APP_SOURCES = $(wildcard $(APPS_DIR)/*.c)

# Object files
KERNEL_OBJECTS = $(KERNEL_SOURCES:%.c=$(BUILD_DIR)/%.o) $(KERNEL_SOURCES:%.S=$(BUILD_DIR)/%.o)
DRIVER_OBJECTS = $(DRIVER_SOURCES:%.c=$(BUILD_DIR)/%.o) $(DRIVER_SOURCES:%.S=$(BUILD_DIR)/%.o)
FS_OBJECTS = $(FS_SOURCES:%.c=$(BUILD_DIR)/%.o)
SHELL_OBJECTS = $(SHELL_SOURCES:%.c=$(BUILD_DIR)/%.o)
NET_OBJECTS = $(NET_SOURCES:%.c=$(BUILD_DIR)/%.o)
APP_OBJECTS = $(APP_SOURCES:%.c=$(BUILD_DIR)/%.o)

ALL_OBJECTS = $(KERNEL_OBJECTS) $(DRIVER_OBJECTS) $(FS_OBJECTS) \
              $(SHELL_OBJECTS) $(NET_OBJECTS) $(APP_OBJECTS)

# Default target
.PHONY: all clean iso run debug help
all: $(KERNEL_BIN) $(ISO_FILE)

# Create build directories
$(BUILD_DIR):
	$(V)mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)
	$(V)mkdir -p $(BUILD_DIR)/$(DRIVERS_DIR)
	$(V)mkdir -p $(BUILD_DIR)/$(FS_DIR)
	$(V)mkdir -p $(BUILD_DIR)/$(SHELL_DIR)
	$(V)mkdir -p $(BUILD_DIR)/$(NET_DIR)
	$(V)mkdir -p $(BUILD_DIR)/$(APPS_DIR)
	$(V)mkdir -p $(ISO_DIR)/boot/grub

# Enhanced build reporting
define build_report
	@echo "=== Build Report ==="
	@echo "Build Type: $(BUILD_TYPE)"
	@echo "Debug: $(DEBUG)"
	@echo "Objects: $(words $(ALL_OBJECTS))"
	@echo "Kernel Size: $$(wc -c < $(KERNEL_BIN)) bytes"
	@echo "Build Time: $$(date)"
endef

# Compile C source with enhanced error handling
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "[CC] $<"
	$(V)$(CC) $(CFLAGS) -c $< -o $@ \
		|| (echo "\n!!! COMPILATION FAILED !!!"; \
		    echo "File: $<"; \
		    echo "Command: $(CC) $(CFLAGS) -c $< -o $@"; \
		    exit 1)

# Compile assembly source with error handling
$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	@echo "[ASM] $<"
	$(V)$(ASM) $(ASMFLAGS) $< -o $@ \
		|| (echo "\n!!! ASSEMBLY FAILED !!!"; \
		    echo "File: $<"; \
		    echo "Command: $(ASM) $(ASMFLAGS) $< -o $@"; \
		    exit 1)

# Link kernel with enhanced error reporting
$(KERNEL_ELF): $(ALL_OBJECTS) linker.ld
	@echo "[LD] Linking kernel..."
	$(V)$(LD) $(LDFLAGS) -o $@ $(ALL_OBJECTS) \
		|| (echo "\n!!! LINKING FAILED !!!"; \
		    echo "Objects: $(ALL_OBJECTS)"; \
		    echo "Command: $(LD) $(LDFLAGS) -o $@ $(ALL_OBJECTS)"; \
		    exit 1)
	@echo "[LD] Kernel linked successfully"

# Create binary with validation
$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "[OBJCOPY] Creating binary..."
	$(V)$(OBJCOPY) -O binary $< $@ \
		|| (echo "\n!!! BINARY CREATION FAILED !!!"; \
		    exit 1)
	@echo "[OBJCOPY] Binary created: $@"
	@echo "[SIZE] Kernel size: $$(wc -c < $@) bytes"

# Create ISO image with validation
$(ISO_FILE): $(KERNEL_BIN) grub.cfg
	@echo "[ISO] Creating ISO image..."
	$(V)cp $(KERNEL_BIN) $(ISO_DIR)/boot/kernel.bin
	$(V)cp grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	$(V)grub-mkrescue -o $@ $(ISO_DIR) \
		|| (echo "\n!!! ISO CREATION FAILED !!!"; \
		    echo "Make sure grub-mkrescue is installed"; \
		    exit 1)
	@echo "[ISO] ISO image created: $@"
	$(call build_report)

# Enhanced clean with reporting
clean:
	@echo "[CLEAN] Removing build artifacts..."
	$(V)rm -rf $(BUILD_DIR)
	$(V)rm -f *.o *.elf *.bin *.iso
	@echo "[CLEAN] Clean complete"

# Run in QEMU with enhanced options
run: $(ISO_FILE)
	@echo "[QEMU] Starting SolixOS..."
	qemu-system-i386 -cdrom $(ISO_FILE) -m 128M -serial stdio \
		-d cpu_reset,int,exec,guest_errors \
		-no-reboot -no-shutdown || true

# Debug with GDB
debug: $(KERNEL_ELF)
	@echo "[DEBUG] Starting debug session..."
	qemu-system-i386 -s -S -cdrom $(ISO_FILE) -m 128M -serial stdio \
		-d cpu_reset,int,exec,guest_errors &
	@echo "[DEBUG] GDB server started on localhost:1234"
	@echo "[DEBUG] Run: gdb $(KERNEL_ELF) -ex 'target remote localhost:1234'"
	@echo "[DEBUG] Press Ctrl+C to stop QEMU"
	wait

# Enhanced help system
help:
	@echo "SolixOS Build System - Enhanced Version"
	@echo "====================================="
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build kernel and ISO (default)"
	@echo "  clean        - Remove all build artifacts"
	@echo "  iso          - Build ISO image only"
	@echo "  run          - Run in QEMU"
	@echo "  debug        - Debug with GDB server"
	@echo "  help         - Show this help"
	@echo ""
	@echo "Options:"
	@echo "  BUILD_TYPE=debug|release - Build type (default: release)"
	@echo "  DEBUG=0|1              - Enable debug mode (default: 0)"
	@echo "  VERBOSE=0|1            - Verbose output (default: 0)"
	@echo ""
	@echo "Examples:"
	@echo "  make BUILD_TYPE=debug DEBUG=1"
	@echo "  make VERBOSE=1 run"
	@echo "  make clean && make all"

# Enhanced dependency checking
deps:
	@echo "[DEPS] Checking dependencies..."
	@which gcc > /dev/null || (echo "ERROR: gcc not found"; exit 1)
	@which nasm > /dev/null || (echo "ERROR: nasm not found"; exit 1)
	@which ld > /dev/null || (echo "ERROR: ld not found"; exit 1)
	@which grub-mkrescue > /dev/null || (echo "WARNING: grub-mkrescue not found (needed for ISO)"; exit 1)
	@which qemu-system-i386 > /dev/null || (echo "WARNING: qemu not found (needed for testing)"; exit 1)
	@echo "[DEPS] All dependencies satisfied"

# Performance targets
perf: $(KERNEL_BIN)
	@echo "[PERF] Analyzing performance..."
	@echo "[PERF] Kernel sections:"
	@size $(KERNEL_BIN) || echo "size command not available"
	@echo "[PERF] Object file sizes:"
	@du -h $(ALL_OBJECTS) || echo "du command not available"

# Static analysis (if available)
lint:
	@echo "[LINT] Running static analysis..."
	@which cppcheck > /dev/null && cppcheck --enable=all --std=c99 $(KERNEL_SOURCES) || echo "cppcheck not available"
	@which clang-tidy > /dev/null && clang-tidy $(KERNEL_SOURCES) -- $(CFLAGS) || echo "clang-tidy not available"

# Phony targets
.PHONY: deps perf lint $(BUILD_DIR) $(ISO_DIR)/boot/grub

# Build everything
build: setup kernel iso
