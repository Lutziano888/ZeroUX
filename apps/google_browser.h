#ifndef GOOGLE_BROWSER_H
#define GOOGLE_BROWSER_H

void google_browser_init();
void google_browser_draw(int x, int y, int w, int h, int is_selected);
void google_browser_handle_click(int cursor_x, int cursor_y, int win_x, int win_y);

// VBE Graphics versions
void google_browser_draw_vbe(int x, int y, int w, int h, int is_selected);
void google_browser_handle_click_vbe(int cursor_x, int cursor_y);
void google_browser_handle_input(char key);

#endif
