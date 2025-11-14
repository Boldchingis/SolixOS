#ifndef SOLIX_WIFI_H
#define SOLIX_WIFI_H

#include "types.h"
#include "net.h"

// WiFi constants
#define WIFI_MAX_SSID_LEN 32
#define WIFI_MAX_BSSID_LEN 6
#define WIFI_MAX_NETWORKS 32
#define WIFI_SCAN_TIMEOUT 5000

// WiFi security types
#define WIFI_SECURITY_OPEN 0
#define WIFI_SECURITY_WEP 1
#define WIFI_SECURITY_WPA 2
#define WIFI_SECURITY_WPA2 3

// WiFi network structure
typedef struct wifi_network {
    char ssid[WIFI_MAX_SSID_LEN + 1];
    uint8_t bssid[WIFI_MAX_BSSID_LEN];
    uint8_t security;
    uint8_t channel;
    int8_t rssi;
    bool connected;
} wifi_network_t;

// WiFi device structure
typedef struct wifi_device {
    char name[16];
    uint8_t mac[WIFI_MAX_BSSID_LEN];
    bool scanning;
    bool connected;
    wifi_network_t current_network;
    void* private_data;
    
    // Device operations
    int (*init)(struct wifi_device* dev);
    int (*scan)(struct wifi_device* dev, wifi_network_t* networks, int max_count);
    int (*connect)(struct wifi_device* dev, const char* ssid, const char* password);
    int (*disconnect)(struct wifi_device* dev);
    int (*get_status)(struct wifi_device* dev);
} wifi_device_t;

// WiFi functions
void wifi_init(void);
int wifi_register_device(wifi_device_t* dev);
int wifi_unregister_device(wifi_device_t* dev);
wifi_device_t* wifi_get_device(const char* name);

// WiFi operations
int wifi_scan(wifi_network_t* networks, int max_count);
int wifi_connect(const char* ssid, const char* password);
int wifi_disconnect(void);
int wifi_get_status(void);

// WiFi utility functions
const char* wifi_security_to_string(uint8_t security);
int wifi_rssi_to_percent(int8_t rssi);

#endif
