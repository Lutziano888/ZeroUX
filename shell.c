#include "shell.h"
#include "vga.h"
#include "input.h"
#include "string.h"
#include "rtc.h"
#include "calc.h"

void shell() {
    char input[128];

    vga_print("zeroUX Shell v1.0\n");
    vga_print("type 'help' for commands.\n\n");

    while (1) {
        vga_print("> ");
        read_input(input, 128);

        if (strcmp(input, "help") == 0) {
            vga_print("Commands:\n");
            vga_print("  help   - Show this help\n");
            vga_print("  calc   - Calculator\n");
            vga_print("  clear  - Clear screen\n");
            vga_print("  about  - Info about zeroUX\n");
            vga_print("  echo X - Print X\n");
            vga_print("  time   - Show system time\n");
            vga_print("  reboot - Restart system\n");
        }

        else if (strcmp(input, "calc") == 0) {
            calc_run();
        }

        else if (strcmp(input, "clear") == 0) {
            vga_clear();
        }

        else if (strcmp(input, "about") == 0) {
            vga_print("zeroUX OS - simple C kernel\n");
        }

        else if (strncmp(input, "echo ", 5) == 0) {
            vga_print(input + 5);
            vga_print("\n");
        }

        else if (strcmp(input, "time") == 0) {
            int h, m, s;
            char hb[4], mb[4], sb[4];
            char output[32];

            rtc_get_time(&h, &m, &s);

            int_to_str(h, hb);
            int_to_str(m, mb);
            int_to_str(s, sb);

            strcpy(output, hb);
            strcat(output, ":");
            strcat(output, mb);
            strcat(output, ":");
            strcat(output, sb);
            strcat(output, "\n");

            vga_print(output);
        }

        else if (strcmp(input, "reboot") == 0) {
            asm volatile("int $0x19");
        }

        else {
            vga_print("Unknown command. Type 'help'\n");
        }
    }
}
