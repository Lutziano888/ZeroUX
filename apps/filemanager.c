#include "filemanager.h"
#include "../vbe.h"
#include "../fs.h"
#include "../gui.h"
#include "../string.h"

static int fm_current_dir = 0;
static int selected_file_idx = -1;
static int hover_file_idx = -1;
static int scroll_offset = 0;

void filemanager_init() {
    fm_current_dir = 0;
    selected_file_idx = -1;
    hover_file_idx = -1;
    scroll_offset = 0;
}

// Dark Mode Farbpalette (Ubuntu-inspiriert)
#define COLOR_BG           0xFF2B2B2B
#define COLOR_SIDEBAR      0xFF1E1E1E
#define COLOR_TOOLBAR      0xFF323232
#define COLOR_ACCENT       0xFFFF6B35
#define COLOR_FOLDER       0xFFFFA726
#define COLOR_APP          0xFF42A5F5
#define COLOR_FILE         0xFF9E9E9E
#define COLOR_TEXT         0xFFE0E0E0
#define COLOR_TEXT_DIM     0xFF888888
#define COLOR_BORDER       0xFF404040
#define COLOR_HOVER        0xFF383838
#define COLOR_SELECTED     0xFF4A4A4A
#define COLOR_SCROLLBAR    0xFF505050
#define COLOR_SCROLL_THUMB 0xFF707070

// Helper für Alpha Blending
static unsigned int blend_colors(unsigned int bg, unsigned int fg, int alpha) {
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

// Integer Square Root
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

// Anti-Aliased Corner
// quadrant: 0=TopLeft, 1=TopRight, 2=BottomLeft, 3=BottomRight
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
            } else if (d2 >= r_sq) {
                alpha = 0;
            } else {
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

// Helper für AA Ecken eines Rechtecks
static void draw_aa_corners_rect(int x, int y, int w, int h, int r, unsigned int color, int top, int bottom) {
    if (top) {
        draw_aa_corner(x + r, y + r, r, color, 0);
        draw_aa_corner(x + w - r, y + r, r, color, 1);
    }
    if (bottom) {
        draw_aa_corner(x + r, y + h - r, r, color, 2);
        draw_aa_corner(x + w - r, y + h - r, r, color, 3);
    }
}

// Abgerundete Rechtecke mit AA
static void fill_rounded_rect(int x, int y, int w, int h, int radius, unsigned int color) {
    // Hauptrechteck
    vbe_fill_rect(x + radius, y, w - 2 * radius, h, color);
    vbe_fill_rect(x, y + radius, radius, h - 2 * radius, color);
    vbe_fill_rect(x + w - radius, y + radius, radius, h - 2 * radius, color);
    
    // AA Ecken
    draw_aa_corners_rect(x, y, w, h, radius, color, 1, 1);
}

static void draw_rounded_rect(int x, int y, int w, int h, int radius, unsigned int color) {
    // Linien
    vbe_fill_rect(x + radius, y, w - 2 * radius, 1, color);
    vbe_fill_rect(x + radius, y + h - 1, w - 2 * radius, 1, color);
    vbe_fill_rect(x, y + radius, 1, h - 2 * radius, color);
    vbe_fill_rect(x + w - 1, y + radius, 1, h - 2 * radius, color);
}

// Externe Funktionen für Speicher-Info
extern unsigned int get_total_memory();
extern unsigned int get_free_memory();

// Ubuntu-Style Sidebar
static void draw_sidebar(int x, int y, int h, file_t* files) {
    // Sidebar Hintergrund
    vbe_fill_rect(x, y, 200, h, COLOR_SIDEBAR);
    vbe_fill_rect(x + 199, y, 1, h, COLOR_BORDER);
    
    int item_y = y + 20;
    int item_height = 40;
    
    // Sidebar Items (nur "Home" ist klickbar/aktiv)
    const char* items[] = {"Home", "Documents", "Downloads", "Pictures", "Music"};
    const char* icons[] = {"H", "D", "v", "P", "M"};
    
    for (int i = 0; i < 5; i++) {
        int is_active = (i == 0 && fm_current_dir == 0);
        unsigned int bg = is_active ? COLOR_SELECTED : COLOR_SIDEBAR;
        unsigned int fg = is_active ? COLOR_ACCENT : COLOR_TEXT;
        
        // Hover-Effekt (außer für Home wenn aktiv)
        if (!is_active) {
            // Hier könnte man Hover-Detection einbauen
        }
        
        // Item Hintergrund (abgerundet)
        if (is_active) {
            fill_rounded_rect(x + 8, item_y, 184, item_height - 4, 8, bg);
        }
        
        // Icon (abgerundet mit AA)
        fill_rounded_rect(x + 16, item_y + 6, 28, 28, 8, COLOR_HOVER);
        vbe_draw_text(x + 26, item_y + 13, icons[i], fg, VBE_TRANSPARENT);
        
        // Text
        vbe_draw_text(x + 54, item_y + 12, items[i], fg, VBE_TRANSPARENT);
        
        item_y += item_height;
    }
    
    // Separator
    vbe_fill_rect(x + 16, item_y + 10, 168, 1, COLOR_BORDER);
    
    // Storage Info (echte Werte)
    item_y += 30;
    vbe_draw_text(x + 16, item_y, "Storage", COLOR_TEXT_DIM, VBE_TRANSPARENT);
    
    item_y += 20;
    
    // Speicher-Berechnung (in KB von BIOS, konvertieren zu MB)
    unsigned int total_kb = get_total_memory(); // in KB vom BIOS
    unsigned int free_kb = get_free_memory();   // in KB
    
    // Konvertiere zu MB (sicherer für Anzeige)
    unsigned int total_mem = total_kb / 1024;   // KB -> MB
    unsigned int free_mem = free_kb / 1024;     // KB -> MB
    unsigned int used_mem = total_mem - free_mem;
    
    // Sicherheitscheck: Wenn Werte unrealistisch sind, Fallback
    if (total_mem > 65536 || total_mem == 0) {
        // Unrealistisch (>64GB) oder 0 -> Fallback auf simulierte Werte
        total_mem = 8192;  // 8 GB
        used_mem = 3072;   // 3 GB belegt
    }
    
    // Storage Bar (proportional)
    int bar_width = 168;
    int used_width = 0;
    if (total_mem > 0) {
        used_width = (used_mem * bar_width) / total_mem;
        if (used_width > bar_width) used_width = bar_width;
    }
    
    // Background Bar
    fill_rounded_rect(x + 16, item_y, bar_width, 8, 4, COLOR_HOVER);
    // Used Bar
    if (used_width > 0) {
        fill_rounded_rect(x + 16, item_y, used_width, 8, 4, COLOR_ACCENT);
    }
    
    item_y += 18;
    
    // Text formatieren "XX MB / YY MB" oder "X.X GB / Y.Y GB"
    char storage_text[32];
    if (total_mem < 1024) {
        // MB anzeigen
        char used_str[8], total_str[8];
        int_to_str(used_mem, used_str);
        int_to_str(total_mem, total_str);
        strcpy(storage_text, used_str);
        strcat(storage_text, " MB / ");
        strcat(storage_text, total_str);
        strcat(storage_text, " MB");
    } else {
        // GB anzeigen (mit .x Nachkommastelle)
        int used_gb = used_mem / 1024;
        int used_dec = ((used_mem % 1024) * 10) / 1024;
        int total_gb = total_mem / 1024;
        int total_dec = ((total_mem % 1024) * 10) / 1024;
        
        char temp[8];
        int_to_str(used_gb, storage_text);
        strcat(storage_text, ".");
        temp[0] = '0' + used_dec;
        temp[1] = 0;
        strcat(storage_text, temp);
        strcat(storage_text, " GB / ");
        
        int_to_str(total_gb, temp);
        strcat(storage_text, temp);
        strcat(storage_text, ".");
        temp[0] = '0' + total_dec;
        temp[1] = 0;
        strcat(storage_text, temp);
        strcat(storage_text, " GB");
    }
    
    vbe_draw_text(x + 16, item_y, storage_text, COLOR_TEXT_DIM, VBE_TRANSPARENT);
}

// Scrollbar (Ubuntu-Style)
static void draw_scrollbar(int x, int y, int h, int content_height, int visible_height) {
    if (content_height <= visible_height) return;
    
    // Scrollbar Hintergrund
    fill_rounded_rect(x, y, 12, h, 6, COLOR_SCROLLBAR);
    
    // Thumb berechnen
    float ratio = (float)visible_height / content_height;
    int thumb_height = (int)(h * ratio);
    if (thumb_height < 40) thumb_height = 40;
    
    float scroll_ratio = (float)scroll_offset / (content_height - visible_height);
    int thumb_y = y + (int)((h - thumb_height) * scroll_ratio);
    
    // Thumb zeichnen
    fill_rounded_rect(x + 2, thumb_y, 8, thumb_height, 4, COLOR_SCROLL_THUMB);
}

// Modernes Icon mit Fluent Design (Dark Mode, cleanere AA Rundungen)
static void draw_modern_icon(int x, int y, int type, const char* name, int is_selected, int is_hover) {
    unsigned int icon_color = COLOR_FILE;
    
    // Farbe basierend auf Typ
    if (type == FILE_TYPE_DIR) icon_color = COLOR_FOLDER;
    else if (type == FILE_TYPE_APP) icon_color = COLOR_APP;
    
    // Hintergrund bei Hover/Selection (abgerundet mit AA)
    if (is_selected) {
        fill_rounded_rect(x - 8, y - 8, 76, 96, 12, COLOR_SELECTED);
        draw_rounded_rect(x - 8, y - 8, 76, 96, 12, COLOR_ACCENT);
    } else if (is_hover) {
        fill_rounded_rect(x - 8, y - 8, 76, 96, 12, COLOR_HOVER);
    }
    
    // Icon mit abgerundeten Ecken (AA)
    if (type == FILE_TYPE_DIR) {
        // Moderner Folder
        fill_rounded_rect(x + 5, y + 8, 50, 38, 6, icon_color);
        fill_rounded_rect(x + 5, y + 5, 22, 6, 4, icon_color);
        
        // Folder Details (Linien)
        vbe_fill_rect(x + 15, y + 18, 30, 2, 0x66000000);
        vbe_fill_rect(x + 15, y + 25, 25, 2, 0x66000000);
        vbe_fill_rect(x + 15, y + 32, 20, 2, 0x66000000);
        
        // Glanz-Effekt
        fill_rounded_rect(x + 7, y + 10, 46, 12, 4, 0x22FFFFFF);
        
    } else if (type == FILE_TYPE_APP) {
        // Modernes App Icon (abgerundet mit AA)
        fill_rounded_rect(x + 10, y + 5, 40, 40, 10, icon_color);
        
        // Glanz
        fill_rounded_rect(x + 12, y + 7, 36, 16, 8, 0x33FFFFFF);
        
        // App Symbol (Grid mit AA Rundung)
        for (int row = 0; row < 2; row++) {
            for (int col = 0; col < 2; col++) {
                fill_rounded_rect(x + 20 + col * 10, y + 18 + row * 10, 6, 6, 2, 0xFFFFFFFF);
            }
        }
        
    } else {
        // Modernes File Icon (abgerundet mit AA)
        fill_rounded_rect(x + 12, y + 5, 36, 45, 6, 0xFF3A3A3A);
        draw_rounded_rect(x + 12, y + 5, 36, 45, 6, COLOR_BORDER);
        
        // Eselsohr (cleaner)
        int fold_size = 8;
        vbe_fill_rect(x + 48 - fold_size, y + 5, fold_size, 1, COLOR_BORDER);
        vbe_fill_rect(x + 47, y + 5, 1, fold_size, COLOR_BORDER);
        vbe_fill_rect(x + 48 - fold_size, y + 5, fold_size, fold_size, 0xFF2B2B2B);
        
        // Text-Linien (abgerundet mit AA)
        fill_rounded_rect(x + 18, y + 20, 24, 2, 1, COLOR_TEXT_DIM);
        fill_rounded_rect(x + 18, y + 25, 20, 2, 1, COLOR_TEXT_DIM);
        fill_rounded_rect(x + 18, y + 30, 22, 2, 1, COLOR_TEXT_DIM);
        fill_rounded_rect(x + 18, y + 35, 18, 2, 1, COLOR_TEXT_DIM);
    }
    
    // Dateiname
    char display_name[12];
    int len = strlen(name);
    if (len > 10) {
        for(int i = 0; i < 8; i++) display_name[i] = name[i];
        display_name[8] = '.';
        display_name[9] = '.';
        display_name[10] = 0;
    } else {
        strcpy(display_name, name);
    }
    
    int text_width = strlen(display_name) * 8;
    int text_x = x + (60 - text_width) / 2;
    vbe_draw_text(text_x, y + 58, display_name, COLOR_TEXT, VBE_TRANSPARENT);
}

// Moderne Toolbar mit Breadcrumb
static void draw_toolbar(int x, int y, int w, file_t* files) {
    // Toolbar Hintergrund
    vbe_fill_rect(x, y, w, 50, COLOR_TOOLBAR);
    vbe_fill_rect(x, y + 49, w, 1, COLOR_BORDER);
    
    // Back Button (klickbar, abgerundet mit AA)
    int btn_x = x + 12;
    int btn_y = y + 12;
    
    fill_rounded_rect(btn_x, btn_y, 36, 26, 8, COLOR_HOVER);
    
    // Cleaner Pfeil nach links (chevron style)
    // Obere Hälfte des Pfeils
    for (int i = 0; i < 4; i++) {
        vbe_fill_rect(btn_x + 18 - i, btn_y + 9 + i, 2, 2, COLOR_TEXT);
    }
    // Untere Hälfte des Pfeils
    for (int i = 0; i < 4; i++) {
        vbe_fill_rect(btn_x + 18 - i, btn_y + 13 + i, 2, 2, COLOR_TEXT);
    }
    
    // Breadcrumb Background (abgerundet mit AA)
    fill_rounded_rect(x + 60, y + 12, w - 180, 26, 8, COLOR_BG);
    
    int breadcrumb_x = x + 75;
    int breadcrumb_y = y + 17;  // Zentriert (12 + 26/2 - 8 = 17)
    
    // Home Icon (besser zentriert)
    vbe_fill_rect(breadcrumb_x, breadcrumb_y + 5, 8, 5, COLOR_TEXT_DIM);
    vbe_fill_rect(breadcrumb_x + 2, breadcrumb_y + 2, 4, 3, COLOR_TEXT_DIM);
    vbe_fill_rect(breadcrumb_x + 3, breadcrumb_y + 6, 2, 4, COLOR_BG); // Tür
    
    breadcrumb_x += 18;
    
    if (fm_current_dir == 0) {
        vbe_draw_text(breadcrumb_x, breadcrumb_y, "Home", COLOR_TEXT, VBE_TRANSPARENT);
    } else {
        vbe_draw_text(breadcrumb_x, breadcrumb_y, "Home", COLOR_TEXT_DIM, VBE_TRANSPARENT);
        breadcrumb_x += 50;
        vbe_draw_text(breadcrumb_x - 8, breadcrumb_y, ">", COLOR_TEXT_DIM, VBE_TRANSPARENT);
        vbe_draw_text(breadcrumb_x, breadcrumb_y, files[fm_current_dir].name, COLOR_ACCENT, VBE_TRANSPARENT);
    }
    
    // Search Box (rechts, klickbar, besser zentriert)
    int search_x = x + w - 160;
    fill_rounded_rect(search_x, y + 12, 140, 26, 8, COLOR_BG);
    vbe_draw_text(search_x + 10, y + 17, "Search...", COLOR_TEXT_DIM, VBE_TRANSPARENT);
}

void filemanager_draw_vbe(int x, int y, int w, int h, int is_active) {
    // Haupthintergrund
    vbe_fill_rect(x, y, w, h, COLOR_BG);
    
    file_t* files = fs_get_table();
    
    // Sidebar zeichnen
    draw_sidebar(x, y, h, files);
    
    // Toolbar zeichnen (rechts von Sidebar)
    draw_toolbar(x + 200, y, w - 200, files);
    
    // Content-Bereich
    int content_x = x + 200;
    int content_y = y + 55;
    int content_w = w - 220;
    int content_h = h - 80;
    
    // Grid Layout
    int start_x = content_x + 20;
    int start_y = content_y + 15;
    int cur_x = start_x;
    int cur_y = start_y - scroll_offset;
    int gap_x = 90;
    int gap_y = 100;
    
    int icon_count = 0;
    int total_height = 0;
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use && files[i].parent_idx == fm_current_dir) {
            // Nur zeichnen wenn sichtbar
            if (cur_y + 100 >= content_y && cur_y < content_y + content_h) {
                int is_selected = (i == selected_file_idx);
                int is_hover = (i == hover_file_idx);
                draw_modern_icon(cur_x, cur_y, files[i].type, files[i].name, is_selected, is_hover);
            }
            
            cur_x += gap_x;
            if (cur_x > content_x + content_w - 70) {
                cur_x = start_x;
                cur_y += gap_y;
                total_height = cur_y + 100;
            }
            icon_count++;
        }
    }
    
    // Scrollbar zeichnen
    draw_scrollbar(x + w - 16, content_y, content_h, total_height, content_h);
    
    // Statusleiste (abgerundet mit AA)
    int status_y = y + h - 32;
    fill_rounded_rect(x + 200, status_y, w - 200, 28, 8, COLOR_TOOLBAR);
    vbe_fill_rect(x + 200, status_y, w - 200, 1, COLOR_BORDER);
    
    char status[32];
    strcpy(status, "");
    char num[8];
    num[0] = '0' + (icon_count / 10);
    num[1] = '0' + (icon_count % 10);
    num[2] = 0;
    if (icon_count < 10) {
        num[0] = '0' + icon_count;
        num[1] = 0;
    }
    strcat(status, num);
    strcat(status, " items");
    
    vbe_draw_text(x + 215, status_y + 9, status, COLOR_TEXT_DIM, VBE_TRANSPARENT);
    
    if (selected_file_idx >= 0 && files[selected_file_idx].in_use) {
        vbe_draw_text(x + 300, status_y + 9, files[selected_file_idx].name, COLOR_ACCENT, VBE_TRANSPARENT);
    }
}

void filemanager_handle_click_vbe(int x, int y) {
    // Check Back Button
    if (y < 50 && x >= 212 && x <= 248 && y >= 12 && y <= 38) {
        file_t* files = fs_get_table();
        if (fm_current_dir != 0) {
            fm_current_dir = files[fm_current_dir].parent_idx;
            selected_file_idx = -1;
        }
        return;
    }
    
    // Check Icons
    int start_x = 220;
    int start_y = 70;
    int cur_x = start_x;
    int cur_y = start_y - scroll_offset;
    int gap_x = 90;
    int gap_y = 100;
    
    file_t* files = fs_get_table();
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use && files[i].parent_idx == fm_current_dir) {
            if (x >= cur_x - 8 && x < cur_x + 68 && y >= cur_y - 8 && y < cur_y + 88) {
                selected_file_idx = i;
                
                if (files[i].type == FILE_TYPE_DIR) {
                    fm_current_dir = i;
                    selected_file_idx = -1;
                } else if (files[i].type == FILE_TYPE_APP) {
                    if (strncmp(files[i].content, "app:", 4) == 0) {
                        int app_id = files[i].content[4] - '0';
                        gui_open_window(app_id);
                    }
                }
                return;
            }
            
            cur_x += gap_x;
            if (cur_x > 800 - 70) {
                cur_x = start_x;
                cur_y += gap_y;
            }
        }
    }
    
    selected_file_idx = -1;
}