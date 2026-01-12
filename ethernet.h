#ifndef ETHERNET_H
#define ETHERNET_H

// Ethernet MAC address
typedef struct {
    unsigned char addr[6];
} MAC_Address;

// IP address
typedef struct {
    unsigned char addr[4];
} IP_Address;

// Ethernet frame structure
#pragma pack(push, 1)
typedef struct {
    MAC_Address dest_mac;
    MAC_Address src_mac;
    unsigned short ethertype;
    unsigned char payload[1500];
    unsigned short length;
} EthernetFrame;
#pragma pack(pop)

// Initialization
void ethernet_init();

// Get MAC address
MAC_Address ethernet_get_mac();

// Get IP address
IP_Address ethernet_get_ip();

// Set IP address (for DHCP)
void ethernet_set_ip(unsigned char ip[4]);

// Send Ethernet frame
int ethernet_send_frame(EthernetFrame* frame);

// Receive Ethernet frame
int ethernet_recv_frame(EthernetFrame* frame);

// Check if link is up
int ethernet_is_connected();

// Network utilities
void ethernet_dhcp_request();

#endif
