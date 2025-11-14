#include "keyboard.h"
#include "screen.h"
#include "interrupts.h"

// Keyboard state
static char keyboard_buffer[256];
static int buffer_head = 0;
static int buffer_tail = 0;
static bool shift_pressed = false;
static bool caps_lock = false;

// US keyboard layout
static const char scancode_to_char[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+',
    0, 0, 0, 0, 0, 0, 0
};

// Shifted keyboard layout
static const char scancode_to_char_shift[128] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+',
    0, 0, 0, 0, 0, 0, 0
};

// Initialize keyboard
void keyboard_init(void) {
    // Register keyboard interrupt handler
    irq_register_handler(IRQ_KEYBOARD, keyboard_handler);
    
    // Clear buffer
    buffer_head = 0;
    buffer_tail = 0;
    shift_pressed = false;
    caps_lock = false;
}

// Keyboard interrupt handler
void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    
    // Handle key release (high bit set)
    if (scancode & 0x80) {
        scancode &= 0x7F;
        
        // Handle special key releases
        switch (scancode) {
            case 42:  // Left shift
            case 54:  // Right shift
                shift_pressed = false;
                break;
            case 58:  // Caps lock
                caps_lock = !caps_lock;
                break;
        }
    } else {
        // Key press
        char c = 0;
        
        // Handle special keys
        switch (scancode) {
            case 42:  // Left shift
            case 54:  // Right shift
                shift_pressed = true;
                break;
            case 14:  // Backspace
                c = KEY_BACKSPACE;
                break;
            case 15:  // Tab
                c = KEY_TAB;
                break;
            case 28:  // Enter
                c = KEY_ENTER;
                break;
            case 1:   // Escape
                c = KEY_ESCAPE;
                break;
            default:
                // Regular character
                if (scancode < 128) {
                    if (shift_pressed ^ caps_lock) {
                        c = scancode_to_char_shift[scancode];
                    } else {
                        c = scancode_to_char[scancode];
                    }
                }
                break;
        }
        
        // Add character to buffer
        if (c != 0) {
            int next_head = (buffer_head + 1) % 256;
            if (next_head != buffer_tail) {
                keyboard_buffer[buffer_head] = c;
                buffer_head = next_head;
            }
        }
    }
}

// Get character from keyboard buffer
char keyboard_getchar(void) {
    while (buffer_head == buffer_tail) {
        // Wait for key press
        __asm__ volatile("hlt");
    }
    
    char c = keyboard_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % 256;
    
    return c;
}

// Check if keyboard input is available
bool keyboard_available(void) {
    return buffer_head != buffer_tail;
}
