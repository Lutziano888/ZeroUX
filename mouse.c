// ============================================================================
// MOUSE.C - KORRIGIERTE VERSION (Kein Crash mehr!)
// ============================================================================

#include "mouse.h"
#include "port.h"

#define MOUSE_PORT_DATA    0x60
#define MOUSE_PORT_STATUS  0x64
#define MOUSE_PORT_CMD     0x64

// KRITISCH: Timeout-Werte erhöhen (war zu klein!)
#define MOUSE_TIMEOUT 100000

// Paket-Buffer (verhindert Buffer-Overflow)
static unsigned char packet_buffer[3];
static int packet_index = 0;
static int packet_ready = 0;

// Status-Flags
static int mouse_enabled = 0;

// Sensitivity (100 = 1.0x, 50 = 0.5x, 200 = 2.0x)
static int mouse_sensitivity = 100;

void mouse_set_sensitivity(int percent) {
    if (percent < 10) percent = 10;
    if (percent > 500) percent = 500;
    mouse_sensitivity = percent;
}

int mouse_get_sensitivity() { return mouse_sensitivity; }

// ============================================================================
// PROBLEM 1 GELÖST: Besseres Warten mit Timeout-Check
// ============================================================================

static int mouse_wait_input() {
    int timeout = MOUSE_TIMEOUT;
    while (timeout--) {
        if (inb(MOUSE_PORT_STATUS) & 0x01) {
            return 1; // Daten verfügbar
        }
        // Mini-Delay um CPU zu entlasten
        for (volatile int i = 0; i < 10; i++);
    }
    return 0; // Timeout
}

static int mouse_wait_output() {
    int timeout = MOUSE_TIMEOUT;
    while (timeout--) {
        if (!(inb(MOUSE_PORT_STATUS) & 0x02)) {
            return 1; // Bereit für Output
        }
        for (volatile int i = 0; i < 10; i++);
    }
    return 0; // Timeout
}

// ============================================================================
// PROBLEM 2 GELÖST: Sicheres Schreiben mit Fehlerprüfung
// ============================================================================

static int mouse_write_safe(unsigned char data) {
    if (!mouse_wait_output()) return 0;
    
    outb(MOUSE_PORT_CMD, 0xD4); // Prefix für Maus-Kommando
    
    if (!mouse_wait_output()) return 0;
    
    outb(MOUSE_PORT_DATA, data);
    return 1;
}

static unsigned char mouse_read_safe() {
    if (!mouse_wait_input()) return 0;
    return inb(MOUSE_PORT_DATA);
}

// ============================================================================
// PROBLEM 3 GELÖST: Buffer leeren VOR Initialisierung
// ============================================================================

static void mouse_flush_buffer() {
    int timeout = 1000;
    while ((inb(MOUSE_PORT_STATUS) & 0x01) && timeout--) {
        inb(MOUSE_PORT_DATA); // Alles rauswerfen
        for (volatile int i = 0; i < 100; i++);
    }
}

// ============================================================================
// PROBLEM 4 GELÖST: Robuste Initialisierung mit Fehlerbehandlung
// ============================================================================

void mouse_init() {
    mouse_enabled = 0;
    packet_index = 0;
    packet_ready = 0;
    
    // Schritt 1: IRQ12 am PIC KOMPLETT deaktivieren (kein Interrupt!)
    unsigned char mask = inb(0xA1);
    outb(0xA1, mask | 0x10); // Bit 4 = IRQ12 maskieren
    
    // Schritt 2: Buffer komplett leeren
    mouse_flush_buffer();
    
    // Schritt 3: Maus-Device aktivieren
    if (!mouse_wait_output()) {
        return; // Fehler, aber kein Crash
    }
    outb(MOUSE_PORT_CMD, 0xA8); // Auxiliary Device Enable
    
    // Kleine Pause
    for (volatile int i = 0; i < 100000; i++);
    
    // Schritt 4: Controller Configuration lesen
    if (!mouse_wait_output()) return;
    outb(MOUSE_PORT_CMD, 0x20); // Read Configuration Byte
    
    unsigned char status = mouse_read_safe();
    
    // WICHTIG: Interrupts AUS, Clock AN
    status &= ~(1 << 1); // IRQ12 deaktivieren (Bit 1 = 0)
    status &= ~(1 << 5); // Clock aktivieren (Bit 5 = 0)
    status |= (1 << 0);  // IRQ1 (Tastatur) anlassen
    
    // Schritt 5: Configuration zurückschreiben
    if (!mouse_wait_output()) return;
    outb(MOUSE_PORT_CMD, 0x60); // Write Configuration Byte
    
    if (!mouse_wait_output()) return;
    outb(MOUSE_PORT_DATA, status);
    
    // Schritt 6: Buffer erneut leeren
    mouse_flush_buffer();
    
    // Schritt 7: Maus Reset (OPTIONAL, kann bei Problemen helfen)
    /*
    if (!mouse_write_safe(0xFF)) return; // Reset
    mouse_read_safe(); // ACK
    mouse_read_safe(); // 0xAA (Self-test passed)
    mouse_read_safe(); // 0x00 (Mouse ID)
    */
    
    // Schritt 8: Defaults setzen
    if (!mouse_write_safe(0xF6)) return; // Set Defaults
    mouse_read_safe(); // ACK
    
    // Schritt 9: Datenübertragung aktivieren
    if (!mouse_write_safe(0xF4)) return; // Enable Data Reporting
    mouse_read_safe(); // ACK
    
    // Schritt 10: Buffer final leeren
    mouse_flush_buffer();
    
    mouse_enabled = 1;
}

// ============================================================================
// PROBLEM 5 GELÖST: Sicheres Packet-Reading mit Validierung
// ============================================================================

int mouse_read_packet(int* dx, int* dy, int* buttons) {
    if (!mouse_enabled) return 0;
    
    *dx = 0;
    *dy = 0;
    *buttons = 0;
    
    // Mehrere Bytes lesen, wenn verfügbar
    int bytes_read = 0;
    
    while (bytes_read < 10 && (inb(MOUSE_PORT_STATUS) & 0x01)) {
        unsigned char status = inb(MOUSE_PORT_STATUS);
        
        // Prüfen: Daten vorhanden UND von Maus (Bit 5)
        if (!(status & 0x20)) {
            // Nicht von Maus -> verwerfen
            inb(MOUSE_PORT_DATA);
            continue;
        }
        
        unsigned char byte = inb(MOUSE_PORT_DATA);
        bytes_read++;
        
        // Byte 1: Muss immer Bit 3 gesetzt haben
        if (packet_index == 0) {
            if (!(byte & 0x08)) {
                // Ungültiges Paket -> neu synchronisieren
                continue;
            }
            packet_buffer[0] = byte;
            packet_index = 1;
        }
        // Byte 2: X-Bewegung
        else if (packet_index == 1) {
            packet_buffer[1] = byte;
            packet_index = 2;
        }
        // Byte 3: Y-Bewegung (Paket komplett!)
        else if (packet_index == 2) {
            packet_buffer[2] = byte;
            packet_index = 0;
            packet_ready = 1;
            break; // Fertig!
        }
    }
    
    // Paket dekodieren
    if (packet_ready) {
        packet_ready = 0;
        
        unsigned char b1 = packet_buffer[0];
        unsigned char b2 = packet_buffer[1];
        unsigned char b3 = packet_buffer[2];
        
        // X-Bewegung
        int x = b2;
        if (b1 & 0x10) x -= 256; // Vorzeichen (9-bit two's complement)
        
        // Y-Bewegung
        int y = b3;
        if (b1 & 0x20) y -= 256;
        
        // Overflow-Check (verhindert Sprünge)
        if (b1 & 0x40) x = 0; // X-Overflow
        if (b1 & 0x80) y = 0; // Y-Overflow
        
        // Apply Sensitivity
        x = (x * mouse_sensitivity) / 100;
        y = (y * mouse_sensitivity) / 100;

        // Bewegung begrenzen (verhindert zu schnelle Bewegungen)
        // Limit erhöht, da Sensitivität den Wert vergrößern kann
        if (x > 100) x = 100;
        if (x < -100) x = -100;
        if (y > 100) y = 100;
        if (y < -100) y = -100;
        
        *dx = x;
        *dy = -y; // Y ist invertiert bei PS/2
        *buttons = b1 & 0x07; // Bits 0-2: Links, Rechts, Mitte
        
        return 1;
    }
    
    return 0;
}

// ============================================================================
// ZUSÄTZLICHE DEBUG-FUNKTION
// ============================================================================

void mouse_debug_status() {
    unsigned char status = inb(MOUSE_PORT_STATUS);
    // Bit 0: Output buffer full
    // Bit 1: Input buffer full
    // Bit 5: Mouse data (vs keyboard)
    // Du könntest diese Werte auf dem Bildschirm anzeigen zum Debuggen
}