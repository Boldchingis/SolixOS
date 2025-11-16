#include "kernel.h"
#include "../include/screen.h"
#include "../include/mm.h"

/**
 * Debug and diagnostic functions implementation
 * Enhanced debugging capabilities for SolixOS
 */

// Simple string length function
static size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

// Simple string to decimal conversion
static void print_dec(uint32_t num) {
    if (num == 0) {
        screen_print("0");
        return;
    }
    
    char buffer[12];
    int i = 11;
    buffer[i] = '\0';
    i--;
    
    while (num > 0 && i >= 0) {
        buffer[i] = '0' + (num % 10);
        num /= 10;
        i--;
    }
    
    screen_print(&buffer[i + 1]);
}

/**
 * Initialize debug system
 */
void debug_init(void) {
    debug_state.debug_level = DEBUG_INFO;
    debug_state.log_index = 0;
    debug_state.panic_count = 0;
    debug_state.last_panic_time = 0;
    
    // Clear log buffer
    for (int i = 0; i < 1024; i++) {
        debug_state.log_buffer[i] = 0;
    }
    
    debug_print(DEBUG_INFO, "Debug system initialized");
}

/**
 * Enhanced debug print with level filtering
 */
void debug_print(uint32_t level, const char* fmt, ...) {
    if (level > debug_state.debug_level) {
        return;
    }
    
    // Add to log buffer
    uint32_t index = debug_state.log_index % 1024;
    debug_state.log_buffer[index] = kernel_get_timestamp();
    debug_state.log_index++;
    
    // Simple format string processing (basic %s and %d support)
    const char* ptr = fmt;
    while (*ptr) {
        if (*ptr == '%' && *(ptr + 1)) {
            ptr++;
            switch (*ptr) {
                case 's': {
                    // Would need va_list for proper implementation
                    screen_print("(string)");
                    break;
                }
                case 'd': {
                    // Would need va_list for proper implementation  
                    screen_print("(number)");
                    break;
                }
                default:
                    screen_print("%");
                    screen_print_char(*ptr);
                    break;
            }
        } else {
            screen_print_char(*ptr);
        }
        ptr++;
    }
}

/**
 * Dump memory region for debugging
 */
void debug_dump_memory(uint32_t addr, uint32_t size) {
    screen_print("Memory dump at 0x");
    print_hex(addr);
    screen_print(", size ");
    print_dec(size);
    screen_print(":\n");
    
    uint8_t* mem = (uint8_t*)addr;
    for (uint32_t i = 0; i < size && i < 256; i += 16) {
        // Print address
        print_hex(addr + i);
        screen_print(": ");
        
        // Print hex bytes
        for (uint32_t j = 0; j < 16 && (i + j) < size; j++) {
            if (mem[i + j] < 16) screen_print("0");
            print_hex(mem[i + j]);
            screen_print(" ");
        }
        
        // Print ASCII representation
        screen_print(" ");
        for (uint32_t j = 0; j < 16 && (i + j) < size; j++) {
            char c = mem[i + j];
            screen_print_char((c >= 32 && c <= 126) ? c : '.');
        }
        screen_print("\n");
    }
}

/**
 * Dump process information for debugging
 */
void debug_dump_process(process_t* proc) {
    if (!proc) return;
    
    screen_print("Process PID ");
    print_dec(proc->pcb.pid);
    screen_print(":\n");
    screen_print("  Parent PID: ");
    print_dec(proc->pcb.ppid);
    screen_print("\n");
    screen_print("  State: ");
    print_dec(proc->pcb.state);
    screen_print("\n");
    screen_print("  Stack: 0x");
    print_hex(proc->pcb.kernel_stack);
    screen_print("\n");
    screen_print("  Priority: ");
    print_dec(proc->priority);
    screen_print("\n");
    screen_print("  CPU time: ");
    print_dec(proc->pcb.cpu_time);
    screen_print("\n");
}

/**
 * Simple stack trace for debugging
 */
void debug_trace_stack(uint32_t max_frames) {
    uint32_t* ebp;
    __asm__ volatile("mov %%ebp, %0" : "=r" (ebp));
    
    screen_print("Stack trace (");
    print_dec(max_frames);
    screen_print(" frames max):\n");
    
    for (uint32_t i = 0; i < max_frames && ebp; i++) {
        uint32_t ret_addr = ebp[1];
        screen_print("  [");
        print_dec(i);
        screen_print("] 0x");
        print_hex(ret_addr);
        screen_print("\n");
        
        // Move to next stack frame
        ebp = (uint32_t*)ebp[0];
        
        // Basic sanity check
        if ((uint32_t)ebp < 0x100000 || (uint32_t)ebp > 0x80000000) {
            screen_print("  Invalid stack frame, stopping trace\n");
            break;
        }
    }
}

/**
 * Debug assertion with panic on failure
 */
void debug_assert(bool condition, const char* msg) {
    if (!condition) {
        screen_print("ASSERTION FAILED: ");
        screen_print(msg);
        screen_print("\n");
        panic(msg);
    }
}

/**
 * Simple delay function
 */
void kernel_delay(uint32_t milliseconds) {
    // Simple busy-wait delay
    // In a real implementation, this would use the timer
    volatile uint32_t count = milliseconds * 10000;
    while (count--) {
        __asm__ volatile("nop");
    }
}

/**
 * Get system timestamp in milliseconds
 */
uint32_t kernel_get_timestamp(void) {
    // Simple timestamp based on timer ticks
    // In a real implementation, this would read from hardware timer
    static uint32_t timestamp = 0;
    return timestamp++;
}

/**
 * Get kernel version string
 */
const char* kernel_get_version(void) {
    return "1.0-Enhanced";
}

/**
 * Print hex value (helper function)
 */
void print_hex(uint32_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    screen_print("0x");
    for (int i = 28; i >= 0; i -= 4) {
        screen_print_char(hex_chars[(value >> i) & 0xF]);
    }
}

/**
 * Enhanced process creation with validation
 */
uint32_t process_create_enhanced(const char* name) {
    uint32_t index = find_free_pid();
    if (index == 0) {
        debug_print(DEBUG_WARN, "No free process slots available");
        return 0;
    }
    
    process_t* proc = &process_table[index];

    // Initialize PCB with enhanced fields
    proc->pcb.pid = next_pid++;
    proc->pcb.ppid = current_process ? current_process->pcb.pid : 0;
    proc->pcb.state = PROCESS_READY;
    proc->pcb.kernel_stack = (uint32_t)kmalloc(KERNEL_STACK_SIZE);
    proc->pcb.user_stack = 0x7FFFF000;
    proc->pcb.exit_code = 0;
    proc->pcb.creation_time = kernel_get_timestamp();
    proc->pcb.cpu_time = 0;

    // Initialize file descriptor table
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        proc->fd_table[i].inode = 0;
        proc->fd_table[i].ref_count = 0;
    }

    // Set current working directory to root
    proc->cwd_inode = 1;
    
    // Set process name
    if (name) {
        int len = strlen(name);
        if (len > 31) len = 31;
        for (int i = 0; i < len; i++) {
            proc->name[i] = name[i];
        }
        proc->name[len] = '\0';
    } else {
        proc->name[0] = '\0';
    }
    
    // Set default priority
    proc->priority = 5; // Medium priority

    debug_print(DEBUG_INFO, "Created process PID %d: %s", proc->pcb.pid, proc->name);
    return proc->pcb.pid;
}

/**
 * Find free PID with optimization
 */
static uint32_t find_free_pid(void) {
    static uint32_t last_search = 0;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        uint32_t index = (last_search + i) % MAX_PROCESSES;
        uint32_t bitmap_index = index / 32;
        uint32_t bit_index = index % 32;

        if (!(process_bitmap[bitmap_index] & (1 << bit_index))) {
            process_bitmap[bitmap_index] |= (1 << bit_index);
            last_search = index;
            return index;
        }
    }
    return 0; // No free PIDs
}
