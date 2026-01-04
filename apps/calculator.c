#include "calculator.h"
#include "../gui.h"
#include "../widgets.h"
#include "../string.h"
#include "gui_colors.h"

// =====================
// State
// =====================
typedef struct {
    char display[32];
    int value;
    int last_value;
    char operation;
    int new_number;
    WindowStyle* current_theme;
} CalcState;

static CalcState calc;

// =====================
// Styles
// =====================
static WidgetStyle STYLE_DISPLAY;
static WidgetStyle STYLE_NUM = { LIGHT_GRAY, BLACK, 0, 0, 0 };
static WidgetStyle STYLE_OPS = { LIGHT_BLUE, WHITE, 0, 0, 0 };
static WidgetStyle STYLE_ACT = { LIGHT_RED,  WHITE, 0, 0, 0 };

static void setup_styles() {
    STYLE_DISPLAY.bg_color     = BLACK;
    STYLE_DISPLAY.fg_color     = WHITE;
    STYLE_DISPLAY.border_color = DARK_GRAY;
}

// =====================
// Logic
// =====================
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
    calc.operation  = op;
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

// =====================
// Init
// =====================
void calculator_init() {
    strcpy(calc.display, "0");
    calc.value = 0;
    calc.last_value = 0;
    calc.operation = 0;
    calc.new_number = 1;
    calc.current_theme = &WINDOW_DARK;

    setup_styles();
}

// =====================
// Draw
// =====================
void calculator_draw(int x, int y, int w, int h, int is_selected) {
    draw_window(x, y, w, h, " Calculator ", calc.current_theme, is_selected);

    // Display
    unsigned short* VGA = (unsigned short*)0xB8000;
    int display_y = y + 2;

    for (int dy = 0; dy < 2; dy++)
        for (int dx = 2; dx < w - 2; dx++)
            VGA[(display_y + dy) * 80 + x + dx] = (0x00 << 8) | ' ';

    int len = 0;
    while (calc.display[len]) len++;

    int start = x + w - 3 - len;
    for (int i = 0; i < len; i++)
        VGA[(display_y + 1) * 80 + start + i] = (0x0F << 8) | calc.display[i];

    // Buttons
    int bx = x + 2;
    int by = y + 4;
    int bw = 5;

    // Row 1
    widget_button_win10(bx,      by, bw, "7", &STYLE_NUM);
    widget_button_win10(bx + 6,  by, bw, "8", &STYLE_NUM);
    widget_button_win10(bx + 12, by, bw, "9", &STYLE_NUM);
    widget_button_win10(bx + 18, by, bw, "/", &STYLE_OPS);

    // Row 2
    widget_button_win10(bx,      by + 2, bw, "4", &STYLE_NUM);
    widget_button_win10(bx + 6,  by + 2, bw, "5", &STYLE_NUM);
    widget_button_win10(bx + 12, by + 2, bw, "6", &STYLE_NUM);
    widget_button_win10(bx + 18, by + 2, bw, "*", &STYLE_OPS);

    // Row 3
    widget_button_win10(bx,      by + 4, bw, "1", &STYLE_NUM);
    widget_button_win10(bx + 6,  by + 4, bw, "2", &STYLE_NUM);
    widget_button_win10(bx + 12, by + 4, bw, "3", &STYLE_NUM);
    widget_button_win10(bx + 18, by + 4, bw, "-", &STYLE_OPS);

    // Row 4
    widget_button_win10(bx,      by + 6, bw, "C", &STYLE_ACT);
    widget_button_win10(bx + 6,  by + 6, bw, "0", &STYLE_NUM);
    widget_button_win10(bx + 12, by + 6, bw, "=", &STYLE_OPS);
    widget_button_win10(bx + 18, by + 6, bw, "+", &STYLE_OPS);
}

// =====================
// Click Handling (FIXED)
// =====================
void calculator_handle_click(int cursor_x, int cursor_y, int win_x, int win_y) {
    int rel_x = cursor_x - win_x;
    int rel_y = cursor_y - win_y;

    int bx = 2;
    int by = 4;
    int bw = 5;
    int bh = 2;

    // Row 1
    if (rel_y >= by && rel_y < by + bh) {
        if      (rel_x >= bx      && rel_x < bx + bw)      calc_input_digit(7);
        else if (rel_x >= bx + 6  && rel_x < bx + 6 + bw)  calc_input_digit(8);
        else if (rel_x >= bx + 12 && rel_x < bx + 12 + bw) calc_input_digit(9);
        else if (rel_x >= bx + 18 && rel_x < bx + 18 + bw) calc_set_operation('/');
    }
    // Row 2
    else if (rel_y >= by + 2 && rel_y < by + 2 + bh) {
        if      (rel_x >= bx      && rel_x < bx + bw)      calc_input_digit(4);
        else if (rel_x >= bx + 6  && rel_x < bx + 6 + bw)  calc_input_digit(5);
        else if (rel_x >= bx + 12 && rel_x < bx + 12 + bw) calc_input_digit(6);
        else if (rel_x >= bx + 18 && rel_x < bx + 18 + bw) calc_set_operation('*');
    }
    // Row 3
    else if (rel_y >= by + 4 && rel_y < by + 4 + bh) {
        if      (rel_x >= bx      && rel_x < bx + bw)      calc_input_digit(1);
        else if (rel_x >= bx + 6  && rel_x < bx + 6 + bw)  calc_input_digit(2);
        else if (rel_x >= bx + 12 && rel_x < bx + 12 + bw) calc_input_digit(3);
        else if (rel_x >= bx + 18 && rel_x < bx + 18 + bw) calc_set_operation('-');
    }
    // Row 4
    else if (rel_y >= by + 6 && rel_y < by + 6 + bh) {
        if      (rel_x >= bx      && rel_x < bx + bw)      calc_clear();
        else if (rel_x >= bx + 6  && rel_x < bx + 6 + bw)  calc_input_digit(0);
        else if (rel_x >= bx + 12 && rel_x < bx + 12 + bw) calc_equals();
        else if (rel_x >= bx + 18 && rel_x < bx + 18 + bw) calc_set_operation('+');
    }
}
