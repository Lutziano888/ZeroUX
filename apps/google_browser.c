#include "google_browser.h"
#include "../vbe.h"
#include "../widgets.h"
#include "../gui.h"
#include "../string.h"
#include "../net.h"

static char search_buf[64] = "";
static int search_len = 0;
static int page_state = 0; // 0=Home, 1=DNS, 2=Connecting, 3=Rendering, 4=Search, 5=Error
static int req_sent = 0;
static int dns_timer = 0;
static int dns_retries = 0;

void google_browser_init() {
    search_len = 0;
    search_buf[0] = '\0';
    page_state = 0;
    req_sent = 0;
    dns_timer = 0;
    dns_retries = 0;
}

void google_browser_handle_input(char key) {
    if (key == '\n') {
        // Enter gedrückt -> Suche starten
        if (search_len > 0) {
            // Unterscheidung: URL (hat Punkt) oder Suchbegriff (kein Punkt)
            int has_dot = 0;
            for(int i=0; i<search_len; i++) if(search_buf[i] == '.') has_dot = 1;
            
            if (!has_dot && !(search_buf[0] >= '0' && search_buf[0] <= '9')) {
                page_state = 4; // Fake Search Results
                return;
            }

            page_state = 1; // Start with DNS
            req_sent = 0;
            dns_timer = 0;
            dns_retries = 0;
            
            // Prüfen ob es eine IP ist (einfacher Check: beginnt mit Zahl)
            if (search_buf[0] >= '0' && search_buf[0] <= '9') {
                // Direkt verbinden (IP eingegeben)
                uint8_t target_ip[4];
                int val = 0, idx = 0;
                char* ptr = search_buf;
                while (*ptr && idx < 4) {
                    if (*ptr == '.') { target_ip[idx++] = val; val = 0; }
                    else if (*ptr >= '0' && *ptr <= '9') { val = val * 10 + (*ptr - '0'); }
                    ptr++;
                }
                target_ip[idx] = val;
                
                net_tcp_connect(target_ip, 80);
                page_state = 2; // Connecting
            } else {
                // Domain eingegeben -> DNS Lookup
                net_dns_lookup(search_buf);
            }
        }
    } else if (search_len < 60) {
        // Text eingeben
        search_buf[search_len++] = key;
        search_buf[search_len] = '\0';
    }
}

void google_browser_handle_backspace() {
    if (search_len > 0) {
        search_len--;
        search_buf[search_len] = '\0';
        // Wenn leer, zurück zur Startseite
        if (search_len == 0) page_state = 0;
    }
}

static void draw_google_logo(int x, int y) {
    // Einfaches "Google" in Farben
    vbe_draw_text(x, y, "G", 0xFF4285F4, VBE_TRANSPARENT);
    vbe_draw_text(x + 12, y, "o", 0xFFEA4335, VBE_TRANSPARENT);
    vbe_draw_text(x + 24, y, "o", 0xFFFBBC05, VBE_TRANSPARENT);
    vbe_draw_text(x + 36, y, "g", 0xFF4285F4, VBE_TRANSPARENT);
    vbe_draw_text(x + 48, y, "l", 0xFF34A853, VBE_TRANSPARENT);
    vbe_draw_text(x + 56, y, "e", 0xFFEA4335, VBE_TRANSPARENT);
}

// Einfacher HTML Renderer (entfernt Tags)
static void draw_formatted_html(int x, int y, int w, int h, const char* html) {
    int cur_x = x;
    int cur_y = y;
    int inside_tag = 0;
    
    // Hintergrund für Content-Bereich löschen
    vbe_fill_rect(x, y, w, h, 0xFFFFFFFF);
    
    while (*html) {
        if (*html == '<') { 
            inside_tag = 1; 
        }
        else if (*html == '>') { 
            inside_tag = 0; 
        }
        else if (!inside_tag) {
            if (*html == '\n' || *html == '\r') {
                // Zeilenumbruch nur machen, wenn wir nicht eh schon am Anfang sind
                if (cur_x > x) {
                    cur_x = x;
                    cur_y += 16;
                }
            } else if (*html >= 32) { // Druckbare Zeichen
                char s[2] = {*html, 0};
                vbe_draw_text(cur_x, cur_y, s, 0xFF000000, VBE_TRANSPARENT);
                cur_x += 8;
                // Zeilenumbruch (Word Wrap simpel)
                if (cur_x > x + w - 10) {
                    cur_x = x;
                    cur_y += 16;
                }
            }
        }
        html++;
        if (cur_y > y + h - 16) break; // Ende des Fensters
    }
}

void google_browser_draw_vbe(int x, int y, int w, int h, int is_active) {
    // Hintergrund weiß
    vbe_fill_rect(x, y, w, h, 0xFFFFFFFF);

    // Top Bar (Address Leiste)
    vbe_fill_rect(x, y, w, 40, 0xFFF1F3F4);
    vbe_draw_rect(x, y + 39, w, 1, 0xFFDADCE0);

    // Zurück/Vorwärts Buttons (Fake)
    vbe_draw_text(x + 10, y + 12, "<", 0xFF5F6368, VBE_TRANSPARENT);
    vbe_draw_text(x + 30, y + 12, ">", 0xFF5F6368, VBE_TRANSPARENT);
    vbe_draw_text(x + 50, y + 12, "C", 0xFF5F6368, VBE_TRANSPARENT); // Refresh

    // Adressleiste / Suchleiste oben
    int bar_x = x + 80;
    int bar_w = w - 100;
    vbe_fill_rect(bar_x, y + 5, bar_w, 30, 0xFFFFFFFF);
    vbe_draw_rect(bar_x, y + 5, bar_w, 30, 0xFFDADCE0); // Rahmen
    
    // Text in der Leiste
    if (search_len > 0) {
        vbe_draw_text(bar_x + 10, y + 12, search_buf, 0xFF000000, VBE_TRANSPARENT);
        // Cursor
        if (is_active && gui_get_caret_state()) {
            int cursor_pos = bar_x + 10 + (search_len * 8);
            vbe_fill_rect(cursor_pos, y + 12, 2, 16, 0xFF000000);
        }
    } else {
        vbe_draw_text(bar_x + 10, y + 12, "Search Google or type a URL", 0xFF808080, VBE_TRANSPARENT);
    }

    // Inhalt
    if (page_state == 0) {
        // === HOME PAGE ===
        int center_x = x + w / 2;
        int center_y = y + h / 3;

        // Großes Logo in der Mitte (simuliert durch Text, könnte man größer skalieren wenn man Font-Scaling hätte)
        // Hier zeichnen wir es einfach mittig
        draw_google_logo(center_x - 30, center_y);

        // Suchbox in der Mitte
        vbe_draw_rect(center_x - 150, center_y + 40, 300, 30, 0xFFDADCE0);
        if (search_len == 0) {
            vbe_draw_text(center_x - 140, center_y + 48, "Search...", 0xFF808080, VBE_TRANSPARENT);
        } else {
            vbe_draw_text(center_x - 140, center_y + 48, search_buf, 0xFF000000, VBE_TRANSPARENT);
        }

        // Buttons
        draw_button_modern(center_x - 80, center_y + 90, 70, 30, 4, "Search", 0xFFF8F9FA);
        draw_button_modern(center_x + 10, center_y + 90, 70, 30, 4, "Lucky", 0xFFF8F9FA);

    } else {
        // === BROWSER PAGE ===
        int res_y = y + 60;
        
        // Status Bar
        vbe_fill_rect(x, y + 40, w, 20, 0xFFEEEEEE);
        const char* net_stat = net_get_status();
        vbe_draw_text(x + 5, y + 44, net_stat, 0xFF555555, VBE_TRANSPARENT);

        // --- STATE MACHINE ---
        
        // 1. DNS RESOLVING
        if (page_state == 1) {
            // Timeout & Retry Logik
            dns_timer++;
            if (dns_timer > 30) { // Ca. 1 Sekunde warten (schnellerer Retry)
                dns_timer = 0;
                dns_retries++;
                if (dns_retries < 4) {
                    net_dns_lookup(search_buf); // Nochmal versuchen
                }
            }

            if (dns_retries > 0 && dns_retries < 4) {
                vbe_draw_text(x + 20, res_y + 15, "Retrying DNS...", 0xFF888888, VBE_TRANSPARENT);
            }
            
            if (dns_retries >= 4) {
                page_state = 5; // Wechsel zu Error Page
            }

            // Animierte Punkte
            char loading_text[32] = "Resolving Host";
            int dots = (dns_timer / 10) % 4;
            for(int i=0; i<dots; i++) strcat(loading_text, ".");
            
            vbe_draw_text(x + 20, res_y, loading_text, 0xFF000000, VBE_TRANSPARENT);
            
            // Check Result
            if (strncmp(net_stat, "DNS Result: ", 12) == 0) {
                // Parse IP from string
                uint8_t target_ip[4];
                int val = 0, idx = 0;
                const char* ptr = net_stat + 12;
                while (*ptr && idx < 4) {
                    if (*ptr == '.') { target_ip[idx++] = val; val = 0; }
                    else if (*ptr >= '0' && *ptr <= '9') { val = val * 10 + (*ptr - '0'); }
                    ptr++;
                }
                target_ip[idx] = val;
                
                net_tcp_connect(target_ip, 80);
                page_state = 2;
            }
            else if (strncmp(net_stat, "DNS Error", 9) == 0 || strncmp(net_stat, "DNS: No Answers", 15) == 0) {
                // Sofortiger Fehler vom Netzwerkstack
                page_state = 5;
            }
        }
        
        // 2. CONNECTING
        else if (page_state == 2) {
            vbe_draw_text(x + 20, res_y, "Connecting to Server...", 0xFF000000, VBE_TRANSPARENT);
            
            if (net_get_tcp_state() == 2) { // ESTABLISHED
                if (!req_sent) {
                    net_send_http_request(search_buf, "/");
                    req_sent = 1;
                }
                page_state = 3;
            }
        }
        
        // 3. RENDERING
        else if (page_state == 3) {
            const char* content = net_get_http_response();
            if (content[0] != '\0') {
                // Prüfen auf Redirects
                if (strstr(content, "301 Moved") || strstr(content, "302 Found")) {
                     vbe_draw_text(x + 20, res_y, "Error: Site requires HTTPS (Encrypted).", 0xFFFF0000, VBE_TRANSPARENT);
                     vbe_draw_text(x + 20, res_y + 20, "This browser only supports HTTP.", 0xFF000000, VBE_TRANSPARENT);
                     draw_formatted_html(x + 20, res_y + 50, w - 40, h - 120, content);
                } else {
                     draw_formatted_html(x + 10, res_y, w - 20, h - 80, content);
                }
            } else {
                vbe_draw_text(x + 20, res_y, "Waiting for Data...", 0xFF888888, VBE_TRANSPARENT);
            }
        }
        
        // 4. SEARCH RESULTS (Simulated)
        else if (page_state == 4) {
            vbe_draw_text(x + 20, res_y, "Search Results for:", 0xFF5F6368, VBE_TRANSPARENT);
            vbe_draw_text(x + 180, res_y, search_buf, 0xFF000000, VBE_TRANSPARENT);
            
            int ry = res_y + 40;
            
            // Result 1
            vbe_draw_text(x + 20, ry, "ZeroUX - The OS", 0xFF1A0DAB, VBE_TRANSPARENT);
            vbe_draw_text(x + 20, ry + 15, "http://zeroux.os", 0xFF006621, VBE_TRANSPARENT);
            vbe_draw_text(x + 20, ry + 30, "The best operating system ever made.", 0xFF545454, VBE_TRANSPARENT);
            
            ry += 60;
            // Result 2
            vbe_draw_text(x + 20, ry, "Wikipedia - The Free Encyclopedia", 0xFF1A0DAB, VBE_TRANSPARENT);
            vbe_draw_text(x + 20, ry + 15, "http://info.cern.ch", 0xFF006621, VBE_TRANSPARENT);
            vbe_draw_text(x + 20, ry + 30, "The first website ever made (HTTP only).", 0xFF545454, VBE_TRANSPARENT);
        }
        
        // 5. ERROR PAGE
        else if (page_state == 5) {
            vbe_draw_text(x + 20, res_y, "Connection Failed", 0xFFFF0000, VBE_TRANSPARENT);
            vbe_draw_text(x + 20, res_y + 20, "Could not resolve hostname (DNS Timeout).", 0xFF000000, VBE_TRANSPARENT);
            vbe_draw_text(x + 20, res_y + 40, "1. Check your internet connection.", 0xFF555555, VBE_TRANSPARENT);
            vbe_draw_text(x + 20, res_y + 55, "2. Try a simple site like 'info.cern.ch'.", 0xFF555555, VBE_TRANSPARENT);
            vbe_draw_text(x + 20, res_y + 80, "Press ENTER to try again.", 0xFF0000FF, VBE_TRANSPARENT);
        }
    }
}

void google_browser_handle_click_vbe(int x, int y) {
    // Klick auf "Search" Button im Home Screen
    // Koordinaten sind relativ zum Fensterinhalt (ohne Titlebar)
    // Wir schätzen die Mitte basierend auf einer Fensterbreite von ca 700
    
    if (page_state == 0) {
        // Mitte ca 350
        if (y >= 150 && y <= 250) { // Grober Bereich der Buttons
            if (search_len > 0) {
                page_state = 1;
            }
        }
    }
}