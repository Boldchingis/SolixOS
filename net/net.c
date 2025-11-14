#include "net.h"
#include "screen.h"
#include "mm.h"
#include "timer.h"
#include <string.h>
#include <stdio.h>

// Network state
static net_device_t* devices[16];
static int num_devices = 0;
static uint8_t broadcast_mac[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ARP cache
typedef struct arp_entry {
    uint32_t ip;
    uint8_t mac[ETH_ALEN];
    uint32_t timestamp;
} arp_entry_t;

static arp_entry_t arp_cache[64];
static int arp_cache_size = 0;

// Sockets
static socket_t sockets[256];
static int num_sockets = 0;

// Initialize networking
void net_init(void) {
    memset(devices, 0, sizeof(devices));
    memset(arp_cache, 0, sizeof(arp_cache));
    memset(sockets, 0, sizeof(sockets));
    
    screen_print("Network stack initialized\n");
}

// Register network device
int net_register_device(net_device_t* dev) {
    if (num_devices >= 16) {
        return -1;
    }
    
    devices[num_devices++] = dev;
    
    screen_print("Registered network device: ");
    screen_print(dev->name);
    screen_print("\n");
    
    return 0;
}

// Unregister network device
int net_unregister_device(net_device_t* dev) {
    for (int i = 0; i < num_devices; i++) {
        if (devices[i] == dev) {
            devices[i] = devices[--num_devices];
            return 0;
        }
    }
    return -1;
}

// Get network device by name
net_device_t* net_get_device(const char* name) {
    for (int i = 0; i < num_devices; i++) {
        if (strcmp(devices[i]->name, name) == 0) {
            return devices[i];
        }
    }
    return NULL;
}

// Ethernet transmit
int eth_transmit(net_device_t* dev, uint8_t* dest, uint16_t type, void* data, size_t len) {
    if (!dev || !dev->transmit || !dev->up) {
        return -1;
    }
    
    size_t total_len = sizeof(eth_hdr_t) + len;
    uint8_t* frame = kmalloc(total_len);
    if (!frame) {
        return -1;
    }
    
    eth_hdr_t* eth = (eth_hdr_t*)frame;
    mac_copy(eth->dest, dest);
    mac_copy(eth->src, dev->mac);
    eth->type = htons(type);
    
    memcpy(frame + sizeof(eth_hdr_t), data, len);
    
    int ret = dev->transmit(dev, frame, total_len);
    kfree(frame);
    
    return ret;
}

// Ethernet receive
void eth_receive(net_device_t* dev, void* data, size_t len) {
    if (len < sizeof(eth_hdr_t)) {
        return;
    }
    
    eth_hdr_t* eth = (eth_hdr_t*)data;
    uint16_t type = ntohs(eth->type);
    
    // Check if frame is for us or broadcast
    if (!mac_equal(eth->dest, dev->mac) && !mac_equal(eth->dest, broadcast_mac)) {
        return;
    }
    
    void* payload = data + sizeof(eth_hdr_t);
    size_t payload_len = len - sizeof(eth_hdr_t);
    
    switch (type) {
        case ETH_P_IP:
            ip_receive(dev, payload, payload_len);
            break;
        case ETH_P_ARP:
            arp_receive(dev, payload, payload_len);
            break;
        default:
            break;
    }
}

// IP transmit
int ip_transmit(uint32_t src, uint32_t dest, uint8_t protocol, void* data, size_t len) {
    // Find suitable device
    net_device_t* dev = NULL;
    for (int i = 0; i < num_devices; i++) {
        if (devices[i]->up) {
            dev = devices[i];
            break;
        }
    }
    
    if (!dev) {
        return -1;
    }
    
    // Get destination MAC via ARP
    uint8_t dest_mac[ETH_ALEN];
    if (!arp_lookup(dest, dest_mac)) {
        // Send ARP request
        arp_request(dest);
        return -1;  // Retry later
    }
    
    size_t total_len = sizeof(ip_hdr_t) + len;
    uint8_t* packet = kmalloc(total_len);
    if (!packet) {
        return -1;
    }
    
    ip_hdr_t* ip = (ip_hdr_t*)packet;
    ip->version_ihl = 0x45;  // IPv4, no options
    ip->tos = 0;
    ip->tot_len = htons(total_len);
    ip->id = htons(1);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = protocol;
    ip->check = 0;
    ip->saddr = htonl(src);
    ip->daddr = htonl(dest);
    
    ip->check = net_checksum(ip, sizeof(ip_hdr_t));
    
    memcpy(packet + sizeof(ip_hdr_t), data, len);
    
    int ret = eth_transmit(dev, dest_mac, ETH_P_IP, packet, total_len);
    kfree(packet);
    
    return ret;
}

// IP receive
void ip_receive(net_device_t* dev, void* data, size_t len) {
    if (len < sizeof(ip_hdr_t)) {
        return;
    }
    
    ip_hdr_t* ip = (ip_hdr_t*)data;
    
    // Verify checksum
    if (net_checksum(ip, sizeof(ip_hdr_t)) != 0) {
        return;
    }
    
    // Check if packet is for us
    uint32_t dest_ip = ntohl(ip->daddr);
    if (dest_ip != dev->ip_addr && dest_ip != 0xFFFFFFFF) {
        return;
    }
    
    void* payload = data + sizeof(ip_hdr_t);
    size_t payload_len = len - sizeof(ip_hdr_t);
    
    switch (ip->protocol) {
        case IPPROTO_ICMP:
            icmp_receive(dev, payload, payload_len);
            break;
        case IPPROTO_TCP:
            tcp_receive_packet(dev, payload, payload_len);
            break;
        case IPPROTO_UDP:
            udp_receive_packet(dev, payload, payload_len);
            break;
        default:
            break;
    }
}

// ARP request
int arp_request(uint32_t target_ip) {
    net_device_t* dev = NULL;
    for (int i = 0; i < num_devices; i++) {
        if (devices[i]->up) {
            dev = devices[i];
            break;
        }
    }
    
    if (!dev) {
        return -1;
    }
    
    size_t packet_size = sizeof(arp_hdr_t);
    arp_hdr_t* arp = kmalloc(packet_size);
    if (!arp) {
        return -1;
    }
    
    arp->htype = htons(1);      // Ethernet
    arp->ptype = htons(ETH_P_IP);
    arp->hlen = ETH_ALEN;
    arp->plen = IP_ADDR_LEN;
    arp->oper = htons(ARP_REQUEST);
    mac_copy(arp->sha, dev->mac);
    arp->spa = htonl(dev->ip_addr);
    memset(arp->tha, 0, ETH_ALEN);
    arp->tpa = htonl(target_ip);
    
    int ret = eth_transmit(dev, broadcast_mac, ETH_P_ARP, arp, packet_size);
    kfree(arp);
    
    return ret;
}

// ARP reply
int arp_reply(uint32_t target_ip, uint8_t* target_mac) {
    net_device_t* dev = NULL;
    for (int i = 0; i < num_devices; i++) {
        if (devices[i]->up) {
            dev = devices[i];
            break;
        }
    }
    
    if (!dev) {
        return -1;
    }
    
    size_t packet_size = sizeof(arp_hdr_t);
    arp_hdr_t* arp = kmalloc(packet_size);
    if (!arp) {
        return -1;
    }
    
    arp->htype = htons(1);      // Ethernet
    arp->ptype = htons(ETH_P_IP);
    arp->hlen = ETH_ALEN;
    arp->plen = IP_ADDR_LEN;
    arp->oper = htons(ARP_REPLY);
    mac_copy(arp->sha, dev->mac);
    arp->spa = htonl(dev->ip_addr);
    mac_copy(arp->tha, target_mac);
    arp->tpa = htonl(target_ip);
    
    int ret = eth_transmit(dev, target_mac, ETH_P_ARP, arp, packet_size);
    kfree(arp);
    
    return ret;
}

// ARP receive
void arp_receive(net_device_t* dev, void* data, size_t len) {
    if (len < sizeof(arp_hdr_t)) {
        return;
    }
    
    arp_hdr_t* arp = (arp_hdr_t*)data;
    uint16_t oper = ntohs(arp->oper);
    
    if (ntohs(arp->htype) != 1 || ntohs(arp->ptype) != ETH_P_IP) {
        return;
    }
    
    uint32_t spa = ntohl(arp->spa);
    uint32_t tpa = ntohl(arp->tpa);
    
    // Add to ARP cache
    arp_cache_add(spa, arp->sha);
    
    if (oper == ARP_REQUEST && tpa == dev->ip_addr) {
        // Send ARP reply
        arp_reply(spa, arp->sha);
    }
}

// ICMP ping
int icmp_ping(uint32_t target_ip) {
    size_t packet_size = sizeof(icmp_hdr_t) + 4;
    uint8_t* packet = kmalloc(packet_size);
    if (!packet) {
        return -1;
    }
    
    icmp_hdr_t* icmp = (icmp_hdr_t*)packet;
    icmp->type = 8;  // Echo request
    icmp->code = 0;
    icmp->check = 0;
    icmp->unused = 0;
    
    // Add timestamp
    uint32_t timestamp = timer_get_ticks();
    memcpy(packet + sizeof(icmp_hdr_t), &timestamp, 4);
    
    icmp->check = net_checksum(packet, packet_size);
    
    int ret = ip_transmit(net_get_device("eth0")->ip_addr, target_ip, IPPROTO_ICMP, packet, packet_size);
    kfree(packet);
    
    return ret;
}

// ICMP receive
void icmp_receive(net_device_t* dev, void* data, size_t len) {
    if (len < sizeof(icmp_hdr_t)) {
        return;
    }
    
    icmp_hdr_t* icmp = (icmp_hdr_t*)data;
    
    if (icmp->type == 8) {  // Echo request
        // Send echo reply
        size_t reply_size = len;
        uint8_t* reply = kmalloc(reply_size);
        if (reply) {
            memcpy(reply, data, len);
            
            icmp_hdr_t* reply_icmp = (icmp_hdr_t*)reply;
            reply_icmp->type = 0;  // Echo reply
            reply_icmp->check = 0;
            reply_icmp->check = net_checksum(reply, reply_size);
            
            ip_transmit(dev->ip_addr, ntohl(((ip_hdr_t*)(data - sizeof(ip_hdr_t)))->saddr), 
                       IPPROTO_ICMP, reply, reply_size);
            kfree(reply);
        }
    } else if (icmp->type == 0) {  // Echo reply
        uint32_t timestamp;
        memcpy(data + sizeof(icmp_hdr_t), &timestamp, 4);
        
        screen_print("Ping reply received, time: ");
        screen_print_dec(timer_get_ticks() - timestamp);
        screen_print(" ms\n");
    }
}

// TCP receive packet
void tcp_receive_packet(net_device_t* dev, void* data, size_t len) {
    if (len < sizeof(tcp_hdr_t)) {
        return;
    }
    
    tcp_hdr_t* tcp = (tcp_hdr_t*)data;
    uint16_t src_port = ntohs(tcp->source);
    uint16_t dest_port = ntohs(tcp->dest);
    
    // Find matching socket
    socket_t* sock = NULL;
    for (int i = 0; i < num_sockets; i++) {
        if (sockets[i].local_port == dest_port && sockets[i].protocol == IPPROTO_TCP) {
            sock = &sockets[i];
            break;
        }
    }
    
    if (!sock) {
        return;
    }
    
    // Handle TCP state machine
    if (tcp->flags & TCP_FLAG_SYN) {
        // Send SYN+ACK
        tcp_hdr_t reply;
        reply.source = htons(sock->local_port);
        reply.dest = htons(src_port);
        reply.seq = 0;
        reply.ack_seq = htonl(ntohl(tcp->seq) + 1);
        reply.data_off = 5 << 4;
        reply.flags = TCP_FLAG_SYN | TCP_FLAG_ACK;
        reply.window = htons(TCP_MAX_WINDOW);
        reply.check = 0;
        reply.urg_ptr = 0;
        
        ip_transmit(dev->ip_addr, ntohl(((ip_hdr_t*)(data - sizeof(ip_hdr_t)))->saddr),
                   IPPROTO_TCP, &reply, sizeof(reply));
        sock->state = 1;  // SYN_RECEIVED
    } else if (tcp->flags & TCP_FLAG_ACK) {
        sock->state = 2;  // ESTABLISHED
    }
    
    // TODO: Handle data transfer
}

// UDP receive packet
void udp_receive_packet(net_device_t* dev, void* data, size_t len) {
    if (len < sizeof(udp_hdr_t)) {
        return;
    }
    
    udp_hdr_t* udp = (udp_hdr_t*)data;
    uint16_t src_port = ntohs(udp->source);
    uint16_t dest_port = ntohs(udp->dest);
    
    // Find matching socket
    socket_t* sock = NULL;
    for (int i = 0; i < num_sockets; i++) {
        if (sockets[i].local_port == dest_port && sockets[i].protocol == IPPROTO_UDP) {
            sock = &sockets[i];
            break;
        }
    }
    
    if (!sock) {
        return;
    }
    
    // Store data in socket buffer
    void* payload = data + sizeof(udp_hdr_t);
    size_t payload_len = len - sizeof(udp_hdr_t);
    
    // TODO: Store in socket receive buffer
}

// Utility functions

uint16_t net_checksum(void* data, size_t len) {
    uint16_t* ptr = (uint16_t*)data;
    uint32_t sum = 0;
    
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    
    if (len > 0) {
        sum += *(uint8_t*)ptr;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

uint32_t ip_aton(const char* ip_str) {
    uint32_t ip = 0;
    int octet = 0;
    
    while (*ip_str) {
        if (*ip_str == '.') {
            ip = (ip << 8) | octet;
            octet = 0;
        } else if (*ip_str >= '0' && *ip_str <= '9') {
            octet = octet * 10 + (*ip_str - '0');
        }
        ip_str++;
    }
    
    ip = (ip << 8) | octet;
    return ip;
}

char* ip_ntoa(uint32_t ip, char* buffer) {
    sprintf(buffer, "%d.%d.%d.%d", 
            (ip >> 24) & 0xFF,
            (ip >> 16) & 0xFF,
            (ip >> 8) & 0xFF,
            ip & 0xFF);
    return buffer;
}

void mac_copy(uint8_t* dest, uint8_t* src) {
    memcpy(dest, src, ETH_ALEN);
}

bool mac_equal(uint8_t* a, uint8_t* b) {
    return memcmp(a, b, ETH_ALEN) == 0;
}

// ARP cache functions
void arp_cache_add(uint32_t ip, uint8_t* mac) {
    // Check if entry already exists
    for (int i = 0; i < arp_cache_size; i++) {
        if (arp_cache[i].ip == ip) {
            mac_copy(arp_cache[i].mac, mac);
            arp_cache[i].timestamp = timer_get_ticks();
            return;
        }
    }
    
    // Add new entry
    if (arp_cache_size < 64) {
        arp_cache[arp_cache_size].ip = ip;
        mac_copy(arp_cache[arp_cache_size].mac, mac);
        arp_cache[arp_cache_size].timestamp = timer_get_ticks();
        arp_cache_size++;
    }
}

bool arp_lookup(uint32_t ip, uint8_t* mac) {
    for (int i = 0; i < arp_cache_size; i++) {
        if (arp_cache[i].ip == ip) {
            mac_copy(mac, arp_cache[i].mac);
            return true;
        }
    }
    return false;
}

// htons/ntohs functions
uint16_t htons(uint16_t hostshort) {
    return ((hostshort & 0xFF) << 8) | ((hostshort >> 8) & 0xFF);
}

uint16_t ntohs(uint16_t netshort) {
    return htons(netshort);
}

uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0xFF) << 24) |
           ((hostlong & 0xFF00) << 8) |
           ((hostlong >> 8) & 0xFF00) |
           ((hostlong >> 24) & 0xFF);
}

uint32_t ntohl(uint32_t netlong) {
    return htonl(netlong);
}
