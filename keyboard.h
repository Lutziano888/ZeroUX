#ifndef KEYBOARD_H
#define KEYBOARD_H

typedef enum {
    ARROW_NONE,
    ARROW_UP,
    ARROW_DOWN,
    ARROW_LEFT,
    ARROW_RIGHT
} arrow_key_t;

unsigned char keyboard_read(); /* returns raw scancode (blocking) */
unsigned char keyboard_read_nonblock(); /* returns 0 if no key */
char scancode_to_ascii(unsigned char scancode);

int is_arrow_key(unsigned char scancode);
arrow_key_t get_arrow_direction(unsigned char scancode);

#endif