#ifndef VBE_H
#define VBE_H

int vbe_set_mode(int width, int height, int bpp);
unsigned int* vbe_get_framebuffer();
void vbe_restore_text_mode();

#endif