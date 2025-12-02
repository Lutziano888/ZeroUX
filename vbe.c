#include "vbe.h"
#include "port.h"

static unsigned int* current_framebuffer = 0;
static int current_width = 0;
static int current_height = 0;

int vbe_set_mode(int width, int height, int bpp) {
    outw(0x01CE, 0x04);
    outw(0x01CF, 0x00);
    
    outw(0x01CE, 0x01);
    outw(0x01CF, width);
    
    outw(0x01CE, 0x02);
    outw(0x01CF, height);
    
    outw(0x01CE, 0x03);
    outw(0x01CF, bpp);
    
    outw(0x01CE, 0x04);
    outw(0x01CF, 0x41);
    
    current_width = width;
    current_height = height;
    current_framebuffer = (unsigned int*)0xE0000000;
    
    return 1;
}

unsigned int* vbe_get_framebuffer() {
    return current_framebuffer;
}

void vbe_restore_text_mode() {
    outw(0x01CE, 0x04);
    outw(0x01CF, 0x00);
    for (volatile int i = 0; i < 100000; i++);
}