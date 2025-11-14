#include "net.h"
#include "screen.h"
#include "timer.h"
#include <string.h>
#include <stdio.h>

// Simple HTTP client (curl-like)
int curl_main(int argc, char** argv) {
    if (argc != 2) {
        screen_print("Usage: curl <url>\n");
        return 1;
    }
    
    // Parse URL (simplified - only supports http://host/path)
    char* url = argv[1];
    if (strncmp(url, "http://", 7) != 0) {
        screen_print("Only HTTP URLs are supported\n");
        return 1;
    }
    
    url += 7;  // Skip "http://"
    
    // Extract host and path
    char host[256];
    char path[512];
    char* slash = strchr(url, '/');
    
    if (slash) {
        int host_len = slash - url;
        strncpy(host, url, host_len);
        host[host_len] = '\0';
        strcpy(path, slash);
    } else {
        strcpy(host, url);
        strcpy(path, "/");
    }
    
    // Resolve hostname (simplified - only supports IP addresses)
    uint32_t host_ip = ip_aton(host);
    if (host_ip == 0) {
        screen_print("Hostname resolution not supported, use IP address\n");
        return 1;
    }
    
    // Create TCP socket
    socket_t sock;
    sock.type = SOCK_STREAM;
    sock.protocol = IPPROTO_TCP;
    sock.local_ip = net_get_device("eth0")->ip_addr;
    sock.local_port = 12345;  // Random local port
    sock.remote_ip = host_ip;
    sock.remote_port = 80;    // HTTP port
    
    // Connect to server
    screen_print("Connecting to ");
    screen_print(host);
    screen_print("...\n");
    
    if (tcp_connect(&sock, host_ip, 80) != 0) {
        screen_print("Connection failed\n");
        return 1;
    }
    
    // Send HTTP request
    char request[1024];
    sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    
    if (tcp_send(&sock, request, strlen(request)) != 0) {
        screen_print("Failed to send request\n");
        return 1;
    }
    
    screen_print("HTTP request sent, waiting for response...\n");
    
    // Receive response
    char buffer[1024];
    int total_received = 0;
    
    while (1) {
        int bytes = tcp_receive(&sock, buffer, sizeof(buffer) - 1);
        if (bytes <= 0) break;
        
        buffer[bytes] = '\0';
        screen_print(buffer);
        total_received += bytes;
        
        if (bytes < sizeof(buffer) - 1) break;
    }
    
    screen_print("\nTotal received: ");
    screen_print_dec(total_received);
    screen_print(" bytes\n");
    
    return 0;
}
