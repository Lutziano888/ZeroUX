#include "welcome.h"
#include "../gui.h"
#include "../widgets.h"

// State
typedef struct {
    int tutorial_step;
    int show_tips;
    WindowStyle* current_theme;  // Aktuelles Theme
} WelcomeState;

static WelcomeState welcome;

// Styles
static WidgetStyle STYLE_HEADER;
static WidgetStyle STYLE_CONTENT;
static WidgetStyle STYLE_TIP;
static WidgetStyle STYLE_CLOSE;

static void setup_styles() {
    STYLE_HEADER.bg_color = 0x1F;
    STYLE_HEADER.fg_color = 0x0F;
    
    STYLE_CONTENT.bg_color = 0x0F;
    STYLE_CONTENT.fg_color = 0x00;
    
    STYLE_TIP.bg_color = 0x0E;
    STYLE_TIP.fg_color = 0x00;
    
    STYLE_CLOSE.bg_color = 0x4F;
    STYLE_CLOSE.fg_color = 0x0F;
}

void welcome_init() {
    welcome.tutorial_step = 0;
    welcome.show_tips = 1;
    
    // Wähle ein Theme - hier kannst du ändern!
    // Optionen: &WINDOW_BLUE, &WINDOW_DARK, &WINDOW_LIGHT, &WINDOW_GREEN, &WINDOW_PURPLE, &WINDOW_RED
    welcome.current_theme = &WINDOW_PURPLE;
    
    setup_styles();
}

void welcome_draw(int x, int y, int w, int h, int is_selected) {
    // Zeichne Fenster mit Theme
    draw_window(x, y, w, h, " Welcome to zeroUX ", welcome.current_theme, is_selected);
    
    // Close Button
    widget_button(x + w - 5, y, "X", &STYLE_CLOSE);
    
    // Content mit Theme-Textfarbe
    VerticalLayout layout;
    layout_vertical_begin(&layout, x + 5, y + 2, 1);
    
    unsigned char text_color = welcome.current_theme->content_fg;
    
    draw_text_at(layout.x, layout.current_y, "Welcome to zeroUX Desktop!", text_color);
    layout_vertical_add(&layout, 2);
    
    draw_text_at(layout.x, layout.current_y, "Getting Started:", text_color);
    layout_vertical_add(&layout, 1);
    
    draw_text_at(layout.x, layout.current_y, "- Move cursor with arrow keys", text_color);
    layout_vertical_add(&layout, 1);
    
    draw_text_at(layout.x, layout.current_y, "- Press ENTER to click", text_color);
    layout_vertical_add(&layout, 1);
    
    draw_text_at(layout.x, layout.current_y, "- TAB: Switch windows", text_color);
    layout_vertical_add(&layout, 1);
    
    draw_text_at(layout.x, layout.current_y, "- ESC: Exit", text_color);
    layout_vertical_add(&layout, 2);
    
    draw_text_at(layout.x, layout.current_y, "Available Apps:", text_color);
    layout_vertical_add(&layout, 1);
    
    draw_text_at(layout.x, layout.current_y, "Calculator - Math operations", 0x0E);
    layout_vertical_add(&layout, 1);
    
    draw_text_at(layout.x, layout.current_y, "Notepad    - Text editing", 0x0E);
}

void welcome_handle_click(int cursor_x, int cursor_y, int win_x, int win_y) {
    int rel_x = cursor_x - win_x;
    int rel_y = cursor_y - win_y;
    
    // Close Button
    if (check_button_click(cursor_x, cursor_y, win_x + 55, win_y, "X")) {
        // TODO: Close window
    }
}