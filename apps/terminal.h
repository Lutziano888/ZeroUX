#ifndef TERMINAL_APP_H
#define TERMINAL_APP_H

void terminal_init();
void terminal_draw_vbe(int x, int y, int w, int h, int is_active);
void terminal_handle_key(char c);
void terminal_handle_enter();
void terminal_handle_backspace();

#endif
