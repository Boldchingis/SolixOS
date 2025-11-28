#ifndef SOLIX_NET_MODERN_H
#define SOLIX_NET_MODERN_H

#include "types.h"
#include "net.h"

/**
 * SolixOS Modern Networking Stack
 * Enhanced TCP/IP stack with IPv6, TLS, and modern protocols
 * Version: 2.0
 */

// IPv6 support
#define IPV6_HDR_SIZE 40
#define IPV6_ADDR_LEN 16
#define IPV6_MTU 1500

// IPv6 extension headers
#define IPV6_HOP_BY_HOP 0
#define IPV6_ROUTING 43
#define IPV6_FRAGMENT 44
#define IPV6_AUTHENTICATION 51
#define IPV6_ESP 50
#define IPV6_DESTINATION 60
#define IPV6_MOBILITY 135

// IPv6 address types
#define IPV6_ADDR_UNICAST 0x01
#define IPV6_ADDR_MULTICAST 0x02
#define IPV6_ADDR_ANYCAST 0x03

// Modern TCP features
#define TCP_WINDOW_SCALE 0x01
#define TCP_SACK_PERMITTED 0x02
#define TCP_TIMESTAMP 0x08
#define TCP_FASTOPEN 0x0B

// TLS/SSL support
#define TLS_VERSION_1_2 0x0303
#define TLS_VERSION_1_3 0x0304
#define TLS_MAX_RECORD_SIZE 16384
#define TLS_CIPHER_SUITE_MAX 65535

// TLS cipher suites
#define TLS_AES128_GCM_SHA256 0x1301
#define TLS_AES256_GCM_SHA384 0x1302
#define TLS_CHACHA20_POLY1305_SHA256 0x1303

// TLS record types
#define TLS_RECORD_CHANGE_CIPHER_SPEC 20
#define TLS_RECORD_ALERT 21
#define TLS_RECORD_HANDSHAKE 22
#define TLS_RECORD_APPLICATION_DATA 23

// HTTP/2 support
#define HTTP2_PREFACE_MAGIC "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
#define HTTP2_MAX_FRAME_SIZE 16384
#define HTTP2_MAX_STREAMS 100

// HTTP/2 frame types
#define HTTP2_FRAME_DATA 0x00
#define HTTP2_FRAME_HEADERS 0x01
#define HTTP2_FRAME_PRIORITY 0x02
#define HTTP2_FRAME_RST_STREAM 0x03
#define HTTP2_FRAME_SETTINGS 0x04
#define HTTP2_FRAME_PUSH_PROMISE 0x05
#define HTTP2_FRAME_PING 0x06
#define HTTP2_FRAME_GOAWAY 0x07
#define HTTP2_FRAME_WINDOW_UPDATE 0x08
#define HTTP2_FRAME_CONTINUATION 0x09

// QUIC support
#define QUIC_VERSION_1 0x00000001
#define QUIC_MAX_UDP_PAYLOAD 1200
#define QUIC_MAX_PACKET_SIZE 1500

// QUIC packet types
#define QUIC_PACKET_INITIAL 0x00
#define QUIC_PACKET_0_RTT 0x01
#define QUIC_PACKET_HANDSHAKE 0x02
#define QUIC_PACKET_RETRY 0x03

// QUIC frame types
#define QUIC_FRAME_PADDING 0x00
#define QUIC_FRAME_PING 0x01
#define QUIC_FRAME_ACK 0x02
#define QUIC_FRAME_ACK_ECN 0x03
#define QUIC_FRAME_RESET_STREAM 0x04
#define QUIC_FRAME_STOP_SENDING 0x05
#define QUIC_FRAME_CRYPTO 0x06
#define QUIC_FRAME_NEW_TOKEN 0x07
#define QUIC_FRAME_STREAM 0x08
#define QUIC_FRAME_STREAM_FIN 0x08
#define QUIC_FRAME_STREAM_LEN 0x10
#define QUIC_FRAME_STREAM_OFF 0x20
#define QUIC_FRAME_MAX_DATA 0x10
#define QUIC_FRAME_MAX_STREAM_DATA 0x11
#define QUIC_FRAME_MAX_STREAMS_BIDI 0x12
#define QUIC_FRAME_MAX_STREAMS_UNIDI 0x13
#define QUIC_FRAME_DATA_BLOCKED 0x14
#define QUIC_FRAME_STREAM_DATA_BLOCKED 0x15
#define QUIC_FRAME_STREAMS_BLOCKED_BIDI 0x16
#define QUIC_FRAME_STREAMS_BLOCKED_UNIDI 0x17
#define QUIC_FRAME_NEW_CONNECTION_ID 0x18
#define QUIC_FRAME_RETIRE_CONNECTION_ID 0x19
#define QUIC_FRAME_PATH_CHALLENGE 0x1A
#define QUIC_FRAME_PATH_RESPONSE 0x1B
#define QUIC_FRAME_CONNECTION_CLOSE 0x1C
#define QUIC_FRAME_CONNECTION_CLOSE_APP 0x1D

// IPv6 address structure
typedef struct ipv6_addr {
    uint8_t bytes[16];
} ipv6_addr_t;

// IPv6 header structure
typedef struct ipv6_hdr {
    uint32_t ver_tc_flow;  // Version (4 bits), Traffic Class (8 bits), Flow Label (20 bits)
    uint16_t payload_len;
    uint8_t next_header;
    uint8_t hop_limit;
    ipv6_addr_t src_addr;
    ipv6_addr_t dst_addr;
} __packed ipv6_hdr_t;

// TCP options structure
typedef struct tcp_options {
    uint8_t mss;
    uint8_t window_scale;
    bool sack_permitted;
    uint32_t timestamp;
    bool fast_open;
    uint8_t options[40];
    uint8_t options_len;
} tcp_options_t;

// Modern TCP socket structure
typedef struct tcp_socket_modern {
    uint32_t local_port;
    uint32_t remote_port;
    uint32_t local_ip;
    uint32_t remote_ip;
    ipv6_addr_t local_ipv6;
    ipv6_addr_t remote_ipv6;
    
    uint32_t state;
    uint32_t sequence;
    uint32_t ack_sequence;
    uint16_t window;
    uint16_t max_window;
    
    tcp_options_t options;
    uint8_t* send_buffer;
    uint8_t* recv_buffer;
    uint32_t send_buffer_size;
    uint32_t recv_buffer_size;
    
    // TLS support
    void* tls_context;
    bool tls_enabled;
    uint8_t tls_version;
    uint16_t cipher_suite;
    
    // HTTP/2 support
    void* http2_context;
    bool http2_enabled;
    uint32_t max_concurrent_streams;
    
    // QUIC support
    void* quic_context;
    bool quic_enabled;
    uint32_t quic_version;
} tcp_socket_modern_t;

// TLS context structure
typedef struct tls_context {
    uint8_t version;
    uint16_t cipher_suite;
    uint8_t* master_secret;
    uint8_t* client_random;
    uint8_t* server_random;
    uint8_t* session_key;
    
    void* crypto_context;
    bool handshake_complete;
    bool verified;
    
    uint8_t* cert_chain;
    uint32_t cert_chain_len;
    uint8_t* private_key;
    uint32_t private_key_len;
} tls_context_t;

// HTTP/2 context structure
typedef struct http2_context {
    uint32_t stream_id_counter;
    uint32_t max_frame_size;
    uint32_t max_concurrent_streams;
    uint32_t initial_window_size;
    
    uint8_t* header_table;
    uint32_t header_table_size;
    uint32_t header_table_max_size;
    
    void* streams[HTTP2_MAX_STREAMS];
    uint32_t active_streams;
    
    uint8_t* send_buffer;
    uint8_t* recv_buffer;
} http2_context_t;

// QUIC context structure
typedef struct quic_context {
    uint32_t version;
    uint8_t* connection_id;
    uint8_t connection_id_len;
    uint8_t* source_connection_id;
    uint8_t source_connection_id_len;
    
    uint32_t local_port;
    uint32_t remote_port;
    ipv6_addr_t local_ipv6;
    ipv6_addr_t remote_ipv6;
    
    void* crypto_context;
    void* tls_context;
    
    uint32_t next_stream_id;
    uint32_t max_stream_data;
    uint32_t max_data;
    
    bool handshake_complete;
    bool connection_established;
} quic_context_t;

// Modern network device structure
typedef struct net_device_modern {
    char name[16];
    uint8_t mac[ETH_ALEN];
    uint32_t ip_addr;
    ipv6_addr_t ipv6_addr;
    uint32_t netmask;
    uint32_t gateway;
    ipv6_addr_t ipv6_gateway;
    
    // Modern features
    bool ipv6_enabled;
    bool tls_offload;
    bool tcp_checksum_offload;
    bool udp_checksum_offload;
    bool large_send_offload;
    
    // Statistics
    uint64_t packets_sent;
    uint64_t packets_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t errors;
    uint64_t dropped;
    
    void* driver_data;
    struct net_device_modern* next;
} net_device_modern_t;

// Modern network stack functions
API int net_modern_init(void);
API int net_modern_add_device(net_device_modern_t* dev);
API int net_modern_remove_device(net_device_modern_t* dev);

// IPv6 functions
API int ipv6_init(void);
API int ipv6_send_packet(net_device_modern_t* dev, ipv6_addr_t* dst, uint8_t proto, 
                         void* data, uint32_t len);
API int ipv6_receive_packet(net_device_modern_t* dev, ipv6_hdr_t* hdr, void* data, uint32_t len);

// Modern TCP functions
API tcp_socket_modern_t* tcp_socket_create_modern(void);
API int tcp_socket_connect_modern(tcp_socket_modern_t* sock, const char* host, uint16_t port);
API int tcp_socket_send_modern(tcp_socket_modern_t* sock, void* data, uint32_t len);
API int tcp_socket_recv_modern(tcp_socket_modern_t* sock, void* data, uint32_t len);
API void tcp_socket_close_modern(tcp_socket_modern_t* sock);

// TLS functions
API tls_context_t* tls_context_create(void);
API int tls_handshake(tls_context_t* ctx, tcp_socket_modern_t* sock);
API int tls_send(tls_context_t* ctx, void* data, uint32_t len);
API int tls_recv(tls_context_t* ctx, void* data, uint32_t len);
API void tls_context_free(tls_context_t* ctx);

// HTTP/2 functions
API http2_context_t* http2_context_create(void);
API int http2_send_headers(http2_context_t* ctx, uint32_t stream_id, void* headers, uint32_t len);
API int http2_send_data(http2_context_t* ctx, uint32_t stream_id, void* data, uint32_t len);
API void http2_context_free(http2_context_t* ctx);

// QUIC functions
API quic_context_t* quic_context_create(void);
API int quic_connect(quic_context_t* ctx, const char* host, uint16_t port);
API int quic_send_stream(quic_context_t* ctx, uint32_t stream_id, void* data, uint32_t len);
API int quic_recv_stream(quic_context_t* ctx, uint32_t stream_id, void* data, uint32_t len);
API void quic_context_free(quic_context_t* ctx);

// Utility functions
API void ipv6_addr_to_string(ipv6_addr_t* addr, char* str);
API int ipv6_string_to_addr(const char* str, ipv6_addr_t* addr);
API bool ipv6_addr_is_multicast(ipv6_addr_t* addr);
API bool ipv6_addr_is_link_local(ipv6_addr_t* addr);

#endif
