#ifndef GUI_H
#define GUI_H

// Öffentliche GUI-Funktionen
void gui_init();
void gui_draw_desktop();
void gui_draw_window(int id);
void gui_draw_all_windows();
void gui_run();

// Hilfsfunktionen für Apps
void draw_box(int x, int y, int w, int h, unsigned char color);
void draw_text_at(int x, int y, const char* text, unsigned char color);
void draw_button(int x, int y, const char* text, unsigned char color);
int check_button_click(int cx, int cy, int bx, int by, const char* text);

#endif