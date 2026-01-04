#ifndef WIDGETS_H
#define WIDGETS_H

// Modern Widget Styles (Windows-inspired)
typedef struct {
    unsigned char bg_color;
    unsigned char fg_color;
    unsigned char border_color;
    unsigned char shadow_color;
    unsigned char highlight_color;
} WidgetStyle;

// Window Themes
typedef struct {
    unsigned char title_bg;
    unsigned char title_fg;
    unsigned char border_color;
    unsigned char content_bg;
    unsigned char content_fg;
    unsigned char shadow_color;
} WindowStyle;

// Predefined Themes
extern WindowStyle WINDOW_BLUE;
extern WindowStyle WINDOW_DARK;
extern WindowStyle WINDOW_LIGHT;
extern WindowStyle WINDOW_GREEN;
extern WindowStyle WINDOW_PURPLE;
extern WindowStyle WINDOW_RED;

// Default Widget Style
extern WidgetStyle STYLE_DEFAULT;

// Layout Helper
typedef struct {
    int x;
    int y;
    int current_y;
    int spacing;
} VerticalLayout;

// Widget Functions
void widget_button(int x, int y, const char* text, WidgetStyle* style);
void widget_button_3d(int x, int y, const char* text, unsigned char color);
void draw_window(int x, int y, int w, int h, const char* title, WindowStyle* style, int is_active);

// Neue Win10 Style Funktionen
void widget_button_win10(int x, int y, int w, const char* text, WidgetStyle* style);
void draw_window(int x, int y, int w, int h, const char* title, WindowStyle* style, int is_active);

// Layout Functions
void layout_vertical_begin(VerticalLayout* layout, int x, int y, int spacing);
void layout_vertical_add(VerticalLayout* layout, int lines);

#endif