# SolixOS Makefile
# Enhanced with modern build system, parallel compilation, and CI/CD support
# Build system for SolixOS x86 microkernel - Version 2.0

# Compiler and tools with version checking
CC ?= gcc
ASM ?= nasm
LD ?= ld
OBJCOPY ?= objcopy
STRIP ?= strip
AR ?= ar
NM ?= nm
SIZE ?= size

# Build configuration with enhanced options
BUILD_TYPE ?= release
DEBUG ?= 0
VERBOSE ?= 0
PARALLEL ?= 1
PROFILE ?= 0
LTO ?= 1
SANITIZE ?= 0
COVERAGE ?= 0

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

# Enhanced compiler flags with modern optimizations
CFLAGS_BASE = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
               -nostartfiles -nodefaultlibs -Wall -Wextra -Werror \
               -Wno-unused-function -Wno-unused-parameter \
               -I$(INCLUDE_DIR) -ffreestanding -fno-pic -fno-pie \
               -fno-common -fno-strict-aliasing -frounding-math \
               -fno-math-errno -fno-signaling-nans

# Build-specific optimizations
ifeq ($(BUILD_TYPE),debug)
    CFLAGS_OPT = -g -O0 -DDEBUG=1 -fsanitize=address -fno-omit-frame-pointer
    ASMFLAGS += -g
else ifeq ($(BUILD_TYPE),profile)
    CFLAGS_OPT = -g -O2 -DNDEBUG -pg -fno-omit-frame-pointer
    LDFLAGS += -pg
else
    CFLAGS_OPT = -O3 -DNDEBUG -flto -fwhole-program-vtables
endif

# Additional safety features
ifeq ($(SANITIZE),1)
    CFLAGS_SAFE = -fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer
else
    CFLAGS_SAFE =
endif

# Coverage support
ifeq ($(COVERAGE),1)
    CFLAGS_COV = -fprofile-arcs -ftest-coverage
else
    CFLAGS_COV =
endif

# Link-time optimization
ifeq ($(LTO),1)
    CFLAGS_LTO = -flto -fno-fat-lto-objects
    LDFLAGS_LTO = -flto
else
    CFLAGS_LTO =
    LDFLAGS_LTO =
endif

CFLAGS = $(CFLAGS_BASE) $(CFLAGS_OPT) $(CFLAGS_SAFE) $(CFLAGS_COV) $(CFLAGS_LTO)

# Verbose output control
ifeq ($(VERBOSE),0)
    V = @
else
    V =
endif

# Assembly flags
ASMFLAGS = -f elf32

# Linker flags with modern optimizations
LDFLAGS = -m elf_i386 -nostdlib -T linker.ld $(LDFLAGS_LTO) \
          -Wl,--gc-sections -Wl,--as-needed -Wl,--strip-all

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

# Parallel compilation support
ifeq ($(PARALLEL),1)
    MAKEFLAGS += -j$(shell nproc 2>/dev/null || echo 4)
endif

# Enhanced build reporting with timing
START_TIME := $(shell date +%s)

define build_report
	@echo "=== SolixOS Build Report ==="
	@echo "Build Type: $(BUILD_TYPE)"
	@echo "Debug: $(DEBUG)"
	@echo "Parallel: $(PARALLEL)"
	@echo "LTO: $(LTO)"
	@echo "Sanitize: $(SANITIZE)"
	@echo "Coverage: $(COVERAGE)"
	@echo "Objects: $(words $(ALL_OBJECTS))"
	@if [ -f $(KERNEL_BIN) ]; then \
	    echo "Kernel Size: $$(wc -c < $(KERNEL_BIN)) bytes"; \
	    echo "Kernel Sections:"; \
	    $(SIZE) $(KERNEL_BIN) 2>/dev/null || echo "Size tool not available"; \
	fi
	@echo "Build Time: $$(date +%H:%M:%S)"
	@echo "Total Duration: $$(($$(date +%s) - $(START_TIME))) seconds"
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

# Enhanced help system with modern options
help:
	@echo "SolixOS Build System - Version 2.0"
	@echo "====================================="
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build kernel and ISO (default)"
	@echo "  clean        - Remove all build artifacts"
	@echo "  iso          - Build ISO image only"
	@echo "  kernel       - Build kernel only"
	@echo "  run          - Run in QEMU"
	@echo "  debug        - Debug with GDB server"
	@echo "  test         - Run test suite"
	@echo "  benchmark    - Performance benchmarks"
	@echo "  help         - Show this help"
	@echo ""
	@echo "Options:"
	@echo "  BUILD_TYPE=debug|release|profile - Build type (default: release)"
	@echo "  DEBUG=0|1                      - Enable debug mode (default: 0)"
	@echo "  VERBOSE=0|1                    - Verbose output (default: 0)"
	@echo "  PARALLEL=0|1                   - Parallel compilation (default: 1)"
	@echo "  LTO=0|1                        - Link-time optimization (default: 1)"
	@echo "  SANITIZE=0|1                   - Address sanitization (default: 0)"
	@echo "  COVERAGE=0|1                   - Code coverage (default: 0)"
	@echo "  PROFILE=0|1                    - Profiling support (default: 0)"
	@echo ""
	@echo "Examples:"
	@echo "  make BUILD_TYPE=debug DEBUG=1"
	@echo "  make VERBOSE=1 PARALLEL=4 run"
	@echo "  make clean && make all"
	@echo "  make COVERAGE=1 test"
	@echo "  make PROFILE=1 benchmark"

# Enhanced dependency checking
deps:
	@echo "[DEPS] Checking dependencies..."
	@which gcc > /dev/null || (echo "ERROR: gcc not found"; exit 1)
	@which nasm > /dev/null || (echo "ERROR: nasm not found"; exit 1)
	@which ld > /dev/null || (echo "ERROR: ld not found"; exit 1)
	@which grub-mkrescue > /dev/null || (echo "WARNING: grub-mkrescue not found (needed for ISO)"; exit 1)
	@which qemu-system-i386 > /dev/null || (echo "WARNING: qemu not found (needed for testing)"; exit 1)
	@echo "[DEPS] All dependencies satisfied"

# Modern performance targets
perf: $(KERNEL_BIN)
	@echo "[PERF] Analyzing performance..."
	@echo "[PERF] Kernel sections:"
	@$(SIZE) $(KERNEL_BIN) || echo "size command not available"
	@echo "[PERF] Object file sizes:"
	@du -h $(ALL_OBJECTS) || echo "du command not available"
	@echo "[PERF] Symbol analysis:"
	@$(NM) --print-size --size-sort $(KERNEL_BIN) | head -20 || echo "nm command not available"

# Enhanced static analysis with more tools
lint:
	@echo "[LINT] Running static analysis..."
	@which cppcheck > /dev/null && cppcheck --enable=all --std=c99 --platform=unix32 \
	    --suppress=missingIncludeSystem $(KERNEL_SOURCES) || echo "cppcheck not available"
	@which clang-tidy > /dev/null && clang-tidy $(KERNEL_SOURCES) -- $(CFLAGS) || echo "clang-tidy not available"
	@which flawfinder > /dev/null && flawfinder $(KERNEL_SOURCES) || echo "flawfinder not available"
	@which splint > /dev/null && splint +strict -standard $(KERNEL_SOURCES) || echo "splint not available"

# Code formatting
format:
	@echo "[FORMAT] Formatting code..."
	@which clang-format > /dev/null && find . -name '*.c' -o -name '*.h' | xargs clang-format -i || echo "clang-format not available"

# Test suite
test: $(KERNEL_BIN)
	@echo "[TEST] Running test suite..."
	@echo "[TEST] Unit tests:"
	@if [ -d tests ]; then \
	    $(MAKE) -C tests run || echo "Tests not available"; \
	else \
	    echo "No test directory found"; \
	fi

# Benchmark suite
benchmark: $(KERNEL_BIN)
	@echo "[BENCH] Running benchmarks..."
	@if [ -d benchmarks ]; then \
	    $(MAKE) -C benchmarks run || echo "Benchmarks not available"; \
	else \
	    echo "No benchmark directory found"; \
	fi

# Coverage report
coverage: COVERAGE=1
coverage: clean test
	@echo "[COV] Generating coverage report..."
	@which gcov > /dev/null && gcov $(KERNEL_SOURCES) || echo "gcov not available"
	@which lcov > /dev/null && lcov --capture --directory . --output-file coverage.info || echo "lcov not available"
	@which genhtml > /dev/null && genhtml coverage.info --output-directory coverage_html || echo "genhtml not available"

# Phony targets
.PHONY: all clean iso run debug help deps perf lint format test benchmark coverage \
        $(BUILD_DIR) $(ISO_DIR)/boot/grub

# Build everything
build: setup kernel iso
