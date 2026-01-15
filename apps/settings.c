#include "settings.h"
#include "../gui.h"
#include "../vbe.h"
#include "../mouse.h"
#include "../string.h"
#include "../ethernet.h"

// Fix für fehlende Prototypen in mouse.h
void mouse_set_sensitivity(int percent);
int mouse_get_sensitivity();

// Tabs
#define TAB_MOUSE    0
#define TAB_DISPLAY  1
#define TAB_PERSON   2
#define TAB_NETWORK  3

static int current_tab = TAB_MOUSE;

// Colors
#define COL_SIDEBAR  0xFF252526
#define COL_ACTIVE   0xFF37373D
#define COL_BG       0xFF1E1E1E
#define COL_ACCENT   0xFF007ACC
#define COL_TEXT     0xFFFFFFFF
#define COL_TEXT_DIM 0xFFAAAAAA

void settings_init() {
    current_tab = TAB_MOUSE;
}

static void draw_sidebar_item(int x, int y, const char* text, int tab_id) {
    unsigned int bg = (current_tab == tab_id) ? COL_ACTIVE : COL_SIDEBAR;
    unsigned int fg = (current_tab == tab_id) ? COL_TEXT : COL_TEXT_DIM;
    
    // Highlight bar
    if (current_tab == tab_id) {
        vbe_fill_rect(x, y, 3, 40, COL_ACCENT);
    }
    
    vbe_fill_rect(x + 3, y, 147, 40, bg);
    vbe_draw_text(x + 15, y + 12, text, fg, VBE_TRANSPARENT);
}

void settings_draw_vbe(int x, int y, int w, int h, int is_active) {
    // Background
    vbe_fill_rect(x, y, w, h, COL_BG);
    
    // Sidebar
    vbe_fill_rect(x, y, 150, h, COL_SIDEBAR);
    draw_sidebar_item(x, y + 10, "Mouse", TAB_MOUSE);
    draw_sidebar_item(x, y + 50, "Display", TAB_DISPLAY);
    draw_sidebar_item(x, y + 90, "Personalization", TAB_PERSON);
    draw_sidebar_item(x, y + 130, "Network", TAB_NETWORK);
    
    // Content Area
    int cx = x + 170;
    int cy = y + 20;
    
    if (current_tab == TAB_MOUSE) {
        vbe_draw_text(cx, cy, "Mouse Settings", COL_TEXT, VBE_TRANSPARENT);
        cy += 40;
        
        vbe_draw_text(cx, cy, "Sensitivity", COL_TEXT_DIM, VBE_TRANSPARENT);
        cy += 25;
        
        int sens = mouse_get_sensitivity();
        char buf[16];
        int_to_str(sens, buf);
        strcat(buf, "%");
        
        // Slider Bar Background
        vbe_fill_rect(cx, cy, 200, 6, 0xFF404040);
        
        // Slider Fill
        int fill_w = (sens * 200) / 200; // Max 200%
        if (fill_w > 200) fill_w = 200;
        vbe_fill_rect(cx, cy, fill_w, 6, COL_ACCENT);
        
        // Knob
        vbe_fill_rect(cx + fill_w - 4, cy - 4, 8, 14, COL_TEXT);
        
        vbe_draw_text(cx + 220, cy - 4, buf, COL_TEXT, VBE_TRANSPARENT);
        
        // Buttons
        draw_button_modern(cx, cy + 40, 40, 30, 4, "-", 0xFF333333);
        draw_button_modern(cx + 50, cy + 40, 40, 30, 4, "+", 0xFF333333);
        
        vbe_draw_text(cx, cy + 90, "Note: Changes apply immediately.", 0xFF666666, VBE_TRANSPARENT);
    }
    else if (current_tab == TAB_DISPLAY) {
        vbe_draw_text(cx, cy, "Display Settings", COL_TEXT, VBE_TRANSPARENT);
        cy += 40;
        
        vbe_draw_text(cx, cy, "Resolution", COL_TEXT_DIM, VBE_TRANSPARENT);
        cy += 25;
        
        // Mockup buttons for resolution
        draw_button_modern(cx, cy, 140, 35, 4, "1920 x 1080", COL_ACCENT);
        draw_button_modern(cx, cy + 45, 140, 35, 4, "1280 x 720", 0xFF333333);
        draw_button_modern(cx, cy + 90, 140, 35, 4, "1024 x 768", 0xFF333333);
        
        vbe_draw_text(cx + 160, cy + 10, "(Current)", 0xFF888888, VBE_TRANSPARENT);
    }
    else if (current_tab == TAB_PERSON) {
        vbe_draw_text(cx, cy, "Personalization", COL_TEXT, VBE_TRANSPARENT);
        cy += 40;
        
        vbe_draw_text(cx, cy, "Desktop Background", COL_TEXT_DIM, VBE_TRANSPARENT);
        cy += 25;
        
        // Color Presets
        // 1. Default Blue
        vbe_fill_rect(cx, cy, 40, 40, 0xFF1A2F4F);
        vbe_draw_rect(cx, cy, 40, 40, 0xFFFFFFFF);
        vbe_draw_text(cx, cy + 45, "Default", 0xFF888888, VBE_TRANSPARENT);
        
        // 2. Midnight
        vbe_fill_rect(cx + 60, cy, 40, 40, 0xFF000000);
        vbe_draw_rect(cx + 60, cy, 40, 40, 0xFF555555);
        vbe_draw_text(cx + 60, cy + 45, "Dark", 0xFF888888, VBE_TRANSPARENT);
        
        // 3. Sunset
        vbe_fill_rect(cx + 120, cy, 40, 40, 0xFF4A1C40);
        vbe_draw_rect(cx + 120, cy, 40, 40, 0xFF555555);
        vbe_draw_text(cx + 120, cy + 45, "Sunset", 0xFF888888, VBE_TRANSPARENT);
        
        // 4. Forest
        vbe_fill_rect(cx + 180, cy, 40, 40, 0xFF1A2F1A);
        vbe_draw_rect(cx + 180, cy, 40, 40, 0xFF555555);
        vbe_draw_text(cx + 180, cy + 45, "Forest", 0xFF888888, VBE_TRANSPARENT);
    }
    else if (current_tab == TAB_NETWORK) {
        vbe_draw_text(cx, cy, "Network Settings", COL_TEXT, VBE_TRANSPARENT);
        cy += 40;
        
        vbe_draw_text(cx, cy, "IP Address", COL_TEXT_DIM, VBE_TRANSPARENT);
        cy += 25;
        
        IP_Address ip = ethernet_get_ip();
        char buf[32];
        char num[8];
        int_to_str(ip.addr[0], buf); strcat(buf, ".");
        int_to_str(ip.addr[1], num); strcat(buf, num); strcat(buf, ".");
        int_to_str(ip.addr[2], num); strcat(buf, num); strcat(buf, ".");
        int_to_str(ip.addr[3], num); strcat(buf, num);
        
        vbe_draw_text(cx, cy, buf, COL_TEXT, VBE_TRANSPARENT);
    }
}

void settings_handle_click_vbe(int x, int y) {
    // Sidebar Click
    if (x < 150) {
        if (y >= 10 && y < 50) current_tab = TAB_MOUSE;
        else if (y >= 50 && y < 90) current_tab = TAB_DISPLAY;
        else if (y >= 90 && y < 130) current_tab = TAB_PERSON;
        else if (y >= 130 && y < 170) current_tab = TAB_NETWORK;
        return;
    }
    
    int cx = 170; // Relative content start x
    int cy = 20;  // Relative content start y
    
    if (current_tab == TAB_MOUSE) {
        int slider_y = cy + 65;
        int btn_y = cy + 105;
        
        // Slider Click (Simple jump)
        if (x >= cx && x <= cx + 200 && y >= slider_y - 5 && y <= slider_y + 15) {
            int val = ((x - cx) * 200) / 200;
            mouse_set_sensitivity(val);
        }
        
        // Buttons
        if (y >= btn_y && y <= btn_y + 30) {
            int sens = mouse_get_sensitivity();
            if (x >= cx && x <= cx + 40) {
                mouse_set_sensitivity(sens - 10);
            } else if (x >= cx + 50 && x <= cx + 90) {
                mouse_set_sensitivity(sens + 10);
            }
        }
    }
    else if (current_tab == TAB_DISPLAY) {
        // Resolution buttons (Mockup logic)
        // In a real OS, this would call vbe_set_mode()
    }
    else if (current_tab == TAB_PERSON) {
        int swatch_y = cy + 65;
        if (y >= swatch_y && y <= swatch_y + 40) {
            if (x >= cx && x <= cx + 40) {
                // Default Blue
                gui_set_desktop_gradient(0x001A2F4F, 0x002C4A6F);
            }
            else if (x >= cx + 60 && x <= cx + 100) {
                // Dark / Midnight
                gui_set_desktop_gradient(0x00050505, 0x00151515);
            }
            else if (x >= cx + 120 && x <= cx + 160) {
                // Sunset (Purple/Orange)
                gui_set_desktop_gradient(0x002C1A4F, 0x006F2C2C);
            }
            else if (x >= cx + 180 && x <= cx + 220) {
                // Forest (Green)
                gui_set_desktop_gradient(0x000F2F1A, 0x001A4F2C);
            }
        }
    }
}