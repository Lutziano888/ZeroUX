#include "calculator.h"
#include "../gui.h"
#include "../widgets.h"
#include "../string.h"

// State
typedef struct {
    char display[32];
    int value;
    int last_value;
    char operation;
    int new_number;
    WindowStyle* current_theme;  // Aktuelles Theme
} CalcState;

static CalcState calc;

// Styles
static WidgetStyle STYLE_DISPLAY;
static WidgetStyle STYLE_NUMBER;
static WidgetStyle STYLE_OPERATOR;
static WidgetStyle STYLE_CLEAR;
static WidgetStyle STYLE_EQUALS;

static void setup_styles() {
    STYLE_DISPLAY.bg_color = 0x70;
    STYLE_DISPLAY.border_color = 0x70;
    
    STYLE_NUMBER.bg_color = 0x0F;
    STYLE_OPERATOR.bg_color = 0x0E;
    STYLE_CLEAR.bg_color = 0x0C;
    STYLE_EQUALS.bg_color = 0x0A;
}

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
    if (calc.operation == '+') {
        calc.value = calc.last_value + calc.value;
    } else if (calc.operation == '-') {
        calc.value = calc.last_value - calc.value;
    } else if (calc.operation == '*') {
        calc.value = calc.last_value * calc.value;
    } else if (calc.operation == '/') {
        if (calc.value != 0) {
            calc.value = calc.last_value / calc.value;
        } else {
            strcpy(calc.display, "Error");
            return;
        }
    }
    int_to_str(calc.value, calc.display);
}

static void calc_set_operation(char op) {
    if (calc.operation && !calc.new_number) {
        calc_do_operation();
    }
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
    
    // Wähle ein Theme - hier kannst du ändern!
    // Optionen: &WINDOW_BLUE, &WINDOW_DARK, &WINDOW_LIGHT, &WINDOW_GREEN, &WINDOW_PURPLE, &WINDOW_RED
    calc.current_theme = &WINDOW_DARK;
    
    setup_styles();
}

void calculator_draw(int x, int y, int w, int h, int is_selected) {
    // Zeichne Fenster mit Theme
    draw_window(x, y, w, h, " Calculator ", calc.current_theme, is_selected);
    
    // Display mit Content-Hintergrund
    int display_y = y + 2;
    draw_box(x + 2, display_y, w - 4, 3, 0x70);
    draw_text_at(x + 4, display_y + 1, calc.display, 0x70);
    
    int by = y + 6;
    
    // Reihe 1: 7 8 9 /
    widget_button(x + 4, by, "7", &STYLE_NUMBER);
    widget_button(x + 8, by, "8", &STYLE_NUMBER);
    widget_button(x + 12, by, "9", &STYLE_NUMBER);
    widget_button(x + 16, by, "/", &STYLE_OPERATOR);
    
    // Reihe 2: 4 5 6 *
    widget_button(x + 4, by + 1, "4", &STYLE_NUMBER);
    widget_button(x + 8, by + 1, "5", &STYLE_NUMBER);
    widget_button(x + 12, by + 1, "6", &STYLE_NUMBER);
    widget_button(x + 16, by + 1, "*", &STYLE_OPERATOR);
    
    // Reihe 3: 1 2 3 -
    widget_button(x + 4, by + 2, "1", &STYLE_NUMBER);
    widget_button(x + 8, by + 2, "2", &STYLE_NUMBER);
    widget_button(x + 12, by + 2, "3", &STYLE_NUMBER);
    widget_button(x + 16, by + 2, "-", &STYLE_OPERATOR);
    
    // Reihe 4: 0 C = +
    widget_button(x + 4, by + 3, "0", &STYLE_NUMBER);
    widget_button(x + 8, by + 3, "C", &STYLE_CLEAR);
    widget_button(x + 12, by + 3, "=", &STYLE_EQUALS);
    widget_button(x + 16, by + 3, "+", &STYLE_OPERATOR);
}

void calculator_handle_click(int cursor_x, int cursor_y, int win_x, int win_y) {
    int rel_x = cursor_x - win_x;
    int rel_y = cursor_y - win_y;
    
    if (rel_y < 6 || rel_y > 9) return;
    
    int btn_y = rel_y - 6;
    
    // Reihe 0: 7 8 9 /
    if (btn_y == 0) {
        if (check_button_click(cursor_x, cursor_y, win_x + 4, win_y + 6, "7")) calc_input_digit(7);
        else if (check_button_click(cursor_x, cursor_y, win_x + 8, win_y + 6, "8")) calc_input_digit(8);
        else if (check_button_click(cursor_x, cursor_y, win_x + 12, win_y + 6, "9")) calc_input_digit(9);
        else if (check_button_click(cursor_x, cursor_y, win_x + 16, win_y + 6, "/")) calc_set_operation('/');
    }
    // Reihe 1: 4 5 6 *
    else if (btn_y == 1) {
        if (check_button_click(cursor_x, cursor_y, win_x + 4, win_y + 7, "4")) calc_input_digit(4);
        else if (check_button_click(cursor_x, cursor_y, win_x + 8, win_y + 7, "5")) calc_input_digit(5);
        else if (check_button_click(cursor_x, cursor_y, win_x + 12, win_y + 7, "6")) calc_input_digit(6);
        else if (check_button_click(cursor_x, cursor_y, win_x + 16, win_y + 7, "*")) calc_set_operation('*');
    }
    // Reihe 2: 1 2 3 -
    else if (btn_y == 2) {
        if (check_button_click(cursor_x, cursor_y, win_x + 4, win_y + 8, "1")) calc_input_digit(1);
        else if (check_button_click(cursor_x, cursor_y, win_x + 8, win_y + 8, "2")) calc_input_digit(2);
        else if (check_button_click(cursor_x, cursor_y, win_x + 12, win_y + 8, "3")) calc_input_digit(3);
        else if (check_button_click(cursor_x, cursor_y, win_x + 16, win_y + 8, "-")) calc_set_operation('-');
    }
    // Reihe 3: 0 C = +
    else if (btn_y == 3) {
        if (check_button_click(cursor_x, cursor_y, win_x + 4, win_y + 9, "0")) calc_input_digit(0);
        else if (check_button_click(cursor_x, cursor_y, win_x + 8, win_y + 9, "C")) calc_clear();
        else if (check_button_click(cursor_x, cursor_y, win_x + 12, win_y + 9, "=")) calc_equals();
        else if (check_button_click(cursor_x, cursor_y, win_x + 16, win_y + 9, "+")) calc_set_operation('+');
    }
}