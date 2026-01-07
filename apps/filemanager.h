#ifndef FILEMANAGER_H
#define FILEMANAGER_H

void filemanager_init();
void filemanager_draw(int x, int y, int w, int h, int is_selected);
void filemanager_handle_click(int cursor_x, int cursor_y, int win_x, int win_y);

// VBE Graphics versions
void filemanager_draw_vbe(int x, int y, int w, int h, int is_selected);
void filemanager_handle_click_vbe(int rel_x, int rel_y);

#endif
