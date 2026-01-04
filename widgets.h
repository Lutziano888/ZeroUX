#ifndef WIDGETS_H
#define WIDGETS_H

// Erweiterte Style-Definitionen
typedef struct {
    unsigned char bg_color;          // Hintergrundfarbe
    unsigned char fg_color;          // Vordergrundfarbe (Text)
    unsigned char border_color;      // Rahmenfarbe
    unsigned char hover_color;       // Hover-Farbe
    unsigned char active_color;      // Aktiv-Farbe
} WidgetStyle;

// Window-Style für komplette Fenster
typedef struct {
    unsigned char window_bg;         // Fenster-Hintergrund
    unsigned char title_bar_bg;      // Titelleiste Hintergrund
    unsigned char title_bar_fg;      // Titelleiste Text
    unsigned char border_active;     // Aktiver Rahmen
    unsigned char border_inactive;   // Inaktiver Rahmen
    unsigned char content_bg;        // Content-Bereich Hintergrund
    unsigned char content_fg;        // Content-Bereich Text
} WindowStyle;

// Vordefinierte Themes
extern WidgetStyle STYLE_DEFAULT;
extern WidgetStyle STYLE_PRIMARY;
extern WidgetStyle STYLE_SUCCESS;
extern WidgetStyle STYLE_WARNING;
extern WidgetStyle STYLE_DANGER;
extern WidgetStyle STYLE_DARK;
extern WidgetStyle STYLE_LIGHT;

// Vordefinierte Window-Themes
extern WindowStyle WINDOW_BLUE;
extern WindowStyle WINDOW_DARK;
extern WindowStyle WINDOW_LIGHT;
extern WindowStyle WINDOW_GREEN;
extern WindowStyle WINDOW_PURPLE;
extern WindowStyle WINDOW_RED;

// Verbesserte Widget-Funktionen
typedef struct {
    int x, y, w, h;
    char text[32];
    WidgetStyle style;
    int visible;
    int enabled;
} Widget;

typedef struct {
    int x, y, w, h;
    char label[32];
    char value[64];
    WidgetStyle style;
    int max_length;
    int cursor_pos;
} TextInput;

typedef struct {
    int x, y, w, h;
    char items[10][32];
    int item_count;
    int selected;
    WidgetStyle style;
} ListBox;

// Widget-Funktionen mit erweiterten Styling-Optionen
void widget_button(int x, int y, const char* text, WidgetStyle* style);
void widget_label(int x, int y, const char* text, WidgetStyle* style);
void widget_textbox(TextInput* input);
void widget_checkbox(int x, int y, const char* label, int checked, WidgetStyle* style);
void widget_listbox(ListBox* list);
void widget_separator(int x, int y, int width, unsigned char color);
void widget_panel(int x, int y, int w, int h, const char* title, WidgetStyle* style);

// Neue erweiterte Window-Funktionen
void draw_window(int x, int y, int w, int h, const char* title, WindowStyle* style, int is_active);
void draw_window_content_area(int x, int y, int w, int h, WindowStyle* style);
void fill_rect(int x, int y, int w, int h, unsigned char color);

// Layout-Helpers
typedef struct {
    int x, y;
    int spacing;
    int current_y;
} VerticalLayout;

typedef struct {
    int x, y;
    int spacing;
    int current_x;
} HorizontalLayout;

void layout_vertical_begin(VerticalLayout* layout, int x, int y, int spacing);
void layout_vertical_add(VerticalLayout* layout, int height);
void layout_horizontal_begin(HorizontalLayout* layout, int x, int y, int spacing);
void layout_horizontal_add(HorizontalLayout* layout, int width);

#endif