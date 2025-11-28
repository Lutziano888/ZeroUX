#include "input.h"
#include "keyboard.h"
#include "vga.h"
#include "string.h"

void read_input(char* out, int max) {
    int pos = 0;
    char buffer[128];
    
    while (1) {
        unsigned char sc = keyboard_read();
        
        /* Pfeiltasten behandeln */
        if (is_arrow_key(sc)) {
            arrow_key_t dir = get_arrow_direction(sc);
            
            if (dir == ARROW_UP) {
                vga_scroll_up();
            } else if (dir == ARROW_DOWN) {
                vga_scroll_down();
            }
            // LEFT und RIGHT können für History verwendet werden
            continue;
        }
        
        /* ignore key-release scancodes */
        if (sc & 0x80) continue;

        char c = scancode_to_ascii(sc);
        if (!c) continue;

        /* Enter -> finish */
        if (c == '\n') {
            buffer[pos] = 0;
            strcpy(out, buffer);
            vga_print("\n");
            vga_scroll_to_bottom();  // Zurück zum Ende scrollen
            return;
        }

        /* Backspace */
        if (c == '\b') {
            if (pos > 0) {
                pos--;
                vga_backspace();
            }
            continue;
        }

        /* Normal char */
        if (pos < max - 1) {
            buffer[pos++] = c;
            char tmp[2] = {c, 0};
            vga_scroll_to_bottom();  // Bei Eingabe ans Ende springen
            vga_print(tmp); /* echo */
        }
    }
}