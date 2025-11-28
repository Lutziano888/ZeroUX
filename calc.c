#include "string.h"
#include "vga.h"
#include "input.h"

char read_char() {
    char buf[2];
    read_input(buf, 2);
    return buf[0];
}

void calc_run() {
    char a_str[32];
    char b_str[32];
    char op;

    vga_println("Calculator");
    vga_println("----------");

    vga_print("Enter first number: ");
    read_input(a_str, 32);

    vga_print("Operator (+ - * /): ");
    op = read_char();

    vga_print("\nEnter second number: ");
    read_input(b_str, 32);

    int a = str_to_int(a_str);
    int b = str_to_int(b_str);
    int result = 0;

    switch(op) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/':
            if (b == 0) {
                vga_println("Error: Division by zero!");
                return;
            }
            result = a / b;
            break;
        default:
            vga_println("Invalid operator!");
            return;
    }

    char out[32];
    int_to_str(result, out);

    vga_print("Result: ");
    vga_println(out);
}
