#include "vbe.h"
#include "gui.h"
#include "ethernet.h"
#include "string.h"
#include "port.h"
#include "mouse.h"

// NOTLÖSUNG: pmm.c direkt einbinden, da das Makefile nicht angepasst wurde.
// Sobald das Makefile pmm.o kompiliert, ändere dies zurück zu #include "pmm.h"!
#include "pmm.c"
#include "net.c"

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

// Vereinfachte Multiboot Info Struktur
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} multiboot_info_t;

// Linker Symbol für das Ende des Kernels (wird oft im Linker-Skript definiert)
// Falls nicht vorhanden, schätzen wir sicherheitshalber.
extern uint32_t end; 

void kernel_main(unsigned long magic, unsigned long addr) {
    // Initialize serial for debugging
    serial_puts("zeroUX kernel starting...\n");
    
    // --- Memory Management Initialization ---
    multiboot_info_t* mboot_ptr = (multiboot_info_t*)addr;
    uint32_t mem_size_kb = 128 * 1024; // Fallback: 128 MB

    if (magic == 0x2BADB002 && (mboot_ptr->flags & 1)) {
        mem_size_kb = mboot_ptr->mem_lower + mboot_ptr->mem_upper;
        serial_puts("Multiboot info detected. RAM size: OK\n");
    }

    // Bitmap bei 4MB platzieren (sicherer Abstand zum Kernel bei 1MB)
    // Wir nehmen an, der Kernel ist kleiner als 3MB.
    pmm_init(mem_size_kb, 0x400000); 

    // Die ersten 5MB als belegt markieren (Kernel + Bitmap + BIOS Area)
    pmm_mark_region_used(0x000000, 0x500000);
    
    serial_puts("PMM initialized.\n");
    // ----------------------------------------

    // Initialize Ethernet network interface
    serial_puts("Initializing Ethernet...\n");
    ethernet_init();
    serial_puts("Ethernet init done\n");

    // Maus-Initialisierung mit Fehlerbehandlung
    serial_puts("Initializing Mouse...\n");
    mouse_init();
    serial_puts("Mouse init done (Polling mode)\n");
    
    // WICHTIG: Kurze Pause nach Maus-Init
    for (volatile int i = 0; i < 1000000; i++);
    
    // Initialize TCP/IP Stack
    net_init();
    
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

    // Zeige RAM Info auf dem Bildschirm
    char mem_str[32];
    strcpy(mem_str, "RAM: ");
    char num_buf[16];
    int_to_str(mem_size_kb / 1024, num_buf);
    strcat(mem_str, num_buf);
    strcat(mem_str, " MB detected");
    vbe_draw_text(10, 10, mem_str, VBE_LIGHT_GRAY, VBE_TRANSPARENT);
    
    // Network Info
    vbe_draw_text(10, 25, "Waiting for DHCP...", VBE_LIGHT_GRAY, VBE_TRANSPARENT);
    
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