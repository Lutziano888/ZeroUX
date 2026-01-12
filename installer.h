#ifndef INSTALLER_H
#define INSTALLER_H

void installer_init();
void installer_draw_vbe(int x, int y, int w, int h, int is_active);
void installer_handle_click_vbe(int x, int y);
void installer_handle_key(char c);
void installer_handle_backspace();

#endif