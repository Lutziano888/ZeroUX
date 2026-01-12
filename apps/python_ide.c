#include "python_ide.h"
#include "python_interpreter.h"
#include "../vbe.h"
#include "../gui.h"
#include "../string.h"

static char code_buf[1024];
static int code_len = 0;
static char output_buf[1024];

void python_ide_init() {
    code_len = 0;
    code_buf[0] = 0;
    output_buf[0] = 0;
    // Beispiel-Code
    strcpy(code_buf, "# Simple Python Script\nprint(\"Hello ZeroUX\")\nx = 10\ny = 5\nprint(\"Result:\")\nprint(x + y)\n");
    code_len = strlen(code_buf);
}

void python_ide_draw_vbe(int x, int y, int w, int h, int is_active) {
    // Hintergrund
    vbe_fill_rect(x, y, w, h, 0xFF282C34); // Dark Theme

    // Toolbar
    vbe_fill_rect(x, y, w, 30, 0xFF21252B);
    
    // Run Button
    draw_button_modern(x + 10, y + 4, 60, 22, 4, "RUN", 0xFF98C379); // Green
    vbe_draw_text(x + 80, y + 8, "Python 3.x (Micro)", 0xFFABB2BF, VBE_TRANSPARENT);

    // Split View: Code (Links) | Output (Rechts)
    int split_x = w / 2;
    
    // Code Area
    vbe_draw_text(x + 10, y + 35, "Code:", 0xFF61AFEF, VBE_TRANSPARENT);
    int cur_y = y + 55;
    int cur_x = x + 10;
    
    // Code rendern (Zeilenweise)
    for (int i = 0; i < code_len; i++) {
        char c = code_buf[i];
        if (c == '\n') {
            cur_y += 14;
            cur_x = x + 10;
        } else {
            char s[2] = {c, 0};
            // Syntax Highlighting (Simpel)
            unsigned int color = 0xFFABB2BF; // Default Text
            if (c >= '0' && c <= '9') color = 0xFFD19A66; // Numbers
            if (c == '"') color = 0xFF98C379; // Strings
            
            vbe_draw_text(cur_x, cur_y, s, color, VBE_TRANSPARENT);
            cur_x += 8;
        }
    }
    
    // Cursor
    if (is_active && gui_get_caret_state()) {
        vbe_fill_rect(cur_x, cur_y, 2, 12, 0xFF528BFF);
    }

    // Separator
    vbe_fill_rect(x + split_x, y + 30, 2, h - 30, 0xFF181A1F);

    // Output Area
    vbe_draw_text(x + split_x + 10, y + 35, "Output:", 0xFFE06C75, VBE_TRANSPARENT);
    cur_y = y + 55;
    cur_x = x + split_x + 10;
    
    char* p = output_buf;
    while (*p) {
        if (*p == '\n') {
            cur_y += 14;
            cur_x = x + split_x + 10;
        } else {
            char s[2] = {*p, 0};
            vbe_draw_text(cur_x, cur_y, s, 0xFFCCCCCC, VBE_TRANSPARENT);
            cur_x += 8;
        }
        p++;
    }
}

void python_ide_handle_click(int x, int y) {
    // Check Run Button (x, y sind relativ zum Fensterinhalt)
    if (x >= 10 && x <= 70 && y >= 4 && y <= 26) {
        python_run_script(code_buf, output_buf, 1024);
    }
}

void python_ide_handle_key(char c) {
    if (code_len < 1023) {
        code_buf[code_len++] = c;
        code_buf[code_len] = 0;
    }
}

void python_ide_handle_backspace() {
    if (code_len > 0) {
        code_len--;
        code_buf[code_len] = 0;
    }
}