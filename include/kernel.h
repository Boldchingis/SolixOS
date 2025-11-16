#ifndef SOLIX_KERNEL_H
#define SOLIX_KERNEL_H

#include "types.h"

/**
 * SolixOS Kernel Header
 * Core kernel definitions and function declarations
 * Version: 1.0 Enhanced
 */

// System constants
#define KERNEL_VIRTUAL_BASE 0xC0000000
#define PAGE_SIZE 4096
#define KERNEL_STACK_SIZE 8192
#define MAX_PROCESSES 64
#define MAX_OPEN_FILES 16
#define KERNEL_VERSION_MAJOR 1
#define KERNEL_VERSION_MINOR 0
#define KERNEL_VERSION_PATCH 0

// Debug and logging levels
#define DEBUG_NONE 0
#define DEBUG_ERROR 1
#define DEBUG_WARN 2
#define DEBUG_INFO 3
#define DEBUG_DEBUG 4

// Process states
#define PROCESS_RUNNING 0
#define PROCESS_READY 1
#define PROCESS_BLOCKED 2
#define PROCESS_TERMINATED 3

// System call numbers
#define SYS_EXIT 1
#define SYS_FORK 2
#define SYS_READ 3
#define SYS_WRITE 4
#define SYS_OPEN 5
#define SYS_CLOSE 6
#define SYS_EXEC 7
#define SYS_WAIT 8
#define SYS_MEMINFO 9
#define SYS_DEBUG 10

/**
 * Process Control Block (PCB)
 * Contains all state information for a process
 */
typedef struct pcb {
    uint32_t pid;           // Process ID
    uint32_t ppid;          // Parent Process ID
    uint32_t state;         // Current process state
    uint32_t esp;           // Stack pointer
    uint32_t ebp;           // Base pointer
    uint32_t eip;           // Instruction pointer
    uint32_t cr3;           // Page directory physical address
    uint32_t kernel_stack;  // Kernel stack pointer
    uint32_t user_stack;    // User stack pointer
    uint32_t exit_code;     // Process exit code
    uint32_t creation_time; // Process creation timestamp
    uint32_t cpu_time;      // Total CPU time used
    struct pcb* next;       // Next process in scheduling queue
} pcb_t;

/**
 * File descriptor structure
 * Represents an open file or device
 */
typedef struct fd {
    uint32_t inode;         // Inode number
    uint32_t offset;        // Current file offset
    uint32_t flags;         // File access flags
    void* private_data;     // Driver-specific data
    uint32_t ref_count;     // Reference count for dup operations
} fd_t;

/**
 * Process structure
 * Combines PCB with file descriptor table and other process data
 */
typedef struct process {
    pcb_t pcb;                           // Process control block
    fd_t fd_table[MAX_OPEN_FILES];       // File descriptor table
    uint32_t cwd_inode;                  // Current working directory inode
    char name[32];                       // Process name
    uint32_t priority;                   // Process scheduling priority
} process_t;

/**
 * Debug and logging structure
 */
struct debug_info {
    uint32_t debug_level;                // Current debug level
    uint32_t log_buffer[1024];           // Circular log buffer
    uint32_t log_index;                  // Current log index
    uint32_t panic_count;                // Number of panics occurred
    uint32_t last_panic_time;            // Timestamp of last panic
};

// Kernel globals
extern process_t* current_process;
extern uint32_t next_pid;
extern struct debug_info debug_state;

// Core kernel functions
void kmain(void* multiboot_info);
void kernel_init(void);
void panic(const char* msg) __attribute__((noreturn));
void kernel_halt(void) __attribute__((noreturn));

// Process management
void process_init(void);
uint32_t process_create(void);
void process_switch(void);
void process_schedule(void);
void process_exit(uint32_t exit_code);
uint32_t process_get_time(void);
void process_set_priority(uint32_t pid, uint32_t priority);

// System calls
void syscall_handler(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

// Debug and diagnostics
void debug_init(void);
void debug_print(uint32_t level, const char* fmt, ...);
void debug_dump_memory(uint32_t addr, uint32_t size);
void debug_dump_process(process_t* proc);
void debug_trace_stack(uint32_t max_frames);
void debug_assert(bool condition, const char* msg);

// Utility functions
void kernel_delay(uint32_t milliseconds);
uint32_t kernel_get_timestamp(void);
const char* kernel_get_version(void);

#endif
