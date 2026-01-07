#include "vbe.h"
#include "gui.h"
#include "ethernet.h"
#include "string.h"
#include "port.h"

// Serial port debugging
static void serial_putchar(char c) {
    while (!(inb(0x3F8 + 5) & 0x20)); // Wait for transmit buffer empty
    outb(0x3F8, c);
}

static void serial_puts(const char* s) {
    while (*s) {
        if (*s == '\n') serial_putchar('\r');
        serial_putchar(*s++);
    }
}

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
    // Initialize serial for debugging
    serial_puts("zeroUX kernel starting...\n");
    
    // Initialize Ethernet network interface
    serial_puts("Initializing Ethernet...\n");
    ethernet_init();
    serial_puts("Ethernet init done\n");
    
    // Initialize VBE graphics mode
    serial_puts("Initializing VBE...\n");
    int vbe_ok = vbe_init();
    serial_puts("VBE init done\n");
    
    // Clear screen with dark background
    vbe_clear(WIN10_BG);
    serial_puts("Screen cleared\n");
    
    // Show boot message
    vbe_draw_text(350, 340, "Initializing zeroUX...", VBE_WHITE, VBE_TRANSPARENT);
    serial_puts("Boot message drawn\n");
    
    // Simple delay
    for (volatile int i = 0; i < 50000000; i++);
    
    // Start GUI
    serial_puts("Starting GUI...\n");
    gui_run();
    serial_puts("GUI exited\n");
    
    // If GUI exits, show message
    vbe_clear(VBE_BLACK);
    vbe_draw_text(400, 350, "System Halted", VBE_WHITE, VBE_TRANSPARENT);
    
    serial_puts("System halted\n");
    while(1) {
        __asm__ volatile("hlt");
    }
}