#include "browser.h"
#include "net.h"
#include "screen.h"
#include "keyboard.h"
#include "timer.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// Global browser instance
static browser_t browser;
static bool browser_active = false;

// Initialize browser
void browser_init(void) {
    memset(&browser, 0, sizeof(browser));
    browser.cursor_x = 0;
    browser.cursor_y = 0;
    browser.showing_links = false;
    browser_active = false;
    
    screen_print("Lynx-like text browser initialized\n");
}

// Show browser interface
void browser_show(void) {
    if (!browser_active) {
        screen_clear();
        browser_active = true;
        browser_show_help();
    }
}

// Hide browser interface
void browser_hide(void) {
    browser_active = false;
    screen_clear();
    screen_print("Returning to shell...\n");
}

// Navigate to URL
void browser_navigate(const char* url) {
    if (!browser_active) {
        browser_show();
    }
    
    browser_status_message("Loading...");
    
    // Validate URL
    if (strncmp(url, "http://", 7) != 0) {
        browser_error_message("Only HTTP URLs are supported");
        return;
    }
    
    // Download page content
    char response[BROWSER_MAX_CONTENT];
    int bytes = browser_download_page(url, response, sizeof(response));
    
    if (bytes <= 0) {
        browser_error_message("Failed to download page");
        return;
    }
    
    // Parse and render page
    strcpy(browser.current_page.url, url);
    browser_parse_html(response, &browser.current_page);
    browser_render_page(&browser.current_page);
    
    // Add to history
    if (browser.history.count < BROWSER_HISTORY_SIZE) {
        strcpy(browser.history.urls[browser.history.count++], url);
        browser.history.current = browser.history.count - 1;
    }
    
    browser_status_message("Page loaded");
}

// Download page content
int browser_download_page(const char* url, char* buffer, size_t buffer_size) {
    // Parse URL
    char host[256];
    char path[512];
    
    if (!browser_parse_url(url, host, sizeof(host), path, sizeof(path))) {
        return -1;
    }
    
    // Create TCP connection
    socket_t sock;
    sock.type = SOCK_STREAM;
    sock.protocol = IPPROTO_TCP;
    sock.local_ip = net_get_device("eth0")->ip_addr;
    sock.local_port = 12345;
    sock.remote_ip = ip_aton(host);
    sock.remote_port = 80;
    
    if (tcp_connect(&sock, sock.remote_ip, 80) != 0) {
        return -1;
    }
    
    // Send HTTP request
    char request[1024];
    sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nUser-Agent: SolixOS-Lynx/1.0\r\n\r\n", path, host);
    
    if (tcp_send(&sock, request, strlen(request)) != 0) {
        return -1;
    }
    
    // Receive response
    int total_received = 0;
    char temp_buffer[1024];
    
    while (total_received < buffer_size - 1) {
        int bytes = tcp_receive(&sock, temp_buffer, sizeof(temp_buffer));
        if (bytes <= 0) break;
        
        if (total_received + bytes < buffer_size) {
            memcpy(buffer + total_received, temp_buffer, bytes);
            total_received += bytes;
        } else {
            break;
        }
    }
    
    buffer[total_received] = '\0';
    
    // Skip HTTP headers
    char* body = strstr(buffer, "\r\n\r\n");
    if (body) {
        body += 4;
        size_t body_len = strlen(body);
        memmove(buffer, body, body_len + 1);
        return body_len;
    }
    
    return 0;
}

// Parse URL into host and path
bool browser_parse_url(const char* url, char* host, size_t host_size, char* path, size_t path_size) {
    if (strncmp(url, "http://", 7) != 0) {
        return false;
    }
    
    const char* host_start = url + 7;
    const char* path_start = strchr(host_start, '/');
    
    if (path_start) {
        int host_len = path_start - host_start;
        if (host_len >= host_size) return false;
        strncpy(host, host_start, host_len);
        host[host_len] = '\0';
        
        if (strlen(path_start) >= path_size) return false;
        strcpy(path, path_start);
    } else {
        if (strlen(host_start) >= host_size) return false;
        strcpy(host, host_start);
        strcpy(path, "/");
    }
    
    return true;
}

// Parse HTML content
void browser_parse_html(const char* html, browser_page_t* page) {
    // Simple HTML parser - extracts text and links
    
    // Initialize page
    page->num_links = 0;
    page->current_line = 0;
    page->num_lines = 0;
    
    // Remove HTML tags and format text
    char formatted[BROWSER_MAX_CONTENT];
    browser_format_text(html, formatted, sizeof(formatted));
    
    // Copy formatted content
    if (strlen(formatted) < BROWSER_MAX_CONTENT) {
        strcpy(page->content, formatted);
    }
    
    // Extract links
    browser_extract_links(html, page);
    
    // Count lines
    char* ptr = page->content;
    page->num_lines = 1;
    while (*ptr) {
        if (*ptr == '\n') {
            page->num_lines++;
        }
        ptr++;
    }
}

// Format HTML text (remove tags)
void browser_format_text(const char* html, char* output, size_t output_size) {
    bool in_tag = false;
    bool in_script = false;
    int output_pos = 0;
    
    for (int i = 0; html[i] && output_pos < output_size - 1; i++) {
        if (strncmp(&html[i], "<script", 7) == 0) {
            in_script = true;
        } else if (strncmp(&html[i], "</script>", 9) == 0) {
            in_script = false;
            i += 8;
            continue;
        }
        
        if (in_script) continue;
        
        if (html[i] == '<') {
            in_tag = true;
        } else if (html[i] == '>') {
            in_tag = false;
        } else if (!in_tag) {
            // Replace multiple spaces with single space
            if (html[i] == ' ' && output_pos > 0 && output[output_pos - 1] == ' ') {
                continue;
            }
            
            // Handle newlines
            if (html[i] == '\n' || html[i] == '\r') {
                if (output_pos > 0 && output[output_pos - 1] != '\n') {
                    output[output_pos++] = '\n';
                }
                continue;
            }
            
            output[output_pos++] = html[i];
        }
    }
    
    output[output_pos] = '\0';
}

// Extract links from HTML
void browser_extract_links(const char* html, browser_page_t* page) {
    const char* ptr = html;
    int link_num = 1;
    
    while (*ptr && page->num_links < BROWSER_MAX_LINKS) {
        const char* href_start = strstr(ptr, "href=\"");
        if (!href_start) break;
        
        href_start += 6;  // Skip 'href="'
        const char* href_end = strchr(href_start, '"');
        if (!href_end) break;
        
        // Extract URL
        int url_len = href_end - href_start;
        if (url_len < BROWSER_MAX_URL) {
            strncpy(page->links[page->num_links].url, href_start, url_len);
            page->links[page->num_links].url[url_len] = '\0';
            
            // Extract link text (simplified)
            const char* text_start = href_end + 1;
            while (*text_start && *text_start != '<') text_start++;
            if (*text_start == '<') {
                text_start++;
                while (*text_start && isspace(*text_start)) text_start++;
            }
            
            const char* text_end = text_start;
            while (*text_end && *text_end != '<') text_end++;
            
            int text_len = text_end - text_start;
            if (text_len > 0 && text_len < 127) {
                strncpy(page->links[page->num_links].text, text_start, text_len);
                page->links[page->num_links].text[text_len] = '\0';
            } else {
                sprintf(page->links[page->num_links].text, "[%d]", link_num++);
            }
            
            page->links[page->num_links].line = -1;  // Will be set during rendering
            page->links[page->num_links].col = -1;
            page->num_links++;
        }
        
        ptr = href_end + 1;
    }
}

// Render page to screen
void browser_render_page(browser_page_t* page) {
    screen_clear();
    
    // Draw header
    screen_set_color(COLOR_BLUE, COLOR_WHITE);
    screen_print(page->url);
    screen_print("\n");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    screen_print("\n");
    
    // Render content
    char* ptr = page->content;
    int line = 0;
    int col = 0;
    int lines_rendered = 0;
    int max_lines = screen_get_height() - 4;  // Leave space for header and status
    
    // Skip to current line
    for (int i = 0; i < page->current_line && *ptr; i++) {
        while (*ptr && *ptr != '\n') ptr++;
        if (*ptr == '\n') ptr++;
    }
    
    // Render visible lines
    while (*ptr && lines_rendered < max_lines) {
        if (*ptr == '\n') {
            screen_print("\n");
            line++;
            col = 0;
            lines_rendered++;
            ptr++;
        } else {
            // Check if this is a link
            bool is_link = false;
            int link_idx = -1;
            
            for (int i = 0; i < page->num_links; i++) {
                if (page->links[i].line == line && page->links[i].col == col) {
                    is_link = true;
                    link_idx = i;
                    break;
                }
            }
            
            if (is_link) {
                screen_set_color(COLOR_BLUE, COLOR_BLACK);
                screen_print("[");
                screen_print(page->links[link_idx].text);
                screen_print("]");
                screen_set_color(COLOR_WHITE, COLOR_BLACK);
                col += strlen(page->links[link_idx].text) + 2;
            } else {
                screen_putc(*ptr);
                col++;
            }
            ptr++;
        }
    }
    
    // Draw status bar
    screen_set_cursor(0, screen_get_height() - 1);
    screen_set_color(COLOR_BLACK, COLOR_WHITE);
    
    char status[256];
    sprintf(status, "Line %d/%d | Links: %d | Press 'h' for help", 
            page->current_line + 1, page->num_lines, page->num_links);
    screen_print(status);
    
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
}

// Browser navigation functions
void browser_back(void) {
    if (browser.history.current > 0) {
        browser.history.current--;
        browser_navigate(browser.history.urls[browser.history.current]);
    }
}

void browser_forward(void) {
    if (browser.history.current < browser.history.count - 1) {
        browser.history.current++;
        browser_navigate(browser.history.urls[browser.history.current]);
    }
}

void browser_refresh(void) {
    if (strlen(browser.current_page.url) > 0) {
        browser_navigate(browser.current_page.url);
    }
}

void browser_scroll_up(void) {
    if (browser.current_page.current_line > 0) {
        browser.current_page.current_line--;
        browser_render_page(&browser.current_page);
    }
}

void browser_scroll_down(void) {
    if (browser.current_page.current_line < browser.current_page.num_lines - 1) {
        browser.current_page.current_line++;
        browser_render_page(&browser.current_page);
    }
}

void browser_page_up(void) {
    int lines = screen_get_height() - 4;
    browser.current_page.current_line = (browser.current_page.current_line > lines) ? 
                                       browser.current_page.current_line - lines : 0;
    browser_render_page(&browser.current_page);
}

void browser_page_down(void) {
    int lines = screen_get_height() - 4;
    browser.current_page.current_line = (browser.current_page.current_line + lines < browser.current_page.num_lines) ?
                                       browser.current_page.current_line + lines : 
                                       browser.current_page.num_lines - 1;
    browser_render_page(&browser.current_page);
}

void browser_home(void) {
    browser.current_page.current_line = 0;
    browser_render_page(&browser.current_page);
}

void browser_end(void) {
    browser.current_page.current_line = browser.current_page.num_lines - 1;
    browser_render_page(&browser.current_page);
}

// Follow link by number
void browser_follow_link(int link_num) {
    if (link_num > 0 && link_num <= browser.current_page.num_links) {
        browser_navigate(browser.current_page.links[link_num - 1].url);
    }
}

// Toggle link display
void browser_toggle_links(void) {
    browser.showing_links = !browser.showing_links;
    
    if (browser.showing_links) {
        screen_clear();
        screen_print("Available links:\n\n");
        
        for (int i = 0; i < browser.current_page.num_links; i++) {
            screen_print("[");
            screen_print_dec(i + 1);
            screen_print("] ");
            screen_print(browser.current_page.links[i].text);
            screen_print(" -> ");
            screen_print(browser.current_page.links[i].url);
            screen_print("\n");
        }
        
        screen_print("\nPress any key to return to page...");
        keyboard_getchar();
        browser.showing_links = false;
        browser_render_page(&browser.current_page);
    }
}

// Handle keyboard input
void browser_handle_input(char c) {
    switch (c) {
        case 'q':
            browser_hide();
            break;
        case 'h':
            browser_show_help();
            break;
        case 'r':
            browser_refresh();
            break;
        case 'b':
            browser_back();
            break;
        case 'f':
            browser_forward();
            break;
        case 'l':
            browser_toggle_links();
            break;
        case 'g':
            {
                screen_print("\nEnter URL: ");
                char url[BROWSER_MAX_URL];
                int pos = 0;
                
                while (pos < BROWSER_MAX_URL - 1) {
                    char input = keyboard_getchar();
                    if (input == '\n') break;
                    if (input == '\b' && pos > 0) {
                        pos--;
                    } else if (input >= 32 && input < 127) {
                        url[pos++] = input;
                        screen_putc(input);
                    }
                }
                url[pos] = '\0';
                screen_print("\n");
                
                if (strlen(url) > 0) {
                    browser_navigate(url);
                }
            }
            break;
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9':
            browser_follow_link(c - '0');
            break;
        case '0':
            browser_follow_link(10);
            break;
        case '\033':  // Arrow keys
            {
                char seq[2];
                if (keyboard_getchar() == '[') {
                    char arrow = keyboard_getchar();
                    switch (arrow) {
                        case 'A':  // Up
                            browser_scroll_up();
                            break;
                        case 'B':  // Down
                            browser_scroll_down();
                            break;
                        case '5':  // Page Up
                            if (keyboard_getchar() == '~') browser_page_up();
                            break;
                        case '6':  // Page Down
                            if (keyboard_getchar() == '~') browser_page_down();
                            break;
                    }
                }
            }
            break;
        case ' ':
            browser_page_down();
            break;
    }
}

// Show help screen
void browser_show_help(void) {
    screen_clear();
    screen_print("SolixOS Text Browser - Help\n\n");
    screen_print("Navigation:\n");
    screen_print("  g        - Go to URL\n");
    screen_print("  r        - Refresh page\n");
    screen_print("  b        - Go back\n");
    screen_print("  f        - Go forward\n");
    screen_print("  l        - List links\n");
    screen_print("  q        - Quit browser\n\n");
    screen_print("Scrolling:\n");
    screen_print("  Up/Down  - Scroll one line\n");
    screen_print("  Space    - Page down\n");
    screen_print("  Home/End - Go to top/bottom\n\n");
    screen_print("Links:\n");
    screen_print("  0-9      - Follow link number\n");
    screen_print("  h        - Show this help\n\n");
    screen_print("Press any key to continue...");
    keyboard_getchar();
    browser_render_page(&browser.current_page);
}

// Status message
void browser_status_message(const char* message) {
    screen_set_cursor(0, screen_get_height() - 1);
    screen_set_color(COLOR_BLACK, COLOR_WHITE);
    screen_print(message);
    for (int i = strlen(message); i < screen_get_width(); i++) {
        screen_putc(' ');
    }
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
}

// Error message
void browser_error_message(const char* error) {
    screen_set_color(COLOR_RED, COLOR_BLACK);
    screen_print("Error: ");
    screen_print(error);
    screen_print("\n");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    timer_wait(2000);
}

// Main browser loop
int lynx_main(int argc, char** argv) {
    browser_init();
    
    if (argc == 2) {
        browser_navigate(argv[1]);
    } else {
        browser_show();
    }
    
    while (browser_active) {
        char c = keyboard_getchar();
        browser_handle_input(c);
    }
    
    return 0;
}
