# SolixOS Build System
# Lightweight Command-Line Operating System

# Compiler and tools
CC = gcc
AS = nasm
LD = ld
OBJCOPY = objcopy
MKISOFS = mkisofs
QEMU = qemu-system-i386

# Directories
BUILD_DIR = build
ISO_DIR = iso
KERNEL_DIR = kernel
DRIVER_DIR = drivers
FS_DIR = fs
SHELL_DIR = shell
NET_DIR = net
APPS_DIR = apps
BROWSER_DIR = apps/browser
PKG_DIR = apps/pkg
NET_APPS_DIR = apps/net

# Compiler flags
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs \
         -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter \
         -c -Iinclude -I$(KERNEL_DIR) -I$(DRIVER_DIR) -I$(FS_DIR) -I$(SHELL_DIR) -I$(NET_DIR) \
         -I$(BROWSER_DIR) -I$(PKG_DIR) -I$(NET_APPS_DIR)

ASFLAGS = -f elf32
LDFLAGS = -melf_i386 -Tlinker.ld

# Source files
KERNEL_SOURCES = $(KERNEL_DIR)/kernel.c $(KERNEL_DIR)/mm.c $(KERNEL_DIR)/interrupts.c
DRIVER_SOURCES = $(DRIVER_DIR)/screen.c $(DRIVER_DIR)/keyboard.c $(DRIVER_DIR)/timer.c $(DRIVER_DIR)/ethernet.c $(DRIVER_DIR)/wifi.c
FS_SOURCES = $(FS_DIR)/solixfs.c $(FS_DIR)/vfs.c
SHELL_SOURCES = $(SHELL_DIR)/shell.c
NET_SOURCES = $(NET_DIR)/net.c
BROWSER_SOURCES = $(BROWSER_DIR)/lynx.c
PKG_SOURCES = $(PKG_DIR)/pkg.c
NET_APPS_SOURCES = $(NET_APPS_DIR)/ping.c $(NET_APPS_DIR)/ifconfig.c $(NET_APPS_DIR)/curl.c
ASM_SOURCES = $(KERNEL_DIR)/isr.asm

# Object files
KERNEL_OBJS = $(KERNEL_SOURCES:.c=.o)
DRIVER_OBJS = $(DRIVER_SOURCES:.c=.o)
FS_OBJS = $(FS_SOURCES:.c=.o)
SHELL_OBJS = $(SHELL_SOURCES:.c=.o)
NET_OBJS = $(NET_SOURCES:.c=.o)
BROWSER_OBJS = $(BROWSER_SOURCES:.c=.o)
PKG_OBJS = $(PKG_SOURCES:.c=.o)
NET_APPS_OBJS = $(NET_APPS_SOURCES:.c=.o)
ASM_OBJS = $(ASM_SOURCES:.asm=.o)

ALL_OBJS = $(KERNEL_OBJS) $(DRIVER_OBJS) $(FS_OBJS) $(SHELL_OBJS) $(NET_OBJS) $(BROWSER_OBJS) $(PKG_OBJS) $(NET_APPS_OBJS) $(ASM_OBJS)

# Targets
.PHONY: all clean iso run qemu

all: kernel.iso

# Bootloader
bootloader:
	$(AS) -f bin -o $(BUILD_DIR)/boot.bin bootloader/boot.asm

# Kernel
kernel: bootloader
	$(MAKE) -C $(KERNEL_DIR) CC=$(CC) CFLAGS="$(CFLAGS)"
	$(MAKE) -C $(DRIVER_DIR) CC=$(CC) CFLAGS="$(CFLAGS)"
	$(MAKE) -C $(FS_DIR) CC=$(CC) CFLAGS="$(CFLAGS)"
	$(MAKE) -C $(SHELL_DIR) CC=$(CC) CFLAGS="$(CFLAGS)"
	$(MAKE) -C $(NET_DIR) CC=$(CC) CFLAGS="$(CFLAGS)"
	$(MAKE) -C $(BROWSER_DIR) CC=$(CC) CFLAGS="$(CFLAGS)"
	$(MAKE) -C $(PKG_DIR) CC=$(CC) CFLAGS="$(CFLAGS)"
	$(MAKE) -C $(NET_APPS_DIR) CC=$(CC) CFLAGS="$(CFLAGS)"
	$(AS) $(ASFLAGS) -o $(BUILD_DIR)/isr.o $(KERNEL_DIR)/isr.asm
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/kernel.bin $(ALL_OBJS) $(BUILD_DIR)/isr.o

# ISO
iso: kernel
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(BUILD_DIR)/boot.bin $(ISO_DIR)/boot/
	cp $(BUILD_DIR)/kernel.bin $(ISO_DIR)/boot/
	cp grub.cfg $(ISO_DIR)/boot/grub/
	$(MKISOFS) -R -b boot/boot.bin -no-emul-boot -boot-load-size 4 -o kernel.iso $(ISO_DIR)

# Run with QEMU
run: iso
	$(QEMU) -cdrom kernel.iso -m 512M

# Debug with QEMU and GDB
debug: iso
	$(QEMU) -cdrom kernel.iso -m 512M -s -S

# Clean
clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR) *.o *.bin *.iso

# Create directories
setup:
	mkdir -p $(BUILD_DIR) $(ISO_DIR)/boot/grub

# Build everything
build: setup kernel iso
