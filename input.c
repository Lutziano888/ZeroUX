#include "input.h"
#include "keyboard.h"
#include "vga.h"
#include "string.h"

void read_input(char* out, int max) {
    int pos = 0;
    char buffer[128];
    int shift = 0;
    
    while (1) {
        unsigned char sc = keyboard_read();
        
        if (sc == 0x2A || sc == 0x36) { shift = 1; continue; }
        if (sc == 0xAA || sc == 0xB6) { shift = 0; continue; }
        
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
        
        if (shift) {
            if (c >= 'a' && c <= 'z') c -= 32;
            else if (c == '1') c = '!';
            else if (c == '2') c = '"';
            else if (c == '4') c = '$';
            else if (c == '5') c = '%';
            else if (c == '6') c = '&';
            else if (c == '7') c = '/';
            else if (c == '8') c = '(';
            else if (c == '9') c = ')';
            else if (c == '0') c = '=';
        }
        
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