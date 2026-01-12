#ifndef MOUSE_H
#define MOUSE_H

void mouse_init();
int mouse_read_packet(int* dx, int* dy, int* buttons);
void mouse_handle_byte();

#endif