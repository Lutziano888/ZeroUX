#include "ethernet.h"
#include "port.h"
#include "string.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

// Intel E1000 Register Offsets
#define E1000_CTRL     0x0000
#define E1000_ICR      0x00C0
#define E1000_RCTL     0x0100
#define E1000_TCTL     0x0400
#define E1000_RDBAL    0x2800
#define E1000_RDBAH    0x2804
#define E1000_RDLEN    0x2808
#define E1000_RDH      0x2810
#define E1000_RDT      0x2818
#define E1000_TDBAL    0x3800
#define E1000_TDBAH    0x3804
#define E1000_TDLEN    0x3808
#define E1000_TDH      0x3810
#define E1000_TDT      0x3818
#define E1000_RAL      0x5400
#define E1000_RAH      0x5404

// Konfiguration
#define NUM_RX_DESC 32
#define NUM_TX_DESC 8

// Descriptor Strukturen (Hardware-Format)
typedef struct {
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint16_t checksum;
    volatile uint8_t status;
    volatile uint8_t errors;
    volatile uint16_t special;
} __attribute__((packed)) rx_desc_t;

typedef struct {
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint8_t cso;
    volatile uint8_t cmd;
    volatile uint8_t status;
    volatile uint8_t css;
    volatile uint16_t special;
} __attribute__((packed)) tx_desc_t;

// Globale Variablen für den Treiber
static uint8_t* mmio_base = 0;
static rx_desc_t rx_descs[NUM_RX_DESC];
static tx_desc_t tx_descs[NUM_TX_DESC];
static uint8_t rx_buffers[NUM_RX_DESC][2048];
static uint8_t tx_buffers[NUM_TX_DESC][2048];

static int rx_cur = 0;
static int tx_cur = 0;
static int is_initialized = 0;
static MAC_Address my_mac;
static IP_Address my_ip = {{0, 0, 0, 0}}; // Start with 0.0.0.0

// Hilfsfunktionen für Memory Mapped I/O
static void mmio_write32(uint32_t offset, uint32_t val) {
    *(volatile uint32_t*)(mmio_base + offset) = val;
}
static uint32_t mmio_read32(uint32_t offset) {
    return *(volatile uint32_t*)(mmio_base + offset);
}

// PCI Konfiguration lesen/schreiben
static uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (1U << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outl(0xCF8, address);
    return inl(0xCFC);
}
static void pci_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (1U << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outl(0xCF8, address);
    outl(0xCFC, value);
}

void ethernet_init() {
    // 1. Suche nach Intel E1000 (Vendor: 0x8086, Device: 0x100E für 82540EM)
    uint8_t bus, slot, func;
    int found = 0;
    for (int b = 0; b < 256 && !found; b++) {
        for (int s = 0; s < 32 && !found; s++) {
            for (int f = 0; f < 8 && !found; f++) {
                uint32_t vid_did = pci_read(b, s, f, 0x00);
                if ((vid_did & 0xFFFF) == 0x8086 && (vid_did >> 16) == 0x100E) {
                    bus = b; slot = s; func = f;
                    found = 1;
                }
            }
        }
    }
    if (!found) return; // Keine Karte gefunden

    // 2. MMIO Basisadresse holen (BAR0)
    uint32_t bar0 = pci_read(bus, slot, func, 0x10);
    mmio_base = (uint8_t*)(bar0 & 0xFFFFFFF0);

    // 3. Bus Master aktivieren (Wichtig für DMA!)
    uint32_t cmd = pci_read(bus, slot, func, 0x04);
    pci_write(bus, slot, func, 0x04, cmd | 0x4);

    // 4. MAC Adresse auslesen
    uint32_t ral = mmio_read32(E1000_RAL);
    uint32_t rah = mmio_read32(E1000_RAH);
    my_mac.addr[0] = ral & 0xFF;
    my_mac.addr[1] = (ral >> 8) & 0xFF;
    my_mac.addr[2] = (ral >> 16) & 0xFF;
    my_mac.addr[3] = (ral >> 24) & 0xFF;
    my_mac.addr[4] = rah & 0xFF;
    my_mac.addr[5] = (rah >> 8) & 0xFF;

    // 5. Empfang (RX) initialisieren
    for (int i = 0; i < NUM_RX_DESC; i++) {
        rx_descs[i].addr = (uint64_t)(uint32_t)rx_buffers[i];
        rx_descs[i].status = 0;
    }
    mmio_write32(E1000_RDBAL, (uint32_t)&rx_descs[0]);
    mmio_write32(E1000_RDBAH, 0);
    mmio_write32(E1000_RDLEN, NUM_RX_DESC * sizeof(rx_desc_t));
    mmio_write32(E1000_RDH, 0);
    mmio_write32(E1000_RDT, NUM_RX_DESC - 1);
    // RCTL: Enable (1<<1), Broadcast (1<<15), Strip CRC (1<<26)
    // NEU: Enable Unicast Promiscuous (1<<3) damit wir Antworten sicher empfangen
    mmio_write32(E1000_RCTL, (1 << 1) | (1 << 3) | (1 << 4) | (1 << 15) | (1 << 26)); 

    // 6. Senden (TX) initialisieren
    for (int i = 0; i < NUM_TX_DESC; i++) {
        tx_descs[i].addr = (uint64_t)(uint32_t)tx_buffers[i];
        tx_descs[i].cmd = 0;
        tx_descs[i].status = 1; // Done
    }
    mmio_write32(E1000_TDBAL, (uint32_t)&tx_descs[0]);
    mmio_write32(E1000_TDBAH, 0);
    mmio_write32(E1000_TDLEN, NUM_TX_DESC * sizeof(tx_desc_t));
    mmio_write32(E1000_TDH, 0);
    mmio_write32(E1000_TDT, 0);
    // TCTL: Enable (1<<1), Pad Short Packets (1<<3)
    mmio_write32(E1000_TCTL, (1 << 1) | (1 << 3));

    // 7. Link Up setzen
    uint32_t ctrl = mmio_read32(E1000_CTRL);
    mmio_write32(E1000_CTRL, ctrl | (1 << 6)); // SLU

    is_initialized = 1;
}

MAC_Address ethernet_get_mac() { return my_mac; }
int ethernet_is_connected() { return is_initialized; }
void ethernet_dhcp_request() {} // Wird jetzt von net.c gesteuert

IP_Address ethernet_get_ip() { return my_ip; }
void ethernet_set_ip(uint8_t* ip) {
    memcpy(my_ip.addr, ip, 4);
}

int ethernet_send_frame(EthernetFrame* frame) {
    if (!is_initialized) return -1;

    int idx = tx_cur;
    tx_desc_t* desc = &tx_descs[idx];
    
    // Daten in den DMA-Buffer kopieren
    int len = frame->length + 14;
    uint8_t* buf = tx_buffers[idx];
    memcpy(buf, frame->dest_mac.addr, 6);
    memcpy(buf + 6, frame->src_mac.addr, 6);
    memcpy(buf + 12, &frame->ethertype, 2);
    memcpy(buf + 14, frame->payload, frame->length);

    desc->length = len;
    desc->cmd = (1 << 0) | (1 << 1) | (1 << 3); // EOP (End of Packet) | IFCS (Insert Checksum) | RS (Report Status)
    desc->status = 0;

    // Tail Pointer erhöhen (startet Übertragung)
    tx_cur = (tx_cur + 1) % NUM_TX_DESC;
    mmio_write32(E1000_TDT, tx_cur);

    // Warten bis fertig (Polling)
    while (!(desc->status & 1)); 
    
    return 0;
}

int ethernet_recv_frame(EthernetFrame* frame) {
    if (!is_initialized) return -1;

    int idx = rx_cur;
    rx_desc_t* desc = &rx_descs[idx];

    // Prüfen ob Descriptor Done (DD) Bit gesetzt ist
    if (!(desc->status & 1)) return -1; 

    // Daten kopieren
    int len = desc->length;
    if (len > 1514) len = 1514;
    uint8_t* buf = rx_buffers[idx];

    memcpy(frame->dest_mac.addr, buf, 6);
    memcpy(frame->src_mac.addr, buf + 6, 6);
    memcpy(&frame->ethertype, buf + 12, 2);
    memcpy(frame->payload, buf + 14, len - 14);
    frame->length = len - 14;

    // Descriptor zurücksetzen für Wiederverwendung
    desc->status = 0;
    
    // Tail Pointer erhöhen (gibt Descriptor an Hardware zurück)
    mmio_write32(E1000_RDT, idx);
    rx_cur = (rx_cur + 1) % NUM_RX_DESC;

    return 0;
}
