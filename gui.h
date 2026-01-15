// ============================================================================
// DATEI 2: gui.h (KOMPLETT)
// ============================================================================
#ifndef GUI_H
#define GUI_H

// Öffentliche GUI-Funktionen
void gui_init();
void gui_draw_desktop();
void gui_draw_window(int id);
void gui_draw_all_windows();
void gui_run();

// Window Visibility Management
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

// Close-Button Funktionen
void draw_close_button(int x, int y, unsigned char color);
int check_close_button_click(int cx, int cy, int win_x, int win_y, int win_w);

// NEU: Hochauflösende Font-Rendering Funktionen
void draw_text_hd(int x, int y, const char* text, unsigned int color, int size);
void draw_text_antialiased(int x, int y, const char* text, unsigned int color);

// Window count constant - ERHÖHT FÜR BROWSER!
#define MAX_WINDOWS 10

// NEU: Auflösungsunterstützung
#define RES_1080P 0
#define RES_720P  1
#define RES_LEGACY 2

// Desktop Personalization
void gui_set_desktop_gradient(unsigned int top, unsigned int bottom);

#endif