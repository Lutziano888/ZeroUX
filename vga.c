#include "vga.h"

static unsigned short* VGA = (unsigned short*)0xB8000;
static int row=0, col=0;

void vga_put(char c) {
    if(c=='\n') { row++; col=0; return; }
    VGA[row*80+col] = (0x0F<<8)|c;
    col++;
    if(col>=80) { col=0; row++; }
}

void vga_print(const char* s) {
    while(*s) vga_put(*s++);
}

void vga_clear() {
    for(int i=0;i<80*25;i++)
        VGA[i]=(0x0F<<8)|' ';
    row=0; col=0;
}

void vga_backspace() {
    if(col>0) { col--; VGA[row*80+col]=(0x0F<<8)|' '; }
}

void vga_println(const char* str) {
    vga_print(str);
    vga_print("\n");
}
