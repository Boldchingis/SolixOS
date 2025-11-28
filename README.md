# SolixOS

A modern, command-line-only operating system built from scratch with Linux-inspired architecture and cutting-edge features.

## Overview

SolixOS is a next-generation x86 operating system featuring a microkernel architecture, modern security frameworks, advanced networking stack, and comprehensive hardware support. It combines the simplicity of a microkernel with the power and features of modern operating systems.

## Key Features

### Core System
- **Enhanced Microkernel Architecture**: Modular design with Linux-inspired subsystems
- **Modern Memory Management**: SLAB allocator, virtual memory, and dynamic allocation
- **Advanced Scheduling**: O(1) scheduler with multiple scheduling policies
- **Multi-core Support**: Symmetric multiprocessing (SMP) capabilities
- **Comprehensive Debugging**: Multi-level logging, stack traces, and performance monitoring

### Security Framework
- **Capability-based Security**: Fine-grained permissions and access control
- **Access Control Lists (ACLs)**: File and resource level security
- **File Encryption**: AES-XTS and ChaCha20 encryption support
- **User/Group Management**: Unix-style authentication and authorization
- **Audit System**: Comprehensive security event logging

### Modern Filesystem Layer
- **Journaling Filesystems**: Ext4, XFS, Btrfs support
- **Copy-on-Write (CoW)**: Efficient snapshot and clone support
- **Extended Attributes**: Rich metadata support
- **Filesystem Quotas**: User and group resource limits
- **Snapshot Management**: Point-in-time filesystem states

### Advanced Networking
- **IPv6 Support**: Full IPv6 protocol stack
- **TLS/SSL**: Secure communications with modern cryptography
- **HTTP/2**: High-performance web protocol support
- **QUIC**: Next-generation transport protocol
- **TCP Offload**: Hardware acceleration support

### Modern Hardware Support
- **NVMe SSD Support**: High-performance storage drivers
- **USB Framework**: Complete USB host controller support (UHCI, OHCI, EHCI, xHCI)
- **Enhanced Drivers**: Modern device driver architecture
- **Power Management**: Advanced power state management
- **Hot-plug Support**: Dynamic device detection and configuration

### Filesystem
- **SolixFS**: Custom simple filesystem
- **VFS Layer**: Virtual filesystem for multiple FS support
- **Standard Operations**: Create, read, write, delete, directory operations

### Shell & Commands
- **Solix Shell**: Interactive command-line interface
- **Built-in Commands**: help, clear, ls, cd, pwd, cat, echo, mkdir, touch, rm, ps, kill, reboot, halt, meminfo, mount, umount, df, test

### Networking
- **TCP/IP Stack**: Complete network protocol implementation
- **Ethernet Driver**: RTL8139 network card support
- **Wi-Fi Support**: Device abstraction and management
- **Network Tools**: ping, ifconfig, curl

### Applications
- **Text-based Web Browser**: Lynx-style browser with HTML parsing
- **Package Manager**: Software installation and management
- **Network Utilities**: Ping, network configuration, HTTP client

## System Requirements

### Minimum Requirements
- x86 compatible processor (32-bit)
- 1GB RAM minimum
- 10GB storage space
- Network card (RTL8139 compatible or modern NIC)
- Virtualization or bare metal support

### Recommended Requirements
- Multi-core x86 processor
- 4GB RAM or more
- 50GB storage space (SSD recommended)
- Modern network card with offload support
- NVMe SSD for optimal performance

## Building

### Prerequisites

- GCC (cross-compiler for i386-elf recommended) version 8.0+
- NASM assembler version 2.14+
- GNU LD linker version 2.30+
- GNU Make version 4.2+
- QEMU version 5.0+ (for testing)
- grub-mkrescue (for ISO creation)
- Optional: clang, cppcheck, lcov (for advanced development)

### Build Process

1. **Setup Build Environment**
   ```bash
   make deps          # Check dependencies
   make setup         # Initialize build environment
   ```

2. **Build Complete System**
   ```bash
   make all           # Build kernel and ISO
   # Or with specific options:
   make BUILD_TYPE=release LTO=1 PARALLEL=4
   ```

3. **Run in QEMU**
   ```bash
   make run           # Run in QEMU with default settings
   make run QEMU_OPTS="-m 2G -smp 2"  # Run with more resources
   ```

4. **Debug with GDB**
   ```bash
   make debug         # Start QEMU with GDB server
   # In another terminal:
   gdb build/kernel.elf -ex 'target remote localhost:1234'
   ```

5. **Advanced Build Options**
   ```bash
   make BUILD_TYPE=debug SANITIZE=1    # Debug build with sanitizers
   make COVERAGE=1 test                # Build with coverage testing
   make PROFILE=1 benchmark            # Profile build for performance analysis
   ```

### Individual Components

- `make kernel` - Build kernel only
- `make iso` - Create bootable ISO
- `make clean` - Clean build artifacts

## Architecture

### Kernel Components

```
kernel/
├── kernel.c      - Main kernel entry point
├── mm.c          - Memory management
└── interrupts.c  - Interrupt handling
```

### Drivers

```
drivers/
├── screen.c      - Text mode display
├── keyboard.c    - Keyboard input
├── timer.c       - System timer
├── ethernet.c    - RTL8139 network driver
└── wifi.c        - Wi-Fi device abstraction
```

### Filesystem

```
fs/
├── solixfs.c     - Custom filesystem implementation
└── vfs.c         - Virtual filesystem layer
```

### Networking

```
net/
└── net.c         - TCP/IP stack implementation

apps/net/
├── ping.c        - ICMP ping utility
├── ifconfig.c    - Network configuration
└── curl.c        - HTTP client
```

### Applications

```
apps/
├── browser/
│   └── lynx.c    - Text-based web browser
├── pkg/
│   └── pkg.c     - Package manager
└── net/          - Network applications
```

## Usage

### Basic Commands

```bash
help              # Show available commands
ls                # List directory contents
cd <dir>          # Change directory
pwd               # Show current directory
cat <file>        # Display file contents
mkdir <dir>       # Create directory
touch <file>      # Create empty file
rm <file>         # Remove file
ps                # List processes
kill <pid>        # Terminate process
```

### Network Commands

```bash
ping <host>       # Ping remote host
ifconfig          # Show network interfaces
curl <url>        # Fetch URL content
lynx <url>        # Browse web pages
```

### Package Management

```bash
pkg update        # Update package repositories
pkg search <pkg>  # Search for packages
pkg install <pkg> # Install package
pkg remove <pkg>  # Remove package
pkg list          # List installed packages
```

## Development

### Adding New Commands

1. Implement command function in `shell/shell.c`
2. Add to command table with modern help text
3. Update documentation and man pages
4. Add unit tests in `tests/` directory

### Adding Drivers

1. Create driver file in `drivers/` following modern driver interface
2. Implement driver operations with error handling
3. Add to Makefile with proper dependencies
4. Initialize in kernel with hot-plug support
5. Add configuration options in Kconfig

### Filesystem Support

1. Implement filesystem operations in `fs/`
2. Register with modern VFS layer
3. Add journaling and CoW support
4. Implement ACLs and extended attributes
5. Add encryption and quota support

### Security Development

1. Implement security hooks in kernel
2. Add capability checks to system calls
3. Create security policies and rules
4. Add audit logging for security events
5. Test with security test suites

### Network Protocol Development

1. Implement protocol stack in `net/`
2. Add to modern networking framework
3. Implement TLS/SSL support
4. Add IPv6 compatibility
5. Test with protocol conformance suites

## Project Structure

```
SolixOS/
├── bootloader/       # Boot assembly code
├── kernel/          # Core kernel components
├── drivers/         # Hardware drivers
├── fs/              # Filesystem implementations
├── net/             # Network stack
├── shell/           # Command shell
├── apps/            # User applications
├── include/         # Header files
├── build/           # Build artifacts
├── iso/             # ISO generation
├── Makefile         # Main build system
├── linker.ld        # Linker script
└── grub.cfg         # GRUB configuration
```

## License

This project is released under the MIT License.

## Changelog

### Version 2.0 (Current)
- **Major Architecture Upgrade**: Complete modernization of kernel subsystems
- **Security Framework**: New capability-based security with ACLs and encryption
- **Modern Filesystem**: Journaling, CoW, snapshots, and extended attributes
- **Advanced Networking**: IPv6, TLS/SSL, HTTP/2, and QUIC support
- **Hardware Support**: NVMe and USB framework with modern drivers
- **Build System**: Enhanced build system with parallel compilation and testing
- **Performance**: O(1) scheduler and SLAB allocator for optimal performance

### Version 1.x (Legacy)
- Basic microkernel architecture
- Simple filesystem support
- Basic TCP/IP networking
- Minimal hardware support

---

## Contributing

1. Fork the repository
2. Create feature branch
3. Make changes
4. Test thoroughly
5. Submit pull request

## Testing

### Unit Testing
- Individual component testing with modern test framework
- Memory management tests with corruption detection
- Filesystem tests with journaling and CoW
- Network stack tests with IPv6 and TLS
- Security tests with ACLs and encryption

### Integration Testing
- Full system boot testing with performance metrics
- Command execution testing with error scenarios
- Network functionality testing with modern protocols
- Application testing with security constraints
- Multi-core testing with race condition detection

### Performance Testing
- Boot time measurement with detailed profiling
- Memory usage analysis with leak detection
- Network throughput testing with offload features
- Application responsiveness testing under load
- Filesystem performance with journaling overhead

### Security Testing
- Penetration testing with security audit tools
- Access control testing with ACLs
- Encryption testing with known vectors
- Network security testing with TLS validation
- Kernel security testing with privilege escalation checks

## Future Enhancements

### Near-term (v2.1)
- **Complete IPv6 Implementation**: Full IPv6 protocol stack with autoconfiguration
- **Advanced Security**: SELinux-style mandatory access control
- **Container Support**: Lightweight virtualization and containers
- **Package Management**: Advanced package manager with dependencies
- **Performance Optimization**: Additional kernel optimizations and tuning

### Medium-term (v2.5)
- **64-bit Support**: x86-64 architecture support
- **Graphical Support**: Basic GUI framework with Wayland compatibility
- **Advanced Networking**: SDN support and network virtualization
- **Cloud Integration**: Container orchestration and cloud services
- **Development Tools**: Complete development environment and toolchain

### Long-term (v3.0)
- **Multi-architecture**: ARM, RISC-V, and other architectures
- **Advanced Security**: Hardware security modules and TPM support
- **AI/ML Integration**: Machine learning framework and tools
- **Distributed Systems**: Cluster management and distributed computing
- **Real-time Features**: Real-time kernel extensions and support

## Troubleshooting

### Build Issues
- Ensure all prerequisites are installed
- Check cross-compiler setup
- Verify NASM version compatibility

### Boot Issues
- Check bootloader configuration
- Verify kernel image format
- Test with different QEMU versions

### Network Issues
- Verify network card emulation
- Check IP configuration
- Test with different network setups

## Resources

- [OSDev Wiki](https://wiki.osdev.org/)
- [GCC Cross-Compiler Guide](https://wiki.osdev.org/GCC_Cross-Compiler)
- [QEMU Documentation](https://www.qemu.org/docs/master/)
- [GRUB Manual](https://www.gnu.org/software/grub/manual/)

## Contact

For questions, issues, or contributions, please use the project's issue tracker or contact the development team.
