#ifndef VBE_H
#define VBE_H

// VBE Colors (RGB format: 0x00RRGGBB)
#define VBE_BLACK       0x00000000
#define VBE_WHITE       0x00FFFFFF
#define VBE_RED         0x00FF0000
#define VBE_GREEN       0x0000FF00
#define VBE_BLUE        0x000000FF
#define VBE_YELLOW      0x00FFFF00
#define VBE_CYAN        0x0000FFFF
#define VBE_MAGENTA     0x00FF00FF
#define VBE_GRAY        0x00808080
#define VBE_DARK_GRAY   0x00404040
#define VBE_LIGHT_GRAY  0x00C0C0C0
#define VBE_ORANGE      0x00FF8800
#define VBE_PURPLE      0x00800080
#define VBE_BROWN       0x00964B00
#define VBE_PINK        0x00FFC0CB
#define VBE_TRANSPARENT 0xFFFFFFFF

// Windows 10 Style Colors
#define WIN10_BG        0x001E1E1E
#define WIN10_PANEL     0x002D2D30
#define WIN10_ACCENT    0x000078D7
#define WIN10_TEXT      0x00FFFFFF
#define WIN10_BORDER    0x00454545

// Initialize VBE mode
int vbe_init();

// Basic drawing
void vbe_set_pixel(int x, int y, unsigned int color);
unsigned int vbe_get_pixel(int x, int y);
void vbe_fill_rect(int x, int y, int w, int h, unsigned int color);
void vbe_draw_rect(int x, int y, int w, int h, unsigned int color);
void vbe_clear(unsigned int color);

// Text rendering
void vbe_draw_char(int x, int y, char c, unsigned int fg, unsigned int bg);
void vbe_draw_text(int x, int y, const char* text, unsigned int fg, unsigned int bg);

// Advanced shapes
void vbe_draw_line(int x1, int y1, int x2, int y2, unsigned int color);
void vbe_draw_circle(int cx, int cy, int radius, unsigned int color);
void vbe_fill_circle(int cx, int cy, int radius, unsigned int color);

// Screen info
int vbe_get_width();
int vbe_get_height();

#endif