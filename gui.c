#include "gui.h"
#include "vbe.h"
#include "keyboard.h"
#include "mouse.h"
#include "string.h"
#include "rtc.h"
#include "ethernet.h"
#include "apps/calculator.h"
#include "apps/notepad.h"
#include "apps/welcome.h"
#include "apps/filemanager.h"
#include "apps/google_browser.h"

static int selected_window = 0;
static int z_order[5] = {0, 1, 2, 3, 4};

// Mouse cursor
static int cursor_x = 512;
static int cursor_y = 384;
static int mouse_visible = 1;

// Window positions
typedef struct {
    int x, y, w, h;
    int visible;
} Window;

static Window windows[5] = {
    {100, 100, 600, 400, 1},  // Welcome
    {150, 150, 500, 450, 1},  // Notepad
    {250, 200, 400, 400, 1},  // Calculator
    {50, 50, 450, 500, 1},    // File Manager
    {200, 80, 700, 450, 1}    // Google Browser
};

void draw_shadow(int x, int y, int w, int h) {
    unsigned int shadow = 0x30000000; // Semi-transparent black
    for (int sy = 0; sy < 8; sy++) {
        for (int sx = 0; sx < w; sx++) {
            unsigned int base = vbe_get_pixel(x + sx + 8, y + h + sy);
            // Simple alpha blend simulation
            vbe_set_pixel(x + sx + 8, y + h + sy, 
                ((base >> 1) & 0x7F7F7F7F));
        }
    }
    for (int sy = 0; sy < h; sy++) {
        for (int sx = 0; sx < 8; sx++) {
            unsigned int base = vbe_get_pixel(x + w + sx, y + sy + 8);
            vbe_set_pixel(x + w + sx, y + sy + 8, 
                ((base >> 1) & 0x7F7F7F7F));
        }
    }
}

void draw_window_modern(int x, int y, int w, int h, const char* title, int is_active) {
    // Shadow
    draw_shadow(x, y, w, h);
    
    // Window background
    vbe_fill_rect(x, y, w, h, WIN10_PANEL);
    
    // Title bar
    unsigned int title_color = is_active ? WIN10_ACCENT : WIN10_BORDER;
    vbe_fill_rect(x, y, w, 30, title_color);
    
    // Title text
    vbe_draw_text(x + 10, y + 7, title, WIN10_TEXT, VBE_TRANSPARENT);
    
    // Window buttons (Close)
    int btn_y = y + 5;
    vbe_fill_rect(x + w - 45, btn_y, 20, 20, 0x00E81123); // Close
    vbe_draw_text(x + w - 40, btn_y + 2, "X", VBE_WHITE, VBE_TRANSPARENT);
    
    // Border
    vbe_draw_rect(x, y, w, h, is_active ? WIN10_ACCENT : WIN10_BORDER);
}

void draw_button_modern(int x, int y, int w, int h, const char* text, unsigned int color) {
    // Button background
    vbe_fill_rect(x, y, w, h, color);
    
    // Border
    vbe_draw_rect(x, y, w, h, 0x00666666);
    
    // Text centered
    int text_len = 0;
    while (text[text_len]) text_len++;
    int text_x = x + (w - text_len * 8) / 2;
    int text_y = y + (h - 16) / 2;
    
    vbe_draw_text(text_x, text_y, text, VBE_WHITE, VBE_TRANSPARENT);
}

void draw_cursor() {
    // Draw a simple point cursor (small filled circle)
    vbe_fill_circle(cursor_x, cursor_y, 4, VBE_YELLOW);
    vbe_draw_circle(cursor_x, cursor_y, 4, VBE_WHITE);
}

void gui_init() {
    vbe_init();
    vbe_clear(WIN10_BG);
    
    ethernet_init();
    mouse_init();
    calculator_init();
    notepad_init();
    welcome_init();
    filemanager_init();
    google_browser_init();
}

void gui_draw_desktop() {
    // Desktop background gradient
    for (int y = 0; y < vbe_get_height(); y++) {
        unsigned int color = 0x001E1E1E + (y / 4);
        vbe_fill_rect(0, y, vbe_get_width(), 1, color);
    }
    
    // Taskbar
    vbe_fill_rect(0, 0, vbe_get_width(), 40, WIN10_PANEL);
    vbe_draw_rect(0, 0, vbe_get_width(), 40, WIN10_BORDER);
    
    // Start button
    draw_button_modern(10, 5, 80, 30, "START", WIN10_ACCENT);
    
    // Ethernet status indicator (green/red dot)
    unsigned int eth_color = ethernet_is_connected() ? 0x0000FF00 : 0x00FF0000;  // Green or Red
    int dot_x = vbe_get_width() - 100;
    int dot_y = 20;
    vbe_fill_circle(dot_x, dot_y, 6, eth_color);
    vbe_draw_circle(dot_x, dot_y, 6, 0x00FFFFFF);  // White border
    
    // System tray with real time
    int hour, min, sec;
    rtc_get_time(&hour, &min, &sec);
    char time_str[16];
    int_to_str(hour, time_str);
    int len = 0;
    while (time_str[len]) len++;
    time_str[len] = ':';
    time_str[len + 1] = '0' + (min / 10);
    time_str[len + 2] = '0' + (min % 10);
    time_str[len + 3] = '\0';
    vbe_draw_text(vbe_get_width() - 60, 15, time_str, VBE_WHITE, VBE_TRANSPARENT);
}

void gui_draw_window(int id) {
    Window* w = &windows[id];
    if (!w->visible) return;
    
    int is_active = (id == selected_window);
    
    if (id == 0) {
        draw_window_modern(w->x, w->y, w->w, w->h, "Welcome", is_active);
        welcome_draw_vbe(w->x, w->y + 30, w->w, w->h - 30, is_active);
    } else if (id == 1) {
        draw_window_modern(w->x, w->y, w->w, w->h, "Notepad", is_active);
        notepad_draw_vbe(w->x, w->y + 30, w->w, w->h - 30, is_active);
    } else if (id == 2) {
        draw_window_modern(w->x, w->y, w->w, w->h, "Calculator", is_active);
        calculator_draw_vbe(w->x, w->y + 30, w->w, w->h - 30, is_active);
    } else if (id == 3) {
        draw_window_modern(w->x, w->y, w->w, w->h, "File Manager", is_active);
        filemanager_draw_vbe(w->x, w->y + 30, w->w, w->h - 30, is_active);
    } else if (id == 4) {
        draw_window_modern(w->x, w->y, w->w, w->h, "Google Browser", is_active);
        google_browser_draw_vbe(w->x, w->y + 30, w->w, w->h - 30, is_active);
    }
}

void gui_draw_all_windows() {
    for (int i = 0; i < 5; i++) {
        int id = z_order[i];
        if (windows[id].visible) {
            gui_draw_window(id);
        }
    }
}

static void bring_to_front(int id) {
    int found_idx = -1;
    for (int i = 0; i < 5; i++) {
        if (z_order[i] == id) {
            found_idx = i;
            break;
        }
    }
    
    if (found_idx != -1) {
        for (int i = found_idx; i < 4; i++) {
            z_order[i] = z_order[i + 1];
        }
        z_order[4] = id;
    }
}

static int check_window_click(int mx, int my) {
    for (int i = 3; i >= 0; i--) {
        int id = z_order[i];
        Window* w = &windows[id];
        
        if (mx >= w->x && mx < w->x + w->w &&
            my >= w->y && my < w->y + w->h) {
            return id;
        }
    }
    return -1;
}

static int check_close_button(int mx, int my, Window* w) {
    // Close button: x + w - 45 to x + w - 25, y to y + 20
    if (mx >= w->x + w->w - 45 && mx < w->x + w->w - 25 &&
        my >= w->y + 5 && my < w->y + 25) {
        return 1;
    }
    return 0;
}

void gui_run() {
    gui_init();
    gui_draw_desktop();
    gui_draw_all_windows();
    draw_cursor();
    
    int move_step = 15;
    
    while (1) {
        // Keyboard handling
        unsigned char sc = keyboard_read_nonblock();
        if (sc && !(sc & 0x80)) {
            int needs_redraw = 0;
            
            if (sc == 0x01) { // ESC
                return;
            }
            
            // TAB - switch windows
            if (sc == 0x0F) {
                selected_window = (selected_window + 1) % 5;
                bring_to_front(selected_window);
                needs_redraw = 1;
            }
            
            // Arrow keys - move cursor
            if (sc == 0x48) { // Up arrow
                cursor_y -= move_step;
                if (cursor_y < 0) cursor_y = 0;
                needs_redraw = 1;
            }
            else if (sc == 0x50) { // Down arrow
                cursor_y += move_step;
                if (cursor_y >= vbe_get_height()) cursor_y = vbe_get_height() - 1;
                needs_redraw = 1;
            }
            else if (sc == 0x4B) { // Left arrow
                cursor_x -= move_step;
                if (cursor_x < 0) cursor_x = 0;
                needs_redraw = 1;
            }
            else if (sc == 0x4D) { // Right arrow
                cursor_x += move_step;
                if (cursor_x >= vbe_get_width()) cursor_x = vbe_get_width() - 1;
                needs_redraw = 1;
            }
            
            // Backspace for Notepad
            if (sc == 0x0E && selected_window == 1) { // Backspace
                notepad_handle_backspace();
                needs_redraw = 1;
            }
            
            // Enter - click on window under cursor or send to apps
            if (sc == 0x1C) { // Enter
                int clicked = check_window_click(cursor_x, cursor_y);
                if (clicked != -1) {
                    selected_window = clicked;
                    bring_to_front(clicked);
                    
                    Window* w = &windows[clicked];
                    
                    // Check if close button was clicked
                    if (check_close_button(cursor_x, cursor_y, w)) {
                        w->visible = 0;
                        needs_redraw = 1;
                    }
                    // Otherwise, handle window content click
                    else {
                        int rel_x = cursor_x - w->x;
                        int rel_y = cursor_y - w->y - 30;
                        
                        if (clicked == 0) {
                            welcome_handle_click_vbe(rel_x, rel_y);
                        } else if (clicked == 1) {
                            notepad_handle_click_vbe(rel_x, rel_y);
                        } else if (clicked == 2) {
                            calculator_handle_click_vbe(rel_x, rel_y);
                        } else if (clicked == 3) {
                            filemanager_handle_click_vbe(rel_x, rel_y);
                        } else if (clicked == 4) {
                            google_browser_handle_click_vbe(rel_x, rel_y);
                        }
                        needs_redraw = 1;
                    }
                }
                // Send Enter to Notepad if active
                if (selected_window == 1) {
                    notepad_handle_char('\n');
                    needs_redraw = 1;
                }
            }
            // Spacebar or other text input for Notepad
            else if (sc == 0x39) { // Spacebar
                if (selected_window == 1) {
                    notepad_handle_char(' ');
                    needs_redraw = 1;
                }
            }
            // Convert scancode to ASCII for text input
            else {
                char ascii = scancode_to_ascii(sc);
                if (ascii && selected_window == 1) {
                    notepad_handle_char(ascii);
                    needs_redraw = 1;
                }
            }
            
            if (needs_redraw) {
                gui_draw_desktop();
                gui_draw_all_windows();
                draw_cursor();
            }
        }
        
        // Small delay to prevent CPU spinning
        for (volatile int i = 0; i < 10000; i++);
    }
}

// Compatibility stubs
void draw_box(int x, int y, int w, int h, unsigned char color) {}
void draw_text_at(int x, int y, const char* text, unsigned char color) {}
void draw_button(int x, int y, const char* text, unsigned char color) {}
int check_button_click(int cx, int cy, int bx, int by, const char* text) { return 0; }