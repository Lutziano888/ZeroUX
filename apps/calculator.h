#ifndef CALCULATOR_H
#define CALCULATOR_H

void calculator_init();
void calculator_draw(int x, int y, int w, int h, int is_selected);
void calculator_handle_click(int cursor_x, int cursor_y, int win_x, int win_y);

#endif