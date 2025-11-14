#include "net.h"
#include "screen.h"
#include <string.h>
#include <stdio.h>

// Network interface configuration
int ifconfig_main(int argc, char** argv) {
    if (argc == 1) {
        // Show all interfaces
        screen_print("Interface      IP Address      Netmask         Gateway         MAC Address\n");
        screen_print("-----------------------------------------------------------------------------\n");
        
        for (int i = 0; i < 16; i++) {
            net_device_t* dev = net_get_device_by_index(i);
            if (!dev) break;
            
            screen_print(dev->name);
            screen_print("            ");
            
            char ip_str[16];
            ip_ntoa(dev->ip_addr, ip_str);
            screen_print(ip_str);
            screen_print("      ");
            
            ip_ntoa(dev->netmask, ip_str);
            screen_print(ip_str);
            screen_print("      ");
            
            ip_ntoa(dev->gateway, ip_str);
            screen_print(ip_str);
            screen_print("      ");
            
            // Print MAC address
            for (int j = 0; j < 6; j++) {
                if (j > 0) screen_print(":");
                screen_print_hex(dev->mac[j]);
            }
            screen_print("\n");
        }
        return 0;
    }
    
    if (argc < 3) {
        screen_print("Usage: ifconfig <interface> <ip_address> [netmask] [gateway]\n");
        return 1;
    }
    
    net_device_t* dev = net_get_device(argv[1]);
    if (!dev) {
        screen_print("Interface not found: ");
        screen_print(argv[1]);
        screen_print("\n");
        return 1;
    }
    
    // Set IP address
    dev->ip_addr = ip_aton(argv[2]);
    
    // Set netmask if provided
    if (argc >= 4) {
        dev->netmask = ip_aton(argv[3]);
    }
    
    // Set gateway if provided
    if (argc >= 5) {
        dev->gateway = ip_aton(argv[4]);
    }
    
    screen_print("Interface ");
    screen_print(dev->name);
    screen_print(" configured\n");
    
    return 0;
}

// Helper function to get device by index
net_device_t* net_get_device_by_index(int index) {
    if (index < 0 || index >= 16) return NULL;
    
    int count = 0;
    for (int i = 0; i < 16; i++) {
        if (devices[i]) {
            if (count == index) {
                return devices[i];
            }
            count++;
        }
    }
    return NULL;
}
