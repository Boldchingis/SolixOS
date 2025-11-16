#include "screen.h"
#include "kernel.h"

/**
 * Enhanced Screen Driver
 * Optimized for performance with buffering and batch operations
 */

// Screen state with performance optimizations
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static uint8_t current_color = COLOR_LIGHT_GREY | (COLOR_BLACK << 4); 
static uint16_t* video_memory = (uint16_t*)VIDEO_MEMORY;

// Performance optimization: write buffer for batch operations
#define WRITE_BUFFER_SIZE 256
static struct {
    uint16_t buffer[WRITE_BUFFER_SIZE];
    uint32_t count;
    uint32_t start_pos;
} write_buffer = {0};

// Performance statistics
static struct {
    uint32_t chars_written;
    uint32_t screen_clears;
    uint32_t scrolls;
    uint32_t buffer_flushes;
} screen_stats = {0};

// Dirty flag for cursor updates
static bool cursor_dirty = false;

/**
 * Flush write buffer to video memory
 */
static inline void flush_write_buffer(void) {
    if (write_buffer.count == 0) return;
    
    // Copy buffer to video memory in one operation
    for (uint32_t i = 0; i < write_buffer.count; i++) {
        video_memory[write_buffer.start_pos + i] = write_buffer.buffer[i];
    }
    
    write_buffer.count = 0;
    write_buffer.start_pos = 0;
    screen_stats.buffer_flushes++;
}

/**
 * Add character to write buffer
 */
static inline void buffer_write_char(uint16_t pos, uint16_t value) {
    if (write_buffer.count >= WRITE_BUFFER_SIZE || 
        (write_buffer.count > 0 && pos != write_buffer.start_pos + write_buffer.count)) {
        flush_write_buffer();
    }
    
    if (write_buffer.count == 0) {
        write_buffer.start_pos = pos;
    }
    
    write_buffer.buffer[write_buffer.count++] = value;
}

/**
 * Initialize screen with performance monitoring
 */
void screen_init(void) {
    screen_clear();
    screen_set_color(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_set_cursor(0, 0);
    
    // Initialize statistics
    screen_stats.chars_written = 0;
    screen_stats.screen_clears = 0;
    screen_stats.scrolls = 0;
    screen_stats.buffer_flushes = 0;
}

/**
 * Optimized screen clear using memset-style operation
 */
void screen_clear(void) {
    flush_write_buffer(); // Ensure all pending writes are flushed
    
    // Use 32-bit writes for better performance
    uint32_t* mem32 = (uint32_t*)video_memory;
    uint32_t fill_value = (current_color << 8) | ' ';
    fill_value |= fill_value << 16; // Duplicate for 32-bit write
    
    for (int i = 0; i < SCREEN_SIZE / 2; i++) {
        mem32[i] = fill_value;
    }
    
    cursor_x = 0;
    cursor_y = 0;
    cursor_dirty = true;
    screen_stats.screen_clears++;
}

/**
 * Optimized character output with buffering
 */
void screen_putc(char c) {
    uint16_t pos = cursor_y * SCREEN_WIDTH + cursor_x;
    uint16_t char_value = (current_color << 8) | c;
    
    switch (c) {
        case '\n':
            flush_write_buffer();
            cursor_x = 0;
            cursor_y++;
            break;
        case '\r':
            flush_write_buffer();
            cursor_x = 0;
            break;
        case '\t':
            flush_write_buffer();
            cursor_x = (cursor_x + 8) & ~7;
            break;
        case '\b':
            flush_write_buffer();
            if (cursor_x > 0) {
                cursor_x--;
                pos = cursor_y * SCREEN_WIDTH + cursor_x;
                buffer_write_char(pos, (current_color << 8) | ' ');
            }
            break;
        default:
            // Buffer character for batch writing
            buffer_write_char(pos, char_value);
            cursor_x++;
            screen_stats.chars_written++;
            break;
    }

    // Handle line overflow
    if (cursor_x >= SCREEN_WIDTH) {
        flush_write_buffer();
        cursor_x = 0;
        cursor_y++;
    }

    // Handle screen overflow
    if (cursor_y >= SCREEN_HEIGHT) {
        flush_write_buffer();
        screen_scroll_up();
        cursor_y = SCREEN_HEIGHT - 1;
    }

    cursor_dirty = true;
}

/**
 * Optimized string printing with length checking
 */
void screen_print(const char* str) {
    if (!str) return;
    
    // Batch process strings for better performance
    while (*str) {
        screen_putc(*str++);
    }
}

/**
 * Print string with maximum length (safer)
 */
void screen_print_n(const char* str, size_t max_len) {
    if (!str || max_len == 0) return;
    
    for (size_t i = 0; i < max_len && str[i]; i++) {
        screen_putc(str[i]);
    }
}

// Print hexadecimal value
void screen_print_hex(uint32_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    
    screen_print("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t digit = (value >> i) & 0xF;
        screen_putc(hex_chars[digit]);
    }
}

// Print decimal value
void screen_print_dec(uint32_t value) {
    if (value == 0) {
        screen_putc('0');
        return;
    }
    
    char buffer[11];
    int i = 10;
    buffer[i] = '\0';
    i--;
    
    while (value > 0) {
        buffer[i] = '0' + (value % 10);
        value /= 10;
        i--;
    }
    
    screen_print(&buffer[i + 1]);
}

// Set text color
void screen_set_color(uint8_t fg, uint8_t bg) {
    current_color = fg | (bg << 4);
}

// Set cursor position
void screen_set_cursor(uint8_t x, uint8_t y) {
    if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
        cursor_x = x;
        cursor_y = y;
        screen_update_cursor();
    }
}

// Get cursor position
void screen_get_cursor(uint8_t* x, uint8_t* y) {
    *x = cursor_x;
    *y = cursor_y;
}

/**
 * Optimized screen scroll using memcpy-style operation
 */
void screen_scroll_up(void) {
    flush_write_buffer(); // Ensure all pending writes are flushed
    
    // Use 32-bit operations for better performance
    uint32_t* src = (uint32_t*)(video_memory + SCREEN_WIDTH);
    uint32_t* dst = (uint32_t*)video_memory;
    uint32_t count = (SCREEN_HEIGHT - 1) * SCREEN_WIDTH / 2;
    
    // Move lines up in 32-bit chunks
    for (uint32_t i = 0; i < count; i++) {
        dst[i] = src[i];
    }

    // Clear last line efficiently
    uint32_t fill_value = (current_color << 8) | ' ';
    fill_value |= fill_value << 16;
    uint32_t* last_line = (uint32_t*)(video_memory + (SCREEN_HEIGHT - 1) * SCREEN_WIDTH);
    
    for (int i = 0; i < SCREEN_WIDTH / 2; i++) {
        last_line[i] = fill_value;
    }
    
    screen_stats.scrolls++;
}

/**
 * Lazy cursor update - only update when actually needed
 */
void screen_update_cursor(void) {
    if (!cursor_dirty) return;
    
    flush_write_buffer(); // Ensure all writes are flushed before cursor update
    
    uint16_t pos = cursor_y * SCREEN_WIDTH + cursor_x;

    // Send position to VGA controller
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
    
    cursor_dirty = false;
}

/**
 * Force immediate cursor update
 */
void screen_update_cursor_now(void) {
    cursor_dirty = true;
    screen_update_cursor();
}

/**
 * Get screen performance statistics
 */
void screen_get_stats(uint32_t* chars, uint32_t* clears, uint32_t* scrolls, uint32_t* flushes) {
    if (chars) *chars = screen_stats.chars_written;
    if (clears) *clears = screen_stats.screen_clears;
    if (scrolls) *scrolls = screen_stats.scrolls;
    if (flushes) *flushes = screen_stats.buffer_flushes;
}

/**
 * Reset screen statistics
 */
void screen_reset_stats(void) {
    screen_stats.chars_written = 0;
    screen_stats.screen_clears = 0;
    screen_stats.scrolls = 0;
    screen_stats.buffer_flushes = 0;
}
