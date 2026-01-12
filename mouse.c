#include "mouse.h"
#include "port.h"

#define MOUSE_PORT_DATA    0x60
#define MOUSE_PORT_STATUS  0x64
#define MOUSE_PORT_CMD     0x64

static void mouse_wait(unsigned char type) {
    int timeout = 100000;
    if (type == 0) { // Warten auf Daten
        while (timeout--) {
            if ((inb(MOUSE_PORT_STATUS) & 1) == 1) return;
        }
    } else { // Warten auf Signal
        while (timeout--) {
            if ((inb(MOUSE_PORT_STATUS) & 2) == 0) return;
        }
    }
}

static void mouse_write(unsigned char data) {
    mouse_wait(1);
    outb(MOUSE_PORT_CMD, 0xD4); // CPU sagt: Nächstes Byte ist für die Maus
    mouse_wait(1);
    outb(MOUSE_PORT_DATA, data);
}

static unsigned char mouse_read() {
    mouse_wait(0);
    return inb(MOUSE_PORT_DATA);
}

void mouse_init() {
    unsigned char status;

    // 0. IRQ 12 am PIC (Interrupt Controller) maskieren
    // Das ist der wichtigste Schutz gegen Crashes! Wir sagen der CPU: "Ignoriere die Maus-Leitung komplett".
    // Slave PIC Data Port ist 0xA1. IRQ 12 ist Bit 4 (0x10).
    outb(0xA1, inb(0xA1) | 0x10);

    // Puffer leeren, falls noch alte Daten da sind, die einen Interrupt auslösen könnten
    while(inb(MOUSE_PORT_STATUS) & 0x01) {
        inb(MOUSE_PORT_DATA);
    }

    // 1. Maus-Controller aktivieren
    mouse_wait(1);
    outb(MOUSE_PORT_CMD, 0xA8);

    // 2. Interrupts DEAKTIVIEREN (Wichtig gegen Crashes!)
    mouse_wait(1);
    outb(MOUSE_PORT_CMD, 0x20); // Status lesen
    mouse_wait(0);
    status = inb(MOUSE_PORT_DATA);
    
    status &= ~(1 << 1); // IRQ 12 (Maus) ausschalten -> Wir nutzen Polling
    status &= ~(1 << 5); // Maus-Clock einschalten (Bit löschen = an)
    
    mouse_wait(1);
    outb(MOUSE_PORT_CMD, 0x60); // Status schreiben
    mouse_wait(1);
    outb(MOUSE_PORT_DATA, status);

    // 3. Maus resetten und Defaults setzen
    mouse_write(0xFF); // Reset
    mouse_read();      // ACK
    mouse_read();      // 0xAA (Self-test passed)
    mouse_read();      // 0x00 (Mouse ID)

    mouse_write(0xF6); // Defaults
    mouse_read();      // ACK

    mouse_write(0xF4); // Datenübertragung aktivieren
    mouse_read();      // ACK
}

int mouse_read_packet(int* dx, int* dy, int* buttons) {
    unsigned char status = inb(MOUSE_PORT_STATUS);
    
    // Prüfen ob Daten da sind (Bit 0) und ob sie von der Maus kommen (Bit 5)
    if ((status & 0x01) && (status & 0x20)) {
        unsigned char b1 = inb(MOUSE_PORT_DATA);
        
        // Kurz warten für die nächsten Bytes (Polling)
        int t = 1000; while(!(inb(MOUSE_PORT_STATUS) & 1) && t--);
        unsigned char b2 = inb(MOUSE_PORT_DATA);
        
        t = 1000; while(!(inb(MOUSE_PORT_STATUS) & 1) && t--);
        unsigned char b3 = inb(MOUSE_PORT_DATA);

        // Paket dekodieren
        // Bit 3 von Byte 1 muss 1 sein
        if ((b1 & 0x08) == 0) return 0;

        int x = b2;
        int y = b3;

        // Vorzeichen behandeln (9-bit two's complement)
        if (b1 & 0x10) x -= 256;
        if (b1 & 0x20) y -= 256;

        *dx = x;
        *dy = -y; // Y ist bei PS/2 umgekehrt
        *buttons = b1 & 0x07; // Bit 0=Links, 1=Rechts, 2=Mitte
        return 1;
    }
    return 0;
}