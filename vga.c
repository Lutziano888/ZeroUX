#include "vga.h"

static unsigned short* VGA = (unsigned short*)0xB8000;
static int row = 0, col = 0;
static int scroll_offset = 0;  // Für manuelles Scrollen

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define BUFFER_HEIGHT 500  // Virtueller Puffer für Historie

static unsigned short vga_buffer[BUFFER_HEIGHT * VGA_WIDTH];
static int buffer_lines = 0;  // Anzahl genutzter Zeilen

// Aktualisiert den sichtbaren Bildschirm aus dem Buffer
static void vga_refresh() {
    int start_line = buffer_lines - VGA_HEIGHT - scroll_offset;
    if (start_line < 0) start_line = 0;
    
    for (int r = 0; r < VGA_HEIGHT; r++) {
        for (int c = 0; c < VGA_WIDTH; c++) {
            int buf_line = start_line + r;
            if (buf_line < buffer_lines) {
                VGA[r * VGA_WIDTH + c] = vga_buffer[buf_line * VGA_WIDTH + c];
            } else {
                VGA[r * VGA_WIDTH + c] = (0x0F << 8) | ' ';
            }
        }
    }
}

// Scrollt den Bildschirm eine Zeile nach oben
static void vga_scroll() {
    row++;
    if (row >= BUFFER_HEIGHT) {
        // Buffer voll - älteste Zeilen entfernen
        for (int r = 0; r < BUFFER_HEIGHT - 1; r++) {
            for (int c = 0; c < VGA_WIDTH; c++) {
                vga_buffer[r * VGA_WIDTH + c] = vga_buffer[(r + 1) * VGA_WIDTH + c];
            }
        }
        row = BUFFER_HEIGHT - 1;
    }
    
    // Neue Zeile leeren
    for (int c = 0; c < VGA_WIDTH; c++) {
        vga_buffer[row * VGA_WIDTH + c] = (0x0F << 8) | ' ';
    }
    
    if (row >= buffer_lines) {
        buffer_lines = row + 1;
    }
    
    // Auto-scroll nur wenn nicht manuell gescrollt
    if (scroll_offset == 0) {
        vga_refresh();
    }
}

void vga_put(char c) {
    if (c == '\n') {
        col = 0;
        vga_scroll();
        return;
    }
    
    // In Buffer schreiben
    vga_buffer[row * VGA_WIDTH + col] = (0x0F << 8) | c;
    col++;
    
    if (col >= VGA_WIDTH) {
        col = 0;
        vga_scroll();
    }
    
    // Anzeige aktualisieren wenn nicht manuell gescrollt
    if (scroll_offset == 0) {
        vga_refresh();
    }
}

void vga_print(const char* s) {
    while (*s) vga_put(*s++);
}

void vga_clear() {
    // Buffer leeren
    for (int i = 0; i < BUFFER_HEIGHT * VGA_WIDTH; i++)
        vga_buffer[i] = (0x0F << 8) | ' ';
    
    row = 0;
    col = 0;
    buffer_lines = 1;
    scroll_offset = 0;
    
    vga_refresh();
}

void vga_backspace() {
    if (col > 0) {
        col--;
        vga_buffer[row * VGA_WIDTH + col] = (0x0F << 8) | ' ';
        if (scroll_offset == 0) {
            vga_refresh();
        }
    }
}

void vga_println(const char* str) {
    vga_print(str);
    vga_print("\n");
}

// Pfeiltasten-Scrolling
void vga_scroll_up() {
    int max_offset = buffer_lines - VGA_HEIGHT;
    if (max_offset < 0) max_offset = 0;
    
    if (scroll_offset < max_offset) {
        scroll_offset++;
        vga_refresh();
    }
}

void vga_scroll_down() {
    if (scroll_offset > 0) {
        scroll_offset--;
        vga_refresh();
    }
}

// Zum Ende springen (für normale Eingabe)
void vga_scroll_to_bottom() {
    if (scroll_offset != 0) {
        scroll_offset = 0;
        vga_refresh();
    }
}