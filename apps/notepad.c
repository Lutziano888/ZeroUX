#include "notepad.h"
#include "../gui.h"
#include "../widgets.h"
#include "../string.h"

// State
typedef struct {
    char text[200];
    int cursor;
    int active;
    int char_count;
    int line_count;
    WindowStyle* current_theme;  // Aktuelles Theme
} NotepadState;

static NotepadState notepad;

// Styles
static WidgetStyle STYLE_EDITOR;
static WidgetStyle STYLE_STATUSBAR;
static WidgetStyle STYLE_BTN_CLEAR;
static WidgetStyle STYLE_BTN_SAVE;

static void setup_styles() {
    STYLE_EDITOR.bg_color = 0x70;
    STYLE_EDITOR.border_color = 0x70;
    
    STYLE_STATUSBAR.bg_color = 0x08;
    STYLE_STATUSBAR.fg_color = 0x07;
    
    STYLE_BTN_CLEAR.bg_color = 0x0C;
    STYLE_BTN_SAVE.bg_color = 0x0A;
}

static void update_stats() {
    notepad.char_count = 0;
    notepad.line_count = 1;
    
    for (int i = 0; i < 200 && notepad.text[i]; i++) {
        notepad.char_count++;
        if (notepad.text[i] == '\n') {
            notepad.line_count++;
        }
    }
}

void notepad_init() {
    strcpy(notepad.text, "Click in text area to type...");
    notepad.cursor = 0;
    notepad.active = 0;
    update_stats();
    
    // Wähle ein Theme - hier kannst du ändern!
    // Optionen: &WINDOW_BLUE, &WINDOW_DARK, &WINDOW_LIGHT, &WINDOW_GREEN, &WINDOW_PURPLE, &WINDOW_RED
    notepad.current_theme = &WINDOW_LIGHT;
    
    setup_styles();
}

int notepad_is_active() {
    return notepad.active;
}

void notepad_deactivate() {
    notepad.active = 0;
}

void notepad_handle_backspace() {
    if (notepad.cursor > 0) {
        notepad.cursor--;
        for (int i = notepad.cursor; i < 199; i++) {
            notepad.text[i] = notepad.text[i + 1];
        }
        update_stats();
    }
}

void notepad_handle_char(char c) {
    if (notepad.cursor < 199) {
        if (c == '\n' || (c >= 32 && c <= 126)) {
            for (int i = 199; i > notepad.cursor; i--) {
                notepad.text[i] = notepad.text[i - 1];
            }
            notepad.text[notepad.cursor] = c;
            notepad.cursor++;
            notepad.text[notepad.cursor] = '\0';
            update_stats();
        }
    }
}

void notepad_draw(int x, int y, int w, int h, int is_selected) {
    // Zeichne Fenster mit Theme
    draw_window(x, y, w, h, " Notepad ", notepad.current_theme, is_selected);
    
    // Status
    if (notepad.active && is_selected) {
        draw_text_at(x + 2, y + 1, " [TYPING - ESC to stop] ", 0x2F);
    } else {
        draw_text_at(x + 2, y + 1, " [Click text to type] ", 0x0E);
    }
    
    // Editor
    draw_box(x + 2, y + 2, w - 4, 7, notepad.current_theme->content_bg);
    
    // Text rendern mit Theme-Farben
    int line = y + 3;
    int col = x + 3;
    int start_col = x + 3;
    int max_col = x + w - 4;
    int max_line = y + 8;
    
    unsigned short* VGA = (unsigned short*)0xB8000;
    unsigned char text_color = notepad.current_theme->content_fg;
    
    for (int i = 0; i < 200 && notepad.text[i]; i++) {
        if (notepad.text[i] == '\n') {
            line++;
            col = start_col;
            if (line >= max_line) break;
        } else {
            if (col < max_col && col >= 0 && col < 80 && line >= 0 && line < 25) {
                VGA[line * 80 + col] = (text_color << 8) | notepad.text[i];
            }
            col++;
            if (col >= max_col) {
                line++;
                col = start_col;
                if (line >= max_line) break;
            }
        }
    }
    
    // Statusbar
    char stats[50];
    strcpy(stats, "Chars:");
    char num_buf[10];
    int_to_str(notepad.char_count, num_buf);
    strcat(stats, num_buf);
    strcat(stats, " Lines:");
    int_to_str(notepad.line_count, num_buf);
    strcat(stats, num_buf);
    
    draw_text_at(x + 3, y + 9, stats, 0x07);
    
    // Buttons
    widget_button(x + 3, y + 10, "Clear", &STYLE_BTN_CLEAR);
    widget_button(x + 13, y + 10, "Save", &STYLE_BTN_SAVE);
    widget_button(x + 22, y + 10, "Close", &STYLE_DEFAULT);
}

void notepad_handle_click(int cursor_x, int cursor_y, int win_x, int win_y) {
    int rel_x = cursor_x - win_x;
    int rel_y = cursor_y - win_y;
    
    // Clear Button
    if (check_button_click(cursor_x, cursor_y, win_x + 3, win_y + 10, "Clear")) {
        notepad.text[0] = '\0';
        notepad.cursor = 0;
        update_stats();
    }
    // Save Button
    else if (check_button_click(cursor_x, cursor_y, win_x + 13, win_y + 10, "Save")) {
        // TODO: Save
    }
    // Close Button
    else if (check_button_click(cursor_x, cursor_y, win_x + 22, win_y + 10, "Close")) {
        // TODO: Close
    }
    // Text Area Click
    else if (rel_x >= 2 && rel_x <= 48 && rel_y >= 2 && rel_y <= 8) {
        notepad.active = 1;
        if (strcmp(notepad.text, "Click in text area to type...") == 0) {
            notepad.text[0] = '\0';
            notepad.cursor = 0;
            update_stats();
        }
    }
}