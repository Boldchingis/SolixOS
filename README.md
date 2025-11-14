# SolixOS

A lightweight, command-line-only operating system built from scratch.

## Overview

SolixOS is a minimal x86 operating system designed for simplicity and performance. It features a microkernel architecture, custom filesystem, networking stack, and a comprehensive set of command-line tools.

## Features

### Core System
- **Microkernel Architecture**: Modular design with minimal kernel footprint
- **Memory Management**: Virtual memory, paging, and dynamic allocation
- **Multitasking**: Cooperative and preemptive scheduling
- **Hardware Abstraction**: Clean driver interface

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

- x86 compatible processor
- 512MB RAM minimum
- Network card (RTL8139 compatible)
- Virtualization or bare metal support

## Building

### Prerequisites

- GCC (cross-compiler for i386-elf recommended)
- NASM assembler
- GNU LD linker
- GNU Make
- QEMU (for testing)
- mkisofs (for ISO creation)

### Build Process

1. **Setup Build Environment**
   ```bash
   make setup
   ```

2. **Build Complete System**
   ```bash
   make build
   ```

3. **Run in QEMU**
   ```bash
   make run
   ```

4. **Debug with GDB**
   ```bash
   make debug
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
2. Add to command table
3. Update help text

### Adding Drivers

1. Create driver file in `drivers/`
2. Implement driver interface
3. Add to Makefile
4. Initialize in kernel

### Filesystem Support

1. Implement filesystem operations in `fs/`
2. Register with VFS layer
3. Add mount/umount support

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

## Contributing

1. Fork the repository
2. Create feature branch
3. Make changes
4. Test thoroughly
5. Submit pull request

## Testing

### Unit Testing
- Individual component testing
- Memory management tests
- Filesystem tests
- Network stack tests

### Integration Testing
- Full system boot testing
- Command execution testing
- Network functionality testing
- Application testing

### Performance Testing
- Boot time measurement
- Memory usage analysis
- Network throughput testing
- Application responsiveness

## Future Enhancements

- **Advanced Networking**: IPv6 support, advanced routing
- **Graphical Support**: Basic GUI framework
- **More Applications**: Text editors, development tools
- **Security**: User authentication, permissions
- **Filesystems**: Support for ext2, FAT32
- **Hardware Support**: More device drivers

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
