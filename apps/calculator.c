#include "calculator.h"
#include "vbe.h"
#include "../string.h"

typedef struct {
    char display[32];
    int value;
    int last_value;
    char operation;
    int new_number;
} CalcState;

static CalcState calc;

static void calc_input_digit(int digit) {
    if (calc.new_number) {
        calc.value = digit;
        calc.new_number = 0;
    } else {
        calc.value = calc.value * 10 + digit;
    }
    int_to_str(calc.value, calc.display);
}

static void calc_do_operation() {
    switch (calc.operation) {
        case '+': calc.value = calc.last_value + calc.value; break;
        case '-': calc.value = calc.last_value - calc.value; break;
        case '*': calc.value = calc.last_value * calc.value; break;
        case '/':
            if (calc.value == 0) {
                strcpy(calc.display, "Error");
                calc.new_number = 1;
                return;
            }
            calc.value = calc.last_value / calc.value;
            break;
    }
    int_to_str(calc.value, calc.display);
}

static void calc_set_operation(char op) {
    if (calc.operation && !calc.new_number)
        calc_do_operation();
    calc.last_value = calc.value;
    calc.operation = op;
    calc.new_number = 1;
}

static void calc_equals() {
    if (calc.operation) {
        calc_do_operation();
        calc.operation = 0;
    }
    calc.new_number = 1;
}

static void calc_clear() {
    calc.value = 0;
    calc.last_value = 0;
    calc.operation = 0;
    calc.new_number = 1;
    strcpy(calc.display, "0");
}

void calculator_init() {
    strcpy(calc.display, "0");
    calc.value = 0;
    calc.last_value = 0;
    calc.operation = 0;
    calc.new_number = 1;
}

void calculator_draw_vbe(int x, int y, int w, int h, int is_selected) {
    // Display area
    vbe_fill_rect(x + 10, y + 10, w - 20, 50, VBE_BLACK);
    vbe_draw_rect(x + 10, y + 10, w - 20, 50, VBE_GRAY);
    
    // Display text (right-aligned)
    int text_len = 0;
    while (calc.display[text_len]) text_len++;
    int text_x = x + w - 30 - (text_len * 8);
    vbe_draw_text(text_x, y + 25, calc.display, VBE_WHITE, VBE_TRANSPARENT);
    
    // Button layout
    const char* buttons[4][4] = {
        {"7", "8", "9", "/"},
        {"4", "5", "6", "*"},
        {"1", "2", "3", "-"},
        {"C", "0", "=", "+"}
    };
    
    unsigned int colors[4] = {
        0x00505050, 0x00505050, 0x00505050, 0x000078D7,
    };
    
    int btn_w = (w - 60) / 4;
    int btn_h = (h - 100) / 4;
    
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            int btn_x = x + 10 + col * (btn_w + 5);
            int btn_y = y + 70 + row * (btn_h + 5);
            
            unsigned int color = (col == 3) ? colors[3] : 
                                (row == 3 && col == 0) ? 0x00E81123 : colors[0];
            
            vbe_fill_rect(btn_x, btn_y, btn_w, btn_h, color);
            vbe_draw_rect(btn_x, btn_y, btn_w, btn_h, VBE_LIGHT_GRAY);
            
            // Center text
            int text_x = btn_x + (btn_w - 8) / 2;
            int text_y = btn_y + (btn_h - 16) / 2;
            vbe_draw_text(text_x, text_y, buttons[row][col], VBE_WHITE, VBE_TRANSPARENT);
        }
    }
}

void calculator_handle_click_vbe(int rel_x, int rel_y) {
    if (rel_y < 70) return; // Not in button area
    
    // Must match the dimensions used in calculator_draw_vbe
    // For Calculator window: 400x400 -> content area 400x370 (after 30px title)
    // btn_w = (w - 60) / 4 = (400 - 60) / 4 = 85
    // btn_h = (h - 100) / 4 = (370 - 100) / 4 = 67
    int btn_w = 85;
    int btn_h = 67;
    
    int col = (rel_x - 10) / (btn_w + 5);
    int row = (rel_y - 70) / (btn_h + 5);
    
    if (col < 0 || col > 3 || row < 0 || row > 3) return;
    
    // Button mapping
    const int digits[4][4] = {
        {7, 8, 9, -1},
        {4, 5, 6, -2},
        {1, 2, 3, -3},
        {-4, 0, -5, -6}
    };
    
    int action = digits[row][col];
    
    if (action >= 0) {
        calc_input_digit(action);
    } else {
        switch (action) {
            case -1: calc_set_operation('/'); break;
            case -2: calc_set_operation('*'); break;
            case -3: calc_set_operation('-'); break;
            case -4: calc_clear(); break;
            case -5: calc_equals(); break;
            case -6: calc_set_operation('+'); break;
        }
    }
}

// Legacy compatibility
void calculator_draw(int x, int y, int w, int h, int is_selected) {}
void calculator_handle_click(int cx, int cy, int wx, int wy) {}