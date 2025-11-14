#include "screen.h"
#include "kernel.h"

// Screen state
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static uint8_t current_color = COLOR_LIGHT_GREY | (COLOR_BLACK << 4);
static uint16_t* video_memory = (uint16_t*)VIDEO_MEMORY;

// Initialize screen
void screen_init(void) {
    screen_clear();
    screen_set_color(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_set_cursor(0, 0);
}

// Clear screen
void screen_clear(void) {
    for (int i = 0; i < SCREEN_SIZE; i++) {
        video_memory[i] = (current_color << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
    screen_update_cursor();
}

// Put character at current position
void screen_putc(char c) {
    switch (c) {
        case '\n':
            cursor_x = 0;
            cursor_y++;
            break;
        case '\r':
            cursor_x = 0;
            break;
        case '\t':
            cursor_x = (cursor_x + 8) & ~7;
            break;
        case '\b':
            if (cursor_x > 0) {
                cursor_x--;
            }
            break;
        default:
            // Print character
            video_memory[cursor_y * SCREEN_WIDTH + cursor_x] = (current_color << 8) | c;
            cursor_x++;
            break;
    }
    
    // Handle line overflow
    if (cursor_x >= SCREEN_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    // Handle screen overflow
    if (cursor_y >= SCREEN_HEIGHT) {
        screen_scroll_up();
        cursor_y = SCREEN_HEIGHT - 1;
    }
    
    screen_update_cursor();
}

// Print string
void screen_print(const char* str) {
    while (*str) {
        screen_putc(*str++);
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

// Scroll screen up one line
void screen_scroll_up(void) {
    // Move all lines up
    for (int i = 0; i < (SCREEN_HEIGHT - 1) * SCREEN_WIDTH; i++) {
        video_memory[i] = video_memory[i + SCREEN_WIDTH];
    }
    
    // Clear last line
    for (int i = (SCREEN_HEIGHT - 1) * SCREEN_WIDTH; i < SCREEN_SIZE; i++) {
        video_memory[i] = (current_color << 8) | ' ';
    }
}

// Update hardware cursor
void screen_update_cursor(void) {
    uint16_t pos = cursor_y * SCREEN_WIDTH + cursor_x;
    
    // Send position to VGA controller
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}
