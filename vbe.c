#include "vbe.h"
#include "port.h"
#include "font.h"
#include "modern_font.h"

static unsigned int* framebuffer = 0;
static int screen_width = 1920;
static int screen_height = 1089;
static int screen_bpp = 32;

int vbe_init() {
    // Try Bochs VBE extensions (works with qemu-system-i386 -vga std)
    // Write to Bochs VBE Index Port
    outw(0x01CE, 0x04); // VBE_DISPI_INDEX_ENABLE
    outw(0x01CF, 0x00); // Disable first
    
    // Set resolution
    outw(0x01CE, 0x01); // VBE_DISPI_INDEX_XRES
    outw(0x01CF, screen_width);
    
    outw(0x01CE, 0x02); // VBE_DISPI_INDEX_YRES
    outw(0x01CF, screen_height);
    
    outw(0x01CE, 0x03); // VBE_DISPI_INDEX_BPP
    outw(0x01CF, screen_bpp);
    
    // Enable LFB mode
    outw(0x01CE, 0x04); // VBE_DISPI_INDEX_ENABLE
    outw(0x01CF, 0x41); // Enable + LFB mode
    
    // Get framebuffer address from VBE (should be around 0xE0000000 for QEMU std)
    outw(0x01CE, 0x0E); // VBE_DISPI_INDEX_LFBADDR_LO
    unsigned short fb_lo = inw(0x01CF);
    
    outw(0x01CE, 0x0F); // VBE_DISPI_INDEX_LFBADDR_HI  
    unsigned short fb_hi = inw(0x01CF);
    
    // Construct full framebuffer address
    unsigned int fb_addr = ((unsigned int)fb_hi << 16) | fb_lo;
    
    if (fb_addr != 0) {
        framebuffer = (unsigned int*)fb_addr;
    } else {
        // Fallback to standard QEMU Bochs VBE address
        framebuffer = (unsigned int*)0xE0000000;
    }
    
    // Test write to verify framebuffer is working
    if (framebuffer) {
        // Write test pattern
        framebuffer[0] = 0x00FF0000; // Red
        framebuffer[1] = 0x0000FF00; // Green
        framebuffer[2] = 0x000000FF; // Blue
    }
    
    return 1;
}

void vbe_set_pixel(int x, int y, unsigned int color) {
    if (x >= 0 && x < screen_width && y >= 0 && y < screen_height) {
        framebuffer[y * screen_width + x] = color;
    }
}

unsigned int vbe_get_pixel(int x, int y) {
    if (x >= 0 && x < screen_width && y >= 0 && y < screen_height) {
        return framebuffer[y * screen_width + x];
    }
    return 0;
}

void vbe_fill_rect(int x, int y, int w, int h, unsigned int color) {
    for (int py = y; py < y + h && py < screen_height; py++) {
        for (int px = x; px < x + w && px < screen_width; px++) {
            if (px >= 0 && py >= 0) {
                framebuffer[py * screen_width + px] = color;
            }
        }
    }
}

void vbe_draw_rect(int x, int y, int w, int h, unsigned int color) {
    // Top and bottom
    for (int i = 0; i < w; i++) {
        vbe_set_pixel(x + i, y, color);
        vbe_set_pixel(x + i, y + h - 1, color);
    }
    // Left and right
    for (int i = 0; i < h; i++) {
        vbe_set_pixel(x, y + i, color);
        vbe_set_pixel(x + w - 1, y + i, color);
    }
}

void vbe_draw_char(int x, int y, char c, unsigned int fg, unsigned int bg) {
    if (c < 32 || c > 126) c = '?';
    
    const unsigned char* glyph = font_modern_8x16[(unsigned char)c];
    
    for (int py = 0; py < 16; py++) {
        unsigned char row = glyph[py];
        for (int px = 0; px < 8; px++) {
            if (row & (0x80 >> px)) {
                vbe_set_pixel(x + px, y + py, fg);
            } else if (bg != 0xFFFFFFFF) { // Transparent background check
                vbe_set_pixel(x + px, y + py, bg);
            }
        }
    }
}

void vbe_draw_text(int x, int y, const char* text, unsigned int fg, unsigned int bg) {
    int cx = x;
    while (*text) {
        if (*text == '\n') {
            cx = x;
            y += 16;
        } else {
            vbe_draw_char(cx, y, *text, fg, bg);
            cx += 8;
        }
        text++;
    }
}

void vbe_clear(unsigned int color) {
    vbe_fill_rect(0, 0, screen_width, screen_height, color);
}

int vbe_get_width() { return screen_width; }
int vbe_get_height() { return screen_height; }

void vbe_draw_line(int x1, int y1, int x2, int y2, unsigned int color) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = (dx > dy ? dx : dy);
    if (steps < 0) steps = -steps;
    
    float x_inc = (float)dx / steps;
    float y_inc = (float)dy / steps;
    
    float x = x1;
    float y = y1;
    
    for (int i = 0; i <= steps; i++) {
        vbe_set_pixel((int)x, (int)y, color);
        x += x_inc;
        y += y_inc;
    }
}

void vbe_draw_circle(int cx, int cy, int radius, unsigned int color) {
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        vbe_set_pixel(cx + x, cy + y, color);
        vbe_set_pixel(cx + y, cy + x, color);
        vbe_set_pixel(cx - y, cy + x, color);
        vbe_set_pixel(cx - x, cy + y, color);
        vbe_set_pixel(cx - x, cy - y, color);
        vbe_set_pixel(cx - y, cy - x, color);
        vbe_set_pixel(cx + y, cy - x, color);
        vbe_set_pixel(cx + x, cy - y, color);
        
        if (err <= 0) {
            y += 1;
            err += 2*y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

void vbe_fill_circle(int cx, int cy, int radius, unsigned int color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                vbe_set_pixel(cx + x, cy + y, color);
            }
        }
    }
}