#include "welcome.h"
#include "../gui.h"
#include "../widgets.h"
#include "gui_colors.h"

// =====================
// State
// =====================
typedef struct {
    WindowStyle* current_theme;
} WelcomeState;

static WelcomeState welcome;

// Styles
static WidgetStyle STYLE_ACCENT;
static WidgetStyle STYLE_CLOSE;

static void setup_styles() {
    STYLE_ACCENT = (WidgetStyle){ LIGHT_BLUE, WHITE, 0, 0, 0 };
    STYLE_CLOSE  = (WidgetStyle){ LIGHT_RED,  WHITE, 0, 0, 0 };
}

// =====================
// Init
// =====================
void welcome_init() {
    welcome.current_theme = &WINDOW_DARK;
    setup_styles();
}

// =====================
// Draw
// =====================
void welcome_draw(int x, int y, int w, int h, int is_selected) {
    draw_window(x, y, w, h, " Welcome ", welcome.current_theme, is_selected);

    // Close
    widget_button_win10(x + w - 7, y + 1, 5, "X", &STYLE_CLOSE);

    // Header
    draw_text_at(x + 4, y + 3, "Welcome to zeroUX", 0x0F);
    draw_text_at(x + 4, y + 4, "Modern Desktop Environment", 0x08);

    // Apps
    widget_button_win10(x + 4, y + 6, 12, "Calculator", &STYLE_ACCENT);
    draw_text_at(x + 18, y + 6, "Simple math", 0x08);

    widget_button_win10(x + 4, y + 8, 12, "Notepad", &STYLE_ACCENT);
    draw_text_at(x + 18, y + 8, "Edit text files", 0x08);

    // Footer
    draw_text_at(x + 4, y + h - 2, "TAB = switch windows | ENTER = click", 0x0E);
}

// =====================
// Click
// =====================
void welcome_handle_click(int cursor_x, int cursor_y, int win_x, int win_y) {
    int rx = cursor_x - win_x;
    int ry = cursor_y - win_y;

    // Close
    if (ry >= 1 && ry <= 2 && rx >= 55 && rx <= 59) {
        // TODO: close window
    }
}
