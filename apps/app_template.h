#ifndef APP_TEMPLATE_H
#define APP_TEMPLATE_H

void app_init();
void app_draw(int x, int y, int w, int h, int is_selected);
void app_handle_click(int cursor_x, int cursor_y, int win_x, int win_y);
void app_handle_key(unsigned char scancode);

#endif