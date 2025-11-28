#include "vga.h"
#include "shell.h"

void kernel_main() {
    vga_clear();
    vga_print("Booting zeroUX...\n\n");
    shell();
}
