#include "widgets.h"
#include "gui.h"

// Eigene strlen-Funktion
static int str_len(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

// Vordefinierte Themes
WidgetStyle STYLE_DEFAULT = {0x70, 0x0F, 0x08, 0x07, 0x0F};
WidgetStyle STYLE_PRIMARY = {0x1F, 0x0F, 0x1F, 0x17, 0x1F};
WidgetStyle STYLE_SUCCESS = {0x2F, 0x0F, 0x2F, 0x27, 0x2F};
WidgetStyle STYLE_WARNING = {0x6F, 0x0F, 0x6F, 0x67, 0x6F};
WidgetStyle STYLE_DANGER  = {0x4F, 0x0F, 0x4F, 0x47, 0x4F};
WidgetStyle STYLE_DARK    = {0x08, 0x07, 0x08, 0x00, 0x08};
WidgetStyle STYLE_LIGHT   = {0xF0, 0x00, 0x07, 0x70, 0x0F};

// Window-Themes
WindowStyle WINDOW_BLUE = {
    0x11,  // window_bg (dunkelblau)
    0x1F,  // title_bar_bg (blau hell)
    0x0F,  // title_bar_fg (weiß)
    0x1F,  // border_active (blau)
    0x08,  // border_inactive (grau)
    0x17,  // content_bg (blau-grau)
    0x0F   // content_fg (weiß)
};

WindowStyle WINDOW_DARK = {
    0x00,  // window_bg (schwarz)
    0x70,  // title_bar_bg (grau)
    0x0F,  // title_bar_fg (weiß)
    0x0F,  // border_active (weiß)
    0x08,  // border_inactive (dunkelgrau)
    0x08,  // content_bg (dunkelgrau)
    0x0F   // content_fg (weiß)
};

WindowStyle WINDOW_LIGHT = {
    0xF0,  // window_bg (weiß)
    0x70,  // title_bar_bg (grau auf weiß)
    0x00,  // title_bar_fg (schwarz)
    0x0F,  // border_active (weiß auf schwarz)
    0x07,  // border_inactive (grau)
    0xF0,  // content_bg (weiß)
    0x00   // content_fg (schwarz)
};

WindowStyle WINDOW_GREEN = {
    0x22,  // window_bg (dunkelgrün)
    0x2F,  // title_bar_bg (grün hell)
    0x0F,  // title_bar_fg (weiß)
    0x2F,  // border_active (grün)
    0x08,  // border_inactive (grau)
    0x27,  // content_bg (grün-grau)
    0x0F   // content_fg (weiß)
};

WindowStyle WINDOW_PURPLE = {
    0x55,  // window_bg (dunkel lila)
    0x5F,  // title_bar_bg (lila hell)
    0x0F,  // title_bar_fg (weiß)
    0x5F,  // border_active (lila)
    0x08,  // border_inactive (grau)
    0x57,  // content_bg (lila-grau)
    0x0F   // content_fg (weiß)
};

WindowStyle WINDOW_RED = {
    0x44,  // window_bg (dunkelrot)
    0x4F,  // title_bar_bg (rot hell)
    0x0F,  // title_bar_fg (weiß)
    0x4F,  // border_active (rot)
    0x08,  // border_inactive (grau)
    0x47,  // content_bg (rot-grau)
    0x0F   // content_fg (weiß)
};

void widget_button(int x, int y, const char* text, WidgetStyle* style) {
    unsigned char color = style ? style->bg_color : 0x0F;
    draw_button(x, y, text, color);
}

void widget_label(int x, int y, const char* text, WidgetStyle* style) {
    unsigned char color = style ? style->fg_color : 0x0F;
    draw_text_at(x, y, text, color);
}

void widget_panel(int x, int y, int w, int h, const char* title, WidgetStyle* style) {
    unsigned char color = style ? style->border_color : 0x08;
    draw_box(x, y, w, h, color);
    
    if (title && title[0]) {
        draw_text_at(x + 2, y, " ", color);
        draw_text_at(x + 3, y, title, color);
        draw_text_at(x + 3 + str_len(title), y, " ", color);
    }
}

void widget_separator(int x, int y, int width, unsigned char color) {
    unsigned short* VGA = (unsigned short*)0xB8000;
    for (int i = 0; i < width; i++) {
        if (x + i >= 0 && x + i < 80 && y >= 0 && y < 25) {
            VGA[y * 80 + (x + i)] = (color << 8) | 0xC4;
        }
    }
}

void widget_checkbox(int x, int y, const char* label, int checked, WidgetStyle* style) {
    unsigned char color = style ? style->fg_color : 0x0F;
    
    draw_text_at(x, y, "[", color);
    draw_text_at(x + 1, y, checked ? "X" : " ", color);
    draw_text_at(x + 2, y, "] ", color);
    draw_text_at(x + 4, y, label, color);
}

void widget_textbox(TextInput* input) {
    if (!input) return;
    
    unsigned char color = input->style.bg_color ? input->style.bg_color : 0x70;
    
    draw_text_at(input->x, input->y, input->label, 0x0F);
    draw_box(input->x, input->y + 1, input->w, 3, color);
    draw_text_at(input->x + 2, input->y + 2, input->value, color);
}

void widget_listbox(ListBox* list) {
    if (!list) return;
    
    unsigned char color = list->style.bg_color ? list->style.bg_color : 0x70;
    unsigned char sel_color = list->style.active_color ? list->style.active_color : 0x1F;
    
    draw_box(list->x, list->y, list->w, list->h, color);
    
    for (int i = 0; i < list->item_count && i < list->h - 2; i++) {
        unsigned char item_color = (i == list->selected) ? sel_color : color;
        draw_text_at(list->x + 2, list->y + 1 + i, list->items[i], item_color);
    }
}

void layout_vertical_begin(VerticalLayout* layout, int x, int y, int spacing) {
    layout->x = x;
    layout->y = y;
    layout->spacing = spacing;
    layout->current_y = y;
}

void layout_vertical_add(VerticalLayout* layout, int height) {
    layout->current_y += height + layout->spacing;
}

void layout_horizontal_begin(HorizontalLayout* layout, int x, int y, int spacing) {
    layout->x = x;
    layout->y = y;
    layout->spacing = spacing;
    layout->current_x = x;
}

void layout_horizontal_add(HorizontalLayout* layout, int width) {
    layout->current_x += width + layout->spacing;
}

// ============================================================================
// ERWEITERTE WINDOW-FUNKTIONEN
// ============================================================================

// Rechteck mit Farbe füllen
void fill_rect(int x, int y, int w, int h, unsigned char color) {
    unsigned short* VGA = (unsigned short*)0xB8000;
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            int px = x + i;
            int py = y + j;
            if (px >= 0 && px < 80 && py >= 0 && py < 25) {
                VGA[py * 80 + px] = (color << 8) | ' ';
            }
        }
    }
}

// Komplettes Fenster mit Style zeichnen
void draw_window(int x, int y, int w, int h, const char* title, WindowStyle* style, int is_active) {
    if (!style) {
        // Fallback auf Standard
        draw_box(x, y, w, h, is_active ? 0x1F : 0x08);
        if (title) draw_text_at(x + 2, y, title, is_active ? 0x1F : 0x08);
        return;
    }
    
    unsigned char border = is_active ? style->border_active : style->border_inactive;
    unsigned char title_bg = style->title_bar_bg;
    unsigned char title_fg = style->title_bar_fg;
    
    // Rahmen zeichnen
    draw_box(x, y, w, h, border);
    
    // Titelleiste
    for (int i = 1; i < w - 1; i++) {
        unsigned short* VGA = (unsigned short*)0xB8000;
        if (x + i >= 0 && x + i < 80 && y >= 0 && y < 25) {
            VGA[y * 80 + (x + i)] = (title_bg << 8) | ' ';
        }
    }
    
    // Titel-Text
    if (title && title[0]) {
        draw_text_at(x + 2, y, " ", title_bg);
        int len = str_len(title);
        for (int i = 0; i < len; i++) {
            unsigned short* VGA = (unsigned short*)0xB8000;
            if (x + 3 + i >= 0 && x + 3 + i < 80 && y >= 0 && y < 25) {
                VGA[y * 80 + (x + 3 + i)] = (title_bg << 8) | title[i];
            }
        }
        draw_text_at(x + 3 + len, y, " ", title_bg);
    }
    
    // Fenster-Hintergrund (Innenfläche)
    fill_rect(x + 1, y + 1, w - 2, h - 2, style->window_bg);
}

// Content-Area mit speziellem Hintergrund
void draw_window_content_area(int x, int y, int w, int h, WindowStyle* style) {
    if (!style) {
        fill_rect(x, y, w, h, 0x70);
        return;
    }
    fill_rect(x, y, w, h, style->content_bg);
}