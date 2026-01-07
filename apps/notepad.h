#ifndef NOTEPAD_H
#define NOTEPAD_H

void notepad_init();
void notepad_draw(int x, int y, int w, int h, int is_selected);
void notepad_handle_click(int cursor_x, int cursor_y, int win_x, int win_y);
int notepad_is_active();
void notepad_deactivate();
void notepad_handle_backspace();
void notepad_handle_char(char c);

// VBE Graphics versions
void notepad_draw_vbe(int x, int y, int w, int h, int is_selected);
void notepad_handle_click_vbe(int cursor_x, int cursor_y);

#endif