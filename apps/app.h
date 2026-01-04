#ifndef APP_H
#define APP_H

typedef struct {
    int x, y, w, h;
    int selected;
    void* state;
} AppContext;

extern AppContext* app;

/* App lifecycle */
void app_begin(int x, int y, int w, int h, int selected);
void app_end();

/* Drawing helpers */
void app_clear();
void app_label(int x, int y, const char* text);
int  app_button(int x, int y, const char* text);

#endif
