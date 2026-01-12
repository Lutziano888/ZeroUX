#ifndef GUI_H
#define GUI_H

// Öffentliche GUI-Funktionen
void gui_init();
void gui_draw_desktop();
void gui_draw_window(int id);
void gui_draw_all_windows();
void gui_run();

// NEU: Window Visibility Management
void gui_close_window(int id);
void gui_open_window(int id);
int gui_is_window_visible(int id);
int gui_get_caret_state();

// Hilfsfunktionen für Apps
void draw_box(int x, int y, int w, int h, unsigned char color);
void draw_text_at(int x, int y, const char* text, unsigned char color);
void draw_button(int x, int y, const char* text, unsigned char color);
int check_button_click(int cx, int cy, int bx, int by, const char* text);
void draw_button_modern(int x, int y, int w, int h, int r, const char* text, unsigned int color);

// NEU: Close-Button Funktionen
void draw_close_button(int x, int y, unsigned char color);
int check_close_button_click(int cx, int cy, int win_x, int win_y, int win_w);

// Window count constant for array sizing
#define MAX_WINDOWS 9

#endif