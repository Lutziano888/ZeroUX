#include "notepad.h"
#include "vbe.h"
#include "../gui.h"
#include "../widgets.h"
#include "../string.h"
#include "../gui_colors.h"

// =====================
// State
// =====================
typedef struct {
    char text[200];
    int cursor;
    int active;
    int char_count;
    int line_count;
    WindowStyle* current_theme;
} NotepadState;

static NotepadState notepad;

// =====================
// Styles (Calculator-like)
// =====================
static WidgetStyle STYLE_EDITOR;
static WidgetStyle STYLE_NUM;
static WidgetStyle STYLE_ACT;
static WidgetStyle STYLE_OPS;

static void setup_styles() {
    STYLE_EDITOR.bg_color     = BLACK;
    STYLE_EDITOR.fg_color     = WHITE;
    STYLE_EDITOR.border_color = DARK_GRAY;

    STYLE_NUM = (WidgetStyle){ LIGHT_GRAY, BLACK, 0, 0, 0 };
    STYLE_OPS = (WidgetStyle){ LIGHT_BLUE, WHITE, 0, 0, 0 };
    STYLE_ACT = (WidgetStyle){ LIGHT_RED,  WHITE, 0, 0, 0 };
}

static void update_stats() {
    notepad.char_count = 0;
    notepad.line_count = 1;

    for (int i = 0; i < 200 && notepad.text[i]; i++) {
        notepad.char_count++;
        if (notepad.text[i] == '\n')
            notepad.line_count++;
    }
}

// =====================
// Init
// =====================
void notepad_init() {
    notepad.text[0] = '\0';
    notepad.cursor = 0;
    notepad.active = 0;
    update_stats();
    setup_styles();
}

int notepad_is_active() {
    return notepad.active;
}

void notepad_deactivate() {
    notepad.active = 0;
}

// =====================
// Input
// =====================
void notepad_handle_backspace() {
    if (notepad.cursor > 0) {
        notepad.cursor--;
        for (int i = notepad.cursor; i < 199; i++)
            notepad.text[i] = notepad.text[i + 1];
        update_stats();
    }
}

void notepad_handle_char(char c) {
    if (!notepad.active) return;

    if (notepad.cursor < 199 && (c == '\n' || (c >= 32 && c <= 126))) {
        for (int i = 199; i > notepad.cursor; i--)
            notepad.text[i] = notepad.text[i - 1];

        notepad.text[notepad.cursor++] = c;
        notepad.text[notepad.cursor] = '\0';
        update_stats();
    }
}

// =====================
// Draw (Legacy - unused with VBE)
// =====================
void notepad_draw(int x, int y, int w, int h, int is_selected) {
    // VGA text mode drawing - not used in VBE graphics mode
}

// =====================
// Click Handling (aligned)
// =====================
void notepad_handle_click(int cursor_x, int cursor_y, int win_x, int win_y) {
    int rx = cursor_x - win_x;
    int ry = cursor_y - win_y;

    // Text area
    if (rx >= 2 && rx <= 46 && ry >= 3 && ry <= 8) {
        notepad.active = 1;
        return;
    }

    // Buttons
    if (ry >= 11 && ry <= 12) {
        if (rx >= 3 && rx <= 9) {
            notepad.text[0] = '\0';
            notepad.cursor = 0;
            update_stats();
        } else if (rx >= 21 && rx <= 27) {
            notepad.active = 0;
        }
    }
}

// =====================
// VBE Drawing
// =====================
void notepad_draw_vbe(int x, int y, int w, int h, int is_selected) {
    // Window background
    vbe_fill_rect(x, y, w, h, WIN10_PANEL);
    vbe_draw_rect(x, y, w, h, WIN10_BORDER);
    
    // Editor area
    vbe_fill_rect(x + 10, y + 10, w - 20, h - 60, VBE_BLACK);
    vbe_draw_rect(x + 10, y + 10, w - 20, h - 60, VBE_GRAY);
    
    // Text display
    int text_x = x + 15;
    int text_y = y + 15;
    int line_height = 18;
    
    int line = 0;
    int col = 0;
    for (int i = 0; i < 200 && notepad.text[i]; i++) {
        if (notepad.text[i] == '\n') {
            line++;
            col = 0;
        } else if (notepad.text[i] >= 32 && notepad.text[i] <= 126) {
            int px = text_x + col * 8;
            int py = text_y + line * line_height;
            if (py < y + h - 60) {
                vbe_draw_char(px, py, notepad.text[i], VBE_WHITE, VBE_TRANSPARENT);
            }
            col++;
        }
    }
    
    // Status line
    int status_y = y + h - 45;
    vbe_draw_text(x + 10, status_y, "Chars: ", VBE_WHITE, VBE_TRANSPARENT);
    
    // Buttons
    int btn_y = y + h - 35;
    int btn_w = (w - 30) / 3;
    
    vbe_fill_rect(x + 10, btn_y, btn_w, 25, 0x00E81123);
    vbe_draw_text(x + 10 + (btn_w - 40) / 2, btn_y + 5, "Clear", VBE_WHITE, VBE_TRANSPARENT);
    
    vbe_fill_rect(x + 10 + btn_w + 5, btn_y, btn_w, 25, WIN10_ACCENT);
    vbe_draw_text(x + 15 + btn_w + (btn_w - 32) / 2, btn_y + 5, "Save", VBE_WHITE, VBE_TRANSPARENT);
    
    vbe_fill_rect(x + 10 + 2 * btn_w + 10, btn_y, btn_w, 25, WIN10_BORDER);
    vbe_draw_text(x + 15 + 2 * btn_w + (btn_w - 40) / 2, btn_y + 5, "Close", VBE_WHITE, VBE_TRANSPARENT);
}

void notepad_handle_click_vbe(int rel_x, int rel_y) {
    if (rel_y < 10) return;
    
    // Edit area - activate notepad when clicked
    if (rel_y >= 10 && rel_y < 280) {
        notepad.active = 1;
    }
}
