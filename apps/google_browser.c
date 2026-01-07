#include "google_browser.h"
#include "vbe.h"
#include "../gui.h"
#include "../widgets.h"
#include "../gui_colors.h"
#include "../string.h"
#include "../ethernet.h"

// =====================
// State
// =====================
#define MAX_URL_LENGTH 256
#define MAX_HISTORY 20
#define BUFFER_SIZE 2048

typedef struct {
    char current_url[MAX_URL_LENGTH];
    char search_query[MAX_URL_LENGTH];
    char address_bar[MAX_URL_LENGTH];
    int address_bar_active;
    int cursor_pos;
    char display_content[BUFFER_SIZE];
    int is_loading;
    int history_index;
    int history_count;
} BrowserState;

static BrowserState browser = {
    .current_url = "https://www.google.com",
    .address_bar = "https://www.google.com",
    .address_bar_active = 0,
    .cursor_pos = 0,
    .is_loading = 0,
    .history_index = 0,
    .history_count = 0
};

// =====================
// Initialization
// =====================
void google_browser_init() {
    // VBE uses 32-bit colors
    // Styles are for text-mode, using single byte
    // For VBE graphics we use direct color values
    
    // Check network connectivity
    ethernet_init();
    if (ethernet_is_connected()) {
        strcpy(browser.display_content, "Google Browser - Connected\n\nWelcome to Google Browser!\nNetwork connection established.");
    } else {
        strcpy(browser.display_content, "Google Browser - Offline Mode\n\nNetwork connection not available.\nPlease check your Ethernet connection.");
    }
}

// =====================
// URL Parsing
// =====================
static void construct_google_search_url(const char* query) {
    strcpy(browser.current_url, "https://www.google.com/search?q=");
    strcat(browser.current_url, query);
}

static void fetch_page(const char* url) {
    browser.is_loading = 1;
    
    if (!ethernet_is_connected()) {
        strcpy(browser.display_content, "Error: No network connection\nPlease connect Ethernet");
        browser.is_loading = 0;
        return;
    }
    
    // Simulate page fetch
    if (strstr(url, "google.com") != 0) {
        strcpy(browser.display_content, 
            "GOOGLE\n\n"
            "[Search Box]\n\n"
            "Suggestions:\n"
            "- Gmail\n"
            "- Google Drive\n"
            "- Google Photos\n"
            "- Google Maps\n"
            "- YouTube\n");
    } else {
        strcpy(browser.display_content, 
            "Page Loading...\n\n"
            "In full implementation, this would fetch:\n");
        strcat(browser.display_content, url);
    }
    
    browser.is_loading = 0;
}

// =====================
// Draw VBE Graphics
// =====================
void google_browser_draw_vbe(int x, int y, int w, int h, int is_selected) {
    // Draw window background
    vbe_draw_rect(x, y, w, h, DARK_GRAY);
    
    // Draw title bar
    vbe_draw_rect(x, y, w, 30, LIGHT_BLUE);
    vbe_draw_text(x + 10, y + 8, "Google Browser", VBE_WHITE, VBE_TRANSPARENT);
    
    // Draw network status
    if (ethernet_is_connected()) {
        vbe_draw_text(x + w - 100, y + 8, "Online", 0x00FF00, VBE_TRANSPARENT);
    } else {
        vbe_draw_text(x + w - 100, y + 8, "Offline", 0xFF0000, VBE_TRANSPARENT);
    }
    
    // Draw address bar
    vbe_draw_rect(x + 10, y + 40, w - 20, 25, 0xFFFFFF);
    vbe_draw_text(x + 15, y + 45, browser.address_bar, 0x000000, VBE_TRANSPARENT);
    
    // Draw buttons
    vbe_draw_rect(x + 10, y + 75, 60, 20, LIGHT_GRAY);
    vbe_draw_text(x + 15, y + 78, "Back", 0x000000, VBE_TRANSPARENT);
    
    vbe_draw_rect(x + 80, y + 75, 60, 20, LIGHT_GRAY);
    vbe_draw_text(x + 85, y + 78, "Forward", 0x000000, VBE_TRANSPARENT);
    
    vbe_draw_rect(x + 150, y + 75, 60, 20, LIGHT_GRAY);
    vbe_draw_text(x + 155, y + 78, "Refresh", 0x000000, VBE_TRANSPARENT);
    
    // Draw content area
    int content_y = y + 110;
    vbe_draw_rect(x + 10, content_y, w - 20, h - 130, 0xFFFFFF);
    
    // Draw page content
    if (browser.is_loading) {
        vbe_draw_text(x + 20, content_y + 20, "Loading...", 0x0000FF, VBE_TRANSPARENT);
    } else {
        int text_y = content_y + 10;
        const char* line = browser.display_content;
        const char* next_line;
        
        while (*line && text_y < content_y + h - 140) {
            next_line = strstr(line, "\n");
            if (next_line) {
                char buffer[256];
                int len = next_line - line;
                if (len > 255) len = 255;
                memcpy(buffer, line, len);
                buffer[len] = '\0';
                vbe_draw_text(x + 20, text_y, buffer, 0x000000, VBE_TRANSPARENT);
                line = next_line + 1;
                text_y += 15;
            } else {
                vbe_draw_text(x + 20, text_y, line, 0x000000, VBE_TRANSPARENT);
                break;
            }
        }
    }
}

void google_browser_draw(int x, int y, int w, int h, int is_selected) {
    // Text mode version not used with VBE
}

// =====================
// Input Handling
// =====================
void google_browser_handle_input(char key) {
    if (!browser.address_bar_active)
        return;
    
    if (key == 0x0D) { // Enter
        strcpy(browser.current_url, browser.address_bar);
        fetch_page(browser.current_url);
        browser.address_bar_active = 0;
    } else if (key == 0x08) { // Backspace
        if (browser.cursor_pos > 0) {
            browser.address_bar[--browser.cursor_pos] = '\0';
        }
    } else if (key >= 32 && key < 127) { // Printable character
        if (browser.cursor_pos < MAX_URL_LENGTH - 1) {
            browser.address_bar[browser.cursor_pos++] = key;
            browser.address_bar[browser.cursor_pos] = '\0';
        }
    }
}

// =====================
// Click Handling
// =====================
void google_browser_handle_click_vbe(int cursor_x, int cursor_y) {
    // Click on address bar to activate
    // Coordinates depend on window position
}

void google_browser_handle_click(int cursor_x, int cursor_y, int win_x, int win_y) {
    // Legacy text mode handler
}
