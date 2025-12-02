#ifndef GUI_H
#define GUI_H

#define GUI_WIDTH 320
#define GUI_HEIGHT 200
#define MAX_WINDOWS 8

typedef struct {
    int x, y;
    int width, height;
    char title[32];
    int active;
    int visible;
    unsigned char color;
} window_t;

// GUI initialisieren
void gui_init();

// Desktop zeichnen
void gui_draw_desktop();

// Fenster erstellen
int gui_create_window(int x, int y, int w, int h, const char* title, unsigned char color);

// Fenster zeichnen
void gui_draw_window(int id);

// Alle Fenster zeichnen
void gui_draw_all_windows();

// Fenster schließen
void gui_close_window(int id);

// Maus-Position aktualisieren
void gui_update_mouse(int x, int y);

// Maus zeichnen
void gui_draw_mouse();

// GUI Event-Loop
void gui_run();

// Pixel setzen
void gui_set_pixel(int x, int y, unsigned char color);

// Rechteck zeichnen
void gui_draw_rect(int x, int y, int w, int h, unsigned char color);

// Text zeichnen (einfach)
void gui_draw_text(int x, int y, const char* text, unsigned char color);

#endif