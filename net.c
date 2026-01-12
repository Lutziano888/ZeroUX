#include "net.h"
#include "string.h"
#include "ethernet.h"

// Wir erwarten diese Funktionen vom Ethernet-Treiber (ethernet.c)
// (Funktionen sind in ethernet.h deklariert, das via kernel.c eingebunden ist)

// Status Buffer für GUI Overlay
static char net_status_buf[128] = "";
// Globaler Paket-Zähler für Debugging
static int rx_packet_count = 0;
static uint16_t last_ethertype = 0;

// HTTP Response Buffer
static char http_response_buf[16384]; // 16KB Buffer für HTML (vergrößert)
static int http_response_len = 0;

const char* net_get_status() {
    return net_status_buf;
}

int net_get_rx_count() {
    return rx_packet_count;
}

uint16_t net_get_last_ethertype() {
    return last_ethertype;
}

const char* net_get_http_response() {
    return http_response_buf;
}

// TCP Client State
static int tcp_state = 0; // 0=CLOSED, 1=SYN_SENT, 2=ESTABLISHED
static uint32_t tcp_seq = 0;
static uint32_t tcp_ack = 0;
static uint16_t tcp_local_port = 55555;
static uint8_t tcp_remote_ip[4];
static uint16_t tcp_remote_port = 0;

// Global Gateway MAC (gelernt via DHCP)
static uint8_t gateway_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Konfiguration (Statische IP für den Anfang)
// Wir holen die IP jetzt dynamisch vom Ethernet-Treiber
extern void ethernet_set_ip(uint8_t* ip);
extern IP_Address ethernet_get_ip();

int net_get_tcp_state() { return tcp_state; }

// ============================================================================
// DEFINITIONS & STRUCTS (Moved to top to fix compilation errors)
// ============================================================================

#define ETH_P_IP   0x0800
#define ETH_P_ARP  0x0806

// IP Protokoll IDs
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP  6
#define IP_PROTO_UDP  17

// Strukturen (Packed ist wichtig, damit keine Lücken entstehen)
#pragma pack(push, 1)

typedef struct {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t type;
} eth_header_t;

typedef struct {
    uint16_t hw_type;   // 1 = Ethernet
    uint16_t proto_type;// 0x0800 = IP
    uint8_t  hw_len;    // 6
    uint8_t  proto_len; // 4
    uint16_t opcode;    // 1 = Req, 2 = Reply
    uint8_t  sender_mac[6];
    uint8_t  sender_ip[4];
    uint8_t  target_mac[6];
    uint8_t  target_ip[4];
} arp_header_t;

typedef struct {
    uint8_t  ver_ihl;     // Version (4) + IHL (4)
    uint8_t  tos;
    uint16_t len;
    uint16_t id;
    uint16_t flags_frag;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t checksum;
    uint8_t  src[4];
    uint8_t  dst[4];
} ip_header_t;

typedef struct {
    uint8_t type; // 8 = Echo Req, 0 = Echo Reply
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} icmp_header_t;

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} udp_header_t;

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t  offset_reserved;
    uint8_t  flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
} tcp_header_t;

typedef struct {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
    uint32_t magic;
    uint8_t options[308];
} dhcp_packet_t;

typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t q_count;
    uint16_t ans_count;
    uint16_t auth_count;
    uint16_t add_count;
} dns_header_t;

#pragma pack(pop)

// Byte Swap Implementierung
uint16_t htons(uint16_t v) { return (v << 8) | (v >> 8); }
uint16_t ntohs(uint16_t v) { return (v << 8) | (v >> 8); }
uint32_t htonl(uint32_t v) { return ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v & 0xFF0000) >> 8) | ((v >> 24) & 0xFF); }
uint32_t ntohl(uint32_t v) { return htonl(v); }

// Checksumme berechnen (Standard Internet Checksum Algorithmus)
static uint16_t calculate_checksum(void* data, int len) {
    uint16_t* ptr = (uint16_t*)data;
    uint32_t sum = 0;
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    if (len == 1) sum += *(uint8_t*)ptr;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}

// Wrapper um rohe Pakete via Ethernet-Treiber zu senden
static void ethernet_send_packet(uint8_t* dst_mac, uint8_t* data, int len, uint16_t protocol) {
    EthernetFrame frame;
    memcpy(frame.dest_mac.addr, dst_mac, 6);
    frame.src_mac = ethernet_get_mac();
    frame.ethertype = htons(protocol);
    if (len > 1500) len = 1500;
    memcpy(frame.payload, data, len);
    frame.length = len;
    ethernet_send_frame(&frame);
}

// Pseudo-Header Checksumme für TCP/UDP
static uint16_t calculate_pseudo_checksum(uint8_t* src_ip, uint8_t* dst_ip, uint8_t proto, uint16_t len, void* data) {
    uint32_t sum = 0;
    uint16_t* s = (uint16_t*)src_ip;
    uint16_t* d = (uint16_t*)dst_ip;
    
    sum += s[0]; sum += s[1];
    sum += d[0]; sum += d[1];
    sum += htons(proto);
    sum += htons(len);
    
    uint16_t* p = (uint16_t*)data;
    while (len > 1) {
        sum += *p++;
        len -= 2;
    }
    if (len == 1) sum += *(uint8_t*)p;
    
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}

// ============================================================================
// TCP CLIENT FUNCTIONS (Now that structs are defined)
// ============================================================================

// Startet eine TCP Verbindung (Sendet SYN)
void net_tcp_connect(uint8_t* ip, uint16_t port) {
    tcp_state = 1; // SYN_SENT
    tcp_local_port++; // Port rotieren
    if (tcp_local_port < 50000) tcp_local_port = 50000;
    
    memcpy(tcp_remote_ip, ip, 4);
    tcp_remote_port = port;
    tcp_seq = 1000; // Start Sequence Number
    tcp_ack = 0;
    
    http_response_len = 0;
    http_response_buf[0] = '\0';
    
    // Paket vorbereiten
    int tcp_len = sizeof(tcp_header_t);
    int ip_len = sizeof(ip_header_t) + tcp_len;
    
    uint8_t packet[128];
    memset(packet, 0, 128);
    
    ip_header_t* iph = (ip_header_t*)packet;
    iph->ver_ihl = 0x45;
    iph->len = htons(ip_len);
    iph->ttl = 64;
    iph->proto = IP_PROTO_TCP;
    IP_Address my_ip = ethernet_get_ip();
    memcpy(iph->src, my_ip.addr, 4);
    memcpy(iph->dst, tcp_remote_ip, 4);
    iph->checksum = calculate_checksum(iph, sizeof(ip_header_t));
    
    tcp_header_t* tcph = (tcp_header_t*)(packet + sizeof(ip_header_t));
    tcph->src_port = htons(tcp_local_port);
    tcph->dst_port = htons(tcp_remote_port);
    tcph->seq = htonl(tcp_seq);
    tcph->ack = 0;
    tcph->offset_reserved = 0x50; // Header Length 5*4
    tcph->flags = 0x02; // SYN Flag
    tcph->window = htons(65535);
    
    tcph->checksum = 0;
    tcph->checksum = calculate_pseudo_checksum(my_ip.addr, tcp_remote_ip, IP_PROTO_TCP, tcp_len, tcph);
    
    ethernet_send_packet(gateway_mac, packet, ip_len, ETH_P_IP);
    if (gateway_mac[0] == 0xFF) {
        strcpy(net_status_buf, "TCP: Sending SYN (Broadcast)...");
    } else {
        strcpy(net_status_buf, "TCP: Sending SYN...");
    }
}

// Sendet HTTP GET Request
void net_send_http_request(const char* host, const char* path) {
    if (tcp_state != 2) return; // Nur wenn verbunden
    
    char req[512];
    // Einfacher HTTP 1.0 Request
    strcpy(req, "GET ");
    strcat(req, path);
    strcat(req, " HTTP/1.0\r\nHost: ");
    strcat(req, host);
    strcat(req, "\r\nUser-Agent: ZeroUX\r\nConnection: close\r\n\r\n");
    
    int payload_len = strlen(req);
    int tcp_len = sizeof(tcp_header_t) + payload_len;
    int ip_len = sizeof(ip_header_t) + tcp_len;
    
    uint8_t packet[1024];
    memset(packet, 0, 1024);
    
    ip_header_t* iph = (ip_header_t*)packet;
    iph->ver_ihl = 0x45;
    iph->len = htons(ip_len);
    iph->ttl = 64;
    iph->proto = IP_PROTO_TCP;
    IP_Address my_ip = ethernet_get_ip();
    memcpy(iph->src, my_ip.addr, 4);
    memcpy(iph->dst, tcp_remote_ip, 4);
    iph->checksum = calculate_checksum(iph, sizeof(ip_header_t));
    
    tcp_header_t* tcph = (tcp_header_t*)(packet + sizeof(ip_header_t));
    tcph->src_port = htons(tcp_local_port);
    tcph->dst_port = htons(tcp_remote_port);
    tcph->seq = htonl(tcp_seq);
    tcph->ack = htonl(tcp_ack);
    tcph->offset_reserved = 0x50;
    tcph->flags = 0x18; // PSH | ACK (Push Data)
    tcph->window = htons(65535);
    
    // Payload kopieren
    memcpy(packet + sizeof(ip_header_t) + sizeof(tcp_header_t), req, payload_len);
    
    tcph->checksum = 0;
    tcph->checksum = calculate_pseudo_checksum(my_ip.addr, tcp_remote_ip, IP_PROTO_TCP, tcp_len, tcph);
    
    ethernet_send_packet(gateway_mac, packet, ip_len, ETH_P_IP);
    strcpy(net_status_buf, "TCP: Request Sent.");
    
    tcp_seq += payload_len; // Sequence Number erhöhen
}

// Sendet rohe Daten über TCP (für Telnet/SSH)
void net_tcp_send(const char* data, int len) {
    if (tcp_state != 2) return; // Nur wenn verbunden
    
    int tcp_len = sizeof(tcp_header_t) + len;
    int ip_len = sizeof(ip_header_t) + tcp_len;
    
    uint8_t packet[1500];
    if (ip_len > 1500) return; // Zu groß für MTU
    memset(packet, 0, 1500);
    
    ip_header_t* iph = (ip_header_t*)packet;
    iph->ver_ihl = 0x45;
    iph->len = htons(ip_len);
    iph->ttl = 64;
    iph->proto = IP_PROTO_TCP;
    IP_Address my_ip = ethernet_get_ip();
    memcpy(iph->src, my_ip.addr, 4);
    memcpy(iph->dst, tcp_remote_ip, 4);
    iph->checksum = calculate_checksum(iph, sizeof(ip_header_t));
    
    tcp_header_t* tcph = (tcp_header_t*)(packet + sizeof(ip_header_t));
    tcph->src_port = htons(tcp_local_port);
    tcph->dst_port = htons(tcp_remote_port);
    tcph->seq = htonl(tcp_seq);
    tcph->ack = htonl(tcp_ack);
    tcph->offset_reserved = 0x50;
    tcph->flags = 0x18; // PSH | ACK
    tcph->window = htons(65535);
    
    memcpy(packet + sizeof(ip_header_t) + sizeof(tcp_header_t), data, len);
    
    tcph->checksum = 0;
    tcph->checksum = calculate_pseudo_checksum(my_ip.addr, tcp_remote_ip, IP_PROTO_TCP, tcp_len, tcph);
    
    ethernet_send_packet(gateway_mac, packet, ip_len, ETH_P_IP);
    tcp_seq += len;
}

int net_tcp_read(char* buf, int max) {
    if (http_response_len == 0) return 0;
    
    int to_copy = (http_response_len < max) ? http_response_len : max;
    memcpy(buf, http_response_buf, to_copy);
    
    // Puffer shiften (Daten verbrauchen)
    if (to_copy < http_response_len) {
        for(int i=0; i < http_response_len - to_copy; i++) {
            http_response_buf[i] = http_response_buf[i + to_copy];
        }
    }
    http_response_len -= to_copy;
    http_response_buf[http_response_len] = 0;
    
    return to_copy;
}

// ============================================================================
// REST OF NETWORK STACK
// ============================================================================

// Sendet ein DHCP Discover Paket
void net_send_dhcp_discover() {
    uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t broadcast_ip[4] = {255, 255, 255, 255};
    uint8_t zero_ip[4] = {0, 0, 0, 0};
    
    int udp_len = sizeof(udp_header_t) + sizeof(dhcp_packet_t);
    int ip_len = sizeof(ip_header_t) + udp_len;
    
    uint8_t packet[1024];
    memset(packet, 0, 1024);
    
    // IP Header
    ip_header_t* ip = (ip_header_t*)packet;
    ip->ver_ihl = 0x45;
    ip->ttl = 64;
    ip->proto = IP_PROTO_UDP;
    memcpy(ip->src, zero_ip, 4);
    memcpy(ip->dst, broadcast_ip, 4);
    ip->len = htons(ip_len);
    ip->checksum = calculate_checksum(ip, sizeof(ip_header_t));
    
    // UDP Header
    udp_header_t* udp = (udp_header_t*)(packet + sizeof(ip_header_t));
    udp->src_port = htons(68); // DHCP Client
    udp->dst_port = htons(67); // DHCP Server
    udp->length = htons(udp_len);
    
    // DHCP Packet
    dhcp_packet_t* dhcp = (dhcp_packet_t*)(packet + sizeof(ip_header_t) + sizeof(udp_header_t));
    dhcp->op = 1; // Boot Request
    dhcp->htype = 1; // Ethernet
    dhcp->hlen = 6;
    dhcp->xid = htonl(0x12345678);
    dhcp->magic = htonl(0x63825363);
    dhcp->flags = htons(0x8000); // Broadcast Flag setzen (bitte antworte an 255.255.255.255)
    
    MAC_Address mac = ethernet_get_mac();
    memcpy(dhcp->chaddr, mac.addr, 6);
    
    // Options
    int opt = 0;
    dhcp->options[opt++] = 53; // Message Type
    dhcp->options[opt++] = 1;  // Length
    dhcp->options[opt++] = 1;  // Discover
    dhcp->options[opt++] = 255; // End
    
    // UDP Checksum (optional for IPv4, set to 0)
    udp->checksum = 0;
    
    ethernet_send_packet(broadcast_mac, packet, ip_len, ETH_P_IP);
}

void net_init() {
    net_send_dhcp_discover();
}

// Hilfsfunktion: String IP "8.8.8.8" zu Bytes parsen
static void parse_ip_str(const char* str, uint8_t* out_ip) {
    int val = 0;
    int idx = 0;
    while (*str && idx < 4) {
        if (*str == '.') {
            out_ip[idx++] = val;
            val = 0;
        } else if (*str >= '0' && *str <= '9') {
            val = val * 10 + (*str - '0');
        }
        str++;
    }
    out_ip[idx] = val;
}

// Ping senden
void net_ping(const char* ip_str) {
    uint8_t target_ip[4];
    parse_ip_str(ip_str, target_ip);
    
    int icmp_len = sizeof(icmp_header_t) + 32; // Header + 32 bytes payload
    int ip_len = sizeof(ip_header_t) + icmp_len;
    
    uint8_t packet[128];
    memset(packet, 0, 128);
    
    ip_header_t* ip = (ip_header_t*)packet;
    ip->ver_ihl = 0x45;
    ip->ttl = 64;
    ip->proto = IP_PROTO_ICMP;
    IP_Address my_ip = ethernet_get_ip();
    memcpy(ip->src, my_ip.addr, 4);
    memcpy(ip->dst, target_ip, 4);
    ip->len = htons(ip_len);
    ip->checksum = calculate_checksum(ip, sizeof(ip_header_t));
    
    icmp_header_t* icmp = (icmp_header_t*)(packet + sizeof(ip_header_t));
    icmp->type = 8; // Echo Request
    icmp->id = htons(1);
    icmp->seq = htons(1);
    icmp->checksum = calculate_checksum(icmp, icmp_len);
    
    ethernet_send_packet(gateway_mac, packet, ip_len, ETH_P_IP);
    strcpy(net_status_buf, "Ping sent...");
}

// DNS Anfrage senden
void net_dns_lookup(const char* domain) {
    uint8_t dns_server_ip[4] = {8, 8, 8, 8}; // Google DNS
    
    int udp_len = sizeof(udp_header_t) + sizeof(dns_header_t) + strlen(domain) + 2 + 4;
    int ip_len = sizeof(ip_header_t) + udp_len;
    
    uint8_t packet[1024];
    memset(packet, 0, 1024);
    
    // IP Header
    ip_header_t* ip = (ip_header_t*)packet;
    ip->ver_ihl = 0x45;
    ip->ttl = 64;
    ip->proto = IP_PROTO_UDP;
    IP_Address my_ip = ethernet_get_ip();
    memcpy(ip->src, my_ip.addr, 4);
    memcpy(ip->dst, dns_server_ip, 4);
    ip->len = htons(ip_len);
    ip->checksum = calculate_checksum(ip, sizeof(ip_header_t));
    
    // UDP Header
    udp_header_t* udp = (udp_header_t*)(packet + sizeof(ip_header_t));
    udp->src_port = htons(55555); // Random High Port
    udp->dst_port = htons(53);    // DNS Port
    udp->length = htons(udp_len);
    udp->checksum = 0;
    
    // DNS Header
    dns_header_t* dns = (dns_header_t*)(packet + sizeof(ip_header_t) + sizeof(udp_header_t));
    dns->id = htons(0x1234);
    dns->flags = htons(0x0100); // Standard Query, Recursion Desired
    dns->q_count = htons(1);
    
    // DNS Query (Domain Name Encoding: "google.com" -> "\x06google\x03com\x00")
    uint8_t* qname = (uint8_t*)(dns + 1);
    const char* ptr = domain;
    int pos = 0;
    
    while (*ptr) {
        const char* next_dot = ptr;
        int label_len = 0;
        while (*next_dot && *next_dot != '.') {
            next_dot++;
            label_len++;
        }
        
        qname[pos++] = label_len;
        for(int i=0; i<label_len; i++) qname[pos++] = *ptr++;
        
        if (*ptr == '.') ptr++;
    }
    qname[pos++] = 0; // End of name
    
    // Type A (1) & Class IN (1)
    qname[pos++] = 0; qname[pos++] = 1;
    qname[pos++] = 0; qname[pos++] = 1;
    
    ethernet_send_packet(gateway_mac, packet, ip_len, ETH_P_IP);
    
    // Debug-Ausgabe: Wohin senden wir?
    if (gateway_mac[0] == 0xFF) {
        strcpy(net_status_buf, "DNS: Sending via Broadcast (No Gateway!)");
    } else {
        char byte_str[8];
        strcpy(net_status_buf, "DNS: Sent via GW [");
        int_to_str(gateway_mac[0], byte_str);
        strcat(net_status_buf, byte_str);
        strcat(net_status_buf, ":...]");
    }
}

// Helper to skip DNS name
static uint8_t* dns_skip_name(uint8_t* ptr, uint8_t* buffer_start, int buffer_len) {
    int loops = 0;
    while ((ptr - buffer_start) < buffer_len && loops++ < 20) { // Safety limit
        if (*ptr == 0) return ptr + 1;
        if ((*ptr & 0xC0) == 0xC0) return ptr + 2; // Pointer
        
        int label_len = *ptr;
        // Safety check: Don't go out of bounds
        if ((ptr - buffer_start) + label_len + 1 >= buffer_len) return buffer_start + buffer_len;
        
        ptr += label_len + 1;
    }
    return ptr;
}

// Verarbeitet UDP Pakete (DHCP)
static void handle_udp(uint8_t* src_mac, uint8_t* src_ip, uint8_t* data, int len) {
    udp_header_t* udp = (udp_header_t*)data;
    
    // DHCP Offer (Server -> Client port 68)
    if (ntohs(udp->dst_port) == 68) {
        dhcp_packet_t* dhcp = (dhcp_packet_t*)(data + sizeof(udp_header_t));
        if (ntohl(dhcp->magic) == 0x63825363 && dhcp->op == 2) { // Boot Reply
            // Wir nehmen einfach die angebotene IP an (vereinfacht)
            uint8_t new_ip[4];
            memcpy(new_ip, &dhcp->yiaddr, 4);
            ethernet_set_ip(new_ip);
            
            // Gateway MAC lernen (vom Absender des DHCP Offers)
            memcpy(gateway_mac, src_mac, 6);
        }
    }
    // DNS Response (Server port 53 -> Client)
    else if (ntohs(udp->src_port) == 53) {
        // DEBUG: Paket empfangen, Analyse startet
        strcpy(net_status_buf, "DNS: Packet Recv. Analyzing...");
        
        uint8_t* payload = data + sizeof(udp_header_t);
        int payload_len = len - sizeof(udp_header_t);
        
        // --- DNS Header Analyse ---
        if (payload_len < 12) return;
        
        uint16_t flags = (payload[2] << 8) | payload[3];
        uint16_t an_count = (payload[6] << 8) | payload[7];
        int rcode = flags & 0x0F;
        
        if (rcode != 0) {
            char msg[64];
            strcpy(msg, "DNS Error RCODE: ");
            char num[8];
            int_to_str(rcode, num);
            strcat(msg, num);
            strcpy(net_status_buf, msg);
            return;
        }
        
        if (an_count == 0) {
            strcpy(net_status_buf, "DNS: No Answers (ANCOUNT=0)");
            return;
        }
        // ---------------------------

        // Prüfen ob es unsere Transaktion ist (ID 0x1234)
        if (payload_len > 2 && payload[0] == 0x12 && payload[1] == 0x34) {
            // OK, ID passt.
        }
        
        // --- Proper DNS Parsing ---
        uint8_t* ptr = payload + 12; // Skip Header
        int qd_count = (payload[4] << 8) | payload[5];
        
        // Skip Queries
        for (int i = 0; i < qd_count; i++) {
            ptr = dns_skip_name(ptr, payload, payload_len);
            ptr += 4; // Type(2) + Class(2)
        }
        
        int found = 0;
        int last_type = 0;
        int last_len = 0;
        
        // Parse Answers
        for (int i = 0; i < an_count; i++) {
            ptr = dns_skip_name(ptr, payload, payload_len);
            
            // Safety check
            if (ptr - payload + 10 > payload_len) break;
            
            uint16_t type = (ptr[0] << 8) | ptr[1];
            uint16_t data_len = (ptr[8] << 8) | ptr[9];
            
            last_type = type;
            last_len = data_len;
            
            // WORKAROUND: Wenn Type 1 (IPv4) erkannt wurde, lesen wir die IP
            if (type == 1 && data_len == 4) {
                uint8_t* ip = ptr + 10; // Skip Header (10 Bytes)
                
                char ip_str[32];
                char num[8];
                strcpy(ip_str, "DNS Result: ");
                int_to_str(ip[0], num); strcat(ip_str, num); strcat(ip_str, ".");
                int_to_str(ip[1], num); strcat(ip_str, num); strcat(ip_str, ".");
                int_to_str(ip[2], num); strcat(ip_str, num); strcat(ip_str, ".");
                int_to_str(ip[3], num); strcat(ip_str, num);
                
                strcpy(net_status_buf, ip_str);
                found = 1;
                break;
            } else if (type == 5) {
                strcpy(net_status_buf, "DNS: CNAME (Alias) found");
            }
            
            ptr += 10 + data_len; // Normal weiter (falls Length stimmt)
        }
        
        if (!found && strncmp(net_status_buf, "DNS: CNAME", 10) != 0) {
            // Debug: Zeige Länge des Pakets an, damit wir wissen, dass was ankam
            char msg[64];
            strcpy(msg, "DNS: RecType ");
            char num[16];
            int_to_str(last_type, num);
            strcat(msg, num);
            strcat(msg, " Len ");
            int_to_str(last_len, num);
            strcat(msg, num);
            strcpy(net_status_buf, msg);
        }
    }
    else {
        // DEBUG: Paket von 8.8.8.8 aber falscher Port?
        if (src_ip[0] == 8 && src_ip[1] == 8 && src_ip[2] == 8 && src_ip[3] == 8) {
             char msg[64];
             strcpy(msg, "Debug: 8.8.8.8 Port ");
             char num[8];
             int_to_str(ntohs(udp->src_port), num);
             strcat(msg, num);
             strcpy(net_status_buf, msg);
        }
    }
}

// Verarbeitet TCP Pakete (Webserver Port 80)
static void handle_tcp(uint8_t* src_mac, uint8_t* src_ip, uint8_t* dst_ip, uint8_t* data, int len) {
    tcp_header_t* tcp = (tcp_header_t*)data;
    int header_len = ((tcp->offset_reserved >> 4) * 4);
    uint8_t* payload = data + header_len;
    int payload_len = len - header_len;
    
    if (ntohs(tcp->dst_port) == 80) {
        // Antwort vorbereiten
        uint8_t reply_buf[1500];
        memset(reply_buf, 0, 1500);
        
        ip_header_t* rip = (ip_header_t*)reply_buf;
        tcp_header_t* rtcp = (tcp_header_t*)(reply_buf + sizeof(ip_header_t));
        uint8_t* rdata = reply_buf + sizeof(ip_header_t) + sizeof(tcp_header_t);
        
        // Flags prüfen
        if (tcp->flags & 0x02) { // SYN
            // Sende SYN-ACK
            rtcp->flags = 0x12; // SYN | ACK
            rtcp->seq = htonl(1000);
            rtcp->ack = htonl(ntohl(tcp->seq) + 1);
        } else if (tcp->flags & 0x18) { // PSH | ACK (Data)
            if (payload_len > 0 && memcmp(payload, "GET", 3) == 0) {
                // HTTP Response
                const char* html = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<html><body style='background-color:#222;color:#fff'><h1>Hello from ZeroUX!</h1><p>Your TCP/IP stack works.</p></body></html>";
                strcpy((char*)rdata, html);
                
                rtcp->flags = 0x18; // PSH | ACK
                rtcp->seq = tcp->ack;
                rtcp->ack = htonl(ntohl(tcp->seq) + payload_len);
            } else {
                return; // Nur auf GET antworten
            }
        } else if (tcp->flags & 0x01) { // FIN
            rtcp->flags = 0x10; // ACK
            rtcp->seq = tcp->ack;
            rtcp->ack = htonl(ntohl(tcp->seq) + 1);
        } else {
            return;
        }
        
        // IP Header füllen
        rip->ver_ihl = 0x45;
        rip->len = htons(sizeof(ip_header_t) + sizeof(tcp_header_t) + strlen((char*)rdata));
        rip->ttl = 64;
        rip->proto = IP_PROTO_TCP;
        memcpy(rip->src, dst_ip, 4);
        memcpy(rip->dst, src_ip, 4);
        rip->checksum = calculate_checksum(rip, sizeof(ip_header_t));
        
        // TCP Header füllen
        rtcp->src_port = tcp->dst_port;
        rtcp->dst_port = tcp->src_port;
        rtcp->offset_reserved = 0x50; // 5 * 4 bytes header
        rtcp->window = htons(65535);
        
        // TCP Checksumme
        int tcp_seg_len = sizeof(tcp_header_t) + strlen((char*)rdata);
        rtcp->checksum = 0;
        rtcp->checksum = calculate_pseudo_checksum(dst_ip, src_ip, IP_PROTO_TCP, tcp_seg_len, rtcp);
        
        ethernet_send_packet(src_mac, reply_buf, ntohs(rip->len), ETH_P_IP);
    }
    // --- TCP CLIENT LOGIC ---
    else if (ntohs(tcp->dst_port) == tcp_local_port) {
        // 1. Handshake: SYN-ACK empfangen?
        if (tcp_state == 1 && (tcp->flags & 0x12) == 0x12) { // SYN | ACK
            tcp_state = 2; // ESTABLISHED
            tcp_seq = ntohl(tcp->ack);
            tcp_ack = ntohl(tcp->seq) + 1;
            
            // Sende ACK zurück (Handshake Abschluss)
            uint8_t packet[128];
            memset(packet, 0, 128);
            
            int tcp_len = sizeof(tcp_header_t);
            int ip_len = sizeof(ip_header_t) + tcp_len;
            
            ip_header_t* iph = (ip_header_t*)packet;
            iph->ver_ihl = 0x45; iph->len = htons(ip_len); iph->ttl = 64; iph->proto = IP_PROTO_TCP;
            IP_Address my_ip = ethernet_get_ip();
            memcpy(iph->src, my_ip.addr, 4); memcpy(iph->dst, tcp_remote_ip, 4);
            iph->checksum = calculate_checksum(iph, sizeof(ip_header_t));
            
            tcp_header_t* tcph = (tcp_header_t*)(packet + sizeof(ip_header_t));
            tcph->src_port = htons(tcp_local_port); tcph->dst_port = htons(tcp_remote_port);
            tcph->seq = htonl(tcp_seq); tcph->ack = htonl(tcp_ack);
            tcph->offset_reserved = 0x50; tcph->flags = 0x10; // ACK
            tcph->window = htons(65535);
            tcph->checksum = calculate_pseudo_checksum(my_ip.addr, tcp_remote_ip, IP_PROTO_TCP, tcp_len, tcph);
            
            ethernet_send_packet(gateway_mac, packet, ip_len, ETH_P_IP);
            strcpy(net_status_buf, "TCP: Connected!");
        }
        // 2. Daten empfangen?
        else if (tcp_state == 2) {
            if (payload_len > 0) {
                // Wir haben Daten (HTML)!
                // In den Puffer kopieren
                if (http_response_len < 16383) {
                    int copy = payload_len;
                    if (http_response_len + copy > 16383) copy = 16383 - http_response_len;
                    memcpy(http_response_buf + http_response_len, payload, copy);
                    http_response_len += copy;
                    http_response_buf[http_response_len] = 0;
                }
                
                char msg[64];
                strcpy(msg, "HTTP: Data Recv Len ");
                char num[16];
                int_to_str(payload_len, num);
                strcat(msg, num);
                strcpy(net_status_buf, msg);
                
                tcp_ack += payload_len;
                
                // Sende ACK für die empfangenen Daten (WICHTIG für Stabilität)
                uint8_t packet[128];
                memset(packet, 0, 128);
                
                int tcp_len = sizeof(tcp_header_t);
                int ip_len = sizeof(ip_header_t) + tcp_len;
                
                ip_header_t* iph = (ip_header_t*)packet;
                iph->ver_ihl = 0x45; iph->len = htons(ip_len); iph->ttl = 64; iph->proto = IP_PROTO_TCP;
                IP_Address my_ip = ethernet_get_ip();
                memcpy(iph->src, my_ip.addr, 4); memcpy(iph->dst, tcp_remote_ip, 4);
                iph->checksum = calculate_checksum(iph, sizeof(ip_header_t));
                
                tcp_header_t* tcph = (tcp_header_t*)(packet + sizeof(ip_header_t));
                tcph->src_port = htons(tcp_local_port); tcph->dst_port = htons(tcp_remote_port);
                tcph->seq = htonl(tcp_seq); tcph->ack = htonl(tcp_ack);
                tcph->offset_reserved = 0x50; tcph->flags = 0x10; // ACK
                tcph->window = htons(65535);
                tcph->checksum = calculate_pseudo_checksum(my_ip.addr, tcp_remote_ip, IP_PROTO_TCP, tcp_len, tcph);
                
                ethernet_send_packet(gateway_mac, packet, ip_len, ETH_P_IP);
            }
            else {
                // Leeres Paket (z.B. ACK) empfangen -> Verbindung lebt noch
                // Nur Status updaten, wenn wir noch auf "Request Sent" stehen
                if (strncmp(net_status_buf, "TCP: Request", 12) == 0) {
                    strcpy(net_status_buf, "TCP: Waiting for reply...");
                }
            }
            if (tcp->flags & 0x01) { // FIN
                tcp_state = 0; // Closed
                if (http_response_len == 0) {
                    strcpy(net_status_buf, "TCP: Closed by Server (No Data)");
                } else {
                    strcpy(net_status_buf, "TCP: Closed by Server (Data received)");
                }
            }
        }
    }
}

// Verarbeitet ARP-Anfragen (Wer hat IP x.x.x.x?)
static void handle_arp(uint8_t* data, int len) {
    arp_header_t* arp = (arp_header_t*)data;
    
    if (ntohs(arp->hw_type) != 1 || ntohs(arp->proto_type) != 0x0800) return;
    
    // Ist das ARP für uns?
    IP_Address my_ip_struct = ethernet_get_ip();
    uint8_t* my_ip = my_ip_struct.addr;
    if (memcmp(arp->target_ip, my_ip, 4) == 0) {
        if (ntohs(arp->opcode) == 1) { // ARP Request
            // Antwort bauen (Reply)
            arp_header_t reply;
            reply.hw_type = htons(1);
            reply.proto_type = htons(0x0800);
            reply.hw_len = 6;
            reply.proto_len = 4;
            reply.opcode = htons(2); // Reply
            
            // Absender sind wir
            MAC_Address my_mac = ethernet_get_mac();
            memcpy(reply.sender_mac, &my_mac, 6);
            memcpy(reply.sender_ip, my_ip, 4);
            
            // Ziel ist der Anfragende
            memcpy(reply.target_mac, arp->sender_mac, 6);
            memcpy(reply.target_ip, arp->sender_ip, 4);
            
            ethernet_send_packet(arp->sender_mac, (uint8_t*)&reply, sizeof(arp_header_t), ETH_P_ARP);
        }
    }
}

// Verarbeitet IP-Pakete
static void handle_ip(uint8_t* src_mac, uint8_t* data, int len) {
    ip_header_t* ip = (ip_header_t*)data;
    
    // Check IP Version 4
    if ((ip->ver_ihl >> 4) != 4) {
         if (strncmp(net_status_buf, "DNS: Sent", 9) == 0) {
             strcpy(net_status_buf, "Debug: Non-IPv4 packet received");
         }
         return;
    }
    
    // Check Destination IP (ist es für uns?)
    IP_Address my_ip_struct = ethernet_get_ip();
    uint8_t* my_ip = my_ip_struct.addr;
    
    // Erlaube Pakete an unsere IP, an Broadcast ODER wenn wir noch keine IP haben (0.0.0.0)
    uint8_t broadcast_ip[4] = {255, 255, 255, 255};
    uint8_t zero_ip[4] = {0, 0, 0, 0};
    if (memcmp(my_ip, zero_ip, 4) != 0 && memcmp(ip->dst, my_ip, 4) != 0 && memcmp(ip->dst, broadcast_ip, 4) != 0) {
        // Debug: TCP Paket verworfen?
        if (ip->proto == IP_PROTO_TCP) {
             strcpy(net_status_buf, "Debug: TCP Dropped (Wrong IP)");
        }
        // Wir lassen UDP (DNS) und TCP (HTTP) durch, auch wenn die IP nicht passt (NAT Fix)
        if (ip->proto != IP_PROTO_UDP && ip->proto != IP_PROTO_TCP) {
            return; 
        }
    }
    
    int header_len = (ip->ver_ihl & 0x0F) * 4;
    uint8_t* payload = data + header_len;
    int payload_len = ntohs(ip->len) - header_len;
    
    // --- DEBUG: ICMP Analyse ---
    if (ip->proto == IP_PROTO_ICMP) {
        icmp_header_t* icmp = (icmp_header_t*)payload;
        char dbg[64];
        strcpy(dbg, "ICMP Type: ");
        char num[8];
        int_to_str(icmp->type, num);
        strcat(dbg, num);
        strcat(dbg, " Code: ");
        int_to_str(icmp->code, num);
        strcat(dbg, num);
        
        // Nur schreiben wenn kein Ping Reply da ist
        if (strncmp(net_status_buf, "Ping Reply", 10) != 0) {
            strcpy(net_status_buf, dbg);
        }
    }

    if (ip->proto == IP_PROTO_ICMP) {
        // Ping Reply Logic
        icmp_header_t* icmp = (icmp_header_t*)payload;
        if (icmp->type == 8) { // Echo Request
            // Wir recyceln den Puffer für die Antwort (Ping Pong)
            icmp->type = 0; // Echo Reply
            icmp->checksum = 0;
            icmp->checksum = calculate_checksum(icmp, payload_len);
            
            // IP Adressen tauschen
            uint8_t tmp_ip[4];
            memcpy(tmp_ip, ip->src, 4);
            memcpy(ip->src, ip->dst, 4);
            memcpy(ip->dst, tmp_ip, 4);
            
            // IP Checksumme neu berechnen
            ip->checksum = 0;
            ip->checksum = calculate_checksum(ip, header_len);
            
            // Zurücksenden an Absender-MAC
            ethernet_send_packet(src_mac, data, ntohs(ip->len), ETH_P_IP);
        } else if (icmp->type == 0) { // Echo Reply (Antwort auf UNSEREN Ping)
            char msg[64];
            strcpy(msg, "Ping Reply from ");
            char num[8];
            int_to_str(ip->src[3], num);
            strcat(msg, "..."); strcat(msg, num);
            strcpy(net_status_buf, msg);
        }
    }
    else if (ip->proto == IP_PROTO_TCP) {
        handle_tcp(src_mac, ip->src, ip->dst, payload, payload_len);
    }
    else if (ip->proto == IP_PROTO_UDP) {
        // DEBUG: Wenn UDP von 8.8.8.8 erkannt wird
        if (ip->src[0] == 8 && ip->src[1] == 8 && ip->src[2] == 8 && ip->src[3] == 8) {
             // Wir setzen den Status hier, falls handle_udp ihn nicht überschreibt
             strcpy(net_status_buf, "Debug: UDP from 8.8.8.8");
        }
        handle_udp(src_mac, ip->src, payload, payload_len);
    }
}

// Hauptfunktion: Wird aufgerufen, wenn ein Paket ankommt
void net_handle_packet(uint8_t* data, int len) {
    if (len < sizeof(eth_header_t)) return;

    // Debug: Zähler erhöhen
    rx_packet_count++;
    
    // Aktualisiere Status-Text mit RX-Count (nur wenn kein wichtigerer Text da ist)
    // WICHTIG: Überschreibe KEINE DNS-Ergebnisse oder Fehler!
    if (strncmp(net_status_buf, "DNS Result", 10) != 0 && 
        strncmp(net_status_buf, "DNS Error", 9) != 0 &&
        (strncmp(net_status_buf, "RX:", 3) == 0 || net_status_buf[0] == '\0')) {
        char count_str[16];
        int_to_str(rx_packet_count, count_str);
        strcpy(net_status_buf, "RX: "); strcat(net_status_buf, count_str);
    }

    eth_header_t* eth = (eth_header_t*)data;
    uint16_t type = ntohs(eth->type);
    last_ethertype = type;
    
    uint8_t* payload = data + sizeof(eth_header_t);
    int payload_len = len - sizeof(eth_header_t);
    
    if (type == ETH_P_ARP) {
        handle_arp(payload, payload_len);
    } else if (type == ETH_P_IP) {
        handle_ip(eth->src, payload, payload_len);
    }
}
