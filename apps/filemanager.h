#ifndef FILEMANAGER_H
#define FILEMANAGER_H

void filemanager_init();
void filemanager_draw_vbe(int x, int y, int w, int h, int is_active);
void filemanager_handle_click_vbe(int x, int y);
void filemanager_handle_right_click(int x, int y);

#endif