#include "gui.h"
#include "port.h"
#include "vbe.h"
#include "keyboard.h"
#include "mouse.h"
#include "string.h"
#include "rtc.h"
#include "ethernet.h"
#include "apps/calculator.h"
#include "apps/notepad.h"
#include "apps/welcome.h"
#include "apps/filemanager.h"
#include "apps/google_browser.h"
#include "apps/memtest_app.h"
#include "apps/terminal.h"
#include "apps/python_ide.h"
#include "fs.h"
#include "net.h"
#include "apps/texteditor.h"

static int selected_window = 0;
static int caret_state = 1;
static int shift_pressed = 0;

int gui_get_caret_state() {
    return caret_state;
}

static int z_order[MAX_WINDOWS] = {0, 1, 2, 3, 4, 5, 6, 7, 8};

// Mouse cursor
static int cursor_x = 512;
static int cursor_y = 384;
static int mouse_visible = 1;

// Start menu state
static int start_menu_open = 0;
static int start_menu_selected = 0;
static char search_query[32] = "";
static int search_query_len = 0;
static int filtered_apps[MAX_WINDOWS];
static int filtered_count = 0;

// Window positions
typedef struct {
    int x, y, w, h;
    int visible;
} Window;

static Window windows[MAX_WINDOWS] = {
    {100, 100, 600, 400, 0},  // Welcome - GESCHLOSSEN
    {150, 150, 500, 450, 0},  // Notepad - GESCHLOSSEN
    {250, 200, 400, 400, 0},  // Calculator - GESCHLOSSEN
    {50, 50, 800, 600, 0},    // File Manager - GESCHLOSSEN
    {200, 80, 700, 450, 0},   // Google Browser - GESCHLOSSEN
    {300, 150, 400, 300, 0},  // Memory Doctor - GESCHLOSSEN
    {100, 100, 640, 400, 0},  // Terminal - GESCHLOSSEN
    {150, 100, 700, 500, 0},  // Python IDE - GESCHLOSSEN
    {200, 150, 500, 400, 0}   // Installer - GESCHLOSSEN
};

// App names for start menu
static const char* app_names[MAX_WINDOWS] = {
    "Welcome",
    "Notepad",
    "Calculator",
    "Files",
    "Google Browser",
    "Memory Doctor",
    "Terminal",
    "Python IDE",
    "Text Editor"
};

// Helper for soft shadow blending
static unsigned int darken_color(unsigned int color, int factor) {
    // factor: 0 (black) to 255 (original)
    unsigned int r = (color >> 16) & 0xFF;
    unsigned int g = (color >> 8) & 0xFF;
    unsigned int b = color & 0xFF;
    
    r = (r * factor) >> 8;
    g = (g * factor) >> 8;
    b = (b * factor) >> 8;
    
    return (r << 16) | (g << 8) | b;
}

// Helper for alpha blending (mixing two colors)
static unsigned int blend_colors(unsigned int bg, unsigned int fg, int alpha) {
    // alpha: 0 (bg) to 255 (fg)
    if (alpha >= 255) return fg;
    if (alpha <= 0) return bg;

    unsigned int r_bg = (bg >> 16) & 0xFF;
    unsigned int g_bg = (bg >> 8) & 0xFF;
    unsigned int b_bg = bg & 0xFF;

    unsigned int r_fg = (fg >> 16) & 0xFF;
    unsigned int g_fg = (fg >> 8) & 0xFF;
    unsigned int b_fg = fg & 0xFF;

    unsigned int r = (r_fg * alpha + r_bg * (255 - alpha)) / 255;
    unsigned int g = (g_fg * alpha + g_bg * (255 - alpha)) / 255;
    unsigned int b = (b_fg * alpha + b_bg * (255 - alpha)) / 255;

    return (r << 16) | (g << 8) | b;
}

// Integer Square Root for distance calculation
static int isqrt(int n) {
    if (n < 0) return 0;
    int x = n;
    int y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

// ERSETZE die KOMPLETTE draw_shadow() Funktion durch:
// ERSETZE die komplette draw_shadow() Funktion (Zeile ~115):
void draw_shadow(int x, int y, int w, int h) {
    int shadow_size = 18;
    int shadow_offset = 3;  // Offset nach unten-rechts
    
    for (int i = 0; i < shadow_size; i++) {
        // Sanfterer Falloff (weniger stark)
        float t = (float)i / shadow_size;
        int alpha = (int)(80 * (1.0f - t * t));  // Max 80 statt 160 (schwächer)
        
        if (alpha <= 5) continue;
        
        // === RECHTE SEITE (oben schwächer) ===
        for (int py = y + shadow_offset + 15; py < y + h; py++) {
            int px = x + w + i;
            if (px >= vbe_get_width() || py >= vbe_get_height()) continue;
            
            // Fade-out nach oben
            float vertical_fade = (float)(py - (y + shadow_offset + 15)) / (h - 15);
            if (vertical_fade < 0) vertical_fade = 0;
            int adjusted_alpha = (int)(alpha * vertical_fade);
            
            if (adjusted_alpha > 5) {
                unsigned int bg = vbe_get_pixel(px, py);
                vbe_set_pixel(px, py, darken_color(bg, 255 - adjusted_alpha));
            }
        }
        
        // === UNTERE SEITE (links schwächer) ===
        for (int px = x + shadow_offset + 15; px < x + w; px++) {
            int py = y + h + i;
            if (px >= vbe_get_width() || py >= vbe_get_height()) continue;
            
            // Fade-out nach links
            float horizontal_fade = (float)(px - (x + shadow_offset + 15)) / (w - 15);
            if (horizontal_fade < 0) horizontal_fade = 0;
            int adjusted_alpha = (int)(alpha * horizontal_fade);
            
            if (adjusted_alpha > 5) {
                unsigned int bg = vbe_get_pixel(px, py);
                vbe_set_pixel(px, py, darken_color(bg, 255 - adjusted_alpha));
            }
        }
    }
    
    // === ECKE UNTEN-RECHTS (Radial, keine Überschneidung) ===
    for (int dy = 0; dy < shadow_size; dy++) {
        for (int dx = 0; dx < shadow_size; dx++) {
            // Nur Ecke zeichnen (außerhalb der geraden Seiten)
            if (dx < 3 || dy < 3) continue;  // Verhindert Überlappung
            
            int dist_x = dx + shadow_offset;
            int dist_y = dy + shadow_offset;
            int dist_sq = dist_x * dist_x + dist_y * dist_y;
            
            float dist = 0;
            int temp = 1;
            while (temp * temp <= dist_sq) {
                dist = temp;
                temp++;
            }
            
            if (dist < shadow_size) {
                float t = dist / shadow_size;
                int corner_alpha = (int)(80 * (1.0f - t * t));
                
                if (corner_alpha > 5) {
                    int px = x + w + dx;
                    int py = y + h + dy;
                    
                    if (px < vbe_get_width() && py < vbe_get_height()) {
                        unsigned int bg = vbe_get_pixel(px, py);
                        vbe_set_pixel(px, py, darken_color(bg, 255 - corner_alpha));
                    }
                }
            }
        }
    }
}

// Optional: Ambient Shadow (sehr subtil)
void draw_ambient_shadow(int x, int y, int w, int h) {
    int ao_size = 3;
    for (int i = 0; i < ao_size; i++) {
        int alpha = 40 - (i * 13);  // Sehr schwach
        for (int px = x + 10; px < x + w - 10; px++) {
            if (px < vbe_get_width() && y + h + i < vbe_get_height()) {
                unsigned int bg = vbe_get_pixel(px, y + h + i);
                vbe_set_pixel(px, y + h + i, darken_color(bg, 255 - alpha));
            }
        }
    }
}

#define CORNER_RADIUS 10

// Draws a single anti-aliased corner
// quadrant: 0=TopLeft, 1=TopRight, 2=BottomLeft, 3=BottomRight
static void draw_aa_corner(int cx, int cy, int r, unsigned int color, int quadrant) {
    int r_sq = r * r;
    // Scale up for precision in AA calculation
    int r_limit_sq = (r - 1) * (r - 1); 
    
    for (int dy = 0; dy < r; dy++) {
        for (int dx = 0; dx < r; dx++) {
            // Determine pixel position based on quadrant
            int px = (quadrant == 1 || quadrant == 3) ? (cx + dx) : (cx - 1 - dx);
            int py = (quadrant == 2 || quadrant == 3) ? (cy + dy) : (cy - 1 - dy);
            
            // Distance from center (cx, cy)
            // dx, dy are offsets from the inner axes outwards
            int d2 = (dx + 1) * (dx + 1) + (dy + 1) * (dy + 1);
            
            // Calculate AA
            int alpha = 255;
            
            // Inner part (solid)
            if (d2 < r_limit_sq) {
                alpha = 255;
            } 
            // Outer part (transparent)
            else if (d2 >= r_sq) {
                alpha = 0;
            }
            // Edge (blended)
            else {
                // Calculate sub-pixel distance for smooth edge
                int dist = isqrt(d2 * 256 * 256); // Fixed point 16.8
                int edge_outer = r * 256;
                
                int diff = edge_outer - dist;
                if (diff < 0) diff = 0;
                if (diff > 255) diff = 255;
                
                alpha = diff;
            }
            
            if (alpha > 0) {
                if (alpha == 255) {
                    vbe_set_pixel(px, py, color);
                } else {
                    unsigned int bg = vbe_get_pixel(px, py);
                    vbe_set_pixel(px, py, blend_colors(bg, color, alpha));
                }
            }
        }
    }
}

// Helper to draw AA corners for a rect
static void draw_aa_corners_rect(int x, int y, int w, int h, int r, unsigned int color, int top, int bottom) {
    if (top) {
        draw_aa_corner(x + r, y + r, r, color, 0);     // TL
        draw_aa_corner(x + w - r, y + r, r, color, 1); // TR
    }
    if (bottom) {
        draw_aa_corner(x + r, y + h - r, r, color, 2);     // BL
        draw_aa_corner(x + w - r, y + h - r, r, color, 3); // BR
    }
}

void draw_window_modern(int x, int y, int w, int h, const char* title, int is_active) {
    // Shadow
    draw_shadow(x, y, w, h);
    draw_ambient_shadow(x, y, w, h);
    
    int r = CORNER_RADIUS; // Corner radius
    unsigned int title_color = is_active ? 0x00404040 : 0x00353535;
    unsigned int body_color = 0x00282828;
    unsigned int border_color = 0x00555555; // Hellerer Rand (Outline)
    
    // --- 1. Rahmen zeichnen (Hintergrund-Form) ---
    // Oberer Teil (Titelleiste)
    vbe_fill_rect(x + r, y, w - 2 * r, 30, border_color);
    vbe_fill_rect(x, y + r, r, 30 - r, border_color);
    vbe_fill_rect(x + w - r, y + r, r, 30 - r, border_color);
    draw_aa_corners_rect(x, y, w, 30, r, border_color, 1, 0);
    
    // Unterer Teil (Body)
    vbe_fill_rect(x, y + 30, w, h - 30 - r, border_color);
    vbe_fill_rect(x + r, y + h - r, w - 2 * r, r, border_color);
    draw_aa_corner(x + r, y + h - r, r, border_color, 2);
    draw_aa_corner(x + w - r, y + h - r, r, border_color, 3);
    
    // --- 2. Inhalt zeichnen (1px kleiner) ---
    int inner_r = r - 1;
    
    // Titelleiste Innen
    vbe_fill_rect(x + 1 + inner_r, y + 1, w - 2 - 2 * inner_r, 29, title_color);
    vbe_fill_rect(x + 1, y + 1 + inner_r, inner_r, 29 - inner_r, title_color);
    vbe_fill_rect(x + w - 1 - inner_r, y + 1 + inner_r, inner_r, 29 - inner_r, title_color);
    draw_aa_corners_rect(x + 1, y + 1, w - 2, 30, inner_r, title_color, 1, 0);
    
    // Body Innen
    vbe_fill_rect(x + 1, y + 30, w - 2, h - 30 - 1 - inner_r, body_color);
    vbe_fill_rect(x + 1 + inner_r, y + h - 1 - inner_r, w - 2 - 2 * inner_r, inner_r, body_color);
    draw_aa_corner(x + 1 + inner_r, y + h - 1 - inner_r, inner_r, body_color, 2);
    draw_aa_corner(x + w - 1 - inner_r, y + h - 1 - inner_r, inner_r, body_color, 3);
    
    // Separator Line (Titlebar / Body)
    vbe_fill_rect(x + 1, y + 30, w - 2, 1, 0x00000000);

    // Title text
    vbe_draw_text(x + 10, y + 7, title, 0xFFE0E0E0, VBE_TRANSPARENT);
    
    // Window buttons (Close) - Rounded
    int btn_y = y + 5;
    int bx = x + w - 45;
    
    // Draw red circle button (using 4 AA corners to make a full circle)
    int cx = bx + 10;
    int cy = btn_y + 10;
    draw_aa_corner(cx, cy, 10, 0x00E81123, 0);
    draw_aa_corner(cx, cy, 10, 0x00E81123, 1);
    draw_aa_corner(cx, cy, 10, 0x00E81123, 2);
    draw_aa_corner(cx, cy, 10, 0x00E81123, 3);
    
    vbe_draw_text(cx - 4, cy - 8, "", VBE_WHITE, VBE_TRANSPARENT);
}

// Puffer zum Speichern der Ecken-Hintergründe
static unsigned int bl_corner[CORNER_RADIUS * CORNER_RADIUS];
static unsigned int br_corner[CORNER_RADIUS * CORNER_RADIUS];

static void save_corners(int x, int y, int w, int h) {
    // Unten-Links speichern
    for (int dy = 0; dy < CORNER_RADIUS; dy++) {
        for (int dx = 0; dx < CORNER_RADIUS; dx++) {
            bl_corner[dy * CORNER_RADIUS + dx] = vbe_get_pixel(x + dx, y + h - CORNER_RADIUS + dy);
        }
    }
    // Unten-Rechts speichern
    for (int dy = 0; dy < CORNER_RADIUS; dy++) {
        for (int dx = 0; dx < CORNER_RADIUS; dx++) {
            br_corner[dy * CORNER_RADIUS + dx] = vbe_get_pixel(x + w - CORNER_RADIUS + dx, y + h - CORNER_RADIUS + dy);
        }
    }
}

static void restore_corners(int x, int y, int w, int h) {
    // Unten-Links wiederherstellen (nur Pixel außerhalb des Radius)
    for (int dy = 0; dy < CORNER_RADIUS; dy++) {
        for (int dx = 0; dx < CORNER_RADIUS; dx++) {
            int rel_x = dx - CORNER_RADIUS;
            int rel_y = dy + 1;
            if (rel_x*rel_x + rel_y*rel_y > CORNER_RADIUS*CORNER_RADIUS) {
                vbe_set_pixel(x + dx, y + h - CORNER_RADIUS + dy, bl_corner[dy * CORNER_RADIUS + dx]);
            }
        }
    }
    // Unten-Rechts wiederherstellen
    for (int dy = 0; dy < CORNER_RADIUS; dy++) {
        for (int dx = 0; dx < CORNER_RADIUS; dx++) {
            int rel_x = dx + 1;
            int rel_y = dy + 1;
            if (rel_x*rel_x + rel_y*rel_y > CORNER_RADIUS*CORNER_RADIUS) {
                vbe_set_pixel(x + w - CORNER_RADIUS + dx, y + h - CORNER_RADIUS + dy, br_corner[dy * CORNER_RADIUS + dx]);
            }
        }
    }
}

void fill_round_rect(int x, int y, int w, int h, int r, unsigned int color) {
    vbe_fill_rect(x + r, y, w - 2 * r, h, color);
    vbe_fill_rect(x, y + r, r, h - 2 * r, color);
    vbe_fill_rect(x + w - r, y + r, r, h - 2 * r, color);
    draw_aa_corners_rect(x, y, w, h, r, color, 1, 1);
}

// ERSETZE die KOMPLETTE draw_button_modern() Funktion durch:
void draw_button_modern(int x, int y, int w, int h, int r, const char* text, unsigned int color) {
    unsigned int border = blend_colors(color, 0x00FFFFFF, 30);
    
    fill_round_rect(x, y, w, h, r, border);
    fill_round_rect(x + 1, y + 1, w - 2, h - 2, r, color);
    
    for (int i = 0; i < h/3; i++) {
        unsigned int grad_color = blend_colors(color, 0x00FFFFFF, 15 - i*15/(h/3));
        vbe_fill_rect(x + 2, y + 2 + i, w - 4, 1, grad_color);
    }
    
    int text_len = 0;
    while (text[text_len]) text_len++;
    int text_x = x + (w - text_len * 8) / 2;
    int text_y = y + (h - 16) / 2;
    
    vbe_draw_text(text_x, text_y, text, VBE_WHITE, VBE_TRANSPARENT);
}

// Helper function to convert string to lowercase
static void str_tolower(char* dest, const char* src) {
    int i = 0;
    while (src[i]) {
        if (src[i] >= 'A' && src[i] <= 'Z') {
            dest[i] = src[i] + 32;
        } else {
            dest[i] = src[i];
        }
        i++;
    }
    dest[i] = '\0';
}

// Helper function to check if substring exists
static int str_contains(const char* haystack, const char* needle) {
    if (!needle[0]) return 1; // Empty needle matches everything
    
    for (int i = 0; haystack[i]; i++) {
        int match = 1;
        for (int j = 0; needle[j]; j++) {
            if (haystack[i + j] != needle[j]) {
                match = 0;
                break;
            }
        }
        if (match) return 1;
    }
    return 0;
}

// Update filtered apps based on search query
static void update_filtered_apps() {
    filtered_count = 0;
    
    if (search_query_len == 0) {
        // No search query - show all apps
        for (int i = 0; i < MAX_WINDOWS; i++) {
            filtered_apps[filtered_count++] = i;
        }
    } else {
        // Convert search query to lowercase
        char query_lower[32];
        str_tolower(query_lower, search_query);
        
        // Filter apps
        for (int i = 0; i < MAX_WINDOWS; i++) {
            char app_lower[32];
            str_tolower(app_lower, app_names[i]);
            
            if (str_contains(app_lower, query_lower)) {
                filtered_apps[filtered_count++] = i;
            }
        }
    }
    
    // Reset selection if out of bounds
    if (start_menu_selected >= filtered_count) {
        start_menu_selected = filtered_count > 0 ? 0 : -1;
    }
}

void draw_start_menu() {
    if (!start_menu_open) return;
    
    int spacing = 12;
    int total_w = 36 + spacing + 200;
    int menu_x = (vbe_get_width() - total_w) / 2;
    int menu_y = 45;
    int menu_w = 300;
    int header_h = 40;
    int item_h = 45;
    int menu_h = header_h + (filtered_count * item_h) + 10;
    
    if (menu_h < 50) menu_h = 50; // Minimum height
    
    // Shadow
    draw_shadow(menu_x, menu_y, menu_w, menu_h);
    
    // Menu background (Rounded)
    int r = 8;
    fill_round_rect(menu_x, menu_y, menu_w, menu_h, r, 0x00444444);
    fill_round_rect(menu_x + 1, menu_y + 1, menu_w - 2, menu_h - 2, r, 0x001F1F1F);
    
    // Header (Rounded Top)
    int header_y = menu_y + 1;
    int header_w = menu_w - 2;
    unsigned int header_color = 0x00252525;
    
    vbe_fill_rect(menu_x + 1, header_y + r, header_w, header_h - r, header_color);
    vbe_fill_rect(menu_x + 1 + r, header_y, header_w - 2*r, r, header_color);
    draw_aa_corners_rect(menu_x + 1, header_y, header_w, header_h, r, header_color, 1, 0);
    
    if (filtered_count > 0) {
        vbe_draw_text(menu_x + 15, header_y + 14, "All Apps", VBE_WHITE, VBE_TRANSPARENT);
    } else {
        vbe_draw_text(menu_x + 15, header_y + 14, "No results", VBE_LIGHT_GRAY, VBE_TRANSPARENT);
    }
    
    // App list (filtered)
    // ERSETZE in draw_start_menu() die App-Liste-Schleife (ca. Zeile 345):
    for (int i = 0; i < filtered_count; i++) {
        int app_id = filtered_apps[i];
        int item_y = header_y + header_h + (i * item_h);
        
        if (i == start_menu_selected) {
            fill_round_rect(menu_x + 5, item_y + 2, menu_w - 10, 40, 10, 0x00555555);
            fill_round_rect(menu_x + 6, item_y + 3, menu_w - 12, 38, 9, WIN10_ACCENT);
        } else {
            fill_round_rect(menu_x + 5, item_y + 2, menu_w - 10, 40, 10, 0x00252525);
        }
        
        unsigned int icon_colors[MAX_WINDOWS] = {
            0x000078D7, 0x00FFB900, 0x0000CC6A, 0x00FF8C00,
            0x00E74856, 0x009933CC, 0x00000000, 0xFFD19A66,
            0xFF2D2D30
        };
        fill_round_rect(menu_x + 15, item_y + 10, 24, 24, 5, icon_colors[app_id]);
        vbe_draw_text(menu_x + 50, item_y + 14, app_names[app_id], VBE_WHITE, VBE_TRANSPARENT);
    }
}

static int cursor_type = 0; // 0 = Default (Point), 1 = Text (I-Beam)

static void update_cursor_shape() {
    cursor_type = 0; // Reset to default
    
    // 1. Taskbar Search Bar
    int spacing = 12;
    int total_w = 36 + spacing + 200;
    int start_x = (vbe_get_width() - total_w) / 2;
    int search_x = start_x + 36 + spacing;
    
    if (cursor_x >= search_x && cursor_x < search_x + 200 && cursor_y >= 5 && cursor_y < 35) {
        cursor_type = 1;
        return;
    }
    
    // Check Windows (iterate from top/front to bottom)
    for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
        int id = z_order[i];
        Window* w = &windows[id];
        
        if (!w->visible) continue;
        
        // Check if cursor is inside window
        if (cursor_x >= w->x && cursor_x < w->x + w->w &&
            cursor_y >= w->y && cursor_y < w->y + w->h) {
            
            int rel_x = cursor_x - w->x;
            int rel_y = cursor_y - w->y;
            
            // Notepad (ID 1) - Content area (below title bar)
            if (id == 1) {
                if (rel_y > 30) {
                    cursor_type = 1;
                }
            }
            // Google Browser (ID 4) - Address Bar
            else if (id == 4) {
                // Address Bar: Relative X [80, w-100], Relative Y [35, 65]
                // (Titlebar 30 + Offset 5 = 35, Height 30)
                if (rel_x >= 80 && rel_x < w->w - 100 && rel_y >= 35 && rel_y < 65) {
                    cursor_type = 1;
                }
            }
            
            // Stop checking other windows if we hit the top one
            return;
        }
    }
}

void draw_cursor() {
    if (!mouse_visible) return;
    if (cursor_type == 1) {
        // Text Cursor: Simple vertical line (2x16) matching the typing caret
        vbe_fill_rect(cursor_x - 1, cursor_y - 8, 2, 16, VBE_BLACK);
    } else {
        // Default: Simple point cursor (small filled circle)
        vbe_fill_circle(cursor_x, cursor_y, 4, VBE_YELLOW);
        vbe_draw_circle(cursor_x, cursor_y, 4, VBE_WHITE);
    }
}

// --- Tiny Font (4x6) for Date Display ---
// 0-9, :, .
static unsigned char tiny_font[12][6] = {
    {0x6, 0x9, 0x9, 0x9, 0x6, 0x0}, // 0
    {0x2, 0x6, 0x2, 0x2, 0x7, 0x0}, // 1
    {0x6, 0x9, 0x2, 0x4, 0xF, 0x0}, // 2
    {0x6, 0x9, 0x4, 0x9, 0x6, 0x0}, // 3
    {0x2, 0x6, 0xA, 0xF, 0x2, 0x0}, // 4
    {0xF, 0x8, 0xE, 0x1, 0xE, 0x0}, // 5
    {0x6, 0x8, 0xE, 0x9, 0x6, 0x0}, // 6
    {0xF, 0x1, 0x2, 0x4, 0x4, 0x0}, // 7
    {0x6, 0x9, 0x6, 0x9, 0x6, 0x0}, // 8
    {0x6, 0x9, 0x7, 0x1, 0x6, 0x0}, // 9
    {0x0, 0x6, 0x6, 0x0, 0x0, 0x0}, // :
    {0x0, 0x0, 0x0, 0x6, 0x6, 0x0}  // .
};

static void draw_tiny_char(int x, int y, char c, unsigned int color) {
    int idx = -1;
    if (c >= '0' && c <= '9') idx = c - '0';
    else if (c == ':') idx = 10;
    else if (c == '.') idx = 11;
    
    if (idx >= 0) {
        for (int py = 0; py < 6; py++) {
            unsigned char row = tiny_font[idx][py];
            for (int px = 0; px < 4; px++) {
                if ((row >> (3 - px)) & 1) {
                    vbe_set_pixel(x + px, y + py, color);
                }
            }
        }
    }
}

static void draw_tiny_text(int x, int y, const char* text, unsigned int color) {
    while (*text) {
        draw_tiny_char(x, y, *text, color);
        x += 5; // 4 width + 1 spacing
        text++;
    }
}

static unsigned char bcd2bin(unsigned char bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static void get_date_cmos(int* day, int* month, int* year) {
    outb(0x70, 0x07); *day = inb(0x71);
    outb(0x70, 0x08); *month = inb(0x71);
    outb(0x70, 0x09); *year = inb(0x71);
    
    outb(0x70, 0x0B);
    if (!(inb(0x71) & 0x04)) {
        *day = bcd2bin(*day);
        *month = bcd2bin(*month);
        *year = bcd2bin(*year);
    }
    *year += 2000;
}

// Fix for linker error: Provide missing memory functions
int get_total_memory() {
    return 128 * 1024 * 1024; // 128 MB (Simulated)
}

int get_free_memory() {
    return 100 * 1024 * 1024; // 100 MB (Simulated)
}

void gui_init() {
    vbe_init();
    vbe_clear(WIN10_BG);
    
    fs_init(); // Initialize Filesystem
    ethernet_init();
    mouse_init();
    calculator_init();
    notepad_init();
    welcome_init();
    filemanager_init();
    google_browser_init();
    memtest_init();
    terminal_init();
    python_ide_init();
    texteditor_init();
}

void gui_draw_desktop() {
    // Desktop background gradient
    for (int y = 0; y < vbe_get_height(); y++) {
        float t = (float)y / vbe_get_height();
        int r = 0x1A + (int)(t * (0x2C - 0x1A));
        int g = 0x2F + (int)(t * (0x4A - 0x2F));
        int b = 0x4F + (int)(t * (0x6F - 0x4F));
        unsigned int color = (r << 16) | (g << 8) | b;
        vbe_fill_rect(0, y, vbe_get_width(), 1, color);
    }
    
    // Taskbar (Dark Acrylic Look)
    vbe_fill_rect(0, 0, vbe_get_width(), 40, 0x00151515);
    vbe_draw_rect(0, 0, vbe_get_width(), 40, 0x00333333);
    
    // ===== START BUTTON (Modern Box with 4 Circles) =====
    int spacing = 12;
    int total_w = 36 + spacing + 200;
    int btn_x = (vbe_get_width() - total_w) / 2;
    
    int btn_y = 5, btn_w = 36, btn_h = 30, btn_r = 8;
    unsigned int start_bg = start_menu_open ? 0x00404040 : 0x00252525;
    unsigned int start_border = 0x00555555;
    
    // Border
    fill_round_rect(btn_x - 1, btn_y - 1, btn_w + 2, btn_h + 2, btn_r + 1, start_border);
    // Main button
    fill_round_rect(btn_x, btn_y, btn_w, btn_h, btn_r, start_bg);
    
    // 4 Circles Icon
    int cx = btn_x + btn_w / 2;
    int cy = btn_y + btn_h / 2;
    int cr = 4; // radius
    int co = 6; // offset
    
    unsigned int icon_color = 0x000078D7; // Uniform Blue

    // Top-Left Circle
    draw_aa_corner(cx - co, cy - co, cr, icon_color, 0);
    draw_aa_corner(cx - co, cy - co, cr, icon_color, 1);
    draw_aa_corner(cx - co, cy - co, cr, icon_color, 2);
    draw_aa_corner(cx - co, cy - co, cr, icon_color, 3);

    // Top-Right Circle
    draw_aa_corner(cx + co, cy - co, cr, icon_color, 0);
    draw_aa_corner(cx + co, cy - co, cr, icon_color, 1);
    draw_aa_corner(cx + co, cy - co, cr, icon_color, 2);
    draw_aa_corner(cx + co, cy - co, cr, icon_color, 3);

    // Bottom-Left Circle
    draw_aa_corner(cx - co, cy + co, cr, icon_color, 0);
    draw_aa_corner(cx - co, cy + co, cr, icon_color, 1);
    draw_aa_corner(cx - co, cy + co, cr, icon_color, 2);
    draw_aa_corner(cx - co, cy + co, cr, icon_color, 3);

    // Bottom-Right Circle
    draw_aa_corner(cx + co, cy + co, cr, icon_color, 0);
    draw_aa_corner(cx + co, cy + co, cr, icon_color, 1);
    draw_aa_corner(cx + co, cy + co, cr, icon_color, 2);
    draw_aa_corner(cx + co, cy + co, cr, icon_color, 3);
    
    // ===== SEARCH BAR (Clean rounded) =====
    int search_x = btn_x + btn_w + spacing, search_y = 5, search_w = 200, search_h = 30, search_r = 8;
    unsigned int search_border = start_menu_open ? WIN10_ACCENT : 0x00555555;
    unsigned int search_bg = 0x00252525;

    // Border
    fill_round_rect(search_x - 1, search_y - 1, search_w + 2, search_h + 2, search_r + 1, search_border);
    // Background
    fill_round_rect(search_x, search_y, search_w, search_h, search_r, search_bg);
    
    // Text/Cursor
    if (search_query_len > 0) {
        vbe_draw_text(search_x + 10, search_y + 7, search_query, VBE_WHITE, VBE_TRANSPARENT);
        if (start_menu_open && caret_state) {
            int cursor_x_pos = search_x + 10 + (search_query_len * 8);
            vbe_fill_rect(cursor_x_pos, search_y + 7, 2, 16, VBE_WHITE);
        }
    } else {
        if (start_menu_open && caret_state) {
            vbe_fill_rect(search_x + 10, search_y + 7, 2, 16, VBE_WHITE);
        } else {
            vbe_draw_text(search_x + 10, search_y + 7, "Type here to search", 0x00888888, VBE_TRANSPARENT);
        }
    }
    
    // ===== TASKBAR APP INDICATORS (Clean rounded) =====
    int taskbar_x = search_x + search_w + 20;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].visible) {
            int ind_r = 8;
            unsigned int indicator_color = (i == selected_window) ? WIN10_ACCENT : 0x002D2D2D;
            unsigned int ind_border = blend_colors(indicator_color, 0x00FFFFFF, 30);
            
            // Border
            fill_round_rect(taskbar_x - 1, 4, 62, 32, ind_r + 1, ind_border);
            // Main
            fill_round_rect(taskbar_x, 5, 60, 30, ind_r, indicator_color);
            
            // App initial
            char initial[2] = {app_names[i][0], '\0'};
            vbe_draw_text(taskbar_x + 24, 13, initial, VBE_WHITE, VBE_TRANSPARENT);
            
            taskbar_x += 65;
        }
    }
    
    // ===== ETHERNET INDICATOR BOX (Clean rounded) =====
    // IP Address Display (links neben der Box)
    IP_Address my_ip = ethernet_get_ip();
    char ip_str[32];
    char num[8];
    int_to_str(my_ip.addr[0], ip_str); strcat(ip_str, ".");
    int_to_str(my_ip.addr[1], num); strcat(ip_str, num); strcat(ip_str, ".");
    int_to_str(my_ip.addr[2], num); strcat(ip_str, num); strcat(ip_str, ".");
    int_to_str(my_ip.addr[3], num); strcat(ip_str, num);
    
    int ip_w = 100;
    if (!ethernet_is_connected()) {
        vbe_draw_text(vbe_get_width() - 300, 13, "No Hardware Found", 0xFFFF0000, VBE_TRANSPARENT);
    } else {
        vbe_draw_text(vbe_get_width() - 300, 13, ip_str, 0xFF888888, VBE_TRANSPARENT);
    }

    int eth_w = 40, eth_h = 30, eth_r = 8;
    int clock_w = 80;
    int clock_x = vbe_get_width() - clock_w - 10;
    int eth_x = clock_x - eth_w - 10;
    
    unsigned int box_color = 0x00252525;
    unsigned int box_border = 0x00404040;
    
    // Border
    fill_round_rect(eth_x - 1, 4, eth_w + 2, eth_h + 2, eth_r + 1, box_border);
    // Background
    fill_round_rect(eth_x, 5, eth_w, eth_h, eth_r, box_color);
    
    // Status dot (perfect circle using AA corners)
    unsigned int eth_color = ethernet_is_connected() ? 0x0000FF00 : 0x00FF0000;
    int dot_x = eth_x + (eth_w / 2);
    int dot_y = 20;
    int dot_r = 6;
    
    draw_aa_corner(dot_x, dot_y, dot_r, eth_color, 0);
    draw_aa_corner(dot_x, dot_y, dot_r, eth_color, 1);
    draw_aa_corner(dot_x, dot_y, dot_r, eth_color, 2);
    draw_aa_corner(dot_x, dot_y, dot_r, eth_color, 3);
    
    // ===== CLOCK BOX (Clean rounded) =====
    // Border
    fill_round_rect(clock_x - 1, 4, clock_w + 2, eth_h + 2, eth_r + 1, box_border);
    // Background
    fill_round_rect(clock_x, 5, clock_w, eth_h, eth_r, box_color);
    
    // Time
    int hour, min, sec;
    rtc_get_time(&hour, &min, &sec);
    char time_str[16];
    int_to_str(hour, time_str);
    int len = 0;
    while (time_str[len]) len++;
    time_str[len] = ':';
    time_str[len + 1] = '0' + (min / 10);
    time_str[len + 2] = '0' + (min % 10);
    time_str[len + 3] = '\0';
    
    vbe_draw_text(clock_x + 20, 8, time_str, VBE_WHITE, VBE_TRANSPARENT);
    
    // Date
    int day, month, year;
    get_date_cmos(&day, &month, &year);
    
    char date_str[16];
    date_str[0] = '0' + (day / 10);
    date_str[1] = '0' + (day % 10);
    date_str[2] = '.';
    date_str[3] = '0' + (month / 10);
    date_str[4] = '0' + (month % 10);
    date_str[5] = '.';
    int_to_str(year, &date_str[6]);
    
    draw_tiny_text(clock_x + 15, 26, date_str, 0xFFCCCCCC);
}

void gui_draw_window(int id) {
    Window* w = &windows[id];
    if (!w->visible) return;
    
    int is_active = (id == selected_window);
    
    if (id == 0) {
        draw_window_modern(w->x, w->y, w->w, w->h, "Welcome", is_active);
        save_corners(w->x, w->y, w->w, w->h);
        welcome_draw_vbe(w->x, w->y + 30, w->w, w->h - 30, is_active);
        restore_corners(w->x, w->y, w->w, w->h);
    } else if (id == 1) {
        draw_window_modern(w->x, w->y, w->w, w->h, "Notepad", is_active);
        save_corners(w->x, w->y, w->w, w->h);
        notepad_draw_vbe(w->x, w->y + 30, w->w, w->h - 30, is_active);
        restore_corners(w->x, w->y, w->w, w->h);
    } else if (id == 2) {
        draw_window_modern(w->x, w->y, w->w, w->h, "Calculator", is_active);
        save_corners(w->x, w->y, w->w, w->h);
        calculator_draw_vbe(w->x, w->y + 30, w->w, w->h - 30, is_active);
        restore_corners(w->x, w->y, w->w, w->h);
    } else if (id == 3) {
        draw_window_modern(w->x, w->y, w->w, w->h, "Files", is_active);
        save_corners(w->x, w->y, w->w, w->h);
        filemanager_draw_vbe(w->x, w->y + 30, w->w, w->h - 30, is_active);
        restore_corners(w->x, w->y, w->w, w->h);
    } else if (id == 4) {
        draw_window_modern(w->x, w->y, w->w, w->h, "Google Browser", is_active);
        save_corners(w->x, w->y, w->w, w->h);
        google_browser_draw_vbe(w->x, w->y + 30, w->w, w->h - 30, is_active);
        restore_corners(w->x, w->y, w->w, w->h);
    } else if (id == 5) {
        draw_window_modern(w->x, w->y, w->w, w->h, "Memory Doctor", is_active);
        save_corners(w->x, w->y, w->w, w->h);
        memtest_draw_vbe(w->x, w->y + 30, w->w, w->h - 30, is_active);
        restore_corners(w->x, w->y, w->w, w->h);
    } else if (id == 6) {
        draw_window_modern(w->x, w->y, w->w, w->h, "Terminal", is_active);
        save_corners(w->x, w->y, w->w, w->h);
        terminal_draw_vbe(w->x, w->y + 30, w->w, w->h - 30, is_active);
        restore_corners(w->x, w->y, w->w, w->h);
    } else if (id == 7) {
        draw_window_modern(w->x, w->y, w->w, w->h, "Python IDE", is_active);
        save_corners(w->x, w->y, w->w, w->h);
        python_ide_draw_vbe(w->x, w->y + 30, w->w, w->h - 30, is_active);
        restore_corners(w->x, w->y, w->w, w->h);
    } else if (id == 8) {
        draw_window_modern(w->x, w->y, w->w, w->h, "Text Editor", is_active);
        save_corners(w->x, w->y, w->w, w->h);
        texteditor_draw_vbe(w->x, w->y + 30, w->w, w->h - 30, is_active);
        restore_corners(w->x, w->y, w->w, w->h);
    }
}

void gui_draw_all_windows() {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        int id = z_order[i];
        if (windows[id].visible) {
            gui_draw_window(id);
        }
    }
}

static void bring_to_front(int id) {
    int found_idx = -1;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (z_order[i] == id) {
            found_idx = i;
            break;
        }
    }
    
    if (found_idx != -1) {
        for (int i = found_idx; i < MAX_WINDOWS - 1; i++) {
            z_order[i] = z_order[i + 1];
        }
        z_order[MAX_WINDOWS - 1] = id;
    }
}

static int check_window_click(int mx, int my) {
    for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
        int id = z_order[i];
        Window* w = &windows[id];
        
        if (!w->visible) continue;
        
        if (mx >= w->x && mx < w->x + w->w &&
            my >= w->y && my < w->y + w->h) {
            return id;
        }
    }
    return -1;
}

static int check_start_button_click(int mx, int my) {
    // START button bereich
    int spacing = 12;
    int total_w = 36 + spacing + 200;
    int btn_x = (vbe_get_width() - total_w) / 2;
    
    if (mx >= btn_x && mx < btn_x + 36 && my >= 5 && my < 35) {
        return 1;
    }
    return 0;
}

static int check_start_menu_click(int mx, int my) {
    if (!start_menu_open) return -1;
    
    int spacing = 12;
    int total_w = 36 + spacing + 200;
    int menu_x = (vbe_get_width() - total_w) / 2;
    
    int menu_y = 45;
    int menu_w = 300;
    int header_h = 40;
    int item_h = 45;
    int menu_h = header_h + (filtered_count * item_h) + 10;
    if (menu_h < 50) menu_h = 50;
    
    // Prüfe ob außerhalb des Menüs geklickt wurde
    if (mx < menu_x || mx > menu_x + menu_w ||
        my < menu_y || my > menu_y + menu_h) {
        return -2; // Außerhalb geklickt -> Menü schließen
    }
    
    // Prüfe welcher App-Eintrag geklickt wurde
    int header_y = menu_y;
    if (my > header_y + header_h) {
        int item_index = (my - header_y - header_h) / item_h;
        if (item_index >= 0 && item_index < filtered_count) {
            return filtered_apps[item_index];
        }
    }
    
    return -1;
}

static int check_search_bar_click(int mx, int my) {
    int spacing = 12;
    int total_w = 36 + spacing + 200;
    int start_x = (vbe_get_width() - total_w) / 2;
    int search_x = start_x + 36 + spacing;
    
    if (mx >= search_x && mx < search_x + 200 && my >= 5 && my < 35) {
        return 1;
    }
    return 0;
}

static int check_close_button(int mx, int my, int id) {
    Window* w = &windows[id];
    
    // Modern style (Standard for all windows)
    // Close button: x + w - 45 to x + w - 25, y to y + 20
    // Expanded hit box for better usability: x + w - 50 to x + w - 20, y to y + 30
    if (mx >= w->x + w->w - 50 && mx < w->x + w->w - 20 &&
        my >= w->y && my < w->y + 30) {
        return 1;
    }
    return 0;
}

void gui_close_window(int id) {
    if (id >= 0 && id < MAX_WINDOWS) {
        windows[id].visible = 0;
    }
}

void gui_open_window(int id) {
    if (id >= 0 && id < MAX_WINDOWS) {
        windows[id].visible = 1;
        selected_window = id;
        bring_to_front(id);
    }
}

int gui_is_window_visible(int id) {
    if (id >= 0 && id < MAX_WINDOWS) {
        return windows[id].visible;
    }
    return 0;
}

// Hilfsfunktion für Mausklicks (vermeidet Code-Duplizierung)
static void handle_left_click(int x, int y) {
    int clicked;
    if (start_menu_open) {
        if ((clicked = check_start_menu_click(x, y)) >= 0) {
            gui_open_window(clicked);
            start_menu_open = 0;
            search_query_len = 0;
            search_query[0] = '\0';
        } else if (check_search_bar_click(x, y)) {
            // Suchfeld Fokus behalten
        } else {
            start_menu_open = 0;
        }
    }
    else if (check_start_button_click(x, y)) {
        start_menu_open = !start_menu_open;
        start_menu_selected = 0;
        search_query_len = 0;
        search_query[0] = '\0';
        update_filtered_apps();
    }
    else if (check_search_bar_click(x, y)) {
        start_menu_open = 1;
        start_menu_selected = 0;
        search_query_len = 0;
        search_query[0] = '\0';
        update_filtered_apps();
    }
    else {
        clicked = check_window_click(x, y);
        if (clicked != -1) {
            selected_window = clicked;
            bring_to_front(clicked);
            
            Window* w = &windows[clicked];
            if (check_close_button(x, y, clicked)) {
                gui_close_window(clicked);
            } else {
                // Inhalt-Klick weiterleiten
                int header_h = 30;
                int rel_x = x - w->x;
                int rel_y = y - w->y - header_h;
                
                if (clicked == 0) welcome_handle_click_vbe(rel_x, rel_y);
                else if (clicked == 1) notepad_handle_click_vbe(rel_x, rel_y);
                else if (clicked == 2) calculator_handle_click_vbe(rel_x, rel_y);
                else if (clicked == 3) filemanager_handle_click_vbe(rel_x, rel_y);
                else if (clicked == 4) google_browser_handle_click_vbe(rel_x, rel_y);
                else if (clicked == 5) memtest_handle_click_vbe(rel_x, rel_y);
                // Terminal (6) handles keys, not clicks usually, but we could focus input
                else if (clicked == 7) python_ide_handle_click(rel_x, rel_y);
            }
        }
    }
}

void gui_run() {
    gui_init();
    gui_draw_desktop();
    gui_draw_all_windows();
    draw_cursor();
    
    int move_step = 15;
    int blink_timer = 0;
    int dhcp_retry_timer = 0;
    
    while (1) {
        int needs_redraw = 0;
        
        // --- NETZWERK POLLING ---
        // Prüfen, ob ein Paket angekommen ist und an den TCP/IP Stack weiterleiten
        EthernetFrame eth_frame;
        if (ethernet_recv_frame(&eth_frame) == 0) { // 0 = Erfolg
            net_handle_packet((unsigned char*)&eth_frame, eth_frame.length + 14);
        }

        // --- MAUS TREIBER (Polling) ---
        int m_dx, m_dy, m_buttons;
        if (mouse_read_packet(&m_dx, &m_dy, &m_buttons)) {
            cursor_x += m_dx;
            cursor_y += m_dy;
            
            // Begrenzung
            if (cursor_x < 0) cursor_x = 0;
            if (cursor_x >= vbe_get_width()) cursor_x = vbe_get_width() - 1;
            if (cursor_y < 0) cursor_y = 0;
            if (cursor_y >= vbe_get_height()) cursor_y = vbe_get_height() - 1;
            
            mouse_visible = 1;
            needs_redraw = 1;
            
            // Linksklick (Bit 0)
            static int prev_left = 0;
            if ((m_buttons & 1) && !prev_left) {
                handle_left_click(cursor_x, cursor_y);
            }
            prev_left = (m_buttons & 1);
        }
        
        // --- DHCP RETRY LOGIC ---
        // Wenn wir noch keine IP haben (0.0.0.0), versuchen wir es periodisch erneut
        dhcp_retry_timer++;
        if (dhcp_retry_timer > 200) { // Alle ca. 2-3 Sekunden (je nach CPU Speed)
            dhcp_retry_timer = 0;
            IP_Address my_ip = ethernet_get_ip();
            if (my_ip.addr[0] == 0) { // Annahme: Erstes Byte 0 heißt keine IP
                net_send_dhcp_discover();
            }
        }
        
        // Blink Timer (Cursor blinken lassen)
        blink_timer++;
        if (blink_timer >= 30) { // Geschwindigkeit anpassen
            blink_timer = 0;
            caret_state = !caret_state;
            // Neu zeichnen, wenn Textfelder aktiv sind
            if (start_menu_open || (selected_window == 4 && windows[4].visible)) {
                needs_redraw = 1;
            }
        }

        // Keyboard handling
        unsigned char sc = keyboard_read_nonblock();
        
        if (sc == 0x2A || sc == 0x36) shift_pressed = 1;
        else if (sc == 0xAA || sc == 0xB6) shift_pressed = 0;
        
        if (sc && !(sc & 0x80)) {
            // Bei Eingabe Timer resetten (Cursor sofort sichtbar machen)
            blink_timer = 0;
            caret_state = 1;
            
            if (sc == 0x01) { // ESC
                if (start_menu_open) {
                    start_menu_open = 0;
                    needs_redraw = 1;
                } else {
                    return;
                }
            }
            
            // TAB - switch windows
            else if (sc == 0x0F) {
                if (start_menu_open) {
                    start_menu_open = 0;
                    needs_redraw = 1;
                } else {
                    // Finde nächstes sichtbares Fenster
                    int start = selected_window;
                    do {
                        selected_window = (selected_window + 1) % MAX_WINDOWS;
                        if (windows[selected_window].visible) {
                            bring_to_front(selected_window);
                            needs_redraw = 1;
                            break;
                        }
                    } while (selected_window != start);
                }
            }
            
            // Arrow keys - move cursor or navigate start menu
            else if (sc == 0x48) { // Up arrow
                if (start_menu_open) {
                    if (filtered_count > 0) {
                        start_menu_selected = (start_menu_selected - 1 + filtered_count) % filtered_count;
                    }
                    needs_redraw = 1;
                } else {
                    cursor_y -= move_step;
                    if (cursor_y < 0) cursor_y = 0;
                    mouse_visible = 1;
                    needs_redraw = 1;
                }
            }
            else if (sc == 0x50) { // Down arrow
                if (start_menu_open) {
                    if (filtered_count > 0) {
                        start_menu_selected = (start_menu_selected + 1) % filtered_count;
                    }
                    needs_redraw = 1;
                } else {
                    cursor_y += move_step;
                    if (cursor_y >= vbe_get_height()) cursor_y = vbe_get_height() - 1;
                    mouse_visible = 1;
                    needs_redraw = 1;
                }
            }
            else if (sc == 0x4B) { // Left arrow
                cursor_x -= move_step;
                if (cursor_x < 0) cursor_x = 0;
                mouse_visible = 1;
                needs_redraw = 1;
            }
            else if (sc == 0x4D) { // Right arrow
                cursor_x += move_step;
                if (cursor_x >= vbe_get_width()) cursor_x = vbe_get_width() - 1;
                mouse_visible = 1;
                needs_redraw = 1;
            }
            
            // Backspace for Notepad or Search
            else if (sc == 0x0E) { // Backspace
                mouse_visible = 0;
                if (start_menu_open && search_query_len > 0) {
                    // Remove last character from search
                    search_query_len--;
                    search_query[search_query_len] = '\0';
                    update_filtered_apps();
                    needs_redraw = 1;
                } else if (selected_window == 1 && windows[1].visible) {
                    notepad_handle_backspace();
                    needs_redraw = 1;
                }
                else if (selected_window == 4 && windows[4].visible) {
                    google_browser_handle_backspace();
                    needs_redraw = 1;
                }
                else if (selected_window == 6 && windows[6].visible) {
                    terminal_handle_backspace();
                    needs_redraw = 1;
                }
                else if (selected_window == 7 && windows[7].visible) {
                    python_ide_handle_backspace();
                    needs_redraw = 1;
                    }
                    else if (selected_window == 8 && windows[8].visible && !start_menu_open) {
                        texteditor_handle_backspace();
                        needs_redraw = 1;
                }
            }
            
            // Enter - click or open app from start menu
            else if (sc == 0x1C) { // Enter
                int clicked;
                // Wenn Start-Menü offen ist
                if (start_menu_open) {
                    // 1. Prüfen ob direkt auf einen App-Eintrag geklickt wurde (Maus)
                    if ((clicked = check_start_menu_click(cursor_x, cursor_y)) >= 0) {
                        gui_open_window(clicked);
                        start_menu_open = 0;
                        search_query_len = 0;
                        search_query[0] = '\0';
                        needs_redraw = 1;
                    }
                    // 2. Standard Enter-Aktion: Ausgewählte App öffnen (auch wenn Cursor woanders ist)
                    // Priorität erhöht: Wenn Apps gefunden wurden, öffnet Enter diese, statt das Suchfeld zu klicken
                    else if (filtered_count > 0 && start_menu_selected >= 0) {
                        gui_open_window(filtered_apps[start_menu_selected]);
                        start_menu_open = 0;
                        search_query_len = 0;
                        search_query[0] = '\0';
                        needs_redraw = 1;
                    }
                    // 3. Check click on search bar (Mouse focus) - nur wenn nichts geöffnet wurde
                    else if (check_search_bar_click(cursor_x, cursor_y)) {
                        // Clicked search bar while open -> keep open
                        mouse_visible = 0;
                        needs_redraw = 1;
                    }
                    // 3. Nur schließen, wenn keine Apps da sind oder explizit daneben geklickt wurde
                    else {
                        start_menu_open = 0;
                        needs_redraw = 1;
                    }
                }
                // Prüfe START button
                else if (check_start_button_click(cursor_x, cursor_y)) {
                    start_menu_open = !start_menu_open;
                    start_menu_selected = 0;
                    search_query_len = 0;
                    search_query[0] = '\0';
                    update_filtered_apps();
                    needs_redraw = 1;
                }
                // Prüfe Search Bar
                else if (check_search_bar_click(cursor_x, cursor_y)) {
                    start_menu_open = 1;
                    start_menu_selected = 0;
                    search_query_len = 0;
                    search_query[0] = '\0';
                    update_filtered_apps();
                    mouse_visible = 0;
                    needs_redraw = 1;
                }
                // Prüfe Window-Klick
                else {
                    // ZUERST prüfen, ob eine App fokussiert ist, die Tastatureingaben (Enter) benötigt.
                    // ABER: Wenn der Cursor auf der Titelleiste oder Toolbar ist, soll Enter klicken!
                    int handled_as_text = 0;
                    Window* w = &windows[selected_window];
                    int in_title_bar = (cursor_y >= w->y && cursor_y < w->y + 30 && cursor_x >= w->x && cursor_x < w->x + w->w);

                    if (!in_title_bar) {
                        if (selected_window == 1 && windows[1].visible) {
                            notepad_handle_char('\n');
                            handled_as_text = 1;
                        }
                        else if (selected_window == 4 && windows[4].visible) {
                            google_browser_handle_input('\n');
                            handled_as_text = 1;
                        }
                        else if (selected_window == 6 && windows[6].visible) {
                            terminal_handle_enter();
                            handled_as_text = 1;
                        }
                        else if (selected_window == 7 && windows[7].visible) {
                            // Python IDE: Toolbar check (y+30 to y+60)
                            int in_toolbar = (cursor_y >= w->y + 30 && cursor_y < w->y + 60 && cursor_x >= w->x && cursor_x < w->x + w->w);
                            if (!in_toolbar) {
                                python_ide_handle_key('\n');
                                handled_as_text = 1;
                            }
                        }
                    }

                    if (handled_as_text) {
                        mouse_visible = 0;
                        needs_redraw = 1;
                    }
                    else {
                        // Wenn keine Text-App aktiv ist (oder Cursor auf UI), verhält sich Enter wie ein Mausklick
                        clicked = check_window_click(cursor_x, cursor_y);
                        if (clicked != -1) {
                            selected_window = clicked;
                            bring_to_front(clicked);
                            
                            Window* w = &windows[clicked];
                            
                            // Check if close button was clicked
                            if (check_close_button(cursor_x, cursor_y, clicked)) {
                                gui_close_window(clicked);
                                needs_redraw = 1;
                            }
                            // Otherwise, handle window content click
                            else {
                                int header_h = 30;
                                int rel_x = cursor_x - w->x;
                                int rel_y = cursor_y - w->y - header_h;
                                
                                if (clicked == 0) {
                                    welcome_handle_click_vbe(rel_x, rel_y);
                                } else if (clicked == 1) {
                                    if (rel_y >= 0) mouse_visible = 0;
                                    notepad_handle_click_vbe(rel_x, rel_y);
                                } else if (clicked == 2) {
                                    calculator_handle_click_vbe(rel_x, rel_y);
                                } else if (clicked == 3) {
                                    filemanager_handle_click_vbe(rel_x, rel_y);
                                } else if (clicked == 4) {
                                    // Address bar check (relative to content area y=30)
                                    if (rel_x >= 80 && rel_x < w->w - 100 && rel_y >= 5 && rel_y < 35) {
                                        mouse_visible = 0;
                                    }
                                    google_browser_handle_click_vbe(rel_x, rel_y);
                                } else if (clicked == 5) {
                                    memtest_handle_click_vbe(rel_x, rel_y);
                                }
                                // Terminal (6) click
                                else if (clicked == 7) python_ide_handle_click(rel_x, rel_y);
                                else if (clicked == 8) texteditor_handle_click(rel_x, rel_y);
                                
                                needs_redraw = 1;
                            }
                        }
                    }
                }
            }
            // Spacebar or other text input for Notepad or Search
            else if (sc == 0x39) { // Spacebar
                mouse_visible = 0;
                if (start_menu_open && search_query_len < 31) {
                    search_query[search_query_len++] = ' ';
                    search_query[search_query_len] = '\0';
                    update_filtered_apps();
                    needs_redraw = 1;
                } else if (selected_window == 1 && windows[1].visible && !start_menu_open) {
                    notepad_handle_char(' ');
                    needs_redraw = 1;
                } else if (selected_window == 4 && windows[4].visible && !start_menu_open) {
                    google_browser_handle_input(' ');
                    needs_redraw = 1;
                } else if (selected_window == 6 && windows[6].visible && !start_menu_open) {
                    terminal_handle_key(' ');
                    needs_redraw = 1;
                } else if (selected_window == 7 && windows[7].visible && !start_menu_open) {
                    python_ide_handle_key(' ');
                    needs_redraw = 1;
                }
            }
            // Convert scancode to ASCII for text input
            else {
                char ascii = scancode_to_ascii(sc);
                
                if (shift_pressed) {
                    if (ascii >= 'a' && ascii <= 'z') ascii -= 32;
                    else if (ascii == '1') ascii = '!';
                    else if (ascii == '2') ascii = '"';
                    else if (ascii == '3') ascii = 0; // Paragraph (nicht in ASCII)
                    else if (ascii == '4') ascii = '$';
                    else if (ascii == '5') ascii = '%';
                    else if (ascii == '6') ascii = '&';
                    else if (ascii == '7') ascii = '/';
                    else if (ascii == '8') ascii = '(';
                    else if (ascii == '9') ascii = ')';
                    else if (ascii == '0') ascii = '=';
                }
                
                if (ascii) {
                    mouse_visible = 0;
                    if (start_menu_open && search_query_len < 31) {
                        // Add to search query
                        search_query[search_query_len++] = ascii;
                        search_query[search_query_len] = '\0';
                        update_filtered_apps();
                        needs_redraw = 1;
                    } else if (selected_window == 1 && windows[1].visible && !start_menu_open) {
                        notepad_handle_char(ascii);
                        needs_redraw = 1;
                    } else if (selected_window == 4 && windows[4].visible && !start_menu_open) {
                        google_browser_handle_input(ascii);
                        needs_redraw = 1;
                    } else if (selected_window == 6 && windows[6].visible && !start_menu_open) {
                        terminal_handle_key(ascii);
                        needs_redraw = 1;
                    } else if (selected_window == 7 && windows[7].visible && !start_menu_open) {
                        python_ide_handle_key(ascii);
                        needs_redraw = 1;
                    }
                    else if (selected_window == 8 && windows[8].visible && !start_menu_open) {
                        texteditor_handle_key(ascii);
                        needs_redraw = 1;
                    }
                }
            }
            
            if (needs_redraw) {
                update_cursor_shape(); // Check what's under the cursor
                gui_draw_desktop();
                gui_draw_all_windows();
                draw_start_menu(); // Start-Menü über allem zeichnen
                draw_cursor();
            }
        }
        
        // Small delay to prevent CPU spinning
        for (volatile int i = 0; i < 10000; i++);
    }
}

// Compatibility stubs
void draw_box(int x, int y, int w, int h, unsigned char color) {}
void draw_text_at(int x, int y, const char* text, unsigned char color) {}
void draw_button(int x, int y, const char* text, unsigned char color) {}
int check_button_click(int cx, int cy, int bx, int by, const char* text) { return 0; }

// Workaround: Include Terminal implementation directly to fix linker errors
#include "apps/terminal.c"