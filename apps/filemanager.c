#include "filemanager.h"
#include "vbe.h"
#include "../fs.h"
#include "../string.h"

typedef struct {
    int selected_file;
    char current_path[128];
} FileManagerState;

static FileManagerState fm;

void filemanager_init() {
    fm.selected_file = 0;
    fs_init();
    fs_pwd(fm.current_path);
}

void filemanager_draw(int x, int y, int w, int h, int is_selected) {
    // Legacy function - not used with VBE
}

void filemanager_handle_click(int cursor_x, int cursor_y, int win_x, int win_y) {
    // Legacy function - not used with VBE
}

void filemanager_draw_vbe(int x, int y, int w, int h, int is_selected) {
    // Window background
    vbe_fill_rect(x, y, w, h, WIN10_PANEL);
    vbe_draw_rect(x, y, w, h, WIN10_BORDER);
    
    // Header - current path
    vbe_fill_rect(x + 10, y + 10, w - 20, 25, WIN10_BORDER);
    vbe_draw_text(x + 15, y + 12, "Path: /", VBE_WHITE, VBE_TRANSPARENT);
    vbe_draw_text(x + 60, y + 12, fm.current_path, VBE_WHITE, VBE_TRANSPARENT);
    
    // File list area
    vbe_fill_rect(x + 10, y + 45, w - 20, h - 100, VBE_BLACK);
    vbe_draw_rect(x + 10, y + 45, w - 20, h - 100, VBE_GRAY);
    
    // Instructions
    vbe_draw_text(x + 20, y + 60, "Simple File Browser", VBE_YELLOW, VBE_TRANSPARENT);
    vbe_draw_text(x + 20, y + 85, "Root directory listing", VBE_LIGHT_GRAY, VBE_TRANSPARENT);
    vbe_draw_text(x + 20, y + 110, "Volume: 10GB Virtual Disk", VBE_LIGHT_GRAY, VBE_TRANSPARENT);
    
    // Buttons
    int btn_y = y + h - 35;
    int btn_w = (w - 30) / 2;
    
    vbe_fill_rect(x + 10, btn_y, btn_w, 25, WIN10_ACCENT);
    vbe_draw_text(x + 15 + (btn_w - 40) / 2, btn_y + 5, "Refresh", VBE_WHITE, VBE_TRANSPARENT);
    
    vbe_fill_rect(x + 15 + btn_w, btn_y, btn_w, 25, WIN10_BORDER);
    vbe_draw_text(x + 20 + btn_w + (btn_w - 32) / 2, btn_y + 5, "New", VBE_WHITE, VBE_TRANSPARENT);
}

void filemanager_handle_click_vbe(int rel_x, int rel_y) {
    // Handle file list clicks or button clicks
    if (rel_y >= 45 && rel_y < 200) {
        // File list clicked
    }
}
