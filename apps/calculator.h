#ifndef CALCULATOR_H
#define CALCULATOR_H

void calculator_init();
void calculator_draw(int x, int y, int w, int h, int is_selected);
void calculator_handle_click(int cursor_x, int cursor_y, int win_x, int win_y);

// VBE Graphics versions
void calculator_draw_vbe(int x, int y, int w, int h, int is_selected);
void calculator_handle_click_vbe(int cursor_x, int cursor_y);

#endif