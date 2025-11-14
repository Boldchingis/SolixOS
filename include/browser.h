#ifndef SOLIX_BROWSER_H
#define SOLIX_BROWSER_H

#include "types.h"
#include "screen.h"

// Browser constants
#define BROWSER_MAX_URL 256
#define BROWSER_MAX_TITLE 128
#define BROWSER_MAX_CONTENT 65536
#define BROWSER_MAX_LINKS 256
#define BROWSER_MAX_LINE_LEN 256
#define BROWSER_HISTORY_SIZE 50

// Link structure
typedef struct browser_link {
    char url[BROWSER_MAX_URL];
    char text[128];
    int line;
    int col;
} browser_link_t;

// Browser page structure
typedef struct browser_page {
    char url[BROWSER_MAX_URL];
    char title[BROWSER_MAX_TITLE];
    char content[BROWSER_MAX_CONTENT];
    browser_link_t links[BROWSER_MAX_LINKS];
    int num_links;
    int current_line;
    int num_lines;
} browser_page_t;

// Browser history
typedef struct browser_history {
    char urls[BROWSER_HISTORY_SIZE][BROWSER_MAX_URL];
    int current;
    int count;
} browser_history_t;

// Browser structure
typedef struct browser {
    browser_page_t current_page;
    browser_history_t history;
    int cursor_x;
    int cursor_y;
    bool showing_links;
} browser_t;

// Browser functions
void browser_init(void);
void browser_show(void);
void browser_hide(void);
void browser_navigate(const char* url);
void browser_back(void);
void browser_forward(void);
void browser_refresh(void);
void browser_scroll_up(void);
void browser_scroll_down(void);
void browser_page_up(void);
void browser_page_down(void);
void browser_home(void);
void browser_end(void);
void browser_follow_link(int link_num);
void browser_toggle_links(void);
void browser_handle_input(char c);

// HTML parsing functions
void browser_parse_html(const char* html, browser_page_t* page);
void browser_render_page(browser_page_t* page);
void browser_extract_links(const char* html, browser_page_t* page);
void browser_format_text(const char* text, char* output, size_t output_size);

// Browser utility functions
void browser_status_message(const char* message);
void browser_error_message(const char* error);
void browser_show_help(void);

#endif
