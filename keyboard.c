#include "keyboard.h"
#include "port.h"

/* Wait for output buffer and read scancode */
unsigned char keyboard_read() {
    /* Wait until output buffer full */
    while ((inb(0x64) & 1) == 0);
    return inb(0x60);
}

/* Minimal US-layout scancode -> ascii mapping for standard keys.
   Only handles simple keys, ignores modifiers and releases. */
char scancode_to_ascii(unsigned char sc) {
    /* ignore key-release (bit 7) */
    if (sc & 0x80) return 0;
    static const char map[128] = {
        0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
        '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
        'a','s','d','f','g','h','j','k','l',';','\'','`', 0, '\\',
        'z','x','c','v','b','n','m',',','.','/', 0, 0, 0, ' ',
        /* rest zeros */
    };
    if (sc < sizeof(map)) return map[sc];
    return 0;
}
