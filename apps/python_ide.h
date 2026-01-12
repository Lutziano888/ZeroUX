#ifndef PYTHON_IDE_H
#define PYTHON_IDE_H

void python_ide_init();
void python_ide_draw_vbe(int x, int y, int w, int h, int is_active);
void python_ide_handle_key(char c);
void python_ide_handle_click(int x, int y);
void python_ide_handle_backspace();

#endif