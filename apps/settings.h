#ifndef SETTINGS_APP_H
#define SETTINGS_APP_H

void settings_init();
void settings_draw_vbe(int x, int y, int w, int h, int is_active);
void settings_handle_click_vbe(int x, int y);

#endif