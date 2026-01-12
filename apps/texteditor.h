#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

void texteditor_init();
void texteditor_draw_vbe(int x, int y, int w, int h, int is_active);
void texteditor_handle_click(int x, int y);
void texteditor_handle_key(char c);
void texteditor_handle_backspace();

#endif
