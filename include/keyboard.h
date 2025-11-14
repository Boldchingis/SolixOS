#ifndef SOLIX_KEYBOARD_H
#define SOLIX_KEYBOARD_H

#include "types.h"

// Keyboard scancodes (US layout)
#define SCAN_ESCAPE 1
#define SCAN_F1 59
#define SCAN_F2 60
#define SCAN_F3 61
#define SCAN_F4 62
#define SCAN_F5 63
#define SCAN_F6 64
#define SCAN_F7 65
#define SCAN_F8 66
#define SCAN_F9 67
#define SCAN_F10 68
#define SCAN_F11 87
#define SCAN_F12 88

// Special keys
#define KEY_BACKSPACE '\b'
#define KEY_TAB '\t'
#define KEY_ENTER '\n'
#define KEY_ESCAPE 27

// Keyboard functions
void keyboard_init(void);
char keyboard_getchar(void);
bool keyboard_available(void);
void keyboard_handler(void);

#endif
