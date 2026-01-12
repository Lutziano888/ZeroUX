#include "memtest_app.h"
#include "../vbe.h"
#include "../widgets.h"
#include "../gui.h"
#include "../pmm.h"
#include "../string.h"

static int test_running = 0;
static int test_progress = 0;
static char status_msg[64] = "Ready to test RAM";
static int errors_found = 0;

void memtest_init() {
    test_running = 0;
    test_progress = 0;
    errors_found = 0;
    strcpy(status_msg, "Ready to test RAM");
}

// Führt den Test synchron aus (blockiert kurzzeitig die GUI)
static void run_memory_test() {
    if (test_running) return;
    
    test_running = 1;
    errors_found = 0;
    strcpy(status_msg, "Testing 1MB RAM...");
    
    // Wir testen 256 Blöcke (256 * 4KB = 1 MB)
    int blocks_to_test = 256;
    
    for (int i = 0; i < blocks_to_test; i++) {
        void* ptr = pmm_alloc_block();
        if (!ptr) {
            strcpy(status_msg, "Error: Out of Memory!");
            test_running = 0;
            return;
        }
        
        unsigned char* mem = (unsigned char*)ptr;
        
        // Muster 1: 0xAA (10101010)
        for (int j = 0; j < 4096; j++) mem[j] = 0xAA;
        for (int j = 0; j < 4096; j++) {
            if (mem[j] != 0xAA) errors_found++;
        }
        
        // Muster 2: 0x55 (01010101)
        for (int j = 0; j < 4096; j++) mem[j] = 0x55;
        for (int j = 0; j < 4096; j++) {
            if (mem[j] != 0x55) errors_found++;
        }
        
        pmm_free_block(ptr);
        
        // Fortschritt aktualisieren
        test_progress = ((i + 1) * 100) / blocks_to_test;
    }
    
    test_running = 0;
    
    if (errors_found == 0) {
        strcpy(status_msg, "Success: RAM OK!");
    } else {
        strcpy(status_msg, "FAILURE: Errors found!");
    }
}

void memtest_draw_vbe(int x, int y, int w, int h, int is_active) {
    // Hintergrund
    vbe_fill_rect(x, y, w, h, 0xFFF0F0F0);
    
    // Header Bereich
    vbe_fill_rect(x + 20, y + 20, w - 40, 80, 0xFFFFFFFF);
    vbe_draw_rect(x + 20, y + 20, w - 40, 80, 0xFFCCCCCC);
    
    // Titel & Status
    vbe_draw_text(x + 30, y + 30, "Memory Doctor", 0xFF000000, 0xFFFFFFFF);
    
    unsigned int status_color = (errors_found > 0) ? 0xFFFF0000 : 0xFF008000;
    if (test_running) status_color = 0xFF0000FF;
    
    vbe_draw_text(x + 30, y + 60, status_msg, status_color, 0xFFFFFFFF);
    
    // Progress Bar Hintergrund
    vbe_fill_rect(x + 20, y + 120, w - 40, 20, 0xFFCCCCCC);
    
    // Progress Bar Füllung
    if (test_progress > 0) {
        int bar_width = ((w - 40) * test_progress) / 100;
        unsigned int bar_color = (errors_found > 0) ? 0xFFFF4444 : 0xFF44FF44;
        vbe_fill_rect(x + 20, y + 120, bar_width, 20, bar_color);
    }
    
    // Prozentanzeige
    char pct[8];
    int_to_str(test_progress, pct);
    int len = 0; while(pct[len]) len++;
    pct[len] = '%'; pct[len+1] = 0;
    vbe_draw_text(x + w/2 - 10, y + 122, pct, 0xFF000000, VBE_TRANSPARENT);
    
    // Start Button
    draw_button_modern(x + w/2 - 60, y + 160, 120, 40, 5, "START TEST", 0xFF0078D7);
    
    // Info Text
    vbe_draw_text(x + 20, y + 220, "Allocates and verifies 1MB", 0xFF808080, VBE_TRANSPARENT);
    vbe_draw_text(x + 20, y + 240, "of physical RAM blocks.", 0xFF808080, VBE_TRANSPARENT);
}

void memtest_handle_click_vbe(int x, int y) {
    // Button Koordinaten relativ zum Fensterinhalt (x + w/2 - 60)
    // Da wir w nicht kennen, schätzen wir oder nutzen feste Layouts.
    // Hier vereinfacht: Wir nehmen an, das Fenster ist ca 400px breit (siehe gui.c)
    // Button ist ca bei x=140 bis 260, y=160 bis 200
    
    if (y >= 160 && y <= 200 && x >= 140 && x <= 260) {
        run_memory_test();
    }
}