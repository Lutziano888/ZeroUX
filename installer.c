#include "installer.h"
#include "vbe.h"
#include "gui.h"
#include "string.h"
#include "net.h"

// Externe Deklaration, da fs.h nicht im Kontext war, aber fs.c erweitert wurde
int fs_save_file(const char* name, const char* data, int len);

static int install_step = 0; 
// 0=Welcome, 1=Source Select, 2=Disk Select, 3=Copying, 4=Done
// 10=Network IP Input, 11=Downloading

static int install_progress = 0;
static int timer = 0;
static char ip_buffer[32] = "";
static int ip_len = 0;
static char status_msg[64] = "Ready.";

void installer_init() {
    install_step = 0;
    install_progress = 0;
    timer = 0;
    ip_len = 0; ip_buffer[0] = 0;
}

void installer_draw_vbe(int x, int y, int w, int h, int is_active) {
    // Hintergrund (Modernes Setup Blau/Lila Verlauf simuliert)
    vbe_fill_rect(x, y, w, h, 0xFF2D2D30);
    
    // Fenster Header
    vbe_fill_rect(x, y, w, 40, 0xFF007ACC);
    vbe_draw_text(x + 10, y + 12, "ZeroUX Installer Setup", 0xFFFFFFFF, VBE_TRANSPARENT);

    int cx = x + w / 2;
    int cy = y + h / 2;

    if (install_step == 0) {
        // === STEP 0: WELCOME ===
        vbe_draw_text(cx - 80, cy - 60, "Welcome to ZeroUX", 0xFFFFFFFF, VBE_TRANSPARENT);
        vbe_draw_text(cx - 120, cy - 30, "This wizard will install ZeroUX on your computer.", 0xFFCCCCCC, VBE_TRANSPARENT);
        
        draw_button_modern(cx - 50, cy + 40, 100, 30, 1, "Next >", 0xFF007ACC);
    }
    else if (install_step == 1) {
        // === STEP 1: SOURCE SELECTION ===
        vbe_draw_text(cx - 80, cy - 80, "Select Installation Source", 0xFFFFFFFF, VBE_TRANSPARENT);
        
        draw_button_modern(cx - 100, cy - 30, 200, 40, 1, "Local Media (CD/USB)", 0xFF333337);
        draw_button_modern(cx - 100, cy + 20, 200, 40, 1, "Network (Windows PC)", 0xFF333337);
        
        vbe_draw_text(cx - 140, cy + 80, "To transfer files from Windows, run:", 0xFF888888, VBE_TRANSPARENT);
        vbe_draw_text(cx - 140, cy + 95, "'python -m http.server' on your PC.", 0xFF888888, VBE_TRANSPARENT);
    }
    else if (install_step == 10) {
        // === STEP 10: NETWORK IP INPUT ===
        vbe_draw_text(cx - 100, cy - 80, "Enter Windows PC IP Address", 0xFFFFFFFF, VBE_TRANSPARENT);
        
        // Input Box
        vbe_fill_rect(cx - 100, cy - 20, 200, 30, 0xFFFFFFFF);
        vbe_draw_text(cx - 90, cy - 12, ip_buffer, 0xFF000000, VBE_TRANSPARENT);
        
        // Cursor
        if (is_active && (timer % 20 < 10)) {
            vbe_fill_rect(cx - 90 + (ip_len * 8), cy - 12, 2, 16, 0xFF000000);
        }
        timer++;

        draw_button_modern(cx - 50, cy + 40, 100, 30, 1, "Connect", 0xFF007ACC);
        vbe_draw_text(cx - 80, cy + 80, status_msg, 0xFFFF0000, VBE_TRANSPARENT);
    }
    else if (install_step == 11) {
        // === STEP 11: DOWNLOADING ===
        vbe_draw_text(cx - 60, cy - 40, "Downloading...", 0xFFFFFFFF, VBE_TRANSPARENT);
        vbe_draw_text(cx - 80, cy, status_msg, 0xFFAAAAAA, VBE_TRANSPARENT);
        
        // Spinner
        int r = 20;
        int angle = (timer * 10) % 360;
        // Simple animation logic would go here
        timer++;
        
        // Logic is handled in click/update, just drawing here
    }
    else if (install_step == 2) {
        // === STEP 2: DISK SELECTION ===
        vbe_draw_text(cx - 60, cy - 80, "Select Drive", 0xFFFFFFFF, VBE_TRANSPARENT);
        
        // Fake Disk List
        vbe_fill_rect(cx - 100, cy - 40, 200, 30, 0xFF3E3E42);
        vbe_draw_rect(cx - 100, cy - 40, 200, 30, 0xFF007ACC); // Selected
        vbe_draw_text(cx - 90, cy - 32, "Disk 0: 512 GB SSD (Empty)", 0xFFFFFFFF, VBE_TRANSPARENT);
        
        vbe_fill_rect(cx - 100, cy, 200, 30, 0xFF333337);
        vbe_draw_text(cx - 90, cy + 8, "Disk 1: USB Drive", 0xFF888888, VBE_TRANSPARENT);

        draw_button_modern(cx - 50, cy + 60, 100, 30, 1, "Install", 0xFF007ACC);
    }
    else if (install_step == 3) {
        // === STEP 3: COPYING FILES ===
        vbe_draw_text(cx - 60, cy - 40, "Installing ZeroUX...", 0xFFFFFFFF, VBE_TRANSPARENT);
        vbe_draw_text(cx - 50, cy - 20, "Copying system files...", 0xFFAAAAAA, VBE_TRANSPARENT);

        // Progress Bar
        vbe_fill_rect(cx - 100, cy + 10, 200, 20, 0xFF000000);
        vbe_fill_rect(cx - 100, cy + 10, install_progress * 2, 20, 0xFF00FF00);
        
        // Simulation des Fortschritts
        timer++;
        if (timer % 5 == 0) {
            install_progress++;
            if (install_progress >= 100) {
                install_step = 4;
            }
        }
    }
    else if (install_step == 4) {
        // === STEP 4: FINISHED ===
        vbe_draw_text(cx - 50, cy - 40, "Installation Complete!", 0xFF00FF00, VBE_TRANSPARENT);
        vbe_draw_text(cx - 90, cy - 10, "Please remove installation media.", 0xFFCCCCCC, VBE_TRANSPARENT);
        
        draw_button_modern(cx - 60, cy + 40, 120, 30, 1, "Reboot Now", 0xFFD9534F);
    }
}

void installer_handle_key(char c) {
    if (install_step == 10) {
        if (ip_len < 15 && ((c >= '0' && c <= '9') || c == '.')) {
            ip_buffer[ip_len++] = c;
            ip_buffer[ip_len] = 0;
        }
    }
}

void installer_handle_backspace() {
    if (install_step == 10 && ip_len > 0) {
        ip_len--;
        ip_buffer[ip_len] = 0;
    }
}

void installer_handle_click_vbe(int x, int y) {
    // Koordinaten sind relativ zum Fensterinhalt (ohne Titlebar, wenn vom Window Manager verwaltet)
    // Wir nehmen hier vereinfacht an, dass x/y relativ zum Fensterinhalt übergeben werden.
    // Da wir die Fenstergröße nicht global haben, schätzen wir die Mitte (angenommen 500x400 Fenster)
    int w = 500; 
    int h = 400;
    int cx = w / 2; // 250
    int cy = h / 2; // 200

    // Einfache Hitbox-Logik für die Buttons
    if (install_step == 0) {
        // "Next" Button bei cx-50, cy+40 (200, 240) Größe 100x30
        if (x >= 200 && x <= 300 && y >= 240 && y <= 270) {
            install_step = 1;
        }
    }
    else if (install_step == 1) {
        // Source Selection
        // Local Media (cx-100, cy-30) -> 150, 170, 200x40
        if (x >= 150 && x <= 350 && y >= 170 && y <= 210) {
            install_step = 2; // Go to Disk Select
        }
        // Network (cx-100, cy+20) -> 150, 220, 200x40
        if (x >= 150 && x <= 350 && y >= 220 && y <= 260) {
            install_step = 10; // Go to IP Input
            ip_len = 0; ip_buffer[0] = 0;
        }
    }
    else if (install_step == 10) {
        // "Connect" Button
        if (x >= 200 && x <= 300 && y >= 240 && y <= 270) {
            if (ip_len > 0) {
                install_step = 11;
                strcpy(status_msg, "Connecting...");
                
                // Parse IP
                uint8_t target_ip[4];
                int val = 0, idx = 0;
                char* ptr = ip_buffer;
                while (*ptr && idx < 4) {
                    if (*ptr == '.') { target_ip[idx++] = val; val = 0; }
                    else if (*ptr >= '0' && *ptr <= '9') { val = val * 10 + (*ptr - '0'); }
                    ptr++;
                }
                target_ip[idx] = val;

                // Try to download "package.bin" (or similar)
                net_tcp_connect(target_ip, 8000); // Default python http.server port
                
                // Simple blocking wait for demo (in real OS use state machine)
                int timeout = 1000000;
                while(timeout-- && net_get_tcp_state() != 2) {
                     // Poll network
                     // Note: In real loop this needs to call ethernet_recv
                }
                
                if (net_get_tcp_state() == 2) {
                    strcpy(status_msg, "Requesting file...");
                    net_send_http_request(ip_buffer, "/install.sys"); // File to download
                    
                    // Wait for data
                    timeout = 2000000;
                    while(timeout--) {
                        if (net_get_http_response()[0] != 0) break;
                    }
                    
                    const char* data = net_get_http_response();
                    if (data[0]) {
                        // Save to FS
                        fs_save_file("install.sys", data, strlen(data));
                        strcpy(status_msg, "Download OK!");
                        install_step = 2; // Proceed to Disk Select
                    } else {
                        strcpy(status_msg, "Error: No Data.");
                        install_step = 10;
                    }
                } else {
                    strcpy(status_msg, "Connection Failed.");
                    install_step = 10;
                }
            }
        }
    }
    else if (install_step == 2) {
        // "Install" Button bei cx-50, cy+60 (200, 260)
        if (x >= 200 && x <= 300 && y >= 260 && y <= 290) {
            install_step = 3;
            install_progress = 0;
        }
    }
    else if (install_step == 4) {
        // "Reboot" Button
        if (x >= 190 && x <= 310 && y >= 240 && y <= 270) {
            // Reboot via Keyboard Controller
            // outb(0x64, 0xFE); // Wäre der echte Reboot Befehl
            install_step = 0; // Reset für Demo
        }
    }
}