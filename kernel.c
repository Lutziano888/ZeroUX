#include "vga.h"
#include "shell.h"
#include "gui.h"

// Multiboot header
struct multiboot_header {
    unsigned int magic;
    unsigned int flags;
    unsigned int checksum;
} __attribute__((packed));

__attribute__((section(".multiboot")))
struct multiboot_header mb_header = {
    .magic = 0x1BADB002,
    .flags = 0x00000000,
    .checksum = -(0x1BADB002 + 0x00000000)
};

void kernel_main() {
    vga_clear();
    vga_print("Booting zeroUX...\n\n");
    
    // Starte GUI statt Shell
    gui_run();
    
    // Falls GUI beendet wird, starte Shell
    shell();
}