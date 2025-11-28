#ifndef VGA_H
#define VGA_H

void vga_put(char c);
void vga_print(const char* s);
void vga_clear();
void vga_backspace();
void vga_println(const char* str);

// Scrolling-Funktionen
void vga_scroll_up();
void vga_scroll_down();
void vga_scroll_to_bottom();

#endif