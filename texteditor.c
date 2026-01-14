#include "texteditor.h"
#include "../vbe.h"
#include "../gui.h"
#include "../fs.h"
#include "../string.h"

// Externe Deklaration, da fs.h unvollständig ist
int fs_save_file(const char* name, const char* data, int len);

#define MAX_TEXT_SIZE 2048
#define MAX_FILENAME 32

static char text_buffer[MAX_TEXT_SIZE];
static int text_len = 0;
static char filename_buffer[MAX_FILENAME] = "new_file.txt";
static int filename_len = 12;
static int focus_mode = 0; // 0 = Text Area, 1 = Filename Input
static char status_msg[64] = "Ready";

void texteditor_init() {
    text_len = 0;
    text_buffer[0] = '\0';
    strcpy(filename_buffer, "new_file.txt");
    filename_len = 12;
    focus_mode = 0;
    strcpy(status_msg, "Ready");
}

static void load_file() {
    file_t* files = fs_get_table();
    int current_dir = fs_get_current_dir(); // Nimmt aktuelles Verzeichnis vom System
    // Oder wir suchen global/rekursiv, aber hier vereinfacht im aktuellen Kontext oder Root
    // Wir suchen einfach in allen Dateien, da wir keine Pfad-Navigation im Editor haben
    
    int found = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use && files[i].type == FILE_TYPE_FILE && strcmp(files[i].name, filename_buffer) == 0) {
            found = i;
            break;
        }
    }

    if (found != -1) {
        strcpy(text_buffer, files[found].content);
        text_len = strlen(text_buffer);
        strcpy(status_msg, "File loaded.");
    } else {
        strcpy(status_msg, "File not found.");
    }
}

static void save_file() {
    if (fs_save_file(filename_buffer, text_buffer, text_len) == 0) {
        strcpy(status_msg, "File saved.");
    } else {
        strcpy(status_msg, "Error saving file.");
    }
}

void texteditor_draw_vbe(int x, int y, int w, int h, int is_active) {
    // Hintergrund
    vbe_fill_rect(x, y, w, h, 0xFF2D2D30);

    // --- Toolbar ---
    vbe_fill_rect(x, y, w, 40, 0xFF3E3E42);
    
    // Filename Input
    unsigned int input_bg = (focus_mode == 1) ? 0xFFFFFFFF : 0xFFCCCCCC;
    unsigned int input_fg = 0xFF000000;
    vbe_fill_rect(x + 10, y + 5, 200, 30, input_bg);
    vbe_draw_text(x + 15, y + 12, filename_buffer, input_fg, VBE_TRANSPARENT);
    
    // Buttons
    draw_button_modern(x + 220, y + 5, 60, 30, 4, "Load", 0xFF007ACC);
    draw_button_modern(x + 290, y + 5, 60, 30, 4, "Save", 0xFF007ACC);
    
    // Status
    vbe_draw_text(x + 360, y + 12, status_msg, 0xFFAAAAAA, VBE_TRANSPARENT);

    // --- Text Area ---
    int area_y = y + 40;
    int area_h = h - 40;
    unsigned int text_bg = (focus_mode == 0) ? 0xFF1E1E1E : 0xFF252526;
    vbe_fill_rect(x, area_y, w, area_h, text_bg);
    
    // Text rendern (mit einfachem Word-Wrap)
    int cur_x = x + 5;
    int cur_y = area_y + 5;
    
    for (int i = 0; i < text_len; i++) {
        char c = text_buffer[i];
        if (c == '\n') {
            cur_x = x + 5;
            cur_y += 16;
        } else {
            char s[2] = {c, 0};
            vbe_draw_text(cur_x, cur_y, s, 0xFFFFFFFF, VBE_TRANSPARENT);
            cur_x += 8;
            if (cur_x > x + w - 10) {
                cur_x = x + 5;
                cur_y += 16;
            }
        }
    }
    
    // Cursor
    if (is_active && gui_get_caret_state()) {
        if (focus_mode == 0) {
            vbe_fill_rect(cur_x, cur_y, 2, 16, 0xFFFFFFFF);
        } else {
            int fn_w = strlen(filename_buffer) * 8;
            vbe_fill_rect(x + 15 + fn_w, y + 12, 2, 16, 0xFF000000);
        }
    }
}

void texteditor_handle_click(int x, int y) {
    // x, y sind relativ zum Fensterinhalt (ohne Titlebar)
    if (y < 40) {
        // Toolbar
        if (x >= 10 && x <= 210) focus_mode = 1; // Filename
        else if (x >= 220 && x <= 280) load_file();
        else if (x >= 290 && x <= 350) save_file();
        else focus_mode = 0;
    } else {
        focus_mode = 0; // Text Area
    }
}

void texteditor_handle_key(char c) {
    if (focus_mode == 0) {
        if (text_len < MAX_TEXT_SIZE - 1) {
            text_buffer[text_len++] = c;
            text_buffer[text_len] = 0;
        }
    } else {
        if (filename_len < MAX_FILENAME - 1 && c != '\n') {
            filename_buffer[filename_len++] = c;
            filename_buffer[filename_len] = 0;
        }
    }
}

void texteditor_handle_backspace() {
    if (focus_mode == 0 && text_len > 0) {
        text_buffer[--text_len] = 0;
    } else if (focus_mode == 1 && filename_len > 0) {
        filename_buffer[--filename_len] = 0;
    }
}