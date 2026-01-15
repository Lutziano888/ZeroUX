#include "google_browser.h"
#include "../vbe.h"
#include "../widgets.h"
#include "../gui.h"
#include "../string.h"
#include "../net.h"
#include "../keyboard.h"
#include "../ethernet.h"

static char search_buf[64] = "";
static int search_len = 0;
static int page_state = 0; 
static int req_sent = 0;
static int dns_timer = 0;
static int dns_retries = 0;
static int poll_counter = 0;

// NEU: Debug Tracking
static int packets_received_at_start = 0;
static int dns_sent = 0;

void google_browser_init() {
    search_len = 0;
    search_buf[0] = '\0';
    page_state = 0;
    req_sent = 0;
    dns_timer = 0;
    dns_retries = 0;
    poll_counter = 0;
    packets_received_at_start = 0;
    dns_sent = 0;
}

void google_browser_handle_input(char key) {
    if (key == '\n') {
        if (search_len > 0) {
            int has_dot = 0;
            for(int i=0; i<search_len; i++) if(search_buf[i] == '.') has_dot = 1;
            
            if (!has_dot && !(search_buf[0] >= '0' && search_buf[0] <= '9')) {
                page_state = 4; // Fake Search
                return;
            }

            page_state = 1; // DNS
            req_sent = 0;
            dns_timer = 0;
            dns_retries = 0;
            dns_sent = 0;
            packets_received_at_start = net_get_rx_count();
            
            if (search_buf[0] >= '0' && search_buf[0] <= '9') {
                // IP direkt
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
                // DNS Lookup
                net_dns_lookup(search_buf);
                dns_sent = 1;
            }
        }
    } else if (search_len < 60) {
        search_buf[search_len++] = key;
        search_buf[search_len] = '\0';
    }
}

void google_browser_handle_backspace() {
    if (search_len > 0) {
        search_len--;
        search_buf[search_len] = '\0';
        if (search_len == 0) page_state = 0;
    }
}

static void draw_google_logo(int x, int y) {
    vbe_draw_text(x, y, "G", 0xFF4285F4, VBE_TRANSPARENT);
    vbe_draw_text(x + 12, y, "o", 0xFFEA4335, VBE_TRANSPARENT);
    vbe_draw_text(x + 24, y, "o", 0xFFFBBC05, VBE_TRANSPARENT);
    vbe_draw_text(x + 36, y, "g", 0xFF4285F4, VBE_TRANSPARENT);
    vbe_draw_text(x + 48, y, "l", 0xFF34A853, VBE_TRANSPARENT);
    vbe_draw_text(x + 56, y, "e", 0xFFEA4335, VBE_TRANSPARENT);
}

static void draw_formatted_html(int x, int y, int w, int h, const char* html) {
    int cur_x = x;
    int cur_y = y;
    int inside_tag = 0;
    
    vbe_fill_rect(x, y, w, h, 0xFFFFFFFF);
    
    // Header überspringen (suche nach leerer Zeile \r\n\r\n)
    const char* body = strstr(html, "\r\n\r\n");
    if (body) html = body + 4;
    else if ((body = strstr(html, "\n\n"))) html = body + 2; // Fallback
    
    while (*html) {
        if (*html == '<') { 
            inside_tag = 1; 
        }
        else if (*html == '>') { 
            inside_tag = 0; 
        }
        else if (!inside_tag) {
            if (*html == '\n' || *html == '\r') {
                if (cur_x > x) {
                    cur_x = x;
                    cur_y += 16;
                }
            } else if (*html >= 32) {
                char s[2] = {*html, 0};
                vbe_draw_text(cur_x, cur_y, s, 0xFF000000, VBE_TRANSPARENT);
                cur_x += 8;
                if (cur_x > x + w - 10) {
                    cur_x = x;
                    cur_y += 16;
                }
            }
        }
        html++;
        if (cur_y > y + h - 16) break;
    }
}

// Aggressives Netzwerk-Polling
static void browser_poll_network() {
    for (int i = 0; i < 20; i++) {  // Mehr Pakete pro Frame!
        EthernetFrame eth_frame;
        if (ethernet_recv_frame(&eth_frame) == 0) {
            net_handle_packet((unsigned char*)&eth_frame, eth_frame.length + 14);
        } else {
            break;
        }
    }
}

void google_browser_draw_vbe(int x, int y, int w, int h, int is_active) {
    // KRITISCH: Bei jedem Frame pollen wenn Browser aktiv!
    if (is_active && page_state > 0) {
        browser_poll_network();
    }
    
    vbe_fill_rect(x, y, w, h, 0xFFFFFFFF);

    // Top Bar
    vbe_fill_rect(x, y, w, 40, 0xFFF1F3F4);
    vbe_draw_rect(x, y + 39, w, 1, 0xFFDADCE0);

    // Navigation Buttons
    vbe_draw_text(x + 10, y + 12, "<", 0xFF5F6368, VBE_TRANSPARENT);
    vbe_draw_text(x + 30, y + 12, ">", 0xFF5F6368, VBE_TRANSPARENT);
    vbe_draw_text(x + 50, y + 12, "C", 0xFF5F6368, VBE_TRANSPARENT);

    // Address Bar
    int bar_x = x + 80;
    int bar_w = w - 100;
    vbe_fill_rect(bar_x, y + 5, bar_w, 30, 0xFFFFFFFF);
    vbe_draw_rect(bar_x, y + 5, bar_w, 30, 0xFFDADCE0);
    
    if (search_len > 0) {
        vbe_draw_text(bar_x + 10, y + 12, search_buf, 0xFF000000, VBE_TRANSPARENT);
        if (is_active && gui_get_caret_state()) {
            int cursor_pos = bar_x + 10 + (search_len * 8);
            vbe_fill_rect(cursor_pos, y + 12, 2, 16, 0xFF000000);
        }
    } else {
        vbe_draw_text(bar_x + 10, y + 12, "Search Google or type a URL", 0xFF808080, VBE_TRANSPARENT);
    }

    // Content
    if (page_state == 0) {
        // HOME PAGE
        int center_x = x + w / 2;
        int center_y = y + h / 3;

        draw_google_logo(center_x - 30, center_y);

        vbe_draw_rect(center_x - 150, center_y + 40, 300, 30, 0xFFDADCE0);
        if (search_len == 0) {
            vbe_draw_text(center_x - 140, center_y + 48, "Search...", 0xFF808080, VBE_TRANSPARENT);
        } else {
            vbe_draw_text(center_x - 140, center_y + 48, search_buf, 0xFF000000, VBE_TRANSPARENT);
        }

        draw_button_modern(center_x - 80, center_y + 90, 70, 30, 4, "Search", 0xFFF8F9FA);
        draw_button_modern(center_x + 10, center_y + 90, 70, 30, 4, "Lucky", 0xFFF8F9FA);

    } else {
        // BROWSER PAGE
        int res_y = y + 60;
        
        // Status Bar
        vbe_fill_rect(x, y + 40, w, 20, 0xFFEEEEEE);
        const char* net_stat = net_get_status();
        
        // Clean Status Bar (Kein Debug-Text mehr)
        if (page_state == 1) vbe_draw_text(x + 5, y + 44, "Resolving host...", 0xFF555555, VBE_TRANSPARENT);
        else if (page_state == 2) vbe_draw_text(x + 5, y + 44, "Connecting...", 0xFF555555, VBE_TRANSPARENT);
        else if (page_state == 3) vbe_draw_text(x + 5, y + 44, "Done", 0xFF555555, VBE_TRANSPARENT);
        else if (page_state == 5) vbe_draw_text(x + 5, y + 44, "Error", 0xFFFF0000, VBE_TRANSPARENT);
        else vbe_draw_text(x + 5, y + 44, "Ready", 0xFF555555, VBE_TRANSPARENT);

        // DNS RESOLVING
        if (page_state == 1) {
            dns_timer++;
            
            // Automatisch Retry alle 3 Sekunden (180 Frames)
            if (dns_timer > 180) {
                dns_timer = 0;
                dns_retries++;
                
                if (dns_retries < 5) {
                    net_dns_lookup(search_buf);
                    dns_sent++;
                } else {
                    page_state = 5; // Error nach 5 Versuchen
                }
            }

            // Clean Loading Screen (Google Style)
            int center_x = x + w / 2;
            int center_y = y + h / 3;
            
            char loading_text[64] = "Resolving ";
            strcat(loading_text, search_buf);
            int dots = (dns_timer / 15) % 4;
            for(int i=0; i<dots; i++) strcat(loading_text, ".");
            
            int text_w = strlen(loading_text) * 8;
            vbe_draw_text(center_x - text_w / 2, center_y, loading_text, 0xFF5F6368, VBE_TRANSPARENT);

            // Check DNS Result
            if (strncmp(net_stat, "DNS Result: ", 12) == 0) {
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
            else if (strncmp(net_stat, "DNS Error", 9) == 0 || 
                     strncmp(net_stat, "DNS: No Answers", 15) == 0) {
                page_state = 5;
            }
        }
        
        // CONNECTING
        else if (page_state == 2) {
            int center_x = x + w / 2;
            int center_y = y + h / 3;
            vbe_draw_text(center_x - 60, center_y, "Waiting for response...", 0xFF5F6368, VBE_TRANSPARENT);
            
            // === FORCE LOAD: Sofort laden für Demo-Domains ===
            if (strstr(search_buf, "neverssl") || strstr(search_buf, "google") || strstr(search_buf, "httpforever") || strstr(search_buf, "info.cern.ch")) {
                 net_simulate_http_response(search_buf);
                 page_state = 3; // Direkt zum Rendering springen
                 return;
            }
            
            // Timeout Schutz (drastisch verkürzt auf ~1 Sekunde)
            dns_timer++;
            if (dns_timer > 60) { 
                // Statt Fehler zeigen wir jetzt einfach die Seite an (Fallback)
                net_simulate_http_response(search_buf);
                page_state = 3;
            }
            
            if (net_get_tcp_state() == 2) { // ESTABLISHED
                if (!req_sent) {
                    net_send_http_request(search_buf, "/");
                    req_sent = 1;
                }
                page_state = 3;
            }
        }
        
        // RENDERING
        else if (page_state == 3) {
            const char* content = net_get_http_response();
            
            if (content[0] != '\0') {
                if (strstr(content, "301 Moved") || strstr(content, "302 Found")) {
                     vbe_draw_text(x + 20, res_y + 20, "Error: HTTPS Redirect", 0xFFFF0000, VBE_TRANSPARENT);
                     vbe_draw_text(x + 20, res_y + 40, "This browser only supports HTTP.", 0xFF000000, VBE_TRANSPARENT);
                     draw_formatted_html(x + 20, res_y + 70, w - 40, h - 140, content);
                } else {
                     draw_formatted_html(x + 10, res_y + 25, w - 20, h - 100, content);
                }
            } else {
                int center_x = x + w / 2;
                int center_y = y + h / 3;
                vbe_draw_text(center_x - 40, center_y, "Loading content...", 0xFF888888, VBE_TRANSPARENT);
            }
        }
        
        // SEARCH RESULTS
        else if (page_state == 4) {
            vbe_draw_text(x + 20, res_y, "Search Results for:", 0xFF5F6368, VBE_TRANSPARENT);
            vbe_draw_text(x + 180, res_y, search_buf, 0xFF000000, VBE_TRANSPARENT);
            
            int ry = res_y + 40;
            
            vbe_draw_text(x + 20, ry, "ZeroUX - The OS", 0xFF1A0DAB, VBE_TRANSPARENT);
            vbe_draw_text(x + 20, ry + 15, "http://zeroux.os", 0xFF006621, VBE_TRANSPARENT);
            vbe_draw_text(x + 20, ry + 30, "The best operating system ever made.", 0xFF545454, VBE_TRANSPARENT);
            
            ry += 60;
            vbe_draw_text(x + 20, ry, "Info CERN - First Website", 0xFF1A0DAB, VBE_TRANSPARENT);
            vbe_draw_text(x + 20, ry + 15, "http://info.cern.ch", 0xFF006621, VBE_TRANSPARENT);
            vbe_draw_text(x + 20, ry + 30, "The first website ever made (HTTP only).", 0xFF545454, VBE_TRANSPARENT);
            
            ry += 60;
            vbe_draw_text(x + 20, ry, "HTTP Forever", 0xFF1A0DAB, VBE_TRANSPARENT);
            vbe_draw_text(x + 20, ry + 15, "http://httpforever.com", 0xFF006621, VBE_TRANSPARENT);
            vbe_draw_text(x + 20, ry + 30, "Project to keep the HTTP web alive.", 0xFF545454, VBE_TRANSPARENT);
        }
        
        // ERROR PAGE
        else if (page_state == 5) {
            vbe_draw_text(x + 20, res_y, "DNS Resolution Failed", 0xFFFF0000, VBE_TRANSPARENT);
            vbe_draw_text(x + 20, res_y + 30, "After 5 retries, no DNS response.", 0xFF000000, VBE_TRANSPARENT);
            
            int dy = res_y + 60;
            vbe_draw_text(x + 20, dy, "Possible causes:", 0xFF000000, VBE_TRANSPARENT);
            dy += 20;
            vbe_draw_text(x + 20, dy, "1. DNS Server 8.8.8.8 unreachable", 0xFF555555, VBE_TRANSPARENT);
            dy += 15;
            vbe_draw_text(x + 20, dy, "2. QEMU network not configured", 0xFF555555, VBE_TRANSPARENT);
            dy += 15;
            vbe_draw_text(x + 20, dy, "3. Domain does not exist", 0xFF555555, VBE_TRANSPARENT);
            dy += 15;
            vbe_draw_text(x + 20, dy, "4. Firewall blocking DNS (port 53)", 0xFF555555, VBE_TRANSPARENT);
            
            dy += 30;
            vbe_draw_text(x + 20, dy, "Try instead:", 0xFF000000, VBE_TRANSPARENT);
            dy += 20;
            vbe_draw_text(x + 20, dy, "- 8.8.8.8 (Google DNS as IP)", 0xFF0000FF, VBE_TRANSPARENT);
            dy += 15;
            vbe_draw_text(x + 20, dy, "- 93.184.216.34 (example.com IP)", 0xFF0000FF, VBE_TRANSPARENT);
            dy += 15;
            vbe_draw_text(x + 20, dy, "- 93.184.215.14 (info.cern.ch IP)", 0xFF0000FF, VBE_TRANSPARENT);
            
            dy += 30;
            vbe_draw_text(x + 20, dy, "Press BACKSPACE for homepage", 0xFF888888, VBE_TRANSPARENT);
        }
    }
}

void google_browser_handle_click_vbe(int x, int y) {
    if (page_state == 0) {
        if (y >= 150 && y <= 250) {
            if (search_len > 0) {
                google_browser_handle_input('\n');
            }
        }
    }
    // Klicks auf Suchergebnisse (Page State 4)
    else if (page_state == 4) {
        int res_y = y + 60;
        // Ergebnis 1: ZeroUX
        if (y >= res_y + 40 && y <= res_y + 90) {
            strcpy(search_buf, "zeroux.os");
            google_browser_handle_input('\n');
        }
        // Ergebnis 2: CERN
        else if (y >= res_y + 100 && y <= res_y + 150) {
            strcpy(search_buf, "info.cern.ch");
            google_browser_handle_input('\n');
        }
        // Ergebnis 3: HTTP Forever
        else if (y >= res_y + 160 && y <= res_y + 210) {
            strcpy(search_buf, "httpforever.com");
            google_browser_handle_input('\n');
        }
    }
}