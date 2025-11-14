#include "wifi.h"
#include "screen.h"
#include "mm.h"
#include "timer.h"
#include <string.h>
#include <stdio.h>

// WiFi state
static wifi_device_t* wifi_devices[8];
static int num_wifi_devices = 0;

// Initialize WiFi subsystem
void wifi_init(void) {
    memset(wifi_devices, 0, sizeof(wifi_devices));
    screen_print("WiFi subsystem initialized\n");
}

// Register WiFi device
int wifi_register_device(wifi_device_t* dev) {
    if (num_wifi_devices >= 8) {
        return -1;
    }
    
    wifi_devices[num_wifi_devices++] = dev;
    
    screen_print("Registered WiFi device: ");
    screen_print(dev->name);
    screen_print("\n");
    
    return 0;
}

// Unregister WiFi device
int wifi_unregister_device(wifi_device_t* dev) {
    for (int i = 0; i < num_wifi_devices; i++) {
        if (wifi_devices[i] == dev) {
            wifi_devices[i] = wifi_devices[--num_wifi_devices];
            return 0;
        }
    }
    return -1;
}

// Get WiFi device by name
wifi_device_t* wifi_get_device(const char* name) {
    for (int i = 0; i < num_wifi_devices; i++) {
        if (strcmp(wifi_devices[i]->name, name) == 0) {
            return wifi_devices[i];
        }
    }
    return NULL;
}

// Scan for WiFi networks
int wifi_scan(wifi_network_t* networks, int max_count) {
    if (num_wifi_devices == 0) {
        return -1;
    }
    
    wifi_device_t* dev = wifi_devices[0];
    if (!dev->scan) {
        return -1;
    }
    
    return dev->scan(dev, networks, max_count);
}

// Connect to WiFi network
int wifi_connect(const char* ssid, const char* password) {
    if (num_wifi_devices == 0) {
        return -1;
    }
    
    wifi_device_t* dev = wifi_devices[0];
    if (!dev->connect) {
        return -1;
    }
    
    screen_print("Connecting to ");
    screen_print(ssid);
    screen_print("...\n");
    
    int result = dev->connect(dev, ssid, password);
    
    if (result == 0) {
        screen_print("Connected to ");
        screen_print(ssid);
        screen_print("\n");
    } else {
        screen_print("Failed to connect to ");
        screen_print(ssid);
        screen_print("\n");
    }
    
    return result;
}

// Disconnect from WiFi network
int wifi_disconnect(void) {
    if (num_wifi_devices == 0) {
        return -1;
    }
    
    wifi_device_t* dev = wifi_devices[0];
    if (!dev->disconnect) {
        return -1;
    }
    
    int result = dev->disconnect(dev);
    
    if (result == 0) {
        screen_print("Disconnected from WiFi\n");
    }
    
    return result;
}

// Get WiFi status
int wifi_get_status(void) {
    if (num_wifi_devices == 0) {
        return -1;
    }
    
    wifi_device_t* dev = wifi_devices[0];
    if (!dev->get_status) {
        return -1;
    }
    
    return dev->get_status(dev);
}

// Utility functions
const char* wifi_security_to_string(uint8_t security) {
    switch (security) {
        case WIFI_SECURITY_OPEN:
            return "Open";
        case WIFI_SECURITY_WEP:
            return "WEP";
        case WIFI_SECURITY_WPA:
            return "WPA";
        case WIFI_SECURITY_WPA2:
            return "WPA2";
        default:
            return "Unknown";
    }
}

int wifi_rssi_to_percent(int8_t rssi) {
    if (rssi >= -50) return 100;
    if (rssi >= -60) return 80;
    if (rssi >= -70) return 60;
    if (rssi >= -80) return 40;
    if (rssi >= -90) return 20;
    return 10;
}
