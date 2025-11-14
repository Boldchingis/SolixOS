#include "net.h"
#include "screen.h"
#include "timer.h"
#include <string.h>
#include <stdio.h>

// Ping command implementation
int ping_main(int argc, char** argv) {
    if (argc != 2) {
        screen_print("Usage: ping <ip_address>\n");
        return 1;
    }
    
    uint32_t target_ip = ip_aton(argv[1]);
    if (target_ip == 0) {
        screen_print("Invalid IP address\n");
        return 1;
    }
    
    screen_print("PING ");
    screen_print(argv[1]);
    screen_print(" (");
    screen_print(argv[1]);
    screen_print(")\n");
    
    // Send 4 ping packets
    for (int i = 0; i < 4; i++) {
        uint32_t start_time = timer_get_ticks();
        
        if (icmp_ping(target_ip) == 0) {
            screen_print("Reply from ");
            screen_print(argv[1]);
            screen_print(": time=");
            screen_print_dec(timer_get_ticks() - start_time);
            screen_print("ms\n");
        } else {
            screen_print("Request timed out\n");
        }
        
        timer_wait(100);  // Wait 1 second between pings
    }
    
    return 0;
}
