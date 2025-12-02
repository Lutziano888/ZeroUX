#include "gui.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"

// Text-Mode GUI (80x25, funktioniert sicher)
static unsigned short* VGA = (unsigned short*)0xB8000;
static int selected_window = 0;

#define GUI_WIDTH 80
#define GUI_HEIGHT 25

// Zeichnet eine Box mit ASCII-Zeichen
static void draw_box(int x, int y, int w, int h, unsigned char color) {
    // Obere Linie
    VGA[y * GUI_WIDTH + x] = (color << 8) | 0xDA; // ┌
    for (int i = 1; i < w - 1; i++) {
        VGA[y * GUI_WIDTH + x + i] = (color << 8) | 0xC4; // ─
    }
    VGA[y * GUI_WIDTH + x + w - 1] = (color << 8) | 0xBF; // ┐
    
    // Seiten
    for (int j = 1; j < h - 1; j++) {
        VGA[(y + j) * GUI_WIDTH + x] = (color << 8) | 0xB3; // │
        for (int i = 1; i < w - 1; i++) {
            VGA[(y + j) * GUI_WIDTH + x + i] = (color << 8) | ' ';
        }
        VGA[(y + j) * GUI_WIDTH + x + w - 1] = (color << 8) | 0xB3; // │
    }
    
    // Untere Linie
    VGA[(y + h - 1) * GUI_WIDTH + x] = (color << 8) | 0xC0; // └
    for (int i = 1; i < w - 1; i++) {
        VGA[(y + h - 1) * GUI_WIDTH + x + i] = (color << 8) | 0xC4; // ─
    }
    VGA[(y + h - 1) * GUI_WIDTH + x + w - 1] = (color << 8) | 0xD9; // ┘
}

static void draw_text_at(int x, int y, const char* text, unsigned char color) {
    int i = 0;
    while (text[i] && x + i < GUI_WIDTH) {
        VGA[y * GUI_WIDTH + x + i] = (color << 8) | text[i];
        i++;
    }
}

void gui_init() {
    // Einfach nur Screen clearen
    for (int i = 0; i < GUI_WIDTH * GUI_HEIGHT; i++) {
        VGA[i] = (0x01 << 8) | ' '; // Blauer Hintergrund
    }
}

void gui_draw_desktop() {
    // Desktop Background (blau)
    for (int i = 0; i < GUI_WIDTH * GUI_HEIGHT; i++) {
        VGA[i] = (0x11 << 8) | 0xB0; // Hellblau mit Pattern
    }
    
    // Taskleiste unten (grau)
    for (int x = 0; x < GUI_WIDTH; x++) {
        VGA[24 * GUI_WIDTH + x] = (0x70 << 8) | ' ';
    }
    
    // Start-Button
    draw_text_at(2, 24, " START ", 0x70);
    
    // Titel oben
    draw_text_at(30, 0, " zeroUX Desktop ", 0x1F);
}

void gui_draw_window(int id) {
    // Fenster 1: Willkommen
    if (id == 0) {
        draw_box(10, 3, 60, 12, 0x1F);
        draw_text_at(12, 3, " Welcome to zeroUX ", 0x1F);
        draw_text_at(15, 5, "zeroUX Text-Mode GUI Desktop", 0x0F);
        draw_text_at(15, 7, "This is a safe text-mode interface", 0x0F);
        draw_text_at(15, 9, "Press ESC to return to shell", 0x0E);
        draw_text_at(15, 10, "Press 1-3 to select windows", 0x0E);
    }
    
    // Fenster 2: System Info
    if (id == 1) {
        draw_box(15, 8, 50, 10, 0x2F);
        draw_text_at(17, 8, " System Information ", 0x2F);
        draw_text_at(18, 10, "OS: zeroUX v1.2", 0x0F);
        draw_text_at(18, 11, "Mode: Text-Mode GUI", 0x0F);
        draw_text_at(18, 12, "Display: 80x25 characters", 0x0F);
        draw_text_at(18, 13, "Memory: 128 MB", 0x0F);
    }
    
    // Fenster 3: Calculator
    if (id == 2) {
        draw_box(20, 13, 40, 8, 0x3F);
        draw_text_at(22, 13, " Calculator ", 0x3F);
        draw_text_at(25, 15, "[ 7 ] [ 8 ] [ 9 ] [ / ]", 0x0F);
        draw_text_at(25, 16, "[ 4 ] [ 5 ] [ 6 ] [ * ]", 0x0F);
        draw_text_at(25, 17, "[ 1 ] [ 2 ] [ 3 ] [ - ]", 0x0F);
        draw_text_at(25, 18, "[ 0 ] [ . ] [ = ] [ + ]", 0x0F);
    }
}

void gui_draw_all_windows() {
    // Alle Fenster zeichnen
    gui_draw_window(0);
    gui_draw_window(1);
    gui_draw_window(2);
    
    // Highlight für ausgewähltes Fenster
    const char* windows[] = {"Welcome", "System", "Calculator"};
    draw_text_at(15, 24, "Active: ", 0x70);
    draw_text_at(23, 24, windows[selected_window], 0x74);
}

void gui_run() {
    gui_init();
    gui_draw_desktop();
    gui_draw_all_windows();
    
    // Info-Text
    draw_text_at(55, 24, "ESC=Exit 1-3=Select", 0x70);
    
    // Event Loop
    while (1) {
        unsigned char sc = keyboard_read();
        
        // Ignore key releases
        if (sc & 0x80) continue;
        
        // ESC = zurück zur Shell
        if (sc == 0x01) {
            return;
        }
        
        // 1-3 = Fenster auswählen
        if (sc == 0x02) { // Taste 1
            selected_window = 0;
            gui_draw_desktop();
            gui_draw_all_windows();
        }
        if (sc == 0x03) { // Taste 2
            selected_window = 1;
            gui_draw_desktop();
            gui_draw_all_windows();
        }
        if (sc == 0x04) { // Taste 3
            selected_window = 2;
            gui_draw_desktop();
            gui_draw_all_windows();
        }
    }
}

// Dummy-Funktionen für Kompatibilität
void gui_set_pixel(int x, int y, unsigned char color) {}
void gui_draw_rect(int x, int y, int w, int h, unsigned char color) {}
void gui_draw_text(int x, int y, const char* text, unsigned char color) {}
int gui_create_window(int x, int y, int w, int h, const char* title, unsigned char color) { return 0; }
void gui_close_window(int id) {}
void gui_update_mouse(int x, int y) {}
void gui_draw_mouse() {}