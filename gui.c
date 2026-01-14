#include "gui.h"
#include "port.h"
#include "vbe.h"
#include "keyboard.h"
#include "mouse.h"
#include "string.h"
#include "rtc.h"
#include "ethernet.h"
#include "fs.h"
#include "net.h"
#include "modern_font.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

// Forward declaration for function in fs.c, likely missing from fs.h
int fs_save_file(const char* name, const char* data, int len);

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

static void draw_aa_corner_hd_fixed(int cx, int cy, int r, unsigned int color, int quadrant);

static void draw_aa_corner(int cx, int cy, int r, unsigned int color, int quadrant) {
    int r_sq = r * r;
    int r_limit_sq = (r - 1) * (r - 1); 
    
    for (int dy = 0; dy < r; dy++) {
        for (int dx = 0; dx < r; dx++) {
            int px = (quadrant == 1 || quadrant == 3) ? (cx + dx) : (cx - 1 - dx);
            int py = (quadrant == 2 || quadrant == 3) ? (cy + dy) : (cy - 1 - dy);
            
            int d2 = (dx + 1) * (dx + 1) + (dy + 1) * (dy + 1);
            int alpha = 255;
            
            if (d2 < r_limit_sq) {
                alpha = 255;
            } 
            else if (d2 >= r_sq) {
                alpha = 0;
            }
            else {
                int dist = isqrt(d2 * 256 * 256);
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
void draw_aa_corners_rect(int x, int y, int w, int h, int r, unsigned int color, int top, int bottom) {
    if (top) {
        draw_aa_corner_hd_fixed(x + r, y + r, r, color, 0);     // TL
        draw_aa_corner_hd_fixed(x + w - r, y + r, r, color, 1); // TR
    }
    if (bottom) {
        draw_aa_corner_hd_fixed(x + r, y + h - r, r, color, 2);     // BL
        draw_aa_corner_hd_fixed(x + w - r, y + h - r, r, color, 3); // BR
    }
}

// Apps direkt einbinden (Unity Build)
#include "apps/welcome.h"
#include "apps/calculator.h"
#include "apps/filemanager.h"
#include "apps/memtest_app.h"
#include "apps/python_interpreter.h"
#include "apps/terminal.c"
#include "apps/python_ide.h"
#include "apps/texteditor.h"
#include "apps/settings.h"
#include "apps/settings.c"

#define MOUSE_PORT_STATUS 0x64
#define TASKBAR_HEIGHT 50

#ifdef WIN10_BG
#undef WIN10_BG
#endif
#define WIN10_BG 0xFF1F1F1F
#define WIN10_ACCENT 0x000078D7

static const char* app_names[] = {
#define APP(ID, NAME, ...) NAME,
#include "app_list.def"
#undef APP
};

#define NUM_APPS (sizeof(app_names) / sizeof(app_names[0]))

// Compile-time check: MAX_WINDOWS muss groß genug sein (sizeof geht nicht im Preprocessor #if)
typedef char check_max_windows_size[(MAX_WINDOWS >= NUM_APPS) ? 1 : -1];

static int selected_window = 0;
static int caret_state = 1;
static int shift_pressed = 0;

int gui_get_caret_state() {
    return caret_state;
}

static int z_order[MAX_WINDOWS] = {0, 1, 2, 3, 4, 5, 6, 7, 8};

static unsigned int cursor_backup[32 * 32]; // Max Cursor-Größe (Vergrößert für HD Cursor)
static int last_cursor_x = -1;
static int last_cursor_y = -1;
static int cursor_backup_valid = 0;

// Mouse cursor
static int cursor_x = 512;
static int cursor_y = 384;
static int mouse_visible = 1;
static int cursor_type = 0; // 0 = Default (Point), 1 = Text (I-Beam)

// Forward declarations
static void update_cursor_shape();
void draw_cursor();
void draw_taskbar_search();
void draw_window_modern_hd(int x, int y, int w, int h, const char* title, int is_active, int is_maximized);
static void draw_aa_corner_hd_fixed(int cx, int cy, int r, unsigned int color, int quadrant);
void draw_text_hd(int x, int y, const char* text, unsigned int color, int size);
static void save_corners(int x, int y, int w, int h);
static void restore_corners(int x, int y, int w, int h);

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
    int is_maximized;
    int prev_x, prev_y, prev_w, prev_h;
} Window;

// Dirty Rectangle System
typedef struct {
    int x, y, w, h;
    int dirty;
} DirtyRect;

static DirtyRect dirty_regions[10];
static int dirty_count = 0;

static int update_start_menu = 0;

// Window Dragging State
static int is_dragging = 0;
static int drag_window_id = -1;
static int drag_off_x = 0;
static int drag_off_y = 0;

static Window windows[MAX_WINDOWS];

// Funktionszeiger-Typen für App-Handler
typedef void (*app_init_func)();
typedef void (*app_draw_func)(int x, int y, int w, int h, int is_active);
typedef void (*app_click_func)(int rel_x, int rel_y);
typedef void (*app_key_func)(char c);
typedef void (*app_backspace_func)();
typedef void (*app_enter_func)();

// Arrays mit Funktionszeigern, die aus app_list.def generiert werden
static app_init_func app_inits[] = {
#define APP(ID, NAME, HEADER, INIT, ...) INIT,
#include "app_list.def"
#undef APP
};

static app_draw_func app_draws[] = {
#define APP(ID, NAME, HEADER, INIT, DRAW, ...) DRAW,
#include "app_list.def"
#undef APP
};

static app_click_func app_clicks[] = {
#define APP(ID, NAME, HEADER, INIT, DRAW, CLICK, ...) CLICK,
#include "app_list.def"
#undef APP
};

static app_key_func app_keys[] = {
#define APP(ID, NAME, HEADER, INIT, DRAW, CLICK, KEY, ...) KEY,
#include "app_list.def"
#undef APP
};

static app_backspace_func app_backspaces[] = {
#define APP(ID, NAME, HEADER, INIT, DRAW, CLICK, KEY, BACKSPACE, ...) BACKSPACE,
#include "app_list.def"
#undef APP
};

static app_enter_func app_enters[] = {
#define APP(ID, NAME, HEADER, INIT, DRAW, CLICK, KEY, BACKSPACE, ENTER, ...) ENTER,
#include "app_list.def"
#undef APP
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

static int fast_sqrt(int n) {
    if (n < 0) return 0;
    if (n == 0) return 0;
    
    int x = n;
    int y = (x + 1) / 2;
    
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    
    return x;
}

static int fast_exp_neg(int x_times_100) {
    // e^(-x) approximation mit Taylor-Reihe
    // Für x zwischen 0 und 5: e^(-x) ≈ 1 - x + x²/2 - x³/6
    // Skaliert mit Faktor 100 für Integer-Mathematik
    
    if (x_times_100 <= 0) return 255;
    if (x_times_100 >= 500) return 0;
    
    int result = 255;
    int x = x_times_100;
    
    // 1. Term: -x
    result -= (x * 255) / 100;
    
    // 2. Term: +x²/2
    result += (x * x * 255) / (100 * 100 * 2);
    
    // 3. Term: -x³/6
    result -= (x * x * x * 255) / (100 * 100 * 100 * 6);
    
    if (result < 0) result = 0;
    if (result > 255) result = 255;
    
    return result;
}

static void save_cursor_background() {
    if (cursor_x < 0 || cursor_y < 0) return;
    
    int size = 32;
    int start_x = cursor_x - 16;
    int start_y = cursor_y - 16;
    
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    
    for (int py = 0; py < size && start_y + py < vbe_get_height(); py++) {
        for (int px = 0; px < size && start_x + px < vbe_get_width(); px++) {
            cursor_backup[py * 32 + px] = vbe_get_pixel(start_x + px, start_y + py);
        }
    }
    
    last_cursor_x = start_x;
    last_cursor_y = start_y;
    cursor_backup_valid = 1;
}

static void restore_cursor_background() {
    if (!cursor_backup_valid) return;
    
    int size = 32; // Muss mit save_cursor_background übereinstimmen
    
    for (int py = 0; py < size && last_cursor_y + py < vbe_get_height(); py++) {
        for (int px = 0; px < size && last_cursor_x + px < vbe_get_width(); px++) {
            vbe_set_pixel(last_cursor_x + px, last_cursor_y + py, 
                         cursor_backup[py * 32 + px]);
        }
    }
    
    cursor_backup_valid = 0;
}

static void mark_dirty(int x, int y, int w, int h) {
    if (dirty_count < 10) {
        dirty_regions[dirty_count].x = x;
        dirty_regions[dirty_count].y = y;
        dirty_regions[dirty_count].w = w;
        dirty_regions[dirty_count].h = h;
        dirty_regions[dirty_count].dirty = 1;
        dirty_count++;
    }
}

static void clear_dirty_regions() {
    dirty_count = 0;
}

static void redraw_dirty_regions() {
    for (int i = 0; i < dirty_count; i++) {
        DirtyRect* r = &dirty_regions[i];
        if (!r->dirty) continue;
        
        // Nur den geänderten Bereich neu zeichnen
        // (Hier müsstest du die Desktop/Window-Zeichnung anpassen)
        
        r->dirty = 0;
    }
}

void gui_handle_mouse_movement(int new_x, int new_y) {
    // 1. Alten Cursor entfernen (Hintergrund wiederherstellen)
    restore_cursor_background();
    
    // 2. Position aktualisieren
    cursor_x = new_x;
    cursor_y = new_y;
    
    // Begrenzung
    if (cursor_x < 0) cursor_x = 0;
    if (cursor_x >= vbe_get_width()) cursor_x = vbe_get_width() - 1;
    if (cursor_y < 0) cursor_y = 0;
    if (cursor_y >= vbe_get_height()) cursor_y = vbe_get_height() - 1;
    
    // 3. Cursor-Shape aktualisieren
    update_cursor_shape();
    
    // 4. Neuen Hintergrund sichern
    save_cursor_background();
    
    // 5. Cursor an neuer Position zeichnen
    draw_cursor();
    
    // WICHTIG: Kein komplettes Neuzeichnen!
}

void vbe_init_1080p() {
    // Initialisiere VBE (Versuche Full HD wenn möglich)
    // Hinweis: Die tatsächliche Modus-Umschaltung hängt vom VBE-Treiber ab.
    // Wir gehen davon aus, dass vbe_init() den besten verfügbaren Modus wählt
    // oder hier angepasst wird (z.B. vbe_set_mode(0x143) für 1920x1080).
    vbe_init();
}

static void draw_char_aa(int x, int y, char c, unsigned int color, int scale) {
    if (c < 0 || c > 127) return;
    
    // Verwende Super-Sampling für glattes Rendering
    for (int py = 0; py < 16 * scale; py++) {
        for (int px = 0; px < 8 * scale; px++) {
            // Sample mit Sub-Pixel-Genauigkeit
            int samples = 0;
            int total_samples = 4; // 2x2 Super-Sampling
            
            for (int sy = 0; sy < 2; sy++) {
                for (int sx = 0; sx < 2; sx++) {
                    float sample_y = (py + sy * 0.5f) / scale;
                    float sample_x = (px + sx * 0.5f) / scale;
                    
                    int font_y = (int)sample_y;
                    int font_x = (int)sample_x;
                    
                    if (font_y >= 0 && font_y < 16 && font_x >= 0 && font_x < 8) {
                        if (font_modern_8x16[(unsigned char)c][font_y] & (1 << (7 - font_x))) {
                             samples++;
                        }
                    }
                }
            }
            
            if (samples > 0) {
                int alpha = (samples * 255) / total_samples;
                unsigned int bg = vbe_get_pixel(x + px, y + py);
                vbe_set_pixel(x + px, y + py, blend_colors(bg, color, alpha));
            }
        }
    }
}


void draw_text_hd(int x, int y, const char* text, unsigned int color, int size) {
    int cx = x;
    while (*text) {
        draw_char_aa(cx, y, *text, color, size);
        cx += 8 * size + size; // Spacing basierend auf Skalierung
        text++;
    }
}

void draw_shadow(int x, int y, int w, int h) {
    int shadow_size = 18;
    int shadow_offset = 3;
    
    for (int i = 0; i < shadow_size; i++) {
        int t_times_100 = (i * 100) / shadow_size;
        int alpha = 80 - (t_times_100 * 80) / 100;
        if (alpha <= 5) continue;
        
        // Rechte Seite
        for (int py = y + shadow_offset + 15; py < y + h; py++) {
            int px = x + w + i;
            if (px >= vbe_get_width() || py >= vbe_get_height()) continue;
            
            int vertical_fade_times_100 = ((py - (y + shadow_offset + 15)) * 100) / (h - 15);
            if (vertical_fade_times_100 < 0) vertical_fade_times_100 = 0;
            int adjusted_alpha = (alpha * vertical_fade_times_100) / 100;
            
            if (adjusted_alpha > 5) {
                unsigned int bg = vbe_get_pixel(px, py);
                vbe_set_pixel(px, py, darken_color(bg, 255 - adjusted_alpha));
            }
        }
        
        // Untere Seite
        for (int px = x + shadow_offset + 15; px < x + w; px++) {
            int py = y + h + i;
            if (px >= vbe_get_width() || py >= vbe_get_height()) continue;
            
            int horizontal_fade_times_100 = ((px - (x + shadow_offset + 15)) * 100) / (w - 15);
            if (horizontal_fade_times_100 < 0) horizontal_fade_times_100 = 0;
            int adjusted_alpha = (alpha * horizontal_fade_times_100) / 100;
            
            if (adjusted_alpha > 5) {
                unsigned int bg = vbe_get_pixel(px, py);
                vbe_set_pixel(px, py, darken_color(bg, 255 - adjusted_alpha));
            }
        }
    }
    
    // Ecke
    for (int dy = 0; dy < shadow_size; dy++) {
        for (int dx = 0; dx < shadow_size; dx++) {
            if (dx < 3 || dy < 3) continue;
            
            int dist_x = dx + shadow_offset;
            int dist_y = dy + shadow_offset;
            int dist = fast_sqrt(dist_x * dist_x + dist_y * dist_y);
            
            if (dist < shadow_size) {
                int t_times_100 = (dist * 100) / shadow_size;
                int corner_alpha = 80 - (t_times_100 * 80) / 100;
                
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
// NEUE HD-VERSION (Multi-Layer, OHNE FLOAT!)
void draw_shadow_hd_fixed(int x, int y, int w, int h) {
    int shadow_size = 32; // Größerer Schatten für weicheren Look
    int shadow_offset = 6;
    
    // Multi-Layer Shadow (4 Schichten für extra Weichheit)
    for (int layer = 0; layer < 4; layer++) {
        int layer_size = shadow_size - (layer * 5);
        int layer_offset = shadow_offset + (layer * 1);
        
        for (int i = 0; i < layer_size; i++) {
            // Gaussian-ähnlicher Falloff (ohne exp!)
            int t_times_100 = (i * 100) / layer_size;
            int gauss = fast_exp_neg(t_times_100 * 5); // Approximierte Gaußkurve
            int alpha = (50 * gauss * (100 - layer * 20)) / (255 * 100); // Etwas transparenter
            
            if (alpha <= 3) continue;
            
            // Rechte Seite
            for (int py = y + layer_offset + 20; py < y + h; py++) {
                int px = x + w + i + layer_offset;
                if (px >= vbe_get_width() || py >= vbe_get_height()) continue;
                
                int v_fade = ((py - (y + layer_offset + 20)) * 100) / (h - 20);
                if (v_fade < 0) v_fade = 0;
                int adj_alpha = (alpha * v_fade) / 100;
                
                if (adj_alpha > 3) {
                    unsigned int bg = vbe_get_pixel(px, py);
                    vbe_set_pixel(px, py, darken_color(bg, 255 - adj_alpha));
                }
            }
            
            // Untere Seite
            for (int px = x + layer_offset + 20; px < x + w; px++) {
                int py = y + h + i + layer_offset;
                if (px >= vbe_get_width() || py >= vbe_get_height()) continue;
                
                int h_fade = ((px - (x + layer_offset + 20)) * 100) / (w - 20);
                if (h_fade < 0) h_fade = 0;
                int adj_alpha = (alpha * h_fade) / 100;
                
                if (adj_alpha > 3) {
                    unsigned int bg = vbe_get_pixel(px, py);
                    vbe_set_pixel(px, py, darken_color(bg, 255 - adj_alpha));
                }
            }
        }
        
        // Ecke (radial)
        for (int dy = 0; dy < layer_size; dy++) {
            for (int dx = 0; dx < layer_size; dx++) {
                if (dx < 5 || dy < 5) continue;
                
                int dist_x = dx + layer_offset;
                int dist_y = dy + layer_offset;
                int dist = fast_sqrt(dist_x * dist_x + dist_y * dist_y);
                
                if (dist < layer_size) {
                    int t_times_100 = (dist * 100) / layer_size;
                    int gauss = fast_exp_neg(t_times_100 * 5);
                    int corner_alpha = (60 * gauss * (100 - layer * 30)) / (255 * 100);
                    
                    if (corner_alpha > 3) {
                        int px = x + w + dx + layer_offset;
                        int py = y + h + dy + layer_offset;
                        
                        if (px < vbe_get_width() && py < vbe_get_height()) {
                            unsigned int bg = vbe_get_pixel(px, py);
                            vbe_set_pixel(px, py, darken_color(bg, 255 - corner_alpha));
                        }
                    }
                }
            }
        }
    }
}

#define CORNER_RADIUS 10

static void draw_aa_corner_hd_fixed(int cx, int cy, int r, unsigned int color, int quadrant) {
    // 4x4 Super-Sampling für ultra-glatte Ecken
    int r_limit_sq = (r * 8) * (r * 8); // Korrektur: Radius muss im Subpixel-Raum (x4) verdoppelt werden für (2*x+1) Logik
    
    for (int dy = 0; dy < r; dy++) {
        for (int dx = 0; dx < r; dx++) {
            int px = cx, py = cy;
            
            switch(quadrant) {
                case 0: px = cx - dx - 1; py = cy - dy - 1; break; // TL
                case 1: px = cx + dx;     py = cy - dy - 1; break; // TR
                case 2: px = cx - dx - 1; py = cy + dy;     break; // BL
                case 3: px = cx + dx;     py = cy + dy;     break; // BR
            }
            
            if (px < 0 || py < 0 || px >= vbe_get_width() || py >= vbe_get_height()) continue;
            
            int samples_inside = 0;
            
            // 4x4 Sub-Pixel Grid
            for (int sy = 0; sy < 4; sy++) {
                for (int sx = 0; sx < 4; sx++) {
                    // Mapping: Pixel (dx,dy) -> 4x4 Subpixels
                    // Center Check: (sub_x + 0.5)^2 + ...
                    // Integer Math: (sub_x*2 + 1)^2
                    int sub_x = dx * 4 + sx;
                    int sub_y = dy * 4 + sy;
                    
                    int dist_sq = (sub_x * 2 + 1) * (sub_x * 2 + 1) + (sub_y * 2 + 1) * (sub_y * 2 + 1);
                    
                    if (dist_sq < r_limit_sq) {
                        samples_inside++;
                    }
                }
            }
            
            int alpha = (samples_inside * 255) / 16;
            
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

void draw_window_modern(int x, int y, int w, int h, const char* title, int is_active) {
    // Shadow
    // draw_shadow(x, y, w, h); // Schatten entfernt
    
    int r = CORNER_RADIUS;
    unsigned int title_color = is_active ? 0x00404040 : 0x00353535;
    unsigned int body_color = 0x00282828;
    unsigned int border_color = 0x00555555;
    
    // Rahmen zeichnen
    vbe_fill_rect(x + r, y, w - 2 * r, 30, border_color);
    vbe_fill_rect(x, y + r, r, 30 - r, border_color);
    vbe_fill_rect(x + w - r, y + r, r, 30 - r, border_color);
    draw_aa_corner(x + r, y + r, r, border_color, 0);
    draw_aa_corner(x + w - r, y + r, r, border_color, 1);
    
    vbe_fill_rect(x, y + 30, w, h - 30 - r, border_color);
    vbe_fill_rect(x + r, y + h - r, w - 2 * r, r, border_color);
    draw_aa_corner(x + r, y + h - r, r, border_color, 2);
    draw_aa_corner(x + w - r, y + h - r, r, border_color, 3);
    
    // Inhalt zeichnen
    int inner_r = r - 1;
    
    vbe_fill_rect(x + 1 + inner_r, y + 1, w - 2 - 2 * inner_r, 29, title_color);
    vbe_fill_rect(x + 1, y + 1 + inner_r, inner_r, 29 - inner_r, title_color);
    vbe_fill_rect(x + w - 1 - inner_r, y + 1 + inner_r, inner_r, 29 - inner_r, title_color);
    draw_aa_corner(x + 1 + inner_r, y + 1 + inner_r, inner_r, title_color, 0);
    draw_aa_corner(x + w - 1 - inner_r, y + 1 + inner_r, inner_r, title_color, 1);
    
    vbe_fill_rect(x + 1, y + 30, w - 2, h - 30 - 1 - inner_r, body_color);
    vbe_fill_rect(x + 1 + inner_r, y + h - 1 - inner_r, w - 2 - 2 * inner_r, inner_r, body_color);
    draw_aa_corner(x + 1 + inner_r, y + h - 1 - inner_r, inner_r, body_color, 2);
    draw_aa_corner(x + w - 1 - inner_r, y + h - 1 - inner_r, inner_r, body_color, 3);
    
    vbe_fill_rect(x + 1, y + 30, w - 2, 1, 0x00000000);

    vbe_draw_text(x + 10, y + 7, title, 0xFFE0E0E0, VBE_TRANSPARENT);
    
    // Close Button (Red Dot)
    int btn_cx = x + w - 20;
    int btn_cy = y + 15;
    int btn_r = 8;
    
    draw_aa_corner(btn_cx, btn_cy, btn_r, 0x00FF5F56, 0);
    draw_aa_corner(btn_cx, btn_cy, btn_r, 0x00FF5F56, 1);
    draw_aa_corner(btn_cx, btn_cy, btn_r, 0x00FF5F56, 2);
    draw_aa_corner(btn_cx, btn_cy, btn_r, 0x00FF5F56, 3);
}

// NEUE HD-VERSION (mit verbesserten Effekten, OHNE FLOAT!)
void draw_window_modern_hd(int x, int y, int w, int h, const char* title, int is_active, int is_maximized) {
    if (is_maximized) {
        // Flacher Stil für maximierte Fenster (keine runden Ecken)
        unsigned int title_color = is_active ? 0x00454545 : 0x003A3A3A;
        unsigned int body_color = 0x002D2D2D;
        
        // Titlebar (Rechteckig)
        vbe_fill_rect(x, y, w, 35, title_color);
        // Body (Rechteckig)
        vbe_fill_rect(x, y + 35, w, h - 35, body_color);
        // Separator
        vbe_fill_rect(x, y + 35, w, 1, 0x00505050);
        
        vbe_draw_text(x + 14, y + 8, title, 0xFFF0F0F0, VBE_TRANSPARENT);
        
        // Buttons (Positionierung relativ zu x+w)
        int btn_cy = y + 17;
        int btn_r = 9;
        
        int btn_cx = x + w - 25;
        draw_aa_corner_hd_fixed(btn_cx, btn_cy, btn_r, 0x00FF5F56, 0);
        draw_aa_corner_hd_fixed(btn_cx, btn_cy, btn_r, 0x00FF5F56, 1);
        draw_aa_corner_hd_fixed(btn_cx, btn_cy, btn_r, 0x00FF5F56, 2);
        draw_aa_corner_hd_fixed(btn_cx, btn_cy, btn_r, 0x00FF5F56, 3);
        
        int max_cx = x + w - 50;
        draw_aa_corner_hd_fixed(max_cx, btn_cy, btn_r, 0x00FFBD2E, 0);
        draw_aa_corner_hd_fixed(max_cx, btn_cy, btn_r, 0x00FFBD2E, 1);
        draw_aa_corner_hd_fixed(max_cx, btn_cy, btn_r, 0x00FFBD2E, 2);
        draw_aa_corner_hd_fixed(max_cx, btn_cy, btn_r, 0x00FFBD2E, 3);
        
        return;
    }
    
    int r = 16; // Etwas runder für modernen Look
    unsigned int title_color = is_active ? 0x00454545 : 0x003A3A3A;
    unsigned int body_color = 0x002D2D2D;
    unsigned int border_color = 0x00606060;
    
    // Outer Border mit Gradient
    for (int i = 0; i < 2; i++) {
        unsigned int grad_color = blend_colors(border_color, 0x00FFFFFF, i * 8);
        
        vbe_fill_rect(x + r + i, y + i, w - 2 * r - 2*i, 35, grad_color);
        vbe_fill_rect(x + i, y + r + i, r - i, 35 - r + i, grad_color);
        vbe_fill_rect(x + w - r + i, y + r + i, r - i, 35 - r + i, grad_color);
        
        if (i == 0) {
            draw_aa_corner_hd_fixed(x + r, y + r, r, grad_color, 0);
            draw_aa_corner_hd_fixed(x + w - r, y + r, r, grad_color, 1);
        }
        
        vbe_fill_rect(x + i, y + 35 + i, w - 2*i, h - 35 - r - 2*i, grad_color);
        vbe_fill_rect(x + r + i, y + h - r + i, w - 2 * r - 2*i, r - i, grad_color);
        
        if (i == 0) {
            draw_aa_corner_hd_fixed(x + r, y + h - r, r, grad_color, 2);
            draw_aa_corner_hd_fixed(x + w - r, y + h - r, r, grad_color, 3);
        }
    }
    
    int inner_r = r - 2;
    
    // Titlebar mit Gradient (ohne Float!)
    for (int py = 0; py < 34; py++) {
        int t_times_100 = (py * 100) / 34;
        unsigned int grad = blend_colors(title_color, 0x00000000, (t_times_100 * 15) / 100);
        vbe_fill_rect(x + 2 + inner_r, y + 2 + py, w - 4 - 2 * inner_r, 1, grad);
    }
    vbe_fill_rect(x + 2, y + 2 + inner_r, inner_r, 34 - inner_r, title_color);
    vbe_fill_rect(x + w - 2 - inner_r, y + 2 + inner_r, inner_r, 34 - inner_r, title_color);
    draw_aa_corner_hd_fixed(x + 2 + inner_r, y + 2 + inner_r, inner_r, title_color, 0);
    draw_aa_corner_hd_fixed(x + w - 2 - inner_r, y + 2 + inner_r, inner_r, title_color, 1);
    
    // Body
    vbe_fill_rect(x + 2, y + 35, w - 4, h - 35 - 2 - inner_r, body_color);
    vbe_fill_rect(x + 2 + inner_r, y + h - 2 - inner_r, w - 4 - 2 * inner_r, inner_r, body_color);
    draw_aa_corner_hd_fixed(x + 2 + inner_r, y + h - 2 - inner_r, inner_r, body_color, 2);
    draw_aa_corner_hd_fixed(x + w - 2 - inner_r, y + h - 2 - inner_r, inner_r, body_color, 3);
    
    // Separator
    for (int i = 0; i < 2; i++) {
        unsigned int sep_color = (i == 0) ? 0x00000000 : 0x00505050;
        vbe_fill_rect(x + 2, y + 35 + i, w - 4, 1, sep_color);
    }

    vbe_draw_text(x + 14, y + 8, title, 0xFFF0F0F0, VBE_TRANSPARENT);
    
    // Close Button (Red Dot)
    int btn_cy = y + 17; // Zentriert in 35px Titlebar
    int btn_r = 9;
    
    int btn_cx = x + w - 25;
    
    draw_aa_corner_hd_fixed(btn_cx, btn_cy, btn_r, 0x00FF5F56, 0);
    draw_aa_corner_hd_fixed(btn_cx, btn_cy, btn_r, 0x00FF5F56, 1);
    draw_aa_corner_hd_fixed(btn_cx, btn_cy, btn_r, 0x00FF5F56, 2);
    draw_aa_corner_hd_fixed(btn_cx, btn_cy, btn_r, 0x00FF5F56, 3);
    
    // Maximize Button (Yellow/Orange Dot)
    int max_cx = x + w - 50;
    
    draw_aa_corner_hd_fixed(max_cx, btn_cy, btn_r, 0x00FFBD2E, 0);
    draw_aa_corner_hd_fixed(max_cx, btn_cy, btn_r, 0x00FFBD2E, 1);
    draw_aa_corner_hd_fixed(max_cx, btn_cy, btn_r, 0x00FFBD2E, 2);
    draw_aa_corner_hd_fixed(max_cx, btn_cy, btn_r, 0x00FFBD2E, 3);
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
        for (int i = 0; i < NUM_APPS; i++) {
            filtered_apps[filtered_count++] = i;
        }
    } else {
        // Convert search query to lowercase
        char query_lower[32];
        str_tolower(query_lower, search_query);
        
        // Filter apps
        for (int i = 0; i < NUM_APPS; i++) {
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

void draw_taskbar_search() {
    int spacing = 12;
    int total_w = 36 + spacing + 200;
    int start_x = (vbe_get_width() - total_w) / 2;
    int search_x = start_x + 36 + spacing;
    int search_y = 10; // Zentriert in 50px Taskbar
    int search_w = 200;
    int search_h = 30;
    int r = 8;
    
    // Background & Border
    unsigned int bg_color = 0x00252525;
    unsigned int border_color = 0x00555555;
    
    if (start_menu_open) {
        bg_color = 0x001F1F1F;
        border_color = WIN10_ACCENT;
    }
    
    fill_round_rect(search_x - 1, search_y - 1, search_w + 2, search_h + 2, r + 1, border_color);
    fill_round_rect(search_x, search_y, search_w, search_h, r, bg_color);
    
    // Text
    int text_y = search_y + 7;
    
    if (search_query_len > 0) {
        int text_visual_width = (search_query_len - 1) * 9 + 8;
        int text_x = search_x + (search_w - text_visual_width) / 2;
        draw_text_hd(text_x, text_y, search_query, VBE_WHITE, 1);
        
        if (start_menu_open && gui_get_caret_state()) {
            int cursor_pos_x = text_x + search_query_len * 9;
            vbe_fill_rect(cursor_pos_x, text_y, 2, 16, VBE_WHITE);
        }
    } else {
        const char* placeholder = "Search";
        int placeholder_len = strlen(placeholder);
        int text_visual_width = (placeholder_len > 0) ? ((placeholder_len - 1) * 9 + 8) : 0;
        int text_x = search_x + (search_w - text_visual_width) / 2;
        draw_text_hd(text_x, text_y, placeholder, 0xFF888888, 1);
    }
}

void draw_start_menu() {
    if (!start_menu_open) return;
    
    int spacing = 12;
    int total_w = 36 + spacing + 200;
    int menu_x = (vbe_get_width() - total_w) / 2;
    int menu_y = TASKBAR_HEIGHT + 5;
    int menu_w = 300;
    int header_h = 40;
    int item_h = 45;
    int menu_h = header_h + (filtered_count * item_h) + 10;
    
    if (menu_h < 50) menu_h = 50; // Minimum height
    
    // Shadow
    // draw_shadow(menu_x, menu_y, menu_w, menu_h); // Schatten entfernt
    
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
        draw_text_hd(menu_x + 15, header_y + 14, "All Apps", VBE_WHITE, 1);
    } else {
        draw_text_hd(menu_x + 15, header_y + 14, "No results", VBE_LIGHT_GRAY, 1);
    }
    
    // App list (filtered)
    for (int i = 0; i < filtered_count; i++) {
        int app_id = filtered_apps[i];
        int item_y = header_y + header_h + (i * item_h);
        
        if (i == start_menu_selected) {
            fill_round_rect(menu_x + 5, item_y + 2, menu_w - 10, 40, 10, 0x00555555);
            fill_round_rect(menu_x + 6, item_y + 3, menu_w - 12, 38, 9, WIN10_ACCENT);
        } else {
            fill_round_rect(menu_x + 5, item_y + 2, menu_w - 10, 40, 10, 0x00252525);
        }
        
        unsigned int icon_colors[NUM_APPS] = {
            0x000078D7, 0x00FFB900, 0x0000CC6A, 0x00FF8C00,
            0x00E74856, 0x009933CC, 0x00000000, 0xFFD19A66
        };
        fill_round_rect(menu_x + 15, item_y + 10, 24, 24, 5, icon_colors[app_id]);
        draw_text_hd(menu_x + 50, item_y + 14, app_names[app_id], VBE_WHITE, 1);
    }
}

static void update_cursor_shape() {
    cursor_type = 0; // Reset to default
    
    // 1. Taskbar Search Bar
    int spacing = 12;
    int total_w = 36 + spacing + 200;
    int start_x = (vbe_get_width() - total_w) / 2;
    int search_x = start_x + 36 + spacing;
    
    if (cursor_x >= search_x && cursor_x < search_x + 200 && cursor_y >= 10 && cursor_y < 40) {
        cursor_type = 1;
        return;
    }
    
    // Check Windows (iterate from top/front to bottom)
    for (int i = NUM_APPS - 1; i >= 0; i--) {
        int id = z_order[i];
        Window* w = &windows[id];
        
        if (!w->visible) continue;
        
        // Check if cursor is inside window
        if (cursor_x >= w->x && cursor_x < w->x + w->w &&
            cursor_y >= w->y && cursor_y < w->y + w->h) {
            
            int rel_x = cursor_x - w->x;
            int rel_y = cursor_y - w->y;
            
            // Stop checking other windows if we hit the top one
            return;
        }
    }
}

void draw_cursor() {
    if (!mouse_visible) return;
    if (cursor_type == 1) {
        // Text Cursor: White with Gray Border (I-Beam style)
        int h = 18;
        int w = 4;
        int start_y = cursor_y - 9;
        int start_x = cursor_x - 2;
        
        // Draw Gray Border
        vbe_fill_rect(start_x, start_y, w, h, 0xFF555555);
        
        // Draw White Inner Line
        vbe_fill_rect(start_x + 1, start_y + 1, w - 2, h - 2, 0xFFFFFFFF);
    } else {
        // Cleaner Circle Cursor (Anti-Aliased)
        int r_outer = 6;
        
        for (int dy = -r_outer; dy <= r_outer; dy++) {
            for (int dx = -r_outer; dx <= r_outer; dx++) {
                int dist_sq = dx*dx + dy*dy;
                int dist_10 = fast_sqrt(dist_sq * 100); // Fixed point x10
                
                if (dist_10 < 35) { // Radius 3.5 -> White Fill
                    vbe_set_pixel(cursor_x + dx, cursor_y + dy, 0xFFFFFFFF);
                } 
                else if (dist_10 < 48) { // Radius 4.8 -> Gray Border
                    vbe_set_pixel(cursor_x + dx, cursor_y + dy, 0xFF555555);
                }
                else if (dist_10 < 60) { // Radius 6.0 -> AA Edge
                    int alpha = 255 - ((dist_10 - 48) * 255) / 12;
                    if (alpha > 0) {
                        unsigned int bg = vbe_get_pixel(cursor_x + dx, cursor_y + dy);
                        vbe_set_pixel(cursor_x + dx, cursor_y + dy, blend_colors(bg, 0xFF555555, alpha));
                    }
                }
            }
        }
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
unsigned int get_total_memory() {
    return 128 * 1024; // 128 MB (Simulated, in KB)
}

unsigned int get_free_memory() {
    return 100 * 1024; // 100 MB (Simulated, in KB)
}

void gui_init() {
    vbe_init();
    vbe_clear(WIN10_BG);

    // Fensterpositionen aus app_list.def initialisieren
    #define APP(ID, NAME, HEADER, INIT, DRAW, CLICK, KEY, BACKSPACE, ENTER, W, H, X, Y) \
        windows[ID] = (Window){X, Y, W, H, 0, 0, 0, 0, 0, 0};
    #include "app_list.def"
    #undef APP

    fs_init(); // Initialize Filesystem
    ethernet_init();
    mouse_init();

    // Alle Apps automatisch initialisieren
    for (int i = 0; i < NUM_APPS; i++) {
        if (app_inits[i]) {
            app_inits[i]();
        }
    }
}

void gui_init_hd() {
    vbe_init_1080p();  // Hochauflösungsmodus setzen
    vbe_clear(WIN10_BG);

    // Fensterpositionen aus app_list.def initialisieren
    #define APP(ID, NAME, HEADER, INIT, DRAW, CLICK, KEY, BACKSPACE, ENTER, W, H, X, Y) \
        windows[ID] = (Window){X, Y, W, H, 0, 0, 0, 0, 0, 0};
    #include "app_list.def"
    #undef APP

    fs_init();
    ethernet_init();
    mouse_init();
    // Alle Apps automatisch initialisieren
    for (int i = 0; i < NUM_APPS; i++) {
        if (app_inits[i]) {
            app_inits[i]();
        }
    }
}

static unsigned int desktop_top_color = 0x001A2F4F;
static unsigned int desktop_bottom_color = 0x002C4A6F;

void gui_set_desktop_gradient(unsigned int top, unsigned int bottom) {
    desktop_top_color = top;
    desktop_bottom_color = bottom;
    // Force redraw
    gui_draw_desktop();
}

static void draw_desktop_gradient_rect(int x, int y, int w, int h) {
    // Clip to screen
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + w > vbe_get_width()) w = vbe_get_width() - x;
    if (y + h > vbe_get_height()) h = vbe_get_height() - y;
    
    int r1 = (desktop_top_color >> 16) & 0xFF;
    int g1 = (desktop_top_color >> 8) & 0xFF;
    int b1 = desktop_top_color & 0xFF;
    
    int r2 = (desktop_bottom_color >> 16) & 0xFF;
    int g2 = (desktop_bottom_color >> 8) & 0xFF;
    int b2 = desktop_bottom_color & 0xFF;
    
    for (int cy = y; cy < y + h; cy++) {
        float t = (float)cy / vbe_get_height();
        int r = r1 + (int)(t * (r2 - r1));
        int g = g1 + (int)(t * (g2 - g1));
        int b = b1 + (int)(t * (b2 - b1));
        unsigned int color = (r << 16) | (g << 8) | b;
        vbe_fill_rect(x, cy, w, 1, color);
    }
}

static void draw_taskbar_full() {
    int w = vbe_get_width();
    int h = TASKBAR_HEIGHT;
    int vh = vbe_get_height();

    // Acrylic Blur Effect (Windows-Style)
    // Wir simulieren "Frosted Glass" durch Sampling versetzter Pixel + Noise + Transparenz
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // 1. "Blur" simulieren durch Sampling versetzter Pixel (Refraktion)
            int sx = x + 3;
            int sy = y + 3;
            if (sx >= w) sx = w - 1;
            if (sy >= vh) sy = vh - 1;

            unsigned int bg = vbe_get_pixel(sx, sy);

            // 2. Noise (Körnung) hinzufügen
            int noise = ((x * 57 + y * 131) % 16) - 8;
            
            int r = (bg >> 16) & 0xFF;
            int g = (bg >> 8) & 0xFF;
            int b = bg & 0xFF;
            
            r += noise; if (r < 0) r = 0; if (r > 255) r = 255;
            g += noise; if (g < 0) g = 0; if (g > 255) g = 255;
            b += noise; if (b < 0) b = 0; if (b > 255) b = 255;
            
            unsigned int noisy_bg = (r << 16) | (g << 8) | b;

            // 3. Dunkler Tint mit Transparenz (Acrylic Overlay)
            // Alpha 190 (0-255) bedeutet ca. 75% Deckkraft des dunklen Tons -> Hintergrund scheint durch
            vbe_set_pixel(x, y, blend_colors(noisy_bg, 0x00151515, 190));
        }
    }
    
    // Separator Line (unten)
    vbe_fill_rect(0, h - 1, w, 1, 0x00333333);
    
    // ===== START BUTTON (Modern Box with 4 Circles) =====
    int spacing = 12;
    int total_w = 36 + spacing + 200;
    int btn_x = (vbe_get_width() - total_w) / 2;
    
    int btn_y = 10, btn_w = 36, btn_h = 30, btn_r = 8; // Zentriert
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
    draw_aa_corner_hd_fixed(cx - co, cy - co, cr, icon_color, 0);
    draw_aa_corner_hd_fixed(cx - co, cy - co, cr, icon_color, 1);
    draw_aa_corner_hd_fixed(cx - co, cy - co, cr, icon_color, 2);
    draw_aa_corner_hd_fixed(cx - co, cy - co, cr, icon_color, 3);

    // Top-Right Circle
    draw_aa_corner_hd_fixed(cx + co, cy - co, cr, icon_color, 0);
    draw_aa_corner_hd_fixed(cx + co, cy - co, cr, icon_color, 1);
    draw_aa_corner_hd_fixed(cx + co, cy - co, cr, icon_color, 2);
    draw_aa_corner_hd_fixed(cx + co, cy - co, cr, icon_color, 3);

    // Bottom-Left Circle
    draw_aa_corner_hd_fixed(cx - co, cy + co, cr, icon_color, 0);
    draw_aa_corner_hd_fixed(cx - co, cy + co, cr, icon_color, 1);
    draw_aa_corner_hd_fixed(cx - co, cy + co, cr, icon_color, 2);
    draw_aa_corner_hd_fixed(cx - co, cy + co, cr, icon_color, 3);

    // Bottom-Right Circle
    draw_aa_corner_hd_fixed(cx + co, cy + co, cr, icon_color, 0);
    draw_aa_corner_hd_fixed(cx + co, cy + co, cr, icon_color, 1);
    draw_aa_corner_hd_fixed(cx + co, cy + co, cr, icon_color, 2);
    draw_aa_corner_hd_fixed(cx + co, cy + co, cr, icon_color, 3);
    
    draw_taskbar_search();
    
    // Variables needed for layout below
    int search_x = btn_x + btn_w + spacing;
    int search_w = 200;
    
    // ===== TASKBAR APP INDICATORS (Clean rounded) =====
    int taskbar_x = search_x + search_w + 20;
    for (int i = 0; i < NUM_APPS; i++) {
        if (windows[i].visible) {
            int ind_r = 8;
            unsigned int indicator_color = (i == selected_window) ? WIN10_ACCENT : 0x002D2D2D;
            unsigned int ind_border = blend_colors(indicator_color, 0x00FFFFFF, 30);
            
            // Border
            fill_round_rect(taskbar_x - 1, 9, 62, 32, ind_r + 1, ind_border);
            // Main
            fill_round_rect(taskbar_x, 10, 60, 30, ind_r, indicator_color);
            
            // App initial
            char initial[2] = {app_names[i][0], '\0'};
            draw_text_hd(taskbar_x + 24, 18, initial, VBE_WHITE, 1);
            
            taskbar_x += 65;
        }
    }
    
    // ===== ETHERNET INDICATOR BOX (Clean rounded) =====
    // IP Address Display entfernt (Cleaner Look)

    int eth_w = 40, eth_h = 30, eth_r = 8;
    int clock_w = 80;
    int clock_x = vbe_get_width() - clock_w - 10;
    int eth_x = clock_x - eth_w - 10;
    
    unsigned int box_color = 0x00252525;
    unsigned int box_border = 0x00404040;
    
    // Border
    fill_round_rect(eth_x - 1, 9, eth_w + 2, eth_h + 2, eth_r + 1, box_border);
    // Background
    fill_round_rect(eth_x, 10, eth_w, eth_h, eth_r, box_color);
    
    // Status dot (perfect circle using AA corners)
    unsigned int eth_color = ethernet_is_connected() ? 0x0000FF00 : 0x00FF0000;
    int dot_x = eth_x + (eth_w / 2);
    int dot_y = 25;
    int dot_r = 6;
    
    draw_aa_corner_hd_fixed(dot_x, dot_y, dot_r, eth_color, 0);
    draw_aa_corner_hd_fixed(dot_x, dot_y, dot_r, eth_color, 1);
    draw_aa_corner_hd_fixed(dot_x, dot_y, dot_r, eth_color, 2);
    draw_aa_corner_hd_fixed(dot_x, dot_y, dot_r, eth_color, 3);
    
    // ===== CLOCK BOX (Clean rounded) =====
    // Border
    fill_round_rect(clock_x - 1, 9, clock_w + 2, eth_h + 2, eth_r + 1, box_border);
    // Background
    fill_round_rect(clock_x, 10, clock_w, eth_h, eth_r, box_color);
    
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
    
    draw_text_hd(clock_x + 20, 13, time_str, VBE_WHITE, 1);
    
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
    
    draw_tiny_text(clock_x + 15, 31, date_str, 0xFFCCCCCC);
}

void gui_draw_desktop() {
    draw_desktop_gradient_rect(0, 0, vbe_get_width(), vbe_get_height());
    draw_taskbar_full();
}

void gui_draw_window(int id) {
    Window* w = &windows[id];
    if (!w->visible) return;
    
    int is_active = (id == selected_window);

    // Generischen Fensterrahmen zeichnen
    draw_window_modern_hd(w->x, w->y, w->w, w->h, app_names[id], is_active, w->is_maximized);
    save_corners(w->x, w->y, w->w, w->h);

    // App-spezifische Zeichenfunktion aufrufen, falls vorhanden
    if (app_draws[id]) {
        // Der Inhaltsbereich befindet sich unter der 35px hohen Titelleiste
        app_draws[id](w->x, w->y + 35, w->w, w->h - 35, is_active);
    }

    restore_corners(w->x, w->y, w->w, w->h);
}

static int check_intersection(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    return !(x2 >= x1 + w1 || x2 + w2 <= x1 || y2 >= y1 + h1 || y2 + h2 <= y1);
}

void gui_redraw_region(int x, int y, int w, int h) {
    // 1. Hintergrund in diesem Bereich wiederherstellen
    draw_desktop_gradient_rect(x, y, w, h);
    
    // 2. Taskbar neu zeichnen, falls betroffen
    if (y < TASKBAR_HEIGHT) {
        draw_taskbar_full();
    }
    
    // 3. Alle Fenster zeichnen, die diesen Bereich berühren
    for (int i = 0; i < NUM_APPS; i++) {
        int id = z_order[i];
        if (windows[id].visible) {
            Window* win = &windows[id];
            // Prüfen ob Fenster den Dirty-Bereich berührt
            if (check_intersection(x, y, w, h, win->x, win->y, win->w, win->h)) {
                gui_draw_window(id);
            }
        }
    }
}

void gui_draw_all_windows() {
    for (int i = 0; i < NUM_APPS; i++) {
        int id = z_order[i];
        if (windows[id].visible) {
            gui_draw_window(id);
        }
    }
}

static void bring_to_front(int id) {
    int found_idx = -1;
    for (int i = 0; i < NUM_APPS; i++) {
        if (z_order[i] == id) {
            found_idx = i;
            break;
        }
    }
    
    if (found_idx != -1) {
        for (int i = found_idx; i < NUM_APPS - 1; i++) {
            z_order[i] = z_order[i + 1];
        }
        z_order[NUM_APPS - 1] = id;
    }
}

static int check_window_click(int mx, int my) {
    for (int i = NUM_APPS - 1; i >= 0; i--) {
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
    
    if (mx >= btn_x && mx < btn_x + 36 && my >= 10 && my < 40) {
        return 1;
    }
    return 0;
}

static int check_start_menu_click(int mx, int my) {
    if (!start_menu_open) return -1;
    
    int spacing = 12;
    int total_w = 36 + spacing + 200;
    int menu_x = (vbe_get_width() - total_w) / 2;
    
    int menu_y = TASKBAR_HEIGHT + 5;
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
    
    if (mx >= search_x && mx < search_x + 200 && my >= 10 && my < 40) {
        return 1;
    }
    return 0;
}

static int check_ethernet_click(int mx, int my) {
    int eth_w = 40;
    int clock_w = 80;
    int clock_x = vbe_get_width() - clock_w - 10;
    int eth_x = clock_x - eth_w - 10;
    
    if (mx >= eth_x && mx < eth_x + eth_w && my >= 10 && my < 40) {
        return 1;
    }
    return 0;
}

static int check_close_button(int mx, int my, int id) {
    Window* w = &windows[id];
    
    // Hitbox für den neuen Close-Button (Red Dot)
    // Button ist bei x+w-25, Radius 9 -> Bereich ca. x+w-40 bis x+w-10
    if (mx >= w->x + w->w - 45 && mx < w->x + w->w - 5 &&
        my >= w->y && my < w->y + 35) {
        return 1;
    }
    return 0;
}

static int check_maximize_button(int mx, int my, int id) {
    Window* w = &windows[id];
    
    // Hitbox für Maximize (Yellow Dot)
    // Button ist bei x+w-50, Radius 9 -> Bereich ca. x+w-65 bis x+w-35
    if (mx >= w->x + w->w - 70 && mx < w->x + w->w - 30 &&
        my >= w->y && my < w->y + 35) {
        return 1;
    }
    return 0;
}

static void toggle_maximize(int id) {
    Window* w = &windows[id];
    if (w->is_maximized) {
        w->x = w->prev_x;
        w->y = w->prev_y;
        w->w = w->prev_w;
        w->h = w->prev_h;
        w->is_maximized = 0;
    } else {
        w->prev_x = w->x;
        w->prev_y = w->y;
        w->prev_w = w->w;
        w->prev_h = w->h;
        w->x = 0;
        w->y = TASKBAR_HEIGHT;
        w->w = vbe_get_width();
        w->h = vbe_get_height() - TASKBAR_HEIGHT;
        w->is_maximized = 1;
    }
}

void gui_close_window(int id) {
    if (id >= 0 && id < NUM_APPS) {
        windows[id].visible = 0;
    }
}

void gui_open_window(int id) {
    if (id >= 0 && id < NUM_APPS) {
        windows[id].visible = 1;
        selected_window = id;
        bring_to_front(id);
    }
}

int gui_is_window_visible(int id) {
    if (id >= 0 && id < NUM_APPS) {
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
    else if (check_ethernet_click(x, y)) {
        gui_open_window(7); // Settings App öffnen
    }
    else {
        clicked = check_window_click(x, y);
        if (clicked != -1) {
            selected_window = clicked;
            bring_to_front(clicked);
            
            Window* w = &windows[clicked];
            if (check_close_button(x, y, clicked)) {
                gui_close_window(clicked);
            } else if (check_maximize_button(x, y, clicked)) {
                toggle_maximize(clicked);
                // needs_full_redraw wird in gui_run gesetzt, da handle_left_click void ist
            } else {
                // Klick an den App-Inhalt weiterleiten
                int header_h = 35; // Höhe der Titelleiste
                int rel_x = x - w->x;
                int rel_y = y - w->y - header_h;
                if (app_clicks[clicked]) {
                    app_clicks[clicked](rel_x, rel_y);
                }
            }
        }
    }
}

static void wait_for_vsync() {
    // Warten auf VSync für stabile 60Hz (VGA Port 0x3DA, Bit 3)
    while (inb(0x3DA) & 8);
    while (!(inb(0x3DA) & 8));
}

void gui_run() {
    gui_init_hd();
    gui_draw_desktop();
    gui_draw_all_windows();
    
    // WICHTIG: Initial Cursor-Backup NACH dem ersten Zeichnen!
    save_cursor_background();
    draw_cursor();
    
    int move_step = 15;
    int blink_timer = 0;
    int dhcp_retry_timer = 0;
    int update_search = 0;
    
    while (1) {
        int needs_full_redraw = 0; // Nur bei Klicks/wichtigen Events
        
        // --- NETZWERK POLLING ---
        EthernetFrame eth_frame;
        while (ethernet_recv_frame(&eth_frame) == 0) {
            net_handle_packet((unsigned char*)&eth_frame, eth_frame.length + 14);
        }

        // --- MAUS TREIBER (OPTIMIERT!) ---
        int m_dx = 0, m_dy = 0, m_buttons = 0;
        
        if (mouse_read_packet(&m_dx, &m_dy, &m_buttons)) {
            if (m_dx > -100 && m_dx < 100 && m_dy > -100 && m_dy < 100) {
                int new_x = cursor_x + m_dx;
                int new_y = cursor_y + m_dy;
                
                // Verwende optimierte Funktion (KEIN needs_redraw!)
                gui_handle_mouse_movement(new_x, new_y);
                mouse_visible = 1;
                
                // Dragging Logic
                if (is_dragging && drag_window_id != -1) {
                    Window* w = &windows[drag_window_id];
                    
                    // Cursor kurz ausblenden, um Artefakte zu vermeiden
                    restore_cursor_background();
                    
                    int old_x = w->x;
                    int old_y = w->y;
                    
                    w->x = cursor_x - drag_off_x;
                    w->y = cursor_y - drag_off_y;
                    
                    // Dirty Rectangle berechnen (Union aus alter und neuer Position)
                    int min_x = (old_x < w->x) ? old_x : w->x;
                    int min_y = (old_y < w->y) ? old_y : w->y;
                    int max_x = (old_x + w->w > w->x + w->w) ? old_x + w->w : w->x + w->w;
                    int max_y = (old_y + w->h > w->y + w->h) ? old_y + w->h : w->y + w->h;
                    
                    gui_redraw_region(min_x, min_y, max_x - min_x + 5, max_y - min_y + 5);
                    
                    save_cursor_background();
                    draw_cursor();
                }
                
                // Klick-Behandlung
                static int prev_left = 0;
                if ((m_buttons & 1) && !prev_left) {
                    int old_menu = start_menu_open;
                    handle_left_click(cursor_x, cursor_y);
                    
                    // Check if maximize triggered a redraw need (hacky but works for now)
                    if (selected_window >= 0 && check_maximize_button(cursor_x, cursor_y, selected_window)) {
                        needs_full_redraw = 1;
                    }
                    
                    // Check Dragging Start (wenn nicht maximiert und in Titlebar)
                    if (selected_window >= 0 && windows[selected_window].visible && !windows[selected_window].is_maximized) {
                        Window* w = &windows[selected_window];
                        // Titlebar Hitbox (ca. 35px Höhe)
                        if (cursor_x >= w->x && cursor_x < w->x + w->w &&
                            cursor_y >= w->y && cursor_y < w->y + 35) {
                            // Nicht auf Buttons klicken
                            if (!check_close_button(cursor_x, cursor_y, selected_window) &&
                                !check_maximize_button(cursor_x, cursor_y, selected_window)) {
                                is_dragging = 1;
                                drag_window_id = selected_window;
                                drag_off_x = cursor_x - w->x;
                                drag_off_y = cursor_y - w->y;
                            }
                        }
                    }
                    
                    if (start_menu_open && !old_menu) {
                        update_start_menu = 1; // Menü geöffnet -> Teil-Update (kein Flackern)
                    } else if (!start_menu_open && old_menu) {
                        needs_full_redraw = 1; // Menü geschlossen -> Voll-Update (Hintergrund wiederherstellen)
                    } else if (start_menu_open && check_search_bar_click(cursor_x, cursor_y)) {
                        update_start_menu = 1; // Klick in Suche -> Teil-Update
                    } else {
                        needs_full_redraw = 1; // Sonstige Klicks -> Voll-Update
                    }
                }
                else if (!(m_buttons & 1) && prev_left) {
                    is_dragging = 0;
                    drag_window_id = -1;
                }
                prev_left = (m_buttons & 1);
            }
        }
        
        // --- DHCP RETRY LOGIC ---
        dhcp_retry_timer++;
        if (dhcp_retry_timer > 200) {
            dhcp_retry_timer = 0;
            IP_Address my_ip = ethernet_get_ip();
            if (my_ip.addr[0] == 0) {
                net_send_dhcp_discover();
            }
        }
        
        // --- BLINK TIMER ---
        blink_timer++;
        if (blink_timer >= 30) {
            blink_timer = 0;
            caret_state = !caret_state;
            
            // Nur betroffene Bereiche als "dirty" markieren
            if (start_menu_open) {
                int spacing = 12;
                int total_w = 36 + spacing + 200;
                int start_x = (vbe_get_width() - total_w) / 2;
                int search_x = start_x + 36 + spacing;
                mark_dirty(search_x, 5, 200, 30);
                update_search = 1; // Nur Suche neu zeichnen
            }
        }

        // --- KEYBOARD HANDLING ---
        unsigned char sc = 0;
        unsigned char kbd_status = inb(MOUSE_PORT_STATUS);

        if ((kbd_status & 0x01) && !(kbd_status & 0x20)) {
            sc = keyboard_read_nonblock();
        }

        if (sc == 0x2A || sc == 0x36) shift_pressed = 1;
        else if (sc == 0xAA || sc == 0xB6) shift_pressed = 0;
        
        if (sc && !(sc & 0x80)) {
            blink_timer = 0;
            caret_state = 1;
            
            if (sc == 0x01) { // ESC
                if (start_menu_open) {
                    start_menu_open = 0;
                    needs_full_redraw = 1; // ← needs_full_redraw statt needs_redraw!
                } else {
                    return;
                }
            }
            
            else if (sc == 0x0F) { // TAB
                if (start_menu_open) {
                    start_menu_open = 0;
                    needs_full_redraw = 1;
                } else {
                    int start = selected_window;
                    do {
                        selected_window = (selected_window + 1) % NUM_APPS;
                        if (windows[selected_window].visible) {
                            bring_to_front(selected_window);
                            needs_full_redraw = 1;
                            break;
                        }
                    } while (selected_window != start);
                }
            }
            
            else if (sc == 0x48) { // Up arrow
                if (start_menu_open) {
                    if (filtered_count > 0) {
                        start_menu_selected = (start_menu_selected - 1 + filtered_count) % filtered_count;
                    }
                    needs_full_redraw = 1;
                } else {
                    // Cursor-Bewegung per Tastatur
                    restore_cursor_background();
                    cursor_y -= move_step;
                    if (cursor_y < 0) cursor_y = 0;
                    update_cursor_shape();
                    save_cursor_background();
                    draw_cursor();
                    mouse_visible = 1;
                    // KEIN needs_full_redraw!
                }
            }
            else if (sc == 0x50) { // Down arrow
                if (start_menu_open) {
                    if (filtered_count > 0) {
                        start_menu_selected = (start_menu_selected + 1) % filtered_count;
                    }
                    needs_full_redraw = 1;
                } else {
                    restore_cursor_background();
                    cursor_y += move_step;
                    if (cursor_y >= vbe_get_height()) cursor_y = vbe_get_height() - 1;
                    update_cursor_shape();
                    save_cursor_background();
                    draw_cursor();
                    mouse_visible = 1;
                }
            }
            else if (sc == 0x4B) { // Left arrow
                restore_cursor_background();
                cursor_x -= move_step;
                if (cursor_x < 0) cursor_x = 0;
                update_cursor_shape();
                save_cursor_background();
                draw_cursor();
                mouse_visible = 1;
            }
            else if (sc == 0x4D) { // Right arrow
                restore_cursor_background();
                cursor_x += move_step;
                if (cursor_x >= vbe_get_width()) cursor_x = vbe_get_width() - 1;
                update_cursor_shape();
                save_cursor_background();
                draw_cursor();
                mouse_visible = 1;
            }
            
            else if (sc == 0x0E) { // Backspace
                mouse_visible = 0;
                if (start_menu_open && search_query_len > 0) {
                    search_query_len--;
                    search_query[search_query_len] = '\0';
                    update_filtered_apps();
                    update_start_menu = 1; // Nur Menü neu zeichnen
                } else if (selected_window != -1 && windows[selected_window].visible && app_backspaces[selected_window]) {
                    app_backspaces[selected_window]();
                    needs_full_redraw = 1;
                }
            }
            
            else if (sc == 0x1C) { // Enter
                int clicked;
                if (start_menu_open) {
                    if ((clicked = check_start_menu_click(cursor_x, cursor_y)) >= 0) {
                        gui_open_window(clicked);
                        start_menu_open = 0;
                        search_query_len = 0;
                        search_query[0] = '\0';
                        needs_full_redraw = 1;
                    }
                    else if (filtered_count > 0 && start_menu_selected >= 0) {
                        gui_open_window(filtered_apps[start_menu_selected]);
                        start_menu_open = 0;
                        search_query_len = 0;
                        search_query[0] = '\0';
                        needs_full_redraw = 1;
                    }
                    else if (check_search_bar_click(cursor_x, cursor_y)) {
                        mouse_visible = 0;
                        needs_full_redraw = 1;
                    }
                    else {
                        start_menu_open = 0;
                        needs_full_redraw = 1;
                    }
                }
                else if (check_start_button_click(cursor_x, cursor_y)) {
                    start_menu_open = !start_menu_open;
                    start_menu_selected = 0;
                    search_query_len = 0;
                    search_query[0] = '\0';
                    update_filtered_apps();
                    needs_full_redraw = 1;
                }
                else if (check_search_bar_click(cursor_x, cursor_y)) {
                    start_menu_open = 1;
                    start_menu_selected = 0;
                    search_query_len = 0;
                    search_query[0] = '\0';
                    update_filtered_apps();
                    mouse_visible = 0;
                    needs_full_redraw = 1;
                }
                else {
                    int handled_as_text = 0;
                    Window* w = &windows[selected_window];
                    int in_title_bar = (cursor_y >= w->y && cursor_y < w->y + 35 && 
                                       cursor_x >= w->x && cursor_x < w->x + w->w);

                    if (!in_title_bar) {
                        if (selected_window != -1 && windows[selected_window].visible) {
                            if (app_enters[selected_window]) {
                                app_enters[selected_window]();
                                handled_as_text = 1;
                            } else if (app_keys[selected_window]) {
                                app_keys[selected_window]('\n'); // Fallback auf Key-Handler mit Newline
                                handled_as_text = 1;
                            }
                        }
                    }

                    if (handled_as_text) {
                        mouse_visible = 0;
                        needs_full_redraw = 1;
                    }
                    else {
                        clicked = check_window_click(cursor_x, cursor_y);
                        if (clicked != -1) {
                            selected_window = clicked;
                            bring_to_front(clicked);
                            
                            if (check_close_button(cursor_x, cursor_y, clicked)) {
                                gui_close_window(clicked);
                                needs_full_redraw = 1;
                            } else if (check_maximize_button(cursor_x, cursor_y, clicked)) {
                                toggle_maximize(clicked);
                                needs_full_redraw = 1;
                            }
                            else {
                                int header_h = 35;
                                int rel_x = cursor_x - w->x;
                                int rel_y = cursor_y - w->y - header_h;
                                if (app_clicks[clicked]) {
                                    app_clicks[clicked](rel_x, rel_y);
                                }
                                needs_full_redraw = 1;
                            }
                        }
                    }
                }
            }
            
            else if (sc == 0x39) { // Spacebar
                mouse_visible = 0;
                if (start_menu_open && search_query_len < 31) {
                    search_query[search_query_len++] = ' ';
                    search_query[search_query_len] = '\0';
                    update_filtered_apps();
                    update_start_menu = 1;
                } else if (selected_window != -1 && windows[selected_window].visible && app_keys[selected_window]) {
                    app_keys[selected_window](' ');
                    needs_full_redraw = 1;
                }
            }
            
            else { // Andere Zeichen
                char ascii = scancode_to_ascii(sc);
                
                if (shift_pressed) {
                    if (ascii >= 'a' && ascii <= 'z') ascii -= 32;
                    else if (ascii == '1') ascii = '!';
                    else if (ascii == '2') ascii = '"';
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
                        search_query[search_query_len++] = ascii;
                        search_query[search_query_len] = '\0';
                        update_filtered_apps();
                        update_start_menu = 1; // Nur Menü neu zeichnen
                    } else if (selected_window != -1 && windows[selected_window].visible && app_keys[selected_window]) {
                        app_keys[selected_window](ascii);
                        needs_full_redraw = 1;
                    }
                }
            }
        }

        // --- REDRAW LOGIC (OPTIMIERT!) ---
        if (needs_full_redraw) {
            restore_cursor_background();
            
            gui_draw_desktop();
            gui_draw_all_windows();
            draw_start_menu();
            
            save_cursor_background();
            draw_cursor();
            
            clear_dirty_regions();
        }
        else if (update_start_menu) {
            restore_cursor_background();
            draw_taskbar_search(); // Suchleiste aktualisieren
            draw_start_menu();     // Menü darüber zeichnen
            save_cursor_background();
            draw_cursor();
            update_start_menu = 0;
        }
        else if (update_search) {
            restore_cursor_background();
            draw_taskbar_search();
            save_cursor_background();
            draw_cursor();
            update_search = 0;
        }
        else if (dirty_count > 0) {
            restore_cursor_background();
            redraw_dirty_regions();
            save_cursor_background();
            draw_cursor();
        }
        
        // Sync to 60Hz
        wait_for_vsync();
    }
}

// Compatibility stubs
void draw_box(int x, int y, int w, int h, unsigned char color) {}
void draw_text_at(int x, int y, const char* text, unsigned char color) {}
void draw_button(int x, int y, const char* text, unsigned char color) {}
int check_button_click(int cx, int cy, int bx, int by, const char* text) { return 0; }