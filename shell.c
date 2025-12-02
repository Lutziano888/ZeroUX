#include "shell.h"
#include "vga.h"
#include "input.h"
#include "string.h"
#include "rtc.h"
#include "calc.h"
#include "fs.h"
#include "gui.h"

void shell() {
    char input[128];

    vga_print("zeroUX Shell v1.2\n");
    vga_print("type 'help' for commands.\n\n");

    // Filesystem initialisieren
    fs_init();

    while (1) {
        char pwd[128];
        fs_pwd(pwd);
        vga_print(pwd);
        vga_print(" > ");
        
        read_input(input, 128);

        if (strcmp(input, "help") == 0) {
            vga_print("Commands:\n");
            vga_print("  help      - Show this help\n");
            vga_print("  calc      - Calculator\n");
            vga_print("  clear     - Clear screen\n");
            vga_print("  about     - Info about zeroUX\n");
            vga_print("  echo X    - Print X\n");
            vga_print("  time      - Show system time\n");
            vga_print("  gui       - Start GUI Desktop\n");
            vga_print("  reboot    - Restart system\n");
            vga_print("\nFilesystem:\n");
            vga_print("  ls        - List files\n");
            vga_print("  pwd       - Print working directory\n");
            vga_print("  cd DIR    - Change directory\n");
            vga_print("  mkdir DIR - Create directory\n");
            vga_print("  touch F   - Create file\n");
            vga_print("  rm FILE   - Remove file/dir\n");
            vga_print("  cat FILE  - Show file content\n");
            vga_print("  write F   - Write to file\n");
        }

        else if (strcmp(input, "gui") == 0) {
            gui_init();
            gui_run();
            // Nach GUI zurück zu Text-Modus
            vga_clear();
            vga_print("Returned from GUI mode\n\n");
        }

        else if (strcmp(input, "calc") == 0) {
            calc_run();
        }

        else if (strcmp(input, "clear") == 0) {
            vga_clear();
        }

        else if (strcmp(input, "about") == 0) {
            vga_print("zeroUX OS - Simple C kernel with GUI\n");
            vga_print("Features: Shell, Filesystem, GUI Desktop\n");
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

        // ===== FILESYSTEM BEFEHLE =====
        else if (strcmp(input, "ls") == 0) {
            fs_ls();
        }

        else if (strcmp(input, "pwd") == 0) {
            char pwd_buf[128];
            fs_pwd(pwd_buf);
            vga_println(pwd_buf);
        }

        else if (strncmp(input, "cd ", 3) == 0) {
            fs_cd(input + 3);
        }

        else if (strncmp(input, "mkdir ", 6) == 0) {
            fs_mkdir(input + 6);
        }

        else if (strncmp(input, "touch ", 6) == 0) {
            fs_touch(input + 6);
        }

        else if (strncmp(input, "rm ", 3) == 0) {
            fs_rm(input + 3);
        }

        else if (strncmp(input, "cat ", 4) == 0) {
            fs_cat(input + 4);
        }

        else if (strncmp(input, "write ", 6) == 0) {
            char* filename = input + 6;
            
            vga_print("Enter content (max 1024 chars):\n");
            char content[1024];
            read_input(content, 1024);
            
            fs_write(filename, content);
        }

        else if (input[0] == 0) {
            // Leere Eingabe ignorieren
        }

        else {
            vga_print("Unknown command. Type 'help'\n");
        }
    }
}