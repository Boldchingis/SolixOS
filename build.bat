@echo off
REM SolixOS Build Script for Windows
REM Automated build system for SolixOS

setlocal enabledelayedexpansion

REM Colors for output (limited in Windows cmd)
set INFO=[INFO]
set SUCCESS=[SUCCESS]
set WARNING=[WARNING]
set ERROR=[ERROR]

REM Print status messages
:print_status
echo %INFO% %~1
goto :eof

:print_success
echo %SUCCESS% %~1
goto :eof

:print_warning
echo %WARNING% %~1
goto :eof

:print_error
echo %ERROR% %~1
goto :eof

REM Check prerequisites
:check_prereqs
call :print_status "Checking prerequisites..."

REM Check for required tools
where gcc >nul 2>&1
if errorlevel 1 (
    call :print_error "GCC not found. Please install MinGW-w64 or similar."
    exit /b 1
)

where nasm >nul 2>&1
if errorlevel 1 (
    call :print_error "NASM not found. Please install NASM."
    exit /b 1
)

where ld >nul 2>&1
if errorlevel 1 (
    call :print_error "LD not found. Please install binutils."
    exit /b 1
)

where make >nul 2>&1
if errorlevel 1 (
    call :print_error "Make not found. Please install GNU Make."
    exit /b 1
)

where qemu-system-i386 >nul 2>&1
if errorlevel 1 (
    call :print_warning "QEMU not found. You may not be able to test the OS."
)

where mkisofs >nul 2>&1
if errorlevel 1 (
    call :print_warning "mkisofs not found. You may not be able to create ISO images."
)

call :print_success "Prerequisites check completed"
goto :eof

REM Clean previous builds
:clean_build
call :print_status "Cleaning previous builds..."
make clean
call :print_success "Build directory cleaned"
goto :eof

REM Setup build environment
:setup_build
call :print_status "Setting up build environment..."
make setup
call :print_success "Build environment ready"
goto :eof

REM Build bootloader
:build_bootloader
call :print_status "Building bootloader..."
make bootloader
if errorlevel 1 (
    call :print_error "Bootloader build failed"
    exit /b 1
)
call :print_success "Bootloader built successfully"
goto :eof

REM Build kernel
:build_kernel
call :print_status "Building kernel components..."

REM Set compiler flags
set CFLAGS=-m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -c -Iinclude -Ikernel -Idrivers -Ifs -Ishell -Inet -Iapps/browser -Iapps/pkg -Iapps/net

call :print_status "Building kernel core..."
make -C kernel CC=gcc CFLAGS="%CFLAGS%"
if errorlevel 1 (
    call :print_error "Kernel build failed"
    exit /b 1
)

call :print_status "Building drivers..."
make -C drivers CC=gcc CFLAGS="%CFLAGS%"
if errorlevel 1 (
    call :print_error "Drivers build failed"
    exit /b 1
)

call :print_status "Building filesystem..."
make -C fs CC=gcc CFLAGS="%CFLAGS%"
if errorlevel 1 (
    call :print_error "Filesystem build failed"
    exit /b 1
)

call :print_status "Building shell..."
make -C shell CC=gcc CFLAGS="%CFLAGS%"
if errorlevel 1 (
    call :print_error "Shell build failed"
    exit /b 1
)

call :print_status "Building network stack..."
make -C net CC=gcc CFLAGS="%CFLAGS%"
if errorlevel 1 (
    call :print_error "Network stack build failed"
    exit /b 1
)

call :print_status "Building applications..."
make -C apps/browser CC=gcc CFLAGS="%CFLAGS%"
if errorlevel 1 (
    call :print_error "Browser build failed"
    exit /b 1
)

make -C apps/pkg CC=gcc CFLAGS="%CFLAGS%"
if errorlevel 1 (
    call :print_error "Package manager build failed"
    exit /b 1
)

make -C apps/net CC=gcc CFLAGS="%CFLAGS%"
if errorlevel 1 (
    call :print_error "Network applications build failed"
    exit /b 1
)

call :print_status "Assembling interrupt handlers..."
nasm -f elf32 -o build/isr.o kernel/isr.asm
if errorlevel 1 (
    call :print_error "Assembly failed"
    exit /b 1
)

call :print_status "Linking kernel..."
ld -melf_i386 -Tlinker.ld -o build/kernel.bin kernel/*.o drivers/*.o fs/*.o shell/*.o net/*.o apps/browser/*.o apps/pkg/*.o apps/net/*.o build/isr.o
if errorlevel 1 (
    call :print_error "Linking failed"
    exit /b 1
)

call :print_success "Kernel built successfully"
goto :eof

REM Create ISO image
:create_iso
call :print_status "Creating ISO image..."
make iso
if errorlevel 1 (
    call :print_error "ISO creation failed"
    exit /b 1
)
call :print_success "ISO image created: kernel.iso"
goto :eof

REM Run tests
:run_tests
call :print_status "Running basic tests..."

if not exist "build\kernel.bin" (
    call :print_error "Kernel binary not found"
    exit /b 1
)

if not exist "kernel.iso" (
    call :print_error "ISO image not found"
    exit /b 1
)

call :print_success "Basic tests passed"
goto :eof

REM Main build function
:main_build
call :print_status "Starting SolixOS build process..."

call :check_prereqs
if errorlevel 1 exit /b 1

call :clean_build
call :setup_build
call :build_bootloader
call :build_kernel
call :create_iso
call :run_tests

call :print_success "Build completed successfully!"
call :print_status "Run 'build.bat run' to start SolixOS in QEMU"
call :print_status "Run 'build.bat debug' to start with GDB support"
goto :eof

REM Run in QEMU
:run_qemu
call :print_status "Starting QEMU..."
where qemu-system-i386 >nul 2>&1
if errorlevel 1 (
    call :print_error "QEMU not found. Please install QEMU."
    exit /b 1
)
make run
goto :eof

REM Debug build
:debug_build
call :print_status "Starting debug build..."
call :main_build
call :print_status "Starting QEMU with GDB support..."
make debug
goto :eof

REM Clean build
:clean_all
call :print_status "Cleaning all build artifacts..."
make clean
call :print_success "All artifacts cleaned"
goto :eof

REM Show help
:show_help
echo SolixOS Build Script
echo Usage: %0 [COMMAND]
echo.
echo Commands:
echo   build    - Build complete system (default)
echo   clean    - Clean all build artifacts
echo   run      - Build and run in QEMU
echo   debug    - Build and run with GDB support
echo   help     - Show this help message
echo.
echo Examples:
echo   %0           - Build complete system
echo   %0 run       - Build and run
echo   %0 clean     - Clean build directory
echo   %0 debug     - Debug build
goto :eof

REM Parse command line arguments
if "%1"=="" (
    set COMMAND=build
) else (
    set COMMAND=%1
)

if "%COMMAND%"=="build" (
    call :main_build
) else if "%COMMAND%"=="clean" (
    call :clean_all
) else if "%COMMAND%"=="run" (
    call :main_build
    call :run_qemu
) else if "%COMMAND%"=="debug" (
    call :debug_build
) else if "%COMMAND%"=="help" (
    call :show_help
) else (
    call :print_error "Unknown command: %COMMAND%"
    call :show_help
    exit /b 1
)

pause
