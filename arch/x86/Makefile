#
# SolixOS x86 Architecture Makefile
# Linux-inspired architecture-specific build configuration
#

# Architecture
ARCH := x86
SRCARCH := x86

# Processor type
PROC := i386

# Architecture-specific flags
KBUILD_CFLAGS += -m32 -march=i386 -mregparm=3 -freg-struct-return
KBUILD_AFLAGS += -m32
KBUILD_LDFLAGS := -m elf_i386

# Head objects
head-y := arch/x86/kernel/head.o arch/x86/kernel/head32.o

# Core objects
core-y += arch/x86/kernel/ arch/x86/mm/ arch/x86/crypto/

# Driver objects
drivers-y += arch/x86/drivers/

# Library objects
libs-y += arch/x86/lib/

# Boot objects
boot-y := arch/x86/boot/

# Linker script
KBUILD_LDS_MODULE := arch/x86/kernel/vmlinux.lds

# Architecture-specific defines
KBUILD_CPPFLAGS += -D__KERNEL__ -DCONFIG_X86_32 -DCONFIG_X86
KBUILD_CPPFLAGS += -DCONFIG_GENERIC_HARDIRQS -DCONFIG_GENERIC_TIME
KBUILD_CPPFLAGS += -DCONFIG_GENERIC_CMOS_UPDATE -DCONFIG_IRQ_WORK

# Processor-specific flags
ifdef CONFIG_M586
    KBUILD_CFLAGS += -march=i586
endif
ifdef CONFIG_M586TSC
    KBUILD_CFLAGS += -march=i586
endif
ifdef CONFIG_M586MMX
    KBUILD_CFLAGS += -march=i586 -mmmx
endif
ifdef CONFIG_M686
    KBUILD_CFLAGS += -march=i686
endif
ifdef CONFIG_MPENTIUMII
    KBUILD_CFLAGS += -march=i686 -mmmx
endif
ifdef CONFIG_MPENTIUMIII
    KBUILD_CFLAGS += -march=i686 -mmmx -msse
endif
ifdef CONFIG_MPENTIUMM
    KBUILD_CFLAGS += -march=i686 -mmmx -msse
endif
ifdef CONFIG_MPENTIUM4
    KBUILD_CFLAGS += -march=i686 -mmmx -msse -msse2
endif
ifdef CONFIG_MK6
    KBUILD_CFLAGS += -march=k6
endif
ifdef CONFIG_MK7
    KBUILD_CFLAGS += -march=athlon -mmmx -msse -msse2
endif
ifdef CONFIG_MCRUSOE
    KBUILD_CFLAGS += -march=i686
endif
ifdef CONFIG_MWINCHIPC6
    KBUILD_CFLAGS += -march=winchip-c6 -mmmx
endif
ifdef CONFIG_MWINCHIP2
    KBUILD_CFLAGS += -march=winchip2 -mmmx
endif
ifdef CONFIG_MWINCHIP3D
    KBUILD_CFLAGS += -march=winchip2 -mmmx -msse
endif
ifdef CONFIG_MCYRIXIII
    KBUILD_CFLAGS += -march=c3 -mmmx -msse
endif
ifdef CONFIG_MVIAC3_2
    KBUILD_CFLAGS += -march=c3 -mmmx -msse
endif
ifdef CONFIG_MVIAC7
    KBUILD_CFLAGS += -march=c3 -mmmx -msse
endif

# Default to i686 if no processor specified
ifndef CONFIG_M586
ifndef CONFIG_M586TSC
ifndef CONFIG_M586MMX
ifndef CONFIG_M686
ifndef CONFIG_MPENTIUMII
ifndef CONFIG_MPENTIUMIII
ifndef CONFIG_MPENTIUMM
ifndef CONFIG_MPENTIUM4
ifndef CONFIG_MK6
ifndef CONFIG_MK7
ifndef CONFIG_MCRUSOE
ifndef CONFIG_MWINCHIPC6
ifndef CONFIG_MWINCHIP2
ifndef CONFIG_MWINCHIP3D
ifndef CONFIG_MCYRIXIII
ifndef CONFIG_MVIAC3_2
ifndef CONFIG_MVIAC7
    KBUILD_CFLAGS += -march=i686
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif

# Architecture-specific compiler flags
KBUILD_CFLAGS += -fno-pic -fno-pie -fno-common
KBUILD_CFLAGS += -ffreestanding -fno-stack-protector
KBUILD_CFLAGS += -fomit-frame-pointer
KBUILD_CFLAGS += -fno-asynchronous-unwind-tables
KBUILD_CFLAGS += -fno-unwind-tables
KBUILD_CFLAGS += -fno-delete-null-pointer-checks
KBUILD_CFLAGS += -fno-strict-aliasing
KBUILD_CFLAGS += -fno-common
KBUILD_CFLAGS += -fno-PIE

# Architecture-specific assembler flags
KBUILD_AFLAGS += -m32 -D__ASSEMBLY__
KBUILD_AFLAGS += -DCONFIG_X86_32
KBUILD_AFLAGS += -DCONFIG_AS_CFI=1
KBUILD_AFLAGS += -DCONFIG_AS_CFI_SIGNAL_FRAME=1
KBUILD_AFLAGS += -DCONFIG_AS_CFI_SECTIONS=1

# Architecture-specific linker flags
KBUILD_LDFLAGS += -m elf_i386
KBUILD_LDFLAGS += -T $(KBUILD_LDS_MODULE)
KBUILD_LDFLAGS += --build-id
KBUILD_LDFLAGS += --hash-style=gnu

# Architecture-specific defines
KBUILD_CPPFLAGS += -D__KERNEL__
KBUILD_CPPFLAGS += -DCONFIG_X86_32
KBUILD_CPPFLAGS += -DCONFIG_X86
KBUILD_CPPFLAGS += -DCONFIG_GENERIC_HARDIRQS
KBUILD_CPPFLAGS += -DCONFIG_GENERIC_TIME
KBUILD_CPPFLAGS += -DCONFIG_GENERIC_CMOS_UPDATE
KBUILD_CPPFLAGS += -DCONFIG_IRQ_WORK

# Include directories
KBUILD_CPPFLAGS += -I$(srctree)/arch/x86/include
KBUILD_CPPFLAGS += -I$(srctree)/arch/x86/include/generated
KBUILD_CPPFLAGS += -I$(objtree)/arch/x86/include/generated

# Architecture-specific targets
PHONY += zImage bzImage vmlinux

# Build boot image
zImage: $(boot-y)/zImage
	@echo "Building zImage..."
	@cp $(boot-y)/zImage $(OUTPUT_DIR)/zImage

# Build compressed image
bzImage: $(boot-y)/bzImage
	@echo "Building bzImage..."
	@cp $(boot-y)/bzImage $(OUTPUT_DIR)/bzImage

# Build vmlinux
vmlinux: $(KERNEL_ELF)
	@echo "Building vmlinux..."

# Boot targets
boot_targets := zImage bzImage
archprepare:
	@echo "Preparing architecture..."

# Architecture-specific clean
archclean:
	@echo "Cleaning architecture..."
	$(Q)$(MAKE) -f $(srctree)/arch/x86/boot/Makefile clean
	$(Q)$(MAKE) -f $(srctree)/arch/x86/kernel/Makefile clean

# Architecture-specific install
archinstall:
	@echo "Installing architecture..."

# Architecture-specific help
archhelp:
	@echo "  zImage     - Build compressed kernel image (zImage)"
	@echo "  bzImage    - Build compressed kernel image (bzImage)"
	@echo "  vmlinux    - Build uncompressed kernel image"

# Include boot Makefile
include $(srctree)/arch/x86/boot/Makefile

# Include kernel Makefile
include $(srctree)/arch/x86/kernel/Makefile

# PHONY targets
.PHONY: $(PHONY) archprepare archclean archinstall archhelp
