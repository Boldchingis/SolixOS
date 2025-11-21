#ifndef SOLIX_PRINTK_H
#define SOLIX_PRINTK_H

#include "types.h"
#include "kernel.h"

/**
 * Linux-Inspired printk System for SolixOS
 * Comprehensive kernel logging with levels, formatting, and buffering
 * Based on Linux printk design principles
 */

// Log levels (Linux-inspired)
#define KERN_EMERG       "<0>"  // System is unusable
#define KERN_ALERT       "<1>"  // Action must be taken immediately
#define KERN_CRIT        "<2>"  // Critical conditions
#define KERN_ERR         "<3>"  // Error conditions
#define KERN_WARNING     "<4>"  // Warning conditions
#define KERN_NOTICE      "<5>"  // Normal but significant condition
#define KERN_INFO        "<6>"  // Informational message
#define KERN_DEBUG       "<7>"  // Debug-level message

// Log level constants
#define LOGLEVEL_EMERG   0
#define LOGLEVEL_ALERT   1
#define LOGLEVEL_CRIT    2
#define LOGLEVEL_ERR     3
#define LOGLEVEL_WARNING 4
#define LOGLEVEL_NOTICE  5
#define LOGLEVEL_INFO    6
#define LOGLEVEL_DEBUG   7

// Default log level
#define DEFAULT_MESSAGE_LOGLEVEL  LOGLEVEL_NOTICE
#define CONSOLE_LOGLEVEL_DEFAULT LOGLEVEL_INFO

// Log buffer size
#define LOG_BUF_LEN      (1 << 17)  // 128KB log buffer
#define LOG_BUF_MASK     (LOG_BUF_LEN - 1)

// Maximum message length
#define LOG_LINE_MAX     1024

// Log flags
#define LOG_NOCONS       (1 << 0)  // Don't print to console
#define LOG_NEWLINE      (1 << 1)  // Append newline
#define LOG_PREFIX       (1 << 2)  // Include log level prefix
#define LOG_CONT         (1 << 3)  // Continuation of previous line

/**
 * Log buffer structure
 */
struct log_buf {
    char buf[LOG_BUF_LEN];
    unsigned int head;    // Write position
    unsigned int tail;    // Read position
    unsigned int len;     // Current length
    unsigned int flags;   // Buffer flags
    unsigned int sequence; // Sequence number for messages
};

/**
 * Console driver structure
 */
struct console {
    char name[16];
    void (*write)(struct console *, const char *, unsigned int);
    void (*read)(struct console *, char *, unsigned int);
    struct console *next;
    int index;
    int flags;
};

/**
 * printk control structure
 */
struct printk_ctrl {
    int console_loglevel;     // Current console log level
    int default_message_loglevel; // Default message log level
    int minimum_console_loglevel; // Minimum console log level
    unsigned long printk_time; // Timestamp messages
    struct console *console_drivers; // List of console drivers
    spinlock_t lock;          // Lock for log buffer access
};

/**
 * External declarations
 */
extern struct printk_ctrl printk_ctrl;
extern struct log_buf log_buf;

/**
 * Core printk functions
 */
extern int printk(const char *fmt, ...);
extern int vprintk(const char *fmt, va_list args);
extern void early_printk(const char *fmt, ...);

/**
 * Log level control
 */
extern void console_loglevel_set(int level);
extern int console_loglevel_get(void);
extern void message_loglevel_set(int level);
extern int message_loglevel_get(void);

/**
 * Console registration
 */
extern void register_console(struct console *console);
extern void unregister_console(struct console *console);

/**
 * Log buffer management
 */
extern void log_buf_clear(void);
extern void log_buf_flush(void);
extern int log_buf_copy(char *buf, int len);
extern int log_buf_read(char *buf, int len);

/**
 * Timestamp support
 */
extern unsigned long printk_timestamp(void);
extern void printk_enable_timestamps(void);
extern void printk_disable_timestamps(void);

/**
 * Panic and emergency printing
 */
extern void panic_printk(const char *fmt, ...);
extern void emergency_printk(const char *fmt, ...);

/**
 * Debug printing helpers
 */
#define pr_emerg(fmt, ...)    printk(KERN_EMERG fmt, ##__VA_ARGS__)
#define pr_alert(fmt, ...)    printk(KERN_ALERT fmt, ##__VA_ARGS__)
#define pr_crit(fmt, ...)     printk(KERN_CRIT fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...)      printk(KERN_ERR fmt, ##__VA_ARGS__)
#define pr_warning(fmt, ...)  printk(KERN_WARNING fmt, ##__VA_ARGS__)
#define pr_warn pr_warning
#define pr_notice(fmt, ...)   printk(KERN_NOTICE fmt, ##__VA_ARGS__)
#define pr_info(fmt, ...)     printk(KERN_INFO fmt, ##__VA_ARGS__)
#define pr_debug(fmt, ...)    printk(KERN_DEBUG fmt, ##__VA_ARGS__)

/**
 * Development and debugging macros
 */
#ifdef DEBUG
#define pr_devel(fmt, ...)    printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#else
#define pr_devel(fmt, ...)    do { } while (0)
#endif

/**
 * Conditional printing based on debug level
 */
#define pr_level(level, fmt, ...) \
    do { \
        if (printk_ctrl.console_loglevel >= (level)) \
            printk(fmt, ##__VA_ARGS__); \
    } while (0)

/**
 * Hex dump helpers
 */
extern void print_hex_dump(const char *prefix_str, int prefix_type, 
                          int rowsize, int groupsize, const void *buf, 
                          size_t len, bool ascii);

extern void print_hex_dump_bytes(const char *prefix_str, int prefix_type, 
                                const void *buf, size_t len);

/**
 * String printing helpers
 */
extern void print_string(const char *str);
extern void print_char(char c);
extern void print_int(int num);
extern void print_uint(unsigned int num);
extern void print_hex(unsigned int num);

/**
 * Formatted printing (simplified printf)
 */
extern int sprintf(char *buf, const char *fmt, ...);
extern int snprintf(char *buf, size_t size, const char *fmt, ...);
extern int vsprintf(char *buf, const char *fmt, va_list args);
extern int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

/**
 * Stack trace printing
 */
extern void dump_stack(void);
extern void show_stack(struct task_struct *task, unsigned long *sp);

/**
 * Memory dump helpers
 */
extern void dump_mem(const char *prefix, const void *addr, size_t len);
extern void dump_regs(const void *regs);

/**
 * Performance and timing
 */
extern void printk_timing_start(void);
extern void printk_timing_stop(void);
extern void show_timing_info(void);

/**
 * Rate limiting for printk
 */
struct ratelimit_state {
    unsigned int interval;
    unsigned int burst;
    unsigned long printed;
    unsigned long missed;
    unsigned long last_jiffies;
};

#define DEFINE_RATELIMIT_STATE(name, interval, burst) \
    struct ratelimit_state name = { interval, burst, 0, 0, 0 }

#define __ratelimit(state) \
    ({ \
        unsigned long now = jiffies; \
        int ret = 0; \
        if (time_after(now, (state)->last_jiffies + (state)->interval)) { \
            (state)->last_jiffies = now; \
            if ((state)->printed < (state)->burst) { \
                (state)->printed++; \
                ret = 1; \
            } else { \
                (state)->missed++; \
            } \
        } \
        ret; \
    })

#define pr_ratelimited(level, fmt, ...) \
    do { \
        static DEFINE_RATELIMIT_STATE(_rs, DEFAULT_RATELIMIT_INTERVAL, \
                                     DEFAULT_RATELIMIT_BURST); \
        if (__ratelimit(&_rs)) \
            printk(level fmt, ##__VA_ARGS__); \
    } while (0)

// Default rate limiting values
#define DEFAULT_RATELIMIT_INTERVAL  (5 * HZ)
#define DEFAULT_RATELIMIT_BURST      10

/**
 * Early console support
 */
extern void early_console_init(void);
extern void early_console_write(const char *str, unsigned int len);
extern void early_console_register(void);

/**
 * Sysctl interface for printk configuration
 */
extern int printk_sysctl_init(void);

/**
 * Configuration options
 */
#define CONFIG_PRINTK_TIME
#define CONFIG_EARLY_PRINTK
#define CONFIG_PRINTK_RATELIMIT

/**
 * Helper macros
 */
#define PRINTK_MAX_SINGLE_MESSAGE_LEN 1024

/**
 * Time helpers
 */
#define jiffies (kernel_get_timestamp() / (1000 / HZ))

#define time_after(a, b)     ((long)(b) - (long)(a) < 0)
#define time_before(a, b)    time_after(b, a)
#define time_after_eq(a, b)   ((long)(a) - (long)(b) >= 0)
#define time_before_eq(a, b)  time_after_eq(b, a)

#endif
