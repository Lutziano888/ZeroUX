#include "welcome.h"
#include "vbe.h"
#include "../gui.h"
#include "../widgets.h"
#include "../gui_colors.h"

// =====================
// State
// =====================
typedef struct {
    WindowStyle* current_theme;
} WelcomeState;

static WelcomeState welcome;

// Styles
static WidgetStyle STYLE_ACCENT;

static void setup_styles() {
    STYLE_ACCENT = (WidgetStyle){ LIGHT_BLUE, WHITE, 0, 0, 0 };
}

// =====================
// Init
// =====================
void welcome_init() {
    setup_styles();
}

// =====================
// Draw (Legacy - unused with VBE)
// =====================
void welcome_draw(int x, int y, int w, int h, int is_selected) {
    // VGA text mode drawing - not used in VBE graphics mode
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

// =====================
// VBE Drawing
// =====================
void welcome_draw_vbe(int x, int y, int w, int h, int is_selected) {
    // Window background
    vbe_fill_rect(x, y, w, h, WIN10_PANEL);
    vbe_draw_rect(x, y, w, h, WIN10_BORDER);
    
    // Title
    vbe_draw_text(x + 20, y + 30, "Welcome to zeroUX", VBE_WHITE, VBE_TRANSPARENT);
    vbe_draw_text(x + 20, y + 55, "Modern Desktop Environment", WIN10_ACCENT, VBE_TRANSPARENT);
    
    // App descriptions
    vbe_draw_text(x + 20, y + 100, "Calculator", VBE_YELLOW, VBE_TRANSPARENT);
    vbe_draw_text(x + 150, y + 100, "Simple math operations", VBE_LIGHT_GRAY, VBE_TRANSPARENT);
    
    // Footer
    vbe_draw_text(x + 20, y + h - 30, "TAB = switch | Click window to focus", VBE_GRAY, VBE_TRANSPARENT);
}

void welcome_handle_click_vbe(int rel_x, int rel_y) {
    // Not needed for this app
}
