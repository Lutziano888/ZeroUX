#include "mouse.h"
#include "port.h"

static int mouse_x = 40;
static int mouse_y = 12;
static unsigned char mouse_buttons = 0;
static int mouse_cycle = 0;
static char mouse_byte[3];
static int initialized = 0;
static int error_count = 0;

#define MAX_ERRORS 100

// Safe wait mit Timeout
static int mouse_wait(unsigned char type, int timeout) {
    if (type == 0) {
        while (timeout-- > 0) {
            if ((inb(0x64) & 1) == 1) return 1;
        }
    } else {
        while (timeout-- > 0) {
            if ((inb(0x64) & 2) == 0) return 1;
        }
    }
    return 0;
}

static void mouse_write(unsigned char data) {
    if (!mouse_wait(1, 1000)) return;
    outb(0x64, 0xD4);
    if (!mouse_wait(1, 1000)) return;
    outb(0x60, data);
}

static unsigned char mouse_read() {
    if (!mouse_wait(0, 1000)) return 0xFF;
    return inb(0x60);
}

void mouse_init() {
    // Versuche Maus zu initialisieren
    mouse_wait(1, 5000);
    outb(0x64, 0xA8);
    
    mouse_wait(1, 5000);
    outb(0x64, 0x20);
    unsigned char status = mouse_read();
    
    status |= 2;
    mouse_wait(1, 5000);
    outb(0x64, 0x60);
    mouse_wait(1, 5000);
    outb(0x60, status);
    
    mouse_write(0xF6);
    mouse_read();
    
    mouse_write(0xF4);
    mouse_read();
    
    initialized = 1;
    error_count = 0;
}

void mouse_update() {
    // Wenn zu viele Fehler: Maus als defekt markieren
    if (error_count > MAX_ERRORS) {
        initialized = 0;
        return;
    }
    
    if (!initialized) return;
    
    // Prüfe ob überhaupt Daten da sind
    unsigned char status = inb(0x64);
    if ((status & 0x01) == 0) return; // Keine Daten
    
    // Prüfe ob von Maus (nicht Keyboard)
    if ((status & 0x20) == 0) {
        // Von Keyboard, nicht Maus - verwerfen
        inb(0x60); // Daten lesen und ignorieren
        return;
    }
    
    unsigned char data = inb(0x60);
    
    switch (mouse_cycle) {
        case 0:
            // Byte 0: Flags
            if (!(data & 0x08)) {
                // Bit 3 nicht gesetzt = ungültig
                error_count++;
                return;
            }
            mouse_byte[0] = data;
            mouse_cycle = 1;
            break;
            
        case 1:
            // Byte 1: X-Movement
            mouse_byte[1] = data;
            mouse_cycle = 2;
            break;
            
        case 2:
            // Byte 2: Y-Movement
            mouse_byte[2] = data;
            mouse_cycle = 0;
            
            // Buttons
            mouse_buttons = mouse_byte[0] & 0x07;
            
            // Movement
            int delta_x = (int)mouse_byte[1];
            int delta_y = (int)mouse_byte[2];
            
            // Sign-extend
            if (mouse_byte[0] & 0x10) delta_x |= 0xFFFFFF00;
            if (mouse_byte[0] & 0x20) delta_y |= 0xFFFFFF00;
            
            // Overflow check
            if (mouse_byte[0] & 0x40) delta_x = 0; // X overflow
            if (mouse_byte[0] & 0x80) delta_y = 0; // Y overflow
            
            // Sanity check: nicht zu große Sprünge
            if (delta_x < -20 || delta_x > 20) delta_x = 0;
            if (delta_y < -20 || delta_y > 20) delta_y = 0;
            
            // Position aktualisieren (langsamer)
            mouse_x += delta_x / 20;
            mouse_y -= delta_y / 20;
            
            // Bounds
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_x >= 80) mouse_x = 79;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_y >= 25) mouse_y = 24;
            
            // Erfolg - Fehler zurücksetzen
            error_count = 0;
            break;
    }
}

int mouse_get_x() { return mouse_x; }
int mouse_get_y() { return mouse_y; }
unsigned char mouse_get_buttons() { return mouse_buttons; }