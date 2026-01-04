#ifndef WELCOME_H
#define WELCOME_H

void welcome_init();
void welcome_draw(int x, int y, int w, int h, int is_selected);
void welcome_handle_click(int cursor_x, int cursor_y, int win_x, int win_y);

#endif