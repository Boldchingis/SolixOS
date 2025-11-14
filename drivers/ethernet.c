#include "net.h"
#include "screen.h"
#include "mm.h"
#include "interrupts.h"
#include <string.h>

// RTL8139 network card driver

#define RTL8139_PORT_BASE 0xC000
#define RTL8139_IRQ 32

// RTL8139 registers
#define RTL8139_IDR 0x00
#define RTL8139_CONFIG 0x52
#define RTL8139_CMD 0x37
#define RTL8139_CAPR 0x38
#define RTL8139_CBA 0x30
#define RTL8139_IMR 0x3C
#define RTL8139_ISR 0x3E
#define RTL8139_TSD 0x10
#define RTL8139_TCR 0x40
#define RTL8139_RCR 0x44
#define RTL8139_MPC 0x44
#define RTL8139_RBSTART 0x30
#define RTL8139_RBSTOP 0x34
#define RTL8139_RBLEN 0x3A

// RTL8139 commands
#define RTL8139_CMD_RESET 0x10
#define RTL8139_CMD_TX_ENABLE 0x04
#define RTL8139_CMD_RX_ENABLE 0x08

// RTL8139 receive buffer size
#define RTL8139_RX_BUFFER_SIZE 8192
#define RTL8139_TX_BUFFER_SIZE 1536

static net_device_t rtl8139_dev;
static uint8_t* rx_buffer;
static uint8_t* tx_buffers[4];
static int current_tx_buffer = 0;

// PCI configuration space access
static uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    
    outl(0xCF8, address);
    return (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
}

static void pci_config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    
    outl(0xCF8, address);
    outw(0xCFC, value);
}

// Find RTL8139 device
static bool find_rtl8139(uint8_t* bus, uint8_t* slot, uint8_t* func) {
    for (uint8_t b = 0; b < 256; b++) {
        for (uint8_t s = 0; s < 32; s++) {
            for (uint8_t f = 0; f < 8; f++) {
                uint16_t vendor = pci_config_read_word(b, s, f, 0x00);
                uint16_t device = pci_config_read_word(b, s, f, 0x02);
                
                if (vendor == 0x10EC && device == 0x8139) {
                    *bus = b;
                    *slot = s;
                    *func = f;
                    return true;
                }
            }
        }
    }
    return false;
}

// RTL8139 device operations
static int rtl8139_open(net_device_t* dev) {
    // Allocate receive buffer
    rx_buffer = kmalloc(RTL8139_RX_BUFFER_SIZE);
    if (!rx_buffer) {
        return -1;
    }
    
    // Allocate transmit buffers
    for (int i = 0; i < 4; i++) {
        tx_buffers[i] = kmalloc(RTL8139_TX_BUFFER_SIZE);
        if (!tx_buffers[i]) {
            return -1;
        }
    }
    
    // Reset the card
    outb(RTL8139_PORT_BASE + RTL8139_CMD, RTL8139_CMD_RESET);
    while (inb(RTL8139_PORT_BASE + RTL8139_CMD) & RTL8139_CMD_RESET) {
        // Wait for reset to complete
    }
    
    // Set receive buffer
    outl(RTL8139_PORT_BASE + RTL8139_RBSTART, (uint32_t)rx_buffer);
    
    // Enable receive and transmit
    outb(RTL8139_PORT_BASE + RTL8139_CMD, RTL8139_CMD_TX_ENABLE | RTL8139_CMD_RX_ENABLE);
    
    // Set receive configuration
    outl(RTL8139_PORT_BASE + RTL8139_RCR, 0x0F | (1 << 7));  // Accept all packets
    
    // Enable interrupts
    outw(RTL8139_PORT_BASE + RTL8139_IMR, 0x0005);  // RX OK and TX OK
    
    dev->up = true;
    
    screen_print("RTL8139 network card initialized\n");
    return 0;
}

static int rtl8139_close(net_device_t* dev) {
    // Disable card
    outb(RTL8139_PORT_BASE + RTL8139_CMD, 0x00);
    
    // Free buffers
    kfree(rx_buffer);
    for (int i = 0; i < 4; i++) {
        kfree(tx_buffers[i]);
    }
    
    dev->up = false;
    return 0;
}

static int rtl8139_transmit(net_device_t* dev, void* data, size_t len) {
    if (len > RTL8139_TX_BUFFER_SIZE) {
        return -1;
    }
    
    // Copy data to transmit buffer
    memcpy(tx_buffers[current_tx_buffer], data, len);
    
    // Set transmit status descriptor
    outl(RTL8139_PORT_BASE + RTL8139_TSD + current_tx_buffer * 4, len);
    
    // Move to next buffer
    current_tx_buffer = (current_tx_buffer + 1) % 4;
    
    return 0;
}

static void rtl8139_receive(net_device_t* dev, void* data, size_t len) {
    // This is called by the interrupt handler
    // Actual packet processing is done in the interrupt handler
}

// RTL8139 interrupt handler
static void rtl8139_handler(interrupt_frame_t* frame) {
    uint16_t status = inw(RTL8139_PORT_BASE + RTL8139_ISR);
    
    if (status & 0x01) {  // RX OK
        // Process received packets
        uint16_t capr = inw(RTL8139_PORT_BASE + RTL8139_CAPR);
        uint16_t cbr = inw(RTL8139_PORT_BASE + RTL8139_CBA);
        
        while (capr != cbr) {
            // Get packet header
            uint16_t* header = (uint16_t*)(rx_buffer + capr + 2);
            uint16_t packet_len = ntohs(header[0]);
            uint16_t packet_status = ntohs(header[1]);
            
            if (packet_len > 0 && packet_len < RTL8139_RX_BUFFER_SIZE) {
                // Pass packet to network stack
                eth_receive(&rtl8139_dev, rx_buffer + capr + 4, packet_len - 4);
            }
            
            // Move to next packet
            capr = (capr + packet_len + 4 + 3) & ~3;
            if (capr >= RTL8139_RX_BUFFER_SIZE) {
                capr -= RTL8139_RX_BUFFER_SIZE;
            }
            
            outw(RTL8139_PORT_BASE + RTL8139_CAPR, capr - 16);
        }
    }
    
    if (status & 0x04) {  // TX OK
        // Transmit complete
    }
    
    // Acknowledge interrupts
    outw(RTL8139_PORT_BASE + RTL8139_ISR, status);
}

// Initialize RTL8139 driver
int rtl8139_init(void) {
    uint8_t bus, slot, func;
    
    if (!find_rtl8139(&bus, &slot, &func)) {
        screen_print("RTL8139 network card not found\n");
        return -1;
    }
    
    // Enable bus mastering
    uint16_t command = pci_config_read_word(bus, slot, func, 0x04);
    command |= 0x04;  // Enable bus mastering
    pci_config_write_word(bus, slot, func, 0x04, command);
    
    // Get MAC address
    uint32_t mac0 = inl(RTL8139_PORT_BASE + RTL8139_IDR);
    uint16_t mac4 = inw(RTL8139_PORT_BASE + RTL8139_IDR + 4);
    
    rtl8139_dev.mac[0] = mac0 & 0xFF;
    rtl8139_dev.mac[1] = (mac0 >> 8) & 0xFF;
    rtl8139_dev.mac[2] = (mac0 >> 16) & 0xFF;
    rtl8139_dev.mac[3] = (mac0 >> 24) & 0xFF;
    rtl8139_dev.mac[4] = mac4 & 0xFF;
    rtl8139_dev.mac[5] = (mac4 >> 8) & 0xFF;
    
    // Set up device structure
    strcpy(rtl8139_dev.name, "eth0");
    rtl8139_dev.ip_addr = ip_aton("192.168.1.100");
    rtl8139_dev.netmask = ip_aton("255.255.255.0");
    rtl8139_dev.gateway = ip_aton("192.168.1.1");
    rtl8139_dev.up = false;
    rtl8139_dev.open = rtl8139_open;
    rtl8139_dev.close = rtl8139_close;
    rtl8139_dev.transmit = rtl8139_transmit;
    rtl8139_dev.receive = rtl8139_receive;
    
    // Register interrupt handler
    register_interrupt_handler(RTL8139_IRQ, rtl8139_handler);
    
    // Register device with network stack
    net_register_device(&rtl8139_dev);
    
    return 0;
}

// I/O port functions
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t value;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline uint32_t inl(uint16_t port) {
    uint32_t value;
    __asm__ volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
