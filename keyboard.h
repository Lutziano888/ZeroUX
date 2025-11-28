#ifndef KEYBOARD_H
#define KEYBOARD_H

unsigned char keyboard_read(); /* returns raw scancode (no conversion) */
char scancode_to_ascii(unsigned char scancode);

#endif
