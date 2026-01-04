#include "app_template.h"
#include "../gui.h"
#include "../widgets.h"

// ============================================================================
// APP STATE - Alle Daten deiner App an einem Ort
// ============================================================================
typedef struct {
    // Deine App-Daten hier
    char title[32];
    int counter;
    int active_tab;
    
    // UI-Elemente können hier gespeichert werden
    TextInput input1;
    ListBox list1;
    
    // Flags
    int needs_redraw;
} AppState;

static AppState app_state;

// ============================================================================
// STYLES - Definiere das Aussehen deiner App
// ============================================================================
static void setup_styles() {
    // Beispiel: Custom Colors für deine App
    STYLE_PRIMARY.bg_color = 0x1F;      // Blau
    STYLE_PRIMARY.fg_color = 0x0F;      // Weiß
    STYLE_PRIMARY.border_color = 0x1F;
    
    STYLE_SUCCESS.bg_color = 0x2F;      // Grün
    STYLE_WARNING.bg_color = 0x6F;      // Gelb
    STYLE_DANGER.bg_color = 0x4F;       // Rot
}

// ============================================================================
// INIT - Initialisiere deine App
// ============================================================================
void app_init() {
    strcpy(app_state.title, "My Cool App");
    app_state.counter = 0;
    app_state.active_tab = 0;
    app_state.needs_redraw = 1;
    
    // TextInput initialisieren
    app_state.input1.x = 0;
    app_state.input1.y = 0;
    app_state.input1.w = 30;
    app_state.input1.h = 1;
    strcpy(app_state.input1.label, "Name:");
    strcpy(app_state.input1.value, "");
    app_state.input1.max_length = 50;
    app_state.input1.cursor_pos = 0;
    
    setup_styles();
}

// ============================================================================
// LAYOUT - Definiere das Layout deiner App
// ============================================================================
static void draw_header(int x, int y, int w) {
    widget_panel(x, y, w, 3, app_state.title, &STYLE_PRIMARY);
    
    char counter_text[32];
    int_to_str(app_state.counter, counter_text);
    widget_label(x + w - 15, y + 1, "Count:", &STYLE_PRIMARY);
    widget_label(x + w - 8, y + 1, counter_text, &STYLE_PRIMARY);
}

static void draw_tabs(int x, int y, int w) {
    // Horizontal Tab-Layout
    HorizontalLayout tab_layout;
    layout_horizontal_begin(&tab_layout, x + 2, y, 2);
    
    const char* tabs[] = {"Home", "Settings", "About"};
    for (int i = 0; i < 3; i++) {
        WidgetStyle style = (i == app_state.active_tab) ? STYLE_PRIMARY : STYLE_DEFAULT;
        widget_button(tab_layout.current_x, y, tabs[i], &style);
        layout_horizontal_add(&tab_layout, 12);
    }
}

static void draw_content(int x, int y, int w, int h) {
    // Vertical Layout für Content
    VerticalLayout layout;
    layout_vertical_begin(&layout, x + 3, y + 6, 2);
    
    if (app_state.active_tab == 0) {
        // Home Tab
        widget_label(layout.x, layout.current_y, "Welcome to the app!", &STYLE_DEFAULT);
        layout_vertical_add(&layout, 1);
        
        widget_separator(layout.x, layout.current_y, w - 6, 0x08);
        layout_vertical_add(&layout, 1);
        
        // Buttons in horizontaler Reihe
        HorizontalLayout btn_layout;
        layout_horizontal_begin(&btn_layout, layout.x, layout.current_y, 2);
        
        widget_button(btn_layout.current_x, layout.current_y, "Increment", &STYLE_SUCCESS);
        layout_horizontal_add(&btn_layout, 14);
        
        widget_button(btn_layout.current_x, layout.current_y, "Decrement", &STYLE_WARNING);
        layout_horizontal_add(&btn_layout, 14);
        
        widget_button(btn_layout.current_x, layout.current_y, "Reset", &STYLE_DANGER);
        
        layout_vertical_add(&layout, 2);
        
    } else if (app_state.active_tab == 1) {
        // Settings Tab
        widget_label(layout.x, layout.current_y, "Settings", &STYLE_DEFAULT);
        layout_vertical_add(&layout, 2);
        
        // TextInput
        app_state.input1.x = layout.x;
        app_state.input1.y = layout.current_y;
        widget_textbox(&app_state.input1);
        layout_vertical_add(&layout, 3);
        
        widget_checkbox(layout.x, layout.current_y, "Enable notifications", 1, &STYLE_DEFAULT);
        layout_vertical_add(&layout, 1);
        
        widget_checkbox(layout.x, layout.current_y, "Dark mode", 0, &STYLE_DEFAULT);
        
    } else {
        // About Tab
        widget_label(layout.x, layout.current_y, "About this App", &STYLE_DEFAULT);
        layout_vertical_add(&layout, 2);
        widget_label(layout.x, layout.current_y, "Version 1.0", &STYLE_DEFAULT);
        layout_vertical_add(&layout, 1);
        widget_label(layout.x, layout.current_y, "Created with zeroUX", &STYLE_DEFAULT);
    }
}

// ============================================================================
// DRAW - Hauptzeichenfunktion
// ============================================================================
void app_draw(int x, int y, int w, int h, int is_selected) {
    unsigned char border_color = is_selected ? 0x1F : 0x08;
    
    // Hauptfenster
    draw_box(x, y, w, h, border_color);
    
    // Komponenten mit klarer Struktur
    draw_header(x, y, w);
    draw_tabs(x, y + 3, w);
    draw_content(x, y + 4, w, h - 4);
}

// ============================================================================
// CLICK HANDLER - Verarbeite Klicks mit klarer Logik
// ============================================================================
void app_handle_click(int cursor_x, int cursor_y, int win_x, int win_y) {
    // Relative Koordinaten berechnen
    int rel_x = cursor_x - win_x;
    int rel_y = cursor_y - win_y;
    
    // Tab-Klicks (y=3, x=2 bis x=38)
    if (rel_y == 3) {
        if (rel_x >= 2 && rel_x < 14) app_state.active_tab = 0;      // Home
        else if (rel_x >= 14 && rel_x < 26) app_state.active_tab = 1; // Settings
        else if (rel_x >= 26 && rel_x < 38) app_state.active_tab = 2; // About
        return;
    }
    
    // Content-Klicks je nach aktivem Tab
    if (app_state.active_tab == 0 && rel_y == 8) {
        // Button Row
        if (rel_x >= 3 && rel_x < 17) {
            // Increment Button
            app_state.counter++;
        } else if (rel_x >= 17 && rel_x < 31) {
            // Decrement Button
            app_state.counter--;
        } else if (rel_x >= 31 && rel_x < 45) {
            // Reset Button
            app_state.counter = 0;
        }
    }
}

// ============================================================================
// KEYBOARD HANDLER (optional für Textfelder etc.)
// ============================================================================
void app_handle_key(unsigned char scancode) {
    if (app_state.active_tab == 1) {
        // TextInput handling
        if (scancode == 0x0E && app_state.input1.cursor_pos > 0) {
            // Backspace
            app_state.input1.cursor_pos--;
            app_state.input1.value[app_state.input1.cursor_pos] = '\0';
        } else {
            char c = scancode_to_ascii(scancode);
            if (c && app_state.input1.cursor_pos < app_state.input1.max_length) {
                app_state.input1.value[app_state.input1.cursor_pos++] = c;
                app_state.input1.value[app_state.input1.cursor_pos] = '\0';
            }
        }
    }
}