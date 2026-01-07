#include "ethernet.h"
#include "port.h"
#include "string.h"

// =====================
// Network Device Status
// =====================
static MAC_Address device_mac = { {0x52, 0x54, 0x00, 0x12, 0x34, 0x56} };
static IP_Address device_ip = { {192, 168, 1, 100} };
static IP_Address gateway_ip = { {192, 168, 1, 1} };
static IP_Address subnet_mask = { {255, 255, 255, 0} };
static int connection_status = 0; // 0 = disconnected, 1 = connected

// =====================
// PCI Configuration Registers for NIC
// =====================
#define PCI_CONFIG_ADDRESS 0x0CF8
#define PCI_CONFIG_DATA    0x0CFC

// Common NIC device IDs
#define INTEL_E1000_DEVICE_ID 0x1004
#define REALTEK_RTL8139_DEVICE_ID 0x8139

// =====================
// NIC Register Offsets (example for e1000)
// =====================
#define REG_CTRL    0x0000  // Device Control
#define REG_STATUS  0x0008  // Device Status
#define REG_CTRL_EXT 0x0018 // Extended Device Control
#define REG_MDIC    0x0020  // MDI Control
#define REG_FCAL    0x0028  // Flow Control Address Low
#define REG_FCAH    0x002C  // Flow Control Address High
#define REG_FCT     0x0030  // Flow Control Type
#define REG_FCS     0x0034  // Flow Control Settings
#define REG_FCTTV   0x0170  // Flow Control Transmit Timer Value

// Device Detection
static unsigned int pci_read_config(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address = (1 << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

static void pci_write_config(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset, unsigned int data) {
    unsigned int address = (1 << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, data);
}

static int find_network_device() {
    // Scan PCI bus for Ethernet controllers (class 0x02)
    int bus, slot, func;
    unsigned int vendor_device, class_revision, class_code;
    
    for (bus = 0; bus < 256; bus++) {
        for (slot = 0; slot < 32; slot++) {
            for (func = 0; func < 8; func++) {
                vendor_device = pci_read_config(bus, slot, func, 0x00);
                
                if (vendor_device == 0xFFFFFFFF)
                    continue;
                
                class_revision = pci_read_config(bus, slot, func, 0x08);
                class_code = (class_revision >> 8) & 0xFFFFFF;
                
                // Network controller class
                if ((class_code & 0xFF0000) == 0x020000) {
                    return 1; // Found network device
                }
            }
        }
    }
    return 0;
}

// =====================
// Initialization
// =====================
void ethernet_init() {
    // Check if network device is available on mainboard
    if (find_network_device()) {
        connection_status = 1;
        // Request DHCP configuration if needed
        ethernet_dhcp_request();
    } else {
        connection_status = 0;
    }
}

MAC_Address ethernet_get_mac() {
    return device_mac;
}

IP_Address ethernet_get_ip() {
    return device_ip;
}

int ethernet_is_connected() {
    return connection_status;
}

// =====================
// Frame Transmission
// =====================
int ethernet_send_frame(EthernetFrame* frame) {
    if (!connection_status)
        return -1; // Not connected
    
    // In a real implementation, this would:
    // 1. Build the Ethernet frame with proper headers
    // 2. Access the NIC's transmit buffer
    // 3. Write the frame data
    // 4. Signal the NIC to transmit
    
    return 0; // Success
}

// =====================
// Frame Reception
// =====================
int ethernet_recv_frame(EthernetFrame* frame) {
    if (!connection_status)
        return -1;
    
    // In a real implementation, this would:
    // 1. Check NIC's receive buffer
    // 2. Read frame data if available
    // 3. Validate frame
    // 4. Return frame to caller
    
    return -1; // No frame available
}

// =====================
// DHCP (simplified stub)
// =====================
void ethernet_dhcp_request() {
    // In a real implementation, this would:
    // 1. Send DHCP DISCOVER packet
    // 2. Wait for DHCP OFFER
    // 3. Send DHCP REQUEST
    // 4. Receive DHCP ACK with assigned IP
    
    // For now, use static IP
    connection_status = 1;
}
