#include "printk.h"
#include "kernel.h"
#include "screen.h"
#include "mm.h"
#include "slab.h"

/**
 * Linux-Inspired printk System Implementation
 * Comprehensive kernel logging with levels, formatting, and buffering
 * Based on Linux printk design principles
 */

// Global printk control structure
struct printk_ctrl printk_ctrl = {
    .console_loglevel = CONSOLE_LOGLEVEL_DEFAULT,
    .default_message_loglevel = DEFAULT_MESSAGE_LOGLEVEL,
    .minimum_console_loglevel = LOGLEVEL_EMERG,
    .printk_time = 1,
    .console_drivers = NULL,
    .lock = SPIN_LOCK_UNLOCKED
};

// Global log buffer
struct log_buf log_buf = {
    .head = 0,
    .tail = 0,
    .len = 0,
    .flags = 0,
    .sequence = 0
};

// Early console support
static int early_console_active = 1;

// Rate limiting state
static DEFINE_RATELIMIT_STATE(printk_ratelimit_state, 
                             DEFAULT_RATELIMIT_INTERVAL, 
                             DEFAULT_RATELIMIT_BURST);

/**
 * Simple string length function
 */
static size_t strlen(const char *s) {
    size_t len = 0;
    while (s && *s++) len++;
    return len;
}

/**
 * Simple string copy function
 */
static char* strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++) != '\0');
    return dest;
}

/**
 * Memory copy function
 */
static void* memcpy(void *dest, const void *src, size_t n) {
    char *d = dest;
    const char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

/**
 * Memory set function
 */
static void* memset(void *s, int c, size_t n) {
    char *p = s;
    while (n--) *p++ = c;
    return s;
}

/**
 * Convert integer to string
 */
static char* itoa(int value, char *str, int base) {
    char *ptr = str;
    char *low;
    
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    
    // Handle negative numbers
    if (value < 0 && base == 10) {
        *ptr++ = '-';
        value = -value;
    }
    
    low = ptr;
    
    // Convert to string
    do {
        int digit = value % base;
        if (digit < 10) {
            *ptr++ = '0' + digit;
        } else {
            *ptr++ = 'A' + digit - 10;
        }
        value /= base;
    } while (value);
    
    *ptr-- = '\0';
    
    // Reverse string
    while (low < ptr) {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    
    return str;
}

/**
 * Convert unsigned integer to string
 */
static char* utoa(unsigned int value, char *str, int base) {
    char *ptr = str;
    char *low;
    
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    
    low = ptr;
    
    // Convert to string
    do {
        int digit = value % base;
        if (digit < 10) {
            *ptr++ = '0' + digit;
        } else {
            *ptr++ = 'A' + digit - 10;
        }
        value /= base;
    } while (value);
    
    *ptr-- = '\0';
    
    // Reverse string
    while (low < ptr) {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    
    return str;
}

/**
 * Convert integer to hexadecimal string
 */
static char* itohex(unsigned int value, char *str, int width) {
    char *ptr = str + width;
    const char *hex_chars = "0123456789ABCDEF";
    
    *ptr-- = '\0';
    
    while (width-- > 0) {
        *ptr-- = hex_chars[value & 0xF];
        value >>= 4;
    }
    
    return str;
}

/**
 * Simple formatted string output (simplified printf)
 */
static int vsnprintf_internal(char *buf, size_t size, const char *fmt, va_list args) {
    char temp[32];
    int count = 0;
    int i;
    
    while (*fmt && count < (int)size - 1) {
        if (*fmt == '%') {
            fmt++;
            
            // Handle format specifiers
            switch (*fmt) {
                case 'd':
                case 'i': {
                    int value = va_arg(args, int);
                    itoa(value, temp, 10);
                    for (i = 0; temp[i] && count < (int)size - 1; i++) {
                        buf[count++] = temp[i];
                    }
                    break;
                }
                case 'u': {
                    unsigned int value = va_arg(args, unsigned int);
                    utoa(value, temp, 10);
                    for (i = 0; temp[i] && count < (int)size - 1; i++) {
                        buf[count++] = temp[i];
                    }
                    break;
                }
                case 'x':
                case 'X': {
                    unsigned int value = va_arg(args, unsigned int);
                    itohex(value, temp, 8);
                    for (i = 0; temp[i] && count < (int)size - 1; i++) {
                        buf[count++] = temp[i];
                    }
                    break;
                }
                case 'c': {
                    char value = va_arg(args, int);
                    if (count < (int)size - 1) {
                        buf[count++] = value;
                    }
                    break;
                }
                case 's': {
                    char *value = va_arg(args, char*);
                    if (value) {
                        for (i = 0; value[i] && count < (int)size - 1; i++) {
                            buf[count++] = value[i];
                        }
                    } else {
                        // Handle NULL string
                        char *null_str = "(null)";
                        for (i = 0; null_str[i] && count < (int)size - 1; i++) {
                            buf[count++] = null_str[i];
                        }
                    }
                    break;
                }
                case 'p': {
                    void *value = va_arg(args, void*);
                    buf[count++] = '0';
                    if (count < (int)size - 1) buf[count++] = 'x';
                    itohex((unsigned int)value, temp, 8);
                    for (i = 0; temp[i] && count < (int)size - 1; i++) {
                        buf[count++] = temp[i];
                    }
                    break;
                }
                case '%': {
                    if (count < (int)size - 1) {
                        buf[count++] = '%';
                    }
                    break;
                }
                default:
                    // Unknown format specifier, just copy the character
                    if (count < (int)size - 1) {
                        buf[count++] = *fmt;
                    }
                    break;
            }
            fmt++;
        } else {
            buf[count++] = *fmt++;
        }
    }
    
    buf[count] = '\0';
    return count;
}

/**
 * Add message to log buffer
 */
static void log_buf_add(const char *msg, int len) {
    if (!msg || len <= 0) return;
    
    spin_lock(&printk_ctrl.lock);
    
    // Copy message to circular buffer
    for (int i = 0; i < len && log_buf.len < LOG_BUF_LEN; i++) {
        log_buf.buf[log_buf.head] = msg[i];
        log_buf.head = (log_buf.head + 1) & LOG_BUF_MASK;
        log_buf.len++;
        
        // Overwrite oldest message if buffer is full
        if (log_buf.len >= LOG_BUF_LEN) {
            log_buf.tail = (log_buf.tail + 1) & LOG_BUF_MASK;
            log_buf.len = LOG_BUF_LEN;
        }
    }
    
    log_buf.sequence++;
    
    spin_unlock(&printk_ctrl.lock);
}

/**
 * Parse log level from message
 */
static int parse_log_level(const char *fmt, int *msg_level) {
    int level = printk_ctrl.default_message_loglevel;
    
    // Check for log level prefix
    if (fmt && fmt[0] == '<' && fmt[1] >= '0' && fmt[1] <= '7' && fmt[2] == '>') {
        level = fmt[1] - '0';
        *msg_level = level;
        return 3; // Skip the prefix
    }
    
    *msg_level = level;
    return 0;
}

/**
 * Write message to console
 */
static void console_write(const char *msg, int len) {
    struct console *con = printk_ctrl.console_drivers;
    
    while (con) {
        if (con->write) {
            con->write(con, msg, len);
        }
        con = con->next;
    }
    
    // Default to screen output if no console drivers
    if (!printk_ctrl.console_drivers) {
        for (int i = 0; i < len; i++) {
            screen_putchar(msg[i]);
        }
    }
}

/**
 * Early console write
 */
static void early_console_write_internal(const char *str, unsigned int len) {
    if (!early_console_active) return;
    
    for (unsigned int i = 0; i < len; i++) {
        screen_putchar(str[i]);
    }
}

/**
 * Main printk implementation
 */
int vprintk(const char *fmt, va_list args) {
    char temp_buf[PRINTK_MAX_SINGLE_MESSAGE_LEN];
    char final_buf[PRINTK_MAX_SINGLE_MESSAGE_LEN + 64]; // Extra space for timestamp
    int msg_level;
    int prefix_len;
    int msg_len;
    int final_len = 0;
    
    if (!fmt) return 0;
    
    // Parse log level
    prefix_len = parse_log_level(fmt, &msg_level);
    fmt += prefix_len;
    
    // Format the message
    msg_len = vsnprintf_internal(temp_buf, sizeof(temp_buf), fmt, args);
    if (msg_len <= 0) return 0;
    
    // Add timestamp if enabled
    if (printk_ctrl.printk_time) {
        unsigned long timestamp = printk_timestamp();
        final_len += snprintf(final_buf + final_len, sizeof(final_buf) - final_len, 
                             "[%lu.%03lu] ", timestamp / 1000, timestamp % 1000);
    }
    
    // Add log level prefix
    const char *level_str[] = {
        KERN_EMERG, KERN_ALERT, KERN_CRIT, KERN_ERR,
        KERN_WARNING, KERN_NOTICE, KERN_INFO, KERN_DEBUG
    };
    
    if (msg_level >= 0 && msg_level <= 7) {
        final_len += snprintf(final_buf + final_len, sizeof(final_buf) - final_len, 
                             "%s", level_str[msg_level]);
    }
    
    // Add the formatted message
    if (final_len + msg_len < (int)sizeof(final_buf)) {
        memcpy(final_buf + final_len, temp_buf, msg_len);
        final_len += msg_len;
    }
    
    // Add newline if not present
    if (final_len > 0 && final_buf[final_len - 1] != '\n' && 
        final_len < (int)sizeof(final_buf) - 1) {
        final_buf[final_len++] = '\n';
        final_buf[final_len] = '\0';
    }
    
    // Add to log buffer
    log_buf_add(final_buf, final_len);
    
    // Write to console if level is high enough
    if (msg_level <= printk_ctrl.console_loglevel) {
        console_write(final_buf, final_len);
    }
    
    return final_len;
}

/**
 * printk with variable arguments
 */
int printk(const char *fmt, ...) {
    va_list args;
    int ret;
    
    va_start(args, fmt);
    ret = vprintk(fmt, args);
    va_end(args);
    
    return ret;
}

/**
 * Early printk (before full printk is available)
 */
void early_printk(const char *fmt, ...) {
    va_list args;
    char temp_buf[256];
    int len;
    
    if (!early_console_active) return;
    
    va_start(args, fmt);
    len = vsnprintf_internal(temp_buf, sizeof(temp_buf), fmt, args);
    va_end(args);
    
    if (len > 0) {
        early_console_write_internal(temp_buf, len);
    }
}

/**
 * Panic printing
 */
void panic_printk(const char *fmt, ...) {
    va_list args;
    
    // Force emergency level
    printk_ctrl.console_loglevel = LOGLEVEL_EMERG;
    
    va_start(args, fmt);
    vprintk(KERN_EMERG fmt, args);
    va_end(args);
}

/**
 * Emergency printing
 */
void emergency_printk(const char *fmt, ...) {
    va_list args;
    
    // Force alert level
    printk_ctrl.console_loglevel = LOGLEVEL_ALERT;
    
    va_start(args, fmt);
    vprintk(KERN_ALERT fmt, args);
    va_end(args);
}

/**
 * Get timestamp for printk
 */
unsigned long printk_timestamp(void) {
    return kernel_get_timestamp();
}

/**
 * Enable timestamps in printk
 */
void printk_enable_timestamps(void) {
    printk_ctrl.printk_time = 1;
}

/**
 * Disable timestamps in printk
 */
void printk_disable_timestamps(void) {
    printk_ctrl.printk_time = 0;
}

/**
 * Set console log level
 */
void console_loglevel_set(int level) {
    if (level >= LOGLEVEL_EMERG && level <= LOGLEVEL_DEBUG) {
        printk_ctrl.console_loglevel = level;
    }
}

/**
 * Get console log level
 */
int console_loglevel_get(void) {
    return printk_ctrl.console_loglevel;
}

/**
 * Set default message log level
 */
void message_loglevel_set(int level) {
    if (level >= LOGLEVEL_EMERG && level <= LOGLEVEL_DEBUG) {
        printk_ctrl.default_message_loglevel = level;
    }
}

/**
 * Get default message log level
 */
int message_loglevel_get(void) {
    return printk_ctrl.default_message_loglevel;
}

/**
 * Register console driver
 */
void register_console(struct console *console) {
    if (!console) return;
    
    spin_lock(&printk_ctrl.lock);
    
    // Add to beginning of list
    console->next = printk_ctrl.console_drivers;
    printk_ctrl.console_drivers = console;
    
    spin_unlock(&printk_ctrl.lock);
    
    pr_info("Console '%s' registered\n", console->name);
}

/**
 * Unregister console driver
 */
void unregister_console(struct console *console) {
    struct console **con;
    
    if (!console) return;
    
    spin_lock(&printk_ctrl.lock);
    
    // Find and remove from list
    con = &printk_ctrl.console_drivers;
    while (*con && *con != console) {
        con = &(*con)->next;
    }
    
    if (*con) {
        *con = console->next;
        console->next = NULL;
    }
    
    spin_unlock(&printk_ctrl.lock);
    
    if (*con) {
        pr_info("Console '%s' unregistered\n", console->name);
    }
}

/**
 * Clear log buffer
 */
void log_buf_clear(void) {
    spin_lock(&printk_ctrl.lock);
    
    log_buf.head = 0;
    log_buf.tail = 0;
    log_buf.len = 0;
    log_buf.sequence = 0;
    memset(log_buf.buf, 0, sizeof(log_buf.buf));
    
    spin_unlock(&printk_ctrl.lock);
}

/**
 * Copy log buffer to user buffer
 */
int log_buf_copy(char *buf, int len) {
    int copied = 0;
    
    if (!buf || len <= 0) return 0;
    
    spin_lock(&printk_ctrl.lock);
    
    int to_copy = (len < log_buf.len) ? len : log_buf.len;
    int pos = log_buf.tail;
    
    for (int i = 0; i < to_copy; i++) {
        buf[i] = log_buf.buf[pos];
        pos = (pos + 1) & LOG_BUF_MASK;
        copied++;
    }
    
    spin_unlock(&printk_ctrl.lock);
    
    return copied;
}

/**
 * Print hex dump
 */
void print_hex_dump(const char *prefix_str, int prefix_type, 
                   int rowsize, int groupsize, const void *buf, 
                   size_t len, bool ascii) {
    const unsigned char *ptr = buf;
    size_t i;
    
    for (i = 0; i < len; i += rowsize) {
        if (prefix_str) {
            printk("%s", prefix_str);
        }
        
        // Print address
        printk("%04lx: ", (unsigned long)(ptr + i));
        
        // Print hex bytes
        for (size_t j = 0; j < rowsize && i + j < len; j++) {
            if (j && j % groupsize == 0) {
                printk(" ");
            }
            printk("%02x ", ptr[i + j]);
        }
        
        // Fill remaining space
        for (size_t j = i + (len - i % rowsize); j < i + rowsize; j++) {
            printk("   ");
        }
        
        // Print ASCII if requested
        if (ascii) {
            printk(" |");
            for (size_t j = 0; j < rowsize && i + j < len; j++) {
                char c = ptr[i + j];
                printk("%c", (c >= 32 && c <= 126) ? c : '.');
            }
            printk("|");
        }
        
        printk("\n");
    }
}

/**
 * Print hex dump bytes
 */
void print_hex_dump_bytes(const char *prefix_str, int prefix_type, 
                         const void *buf, size_t len) {
    print_hex_dump(prefix_str, prefix_type, 16, 1, buf, len, true);
}

/**
 * sprintf implementation
 */
int sprintf(char *buf, const char *fmt, ...) {
    va_list args;
    int ret;
    
    va_start(args, fmt);
    ret = vsnprintf_internal(buf, INT_MAX, fmt, args);
    va_end(args);
    
    return ret;
}

/**
 * snprintf implementation
 */
int snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list args;
    int ret;
    
    va_start(args, fmt);
    ret = vsnprintf_internal(buf, size, fmt, args);
    va_end(args);
    
    return ret;
}

/**
 * vsprintf implementation
 */
int vsprintf(char *buf, const char *fmt, va_list args) {
    return vsnprintf_internal(buf, INT_MAX, fmt, args);
}

/**
 * vsnprintf implementation
 */
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args) {
    return vsnprintf_internal(buf, size, fmt, args);
}

/**
 * Initialize printk system
 */
void printk_init(void) {
    // Initialize log buffer
    log_buf_clear();
    
    // Set default log levels
    printk_ctrl.console_loglevel = CONSOLE_LOGLEVEL_DEFAULT;
    printk_ctrl.default_message_loglevel = DEFAULT_MESSAGE_LOGLEVEL;
    printk_ctrl.minimum_console_loglevel = LOGLEVEL_EMERG;
    
    // Enable timestamps
    printk_enable_timestamps();
    
    // Disable early console
    early_console_active = 0;
    
    pr_info("Linux-inspired printk system initialized\n");
    pr_info("Log buffer size: %d bytes\n", LOG_BUF_LEN);
    pr_info("Console log level: %d\n", printk_ctrl.console_loglevel);
}

/**
 * Early console initialization
 */
void early_console_init(void) {
    early_console_active = 1;
    early_printk("Early printk initialized\n");
}

/**
 * Print stack trace (simplified)
 */
void dump_stack(void) {
    unsigned long *stack_ptr;
    unsigned long addr;
    int i;
    
    __asm__ volatile("mov %%esp, %0" : "=r" (stack_ptr));
    
    printk("Call trace:\n");
    
    for (i = 0; i < 16 && i < 1024; i++) {
        addr = stack_ptr[i];
        if (addr == 0 || addr == 0xFFFFFFFF) break;
        
        printk("  [<%08lx>] %p\n", addr, (void*)addr);
    }
    
    if (i == 16) {
        printk("  ...\n");
    }
}
