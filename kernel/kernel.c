#include "kernel.h"
#include "../include/screen.h"
#include "../include/keyboard.h"

// Kernel state
process_t* current_process = NULL;
uint32_t next_pid = 1;
static process_t process_table[MAX_PROCESSES];
static uint32_t process_bitmap[(MAX_PROCESSES + 31) / 32];

// Multiboot information structure
typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
} __attribute__((packed)) multiboot_info_t;

// Kernel entry point
void kmain(multiboot_info_t* mb_info) {
    // Clear screen and display banner
    screen_clear();
    screen_print("SolixOS v1.0 - Lightweight CLI Operating System\n");
    screen_print("================================================\n\n");
    
    // Initialize kernel subsystems
    kernel_init();
    
    // Start the shell
    screen_print("Starting shell...\n");
    shell_start();
    
    // Kernel panic if we reach here
    panic("Shell terminated unexpectedly");
}

// Initialize all kernel subsystems
void kernel_init(void) {
    screen_print("[*] Initializing kernel subsystems...\n");
    
    // Initialize process management
    process_init();
    screen_print("[+] Process management initialized\n");
    
    // Initialize memory management
    mm_init();
    screen_print("[+] Memory management initialized\n");
    
    // Initialize interrupt system
    interrupts_init();
    screen_print("[+] Interrupt system initialized\n");
    
    // Initialize filesystem
    vfs_init();
    screen_print("[+] Virtual filesystem initialized\n");
    
    // Initialize drivers
    timer_init();
    keyboard_init();
    screen_print("[+] Hardware drivers initialized\n");
    
    // Enable interrupts
    __asm__ volatile("sti");
    screen_print("[+] Interrupts enabled\n");
    
    screen_print("[*] Kernel initialization complete\n\n");
}

// Kernel panic - unrecoverable error
void panic(const char* msg) {
    screen_print("\n\n!!! KERNEL PANIC !!!\n");
    screen_print("Error: ");
    screen_print(msg);
    screen_print("\nSystem halted.\n");
    
    // Disable interrupts and halt
    __asm__ volatile("cli");
    while(1) {
        __asm__ volatile("hlt");
    }
}

// Process initialization
void process_init(void) {
    // Clear process table
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pcb.state = PROCESS_TERMINATED;
    }
    
    // Clear bitmap
    for (int i = 0; i < (MAX_PROCESSES + 31) / 32; i++) {
        process_bitmap[i] = 0;
    }
    
    // Create init process (PID 1)
    current_process = &process_table[0];
    current_process->pcb.pid = next_pid++;
    current_process->pcb.ppid = 0;
    current_process->pcb.state = PROCESS_RUNNING;
    current_process->pcb.kernel_stack = (uint32_t)kmalloc(KERNEL_STACK_SIZE);
    current_process->pcb.user_stack = 0x7FFFF000; // Top of user space
    
    // Mark PID 1 as used
    process_bitmap[0] |= 0x01;
}

// Find free PID
static uint32_t find_free_pid(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        uint32_t bitmap_index = i / 32;
        uint32_t bit_index = i % 32;
        
        if (!(process_bitmap[bitmap_index] & (1 << bit_index))) {
            process_bitmap[bitmap_index] |= (1 << bit_index);
            return i;
        }
    }
    return 0; // No free PIDs
}

// Create new process
uint32_t process_create(void) {
    uint32_t index = find_free_pid();
    if (index == 0) {
        return 0; // No free process slots
    }
    
    process_t* proc = &process_table[index];
    
    // Initialize PCB
    proc->pcb.pid = next_pid++;
    proc->pcb.ppid = current_process->pcb.pid;
    proc->pcb.state = PROCESS_READY;
    proc->pcb.kernel_stack = (uint32_t)kmalloc(KERNEL_STACK_SIZE);
    proc->pcb.user_stack = 0x7FFFF000;
    proc->pcb.exit_code = 0;
    
    // Initialize file descriptor table
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        proc->fd_table[i].inode = 0;
    }
    
    // Set current working directory to root
    proc->cwd_inode = 1;
    
    return proc->pcb.pid;
}

// Process exit
void process_exit(uint32_t exit_code) {
    if (!current_process) return;
    
    current_process->pcb.exit_code = exit_code;
    current_process->pcb.state = PROCESS_TERMINATED;
    
    // Free kernel stack
    kfree((void*)current_process->pcb.kernel_stack);
    
    // Mark PID as free
    uint32_t index = current_process - process_table;
    uint32_t bitmap_index = index / 32;
    uint32_t bit_index = index % 32;
    process_bitmap[bitmap_index] &= ~(1 << bit_index);
    
    // Schedule next process
    process_schedule();
}

// Simple round-robin scheduler
void process_schedule(void) {
    static uint32_t current_index = 0;
    
    // Find next ready process
    for (int i = 0; i < MAX_PROCESSES; i++) {
        current_index = (current_index + 1) % MAX_PROCESSES;
        
        if (process_table[current_index].pcb.state == PROCESS_READY) {
            process_t* next = &process_table[current_index];
            
            if (current_process && current_process->pcb.state == PROCESS_RUNNING) {
                current_process->pcb.state = PROCESS_READY;
            }
            
            next->pcb.state = PROCESS_RUNNING;
            current_process = next;
            process_switch();
            return;
        }
    }
    
    // No ready processes found
    panic("No runnable processes");
}

// Context switch (simplified)
void process_switch(void) {
    // In a real implementation, this would save/restore registers
    // For now, we just return to continue execution
    return;
}
