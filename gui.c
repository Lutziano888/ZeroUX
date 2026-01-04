#include "gui.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "port.h"
#include "apps/calculator.h"
#include "apps/notepad.h"
#include "apps/welcome.h"

#define GUI_WIDTH 80
#define GUI_HEIGHT 25

static unsigned short* VGA = (unsigned short*)0xB8000;
static int selected_window = 0;
static int z_order[3] = {0, 1, 2};

// Cursor
static int cursor_x = 40;
static int cursor_y = 12;
static unsigned short saved_char = 0;
static int cursor_visible = 0;

static void set_char(int x, int y, unsigned char ch, unsigned char color) {
    if (x >= 0 && x < GUI_WIDTH && y >= 0 && y < GUI_HEIGHT) {
        VGA[y * GUI_WIDTH + x] = (color << 8) | ch;
    }
}

void draw_box(int x, int y, int w, int h, unsigned char color) {
    // Ecken
    set_char(x, y, 0xDA, color);
    set_char(x + w - 1, y, 0xBF, color);
    set_char(x, y + h - 1, 0xC0, color);
    set_char(x + w - 1, y + h - 1, 0xD9, color);
    
    // Horizontale Linien
    for (int i = 1; i < w - 1; i++) {
        set_char(x + i, y, 0xC4, color);
        set_char(x + i, y + h - 1, 0xC4, color);
    }
    
    // Vertikale Linien und Füllung
    for (int j = 1; j < h - 1; j++) {
        set_char(x, y + j, 0xB3, color);
        set_char(x + w - 1, y + j, 0xB3, color);
        // Innenfläche leeren
        for (int i = 1; i < w - 1; i++) {
            set_char(x + i, y + j, ' ', color & 0xF0);
        }
    }
}

void draw_text_at(int x, int y, const char* text, unsigned char color) {
    int i = 0;
    while (text[i] && x + i < GUI_WIDTH) {
        set_char(x + i, y, text[i], color);
        i++;
    }
}

static int str_len(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

void draw_button(int x, int y, const char* text, unsigned char color) {
    set_char(x, y, '[', color);
    draw_text_at(x + 1, y, text, color);
    set_char(x + 1 + str_len(text), y, ']', color);
}

static void hide_cursor() {
    if (cursor_visible) {
        VGA[cursor_y * GUI_WIDTH + cursor_x] = saved_char;
        cursor_visible = 0;
    }
}

static void show_cursor() {
    if (cursor_x >= 0 && cursor_x < GUI_WIDTH && cursor_y >= 0 && cursor_y < GUI_HEIGHT) {
        saved_char = VGA[cursor_y * GUI_WIDTH + cursor_x];
        VGA[cursor_y * GUI_WIDTH + cursor_x] = (0xF0 << 8) | 0xDB;
        cursor_visible = 1;
    }
}

static void move_cursor(int dx, int dy) {
    hide_cursor();
    cursor_x += dx;
    cursor_y += dy;
    
    if (cursor_x < 0) cursor_x = 0;
    if (cursor_x >= GUI_WIDTH) cursor_x = GUI_WIDTH - 1;
    if (cursor_y < 0) cursor_y = 0;
    if (cursor_y >= GUI_HEIGHT) cursor_y = GUI_HEIGHT - 1;
    
    show_cursor();
}

void gui_init() {
    for (int i = 0; i < GUI_WIDTH * GUI_HEIGHT; i++) {
        VGA[i] = (0x01 << 8) | ' ';
    }
    
    calculator_init();
    notepad_init();
    welcome_init();
}

void gui_draw_desktop() {
    // Desktop-Hintergrund
    for (int i = 0; i < GUI_WIDTH * GUI_HEIGHT; i++) {
        VGA[i] = (0x11 << 8) | 0xB0;
    }
    
    // Taskbar
    for (int x = 0; x < GUI_WIDTH; x++) {
        set_char(x, 24, ' ', 0x70);
    }
    
    draw_text_at(2, 24, " START ", 0x70);
    draw_text_at(30, 0, " zeroUX Desktop ", 0x1F);
}

void gui_draw_window(int id) {
    int is_selected = (id == selected_window);
    
    if (id == 0) {
        welcome_draw(10, 3, 60, 12, is_selected);
    } else if (id == 1) {
        notepad_draw(15, 8, 50, 13, is_selected);
    } else if (id == 2) {
        calculator_draw(25, 10, 30, 12, is_selected);
    }
}

static void bring_to_front(int id) {
    int found_idx = -1;
    for (int i = 0; i < 3; i++) {
        if (z_order[i] == id) {
            found_idx = i;
            break;
        }
    }

    if (found_idx != -1) {
        for (int i = found_idx; i < 2; i++) {
            z_order[i] = z_order[i + 1];
        }
        z_order[2] = id;
    }
}

void gui_draw_all_windows() {
    // Zeichne Fenster in Z-Order
    for (int i = 0; i < 3; i++) {
        gui_draw_window(z_order[i]);
    }
    
    // Taskbar-Info (optional ausblenden)
    // const char* windows[] = {"Welcome", "Notepad", "Calculator"};
    // draw_text_at(15, 24, "Active: ", 0x70);
    // draw_text_at(23, 24, windows[selected_window], 0x74);
}

int check_button_click(int cx, int cy, int bx, int by, const char* text) {
    int btn_width = str_len(text) + 2;
    return (cx >= bx && cx < bx + btn_width && cy == by);
}

void gui_run() {
    gui_init();
    gui_draw_desktop();
    gui_draw_all_windows();
    
    draw_text_at(35, 24, "Arrows=Move ENTER=Click TAB=Switch ESC=Exit", 0x70);
    
    cursor_x = 40;
    cursor_y = 12;
    show_cursor();
    
    while (1) {
        unsigned char sc = keyboard_read();
        
        if (sc & 0x80) continue;
        
        // Notepad Typing Mode
        if (notepad_is_active()) {
            if (sc == 0x01) { // ESC
                notepad_deactivate();
                hide_cursor();
                gui_draw_desktop();
                gui_draw_all_windows();
                show_cursor();
            } else if (sc == 0x0E) { // Backspace
                notepad_handle_backspace();
                hide_cursor();
                gui_draw_desktop();
                gui_draw_all_windows();
                show_cursor();
            } else {
                char c = scancode_to_ascii(sc);
                if (c) {
                    notepad_handle_char(c);
                    hide_cursor();
                    gui_draw_desktop();
                    gui_draw_all_windows();
                    show_cursor();
                }
            }
            continue;
        }
        
        // Normal Mode
        if (sc == 0x01) { // ESC = Exit
            hide_cursor();
            for (int i = 0; i < 80 * 25; i++) {
                VGA[i] = 0x0720;
            }
            outb(0x3D4, 0x0F);
            outb(0x3D5, 0);
            outb(0x3D4, 0x0E);
            outb(0x3D5, 0);
            return;
        }
        
        // TAB = Switch Window
        if (sc == 0x0F) {
            selected_window = (selected_window + 1) % 3;
            hide_cursor();
            gui_draw_desktop();
            gui_draw_all_windows();
            show_cursor();
            continue;
        }
        
        // Pfeiltasten
        if (sc == 0x48) move_cursor(0, -1);
        else if (sc == 0x50) move_cursor(0, 1);
        else if (sc == 0x4B) move_cursor(-1, 0);
        else if (sc == 0x4D) move_cursor(1, 0);
        
        // ENTER = Click
        else if (sc == 0x1C) {
            int clicked_window = -1;

            // Welches Fenster wurde angeklickt?
            for (int i = 2; i >= 0; i--) {
                int id = z_order[i];
                int win_x, win_y, win_w, win_h;
                
                if (id == 0) { 
                    win_x = 10; win_y = 3; win_w = 60; win_h = 12; 
                } else if (id == 1) { 
                    win_x = 15; win_y = 8; win_w = 50; win_h = 13; 
                } else { 
                    win_x = 25; win_y = 10; win_w = 30; win_h = 12; 
                }

                if (cursor_x >= win_x && cursor_x < win_x + win_w && 
                    cursor_y >= win_y && cursor_y < win_y + win_h) {
                    clicked_window = id;
                    break; 
                }
            }

            if (clicked_window != -1) {
                selected_window = clicked_window;
                bring_to_front(clicked_window);
            }

            // App-Handler aufrufen
            if (selected_window == 0) {
                welcome_handle_click(cursor_x, cursor_y, 10, 3);
            } else if (selected_window == 1) {
                notepad_handle_click(cursor_x, cursor_y, 15, 8);
            } else if (selected_window == 2) {
                calculator_handle_click(cursor_x, cursor_y, 25, 10);
            }

            hide_cursor();
            gui_draw_desktop();
            gui_draw_all_windows();
            show_cursor();
        }
    }
}

// Dummy-Funktionen
void gui_set_pixel(int x, int y, unsigned char color) {}
void gui_draw_rect(int x, int y, int w, int h, unsigned char color) {}
void gui_draw_text(int x, int y, const char* text, unsigned char color) {}
int gui_create_window(int x, int y, int w, int h, const char* title, unsigned char color) { return 0; }
void gui_close_window(int id) {}
void gui_update_mouse(int x, int y) {}
void gui_draw_mouse() {}