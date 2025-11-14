#ifndef SOLIX_KERNEL_H
#define SOLIX_KERNEL_H

#include "types.h"

// System constants
#define KERNEL_VIRTUAL_BASE 0xC0000000
#define PAGE_SIZE 4096
#define KERNEL_STACK_SIZE 8192
#define MAX_PROCESSES 64
#define MAX_OPEN_FILES 16

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

// Process Control Block
typedef struct pcb {
    uint32_t pid;
    uint32_t ppid;
    uint32_t state;
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    uint32_t cr3;
    uint32_t kernel_stack;
    uint32_t user_stack;
    uint32_t exit_code;
    struct pcb* next;
} pcb_t;

// File descriptor structure
typedef struct fd {
    uint32_t inode;
    uint32_t offset;
    uint32_t flags;
    void* private_data;
} fd_t;

// Process structure
typedef struct process {
    pcb_t pcb;
    fd_t fd_table[MAX_OPEN_FILES];
    uint32_t cwd_inode;
} process_t;

// Kernel globals
extern process_t* current_process;
extern uint32_t next_pid;

// Core kernel functions
void kmain(void);
void kernel_init(void);
void panic(const char* msg);

// Process management
void process_init(void);
uint32_t process_create(void);
void process_switch(void);
void process_schedule(void);
void process_exit(uint32_t exit_code);

// System calls
void syscall_handler(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

#endif
