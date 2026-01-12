#ifndef MEMTEST_APP_H
#define MEMTEST_APP_H

void memtest_init();
void memtest_draw_vbe(int x, int y, int w, int h, int is_active);
void memtest_handle_click_vbe(int x, int y);

#endif
