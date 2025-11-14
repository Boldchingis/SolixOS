#ifndef SOLIX_NET_H
#define SOLIX_NET_H

#include "types.h"

// Ethernet constants
#define ETH_ALEN 6
#define ETH_HDR_SIZE 14
#define ETH_MTU 1500

// Ethernet frame types
#define ETH_P_IP 0x0800
#define ETH_P_ARP 0x0806

// IP constants
#define IP_HDR_SIZE 20
#define IP_ADDR_LEN 4

// IP protocols
#define IPPROTO_ICMP 1
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17

// TCP constants
#define TCP_HDR_SIZE 20
#define TCP_MAX_WINDOW 65535

// TCP flags
#define TCP_FLAG_FIN 0x01
#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_RST 0x04
#define TCP_FLAG_PSH 0x08
#define TCP_FLAG_ACK 0x10
#define TCP_FLAG_URG 0x20

// UDP constants
#define UDP_HDR_SIZE 8

// ARP constants
#define ARP_HDR_SIZE 28
#define ARP_REQUEST 1
#define ARP_REPLY 2

// Network device structure
typedef struct net_device {
    char name[16];
    uint8_t mac[ETH_ALEN];
    uint32_t ip_addr;
    uint32_t netmask;
    uint32_t gateway;
    bool up;
    void* private_data;
    
    // Device operations
    int (*open)(struct net_device* dev);
    int (*close)(struct net_device* dev);
    int (*transmit)(struct net_device* dev, void* data, size_t len);
    void (*receive)(struct net_device* dev, void* data, size_t len);
} net_device_t;

// Ethernet header
typedef struct eth_hdr {
    uint8_t dest[ETH_ALEN];
    uint8_t src[ETH_ALEN];
    uint16_t type;
} __attribute__((packed)) eth_hdr_t;

// IP header
typedef struct ip_hdr {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
} __attribute__((packed)) ip_hdr_t;

// TCP header
typedef struct tcp_hdr {
    uint16_t source;
    uint16_t dest;
    uint32_t seq;
    uint32_t ack_seq;
    uint8_t data_off;
    uint8_t flags;
    uint16_t window;
    uint16_t check;
    uint16_t urg_ptr;
} __attribute__((packed)) tcp_hdr_t;

// UDP header
typedef struct udp_hdr {
    uint16_t source;
    uint16_t dest;
    uint16_t len;
    uint16_t check;
} __attribute__((packed)) udp_hdr_t;

// ICMP header
typedef struct icmp_hdr {
    uint8_t type;
    uint8_t code;
    uint16_t check;
    uint32_t unused;
} __attribute__((packed)) icmp_hdr_t;

// ARP header
typedef struct arp_hdr {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t oper;
    uint8_t sha[ETH_ALEN];
    uint32_t spa;
    uint8_t tha[ETH_ALEN];
    uint32_t tpa;
} __attribute__((packed)) arp_hdr_t;

// Socket structure
typedef struct socket {
    int type;           // SOCK_STREAM, SOCK_DGRAM
    int protocol;       // IPPROTO_TCP, IPPROTO_UDP
    uint32_t local_ip;
    uint16_t local_port;
    uint32_t remote_ip;
    uint16_t remote_port;
    uint32_t state;     // TCP state
    void* private_data;
} socket_t;

// Network functions
void net_init(void);
int net_register_device(net_device_t* dev);
int net_unregister_device(net_device_t* dev);
net_device_t* net_get_device(const char* name);

// Ethernet functions
int eth_transmit(net_device_t* dev, uint8_t* dest, uint16_t type, void* data, size_t len);
void eth_receive(net_device_t* dev, void* data, size_t len);

// IP functions
int ip_transmit(uint32_t src, uint32_t dest, uint8_t protocol, void* data, size_t len);
void ip_receive(net_device_t* dev, void* data, size_t len);

// ARP functions
int arp_request(uint32_t target_ip);
int arp_reply(uint32_t target_ip, uint8_t* target_mac);
void arp_receive(net_device_t* dev, void* data, size_t len);

// TCP functions
int tcp_connect(socket_t* sock, uint32_t ip, uint16_t port);
int tcp_send(socket_t* sock, void* data, size_t len);
int tcp_receive(socket_t* sock, void* data, size_t len);
void tcp_receive_packet(net_device_t* dev, void* data, size_t len);

// UDP functions
int udp_send(socket_t* sock, void* data, size_t len);
int udp_receive(socket_t* sock, void* data, size_t len);
void udp_receive_packet(net_device_t* dev, void* data, size_t len);

// ICMP functions
int icmp_ping(uint32_t target_ip);
void icmp_receive(net_device_t* dev, void* data, size_t len);

// Utility functions
uint16_t net_checksum(void* data, size_t len);
uint32_t ip_aton(const char* ip_str);
char* ip_ntoa(uint32_t ip, char* buffer);
void mac_copy(uint8_t* dest, uint8_t* src);
bool mac_equal(uint8_t* a, uint8_t* b);

// htons/ntohs functions
uint16_t htons(uint16_t hostshort);
uint16_t ntohs(uint16_t netshort);
uint32_t htonl(uint32_t hostlong);
uint32_t ntohl(uint32_t netlong);

// ARP cache functions
void arp_cache_add(uint32_t ip, uint8_t* mac);
bool arp_lookup(uint32_t ip, uint8_t* mac);

#endif
