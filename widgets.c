#include "widgets.h"
#include "gui.h"

static unsigned short* VGA = (unsigned short*)0xB8000;

// Window Themes
WindowStyle WINDOW_BLUE = {0x1F, 0x0F, 0x1F, 0xF0, 0x00, 0x08};
WindowStyle WINDOW_DARK = {0x07, 0x0F, 0x08, 0x70, 0x00, 0x00};
WindowStyle WINDOW_LIGHT = {0x70, 0x00, 0x78, 0xF0, 0x00, 0x08};
WindowStyle WINDOW_GREEN = {0x2F, 0x0F, 0x2F, 0xF0, 0x00, 0x08};
WindowStyle WINDOW_PURPLE = {0x5F, 0x0F, 0x5F, 0xF0, 0x00, 0x08};
WindowStyle WINDOW_RED = {0x4F, 0x0F, 0x4F, 0xF0, 0x00, 0x08};

WidgetStyle STYLE_DEFAULT = {0x70, 0x00, 0x70, 0x08, 0x0F};

static void set_char(int x, int y, unsigned char ch, unsigned char color) {
    if (x >= 0 && x < 80 && y >= 0 && y < 25) {
        VGA[y * 80 + x] = (color << 8) | ch;
    }
}

static int str_len(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

// Cleaner Windows 10 Style Button
void widget_button_win10(int x, int y, int w, const char* text, WidgetStyle* style) {
    int len = str_len(text);
    
    // Hintergrund zeichnen (Solid Fill)
    for (int i = 0; i < w; i++) {
        set_char(x + i, y, ' ', (style->bg_color << 4) | style->bg_color);
    }
    
    // Text zentrieren
    int start_x = x + (w - len) / 2;
    for (int i = 0; i < len; i++) {
        if (start_x + i < x + w) {
            set_char(start_x + i, y, text[i], (style->bg_color << 4) | style->fg_color);
        }
    }
}

// Modern 3D Button (Windows 95/98 Style)
void widget_button_3d(int x, int y, const char* text, unsigned char color) {
    int len = str_len(text);
    int width = len + 4;
    
    // Top highlight
    set_char(x, y, 0xDA, 0x0F);
    for (int i = 1; i < width - 1; i++) {
        set_char(x + i, y, 0xC4, 0x0F);
    }
    set_char(x + width - 1, y, 0xBF, 0x08);
    
    // Middle with text
    set_char(x, y + 1, 0xB3, 0x0F);
    set_char(x + 1, y + 1, ' ', color);
    for (int i = 0; i < len; i++) {
        set_char(x + 2 + i, y + 1, text[i], color);
    }
    set_char(x + width - 2, y + 1, ' ', color);
    set_char(x + width - 1, y + 1, 0xB3, 0x08);
    
    // Bottom shadow
    set_char(x, y + 2, 0xC0, 0x08);
    for (int i = 1; i < width - 1; i++) {
        set_char(x + i, y + 2, 0xC4, 0x08);
    }
    set_char(x + width - 1, y + 2, 0xD9, 0x08);
}

// Flat Modern Button (Windows 10/11 Style)
void widget_button(int x, int y, const char* text, WidgetStyle* style) {
    int len = str_len(text);
    int width = len + 4;
    
    // Background
    for (int i = 0; i < width; i++) {
        set_char(x + i, y, ' ', style->bg_color);
    }
    
    // Text centered
    set_char(x + 1, y, ' ', style->bg_color);
    for (int i = 0; i < len; i++) {
        set_char(x + 2 + i, y, text[i], style->bg_color);
    }
    set_char(x + width - 1, y, ' ', style->bg_color);
    
    // Subtle border effect
    set_char(x, y, 0xB3, style->border_color);
    set_char(x + width - 1, y, 0xB3, style->border_color);
}

// Modern Window with Shadow
void draw_window(int x, int y, int w, int h, const char* title, WindowStyle* style, int is_active) {
    unsigned char title_color = is_active ? style->title_bg : style->border_color;
    
    // Title Bar
    set_char(x, y, 0xDA, title_color);
    for (int i = 1; i < w - 1; i++) {
        set_char(x + i, y, 0xC4, title_color);
    }
    set_char(x + w - 1, y, 0xBF, title_color);
    
    // Title Text
    int title_len = str_len(title);
    int title_x = x + (w - title_len) / 2;
    for (int i = 0; i < title_len && i < w - 2; i++) {
        set_char(title_x + i, y, title[i], title_color);
    }
    
    // Window Border
    for (int j = 1; j < h - 1; j++) {
        set_char(x, y + j, 0xB3, style->border_color);
        set_char(x + w - 1, y + j, 0xB3, style->border_color);
        
        // Content Background
        for (int i = 1; i < w - 1; i++) {
            set_char(x + i, y + j, ' ', style->content_bg);
        }
    }
    
    // Bottom Border
    set_char(x, y + h - 1, 0xC0, style->border_color);
    for (int i = 1; i < w - 1; i++) {
        set_char(x + i, y + h - 1, 0xC4, style->border_color);
    }
    set_char(x + w - 1, y + h - 1, 0xD9, style->border_color);
}

// Layout Helpers
void layout_vertical_begin(VerticalLayout* layout, int x, int y, int spacing) {
    layout->x = x;
    layout->y = y;
    layout->current_y = y;
    layout->spacing = spacing;
}

void layout_vertical_add(VerticalLayout* layout, int lines) {
    layout->current_y += lines + layout->spacing;
}