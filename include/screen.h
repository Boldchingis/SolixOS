#ifndef SOLIX_SCREEN_H
#define SOLIX_SCREEN_H

#include "types.h"

// Screen dimensions
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)

// Video memory address
#define VIDEO_MEMORY 0xB8000

// Colors
#define COLOR_BLACK 0
#define COLOR_BLUE 1
#define COLOR_GREEN 2
#define COLOR_CYAN 3
#define COLOR_RED 4
#define COLOR_MAGENTA 5
#define COLOR_BROWN 6
#define COLOR_LIGHT_GREY 7
#define COLOR_DARK_GREY 8
#define COLOR_LIGHT_BLUE 9
#define COLOR_LIGHT_GREEN 10
#define COLOR_LIGHT_CYAN 11
#define COLOR_LIGHT_RED 12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_LIGHT_BROWN 14
#define COLOR_WHITE 15

// Screen functions
void screen_init(void);
void screen_clear(void);
void screen_putc(char c);
void screen_print(const char* str);
void screen_print_hex(uint32_t value);
void screen_print_dec(uint32_t value);
void screen_set_color(uint8_t fg, uint8_t bg);
void screen_set_cursor(uint8_t x, uint8_t y);
void screen_get_cursor(uint8_t* x, uint8_t* y);
void screen_scroll_up(void);

#endif
