#include "notepad.h"
#include "../gui.h"
#include "../widgets.h"
#include "../string.h"
#include "gui_colors.h"

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

    notepad.current_theme = &WINDOW_DARK;
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
// Draw
// =====================
void notepad_draw(int x, int y, int w, int h, int is_selected) {
    draw_window(x, y, w, h, " Notepad ", notepad.current_theme, is_selected);

    unsigned short* VGA = (unsigned short*)0xB8000;

    // Status button
    if (notepad.active)
        widget_button_win10(x + 3, y + 1, 9, "EDITING", &STYLE_OPS);
    else
        widget_button_win10(x + 3, y + 1, 9, "READ", &STYLE_NUM);

    // Editor area
    int ex = x + 2;
    int ey = y + 3;
    int ew = w - 4;
    int eh = 6;

    for (int dy = 0; dy < eh; dy++)
        for (int dx = 0; dx < ew; dx++)
            VGA[(ey + dy) * 80 + ex + dx] = (0x00 << 8) | ' ';

    // Text
    int line = ey;
    int col  = ex + 1;
    int max_col = ex + ew - 1;

    for (int i = 0; i < 200 && notepad.text[i]; i++) {
        if (notepad.text[i] == '\n') {
            line++;
            col = ex + 1;
        } else {
            if (col < max_col && line < ey + eh) {
                VGA[line * 80 + col] = (0x0F << 8) | notepad.text[i];
            }
            col++;
            if (col >= max_col) {
                line++;
                col = ex + 1;
            }
        }
        if (line >= ey + eh) break;
    }

    // Status bar
    int sy = y + 10;
    for (int dx = 2; dx < w - 2; dx++)
        VGA[sy * 80 + x + dx] = (0x08 << 8) | ' ';

    char buf[40], n[10];
    strcpy(buf, "Chars: ");
    int_to_str(notepad.char_count, n);
    strcat(buf, n);
    strcat(buf, "  Lines: ");
    int_to_str(notepad.line_count, n);
    strcat(buf, n);

    draw_text_at(x + 3, sy, buf, 0x08);

    // Buttons
    int by = y + 11;
    widget_button_win10(x + 3,  by, 7, "Clear", &STYLE_ACT);
    widget_button_win10(x + 12, by, 7, "Save",  &STYLE_OPS);
    widget_button_win10(x + 21, by, 7, "Close", &STYLE_NUM);
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
