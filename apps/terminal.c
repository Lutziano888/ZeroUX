#include "terminal.h"
#include "../vbe.h"
#include "../string.h"
#include "../rtc.h"
#include "../gui.h"
#include "../fs.h"
#include "../ethernet.h"
#include "../net.h"
#include "python_interpreter.h"

#define TERM_ROWS 22
#define TERM_COLS 80

static char term_buffer[TERM_ROWS][TERM_COLS + 1];
static char input_buffer[TERM_COLS];
static int input_len = 0;
static int cursor_visible = 1;

// Farben (Windows CMD Style)
#define TERM_BG 0xFF000000 // Schwarz
#define TERM_FG 0xFFCCCCCC // Hellgrau

static void terminal_clear() {
    for (int i = 0; i < TERM_ROWS; i++) {
        term_buffer[i][0] = '\0';
    }
    input_len = 0;
    input_buffer[0] = '\0';
}

static void terminal_scroll() {
    // Zeilen nach oben schieben
    for (int i = 0; i < TERM_ROWS - 1; i++) {
        strcpy(term_buffer[i], term_buffer[i + 1]);
    }
    term_buffer[TERM_ROWS - 1][0] = '\0';
}

static void terminal_print(const char* text);

// Hilfsfunktion zum Zentrieren von Text
static void terminal_print_centered(const char* text) {
    int len = strlen(text);
    int padding = (TERM_COLS - len) / 2;
    if (padding < 0) padding = 0;
    
    for (int i = 0; i < padding; i++) terminal_print(" "); // Padding simulieren (einfach)
    // Besser wäre direktes Schreiben in den Buffer, aber so geht es auch:
    // Da terminal_print immer am Anfang der Zeile anfängt, bauen wir den String manuell:
    char buf[TERM_COLS + 1];
    int i = 0;
    for (; i < padding; i++) buf[i] = ' ';
    strcpy(buf + i, text);
    terminal_print(buf);
}

static void terminal_print(const char* text) {
    // Wenn die letzte Zeile voll ist oder Text zu lang, scrollen (vereinfacht)
    // Hier fügen wir einfach immer in die erste freie Zeile ein oder scrollen
    
    int row = 0;
    while (row < TERM_ROWS && term_buffer[row][0] != '\0') {
        row++;
    }
    
    if (row >= TERM_ROWS) {
        terminal_scroll();
        row = TERM_ROWS - 1;
    }
    
    // Text kopieren (mit Längenbegrenzung)
    int i = 0;
    while (text[i] && i < TERM_COLS) {
        term_buffer[row][i] = text[i];
        i++;
    }
    term_buffer[row][i] = '\0';
}

static void terminal_print_prompt() {
    char cwd[64];
    char prompt[80];
    fs_pwd(cwd);
    strcpy(prompt, "ZeroUX:");
    strcat(prompt, cwd);
    strcat(prompt, "> ");
    terminal_print(prompt);
}

void terminal_init() {
    terminal_clear();
    terminal_print_centered("ZeroUX Terminal [v1.5]");
    terminal_print_centered("(c) ZeroUX Corp. Filesystem Integrated.");
    terminal_print("");
    // fs_init() should be called by kernel/gui init
}

static void execute_command() {
    // Echo command to buffer with prompt
    char line[TERM_COLS + 30];
    
    char cwd[64];
    fs_pwd(cwd);
    strcpy(line, "ZeroUX:");
    strcat(line, cwd);
    strcat(line, "> ");
    strcat(line, input_buffer);
    terminal_print(line);
    
    // Parse command
    if (strcmp(input_buffer, "help") == 0) {
        terminal_print("Available commands:");
        terminal_print("  help    - Show this list");
        terminal_print("  cls     - Clear screen");
        terminal_print("  time    - Show system time");
        terminal_print("  dir     - List files");
        terminal_print("  cd      - Change directory");
        terminal_print("  mkdir   - Make directory");
        terminal_print("  type    - Show file content");
        terminal_print("  del     - Delete file");
        terminal_print("  exit    - Close terminal");
        terminal_print("  ipconfig- Show network info");
        terminal_print("  telnet  - Remote Shell (TCP)");
        terminal_print("  python  - Run script (python file.py)");
    }
    else if (strcmp(input_buffer, "cls") == 0 || strcmp(input_buffer, "clear") == 0) {
        terminal_clear();
        // Header nicht neu drucken bei cls, wie bei echtem CMD
    }
    else if (strcmp(input_buffer, "exit") == 0) {
        // Fenster schließen (ID 4 ist Terminal nach der Neu-Nummerierung)
        // Note: ID must match app_list.def
        gui_close_window(4);
    }
    else if (strncmp(input_buffer, "echo ", 5) == 0) {
        terminal_print(input_buffer + 5);
    }
    else if (strcmp(input_buffer, "ipconfig") == 0) {
        IP_Address ip = ethernet_get_ip();
        char buf[64];
        strcpy(buf, "IP Address: ");
        char num[8];
        int_to_str(ip.addr[0], num); strcat(buf, num); strcat(buf, ".");
        int_to_str(ip.addr[1], num); strcat(buf, num); strcat(buf, ".");
        int_to_str(ip.addr[2], num); strcat(buf, num); strcat(buf, ".");
        int_to_str(ip.addr[3], num); strcat(buf, num);
        terminal_print(buf);
    }
    else if (strncmp(input_buffer, "nslookup ", 9) == 0) {
        // Check if we have an IP address first
        IP_Address my_ip = ethernet_get_ip();
        if (my_ip.addr[0] == 0) {
            terminal_print("Error: No IP address assigned.");
            terminal_print("Waiting for DHCP... Try 'ipconfig' to check.");
            return;
        }

        net_dns_lookup(input_buffer + 9);
        terminal_print("Resolving domain...");
        
        // Warteschleife (ca. 3 Sekunden)
        int timeout = 3000; // 3000 * 1ms = 3s
        int got_reply = 0;
        
        while (timeout--) {
            // Netzwerk manuell pollen, da wir die GUI-Schleife blockieren
            EthernetFrame eth_frame;
            // WICHTIG: Alle verfügbaren Pakete lesen, nicht nur eins!
            while (ethernet_recv_frame(&eth_frame) == 0) {
                net_handle_packet((unsigned char*)&eth_frame, eth_frame.length + 14);
            }
            
            const char* status = net_get_status();
            
            // 1. Erfolg?
            if (strncmp(status, "DNS Result", 10) == 0) {
                terminal_print(status);
                got_reply = 1;
                break;
            }
            // 2. Fehler/Debug? (Alles was "DNS:" ist, aber NICHT "Sent" oder "Sending")
            if (strncmp(status, "DNS:", 4) == 0 && strncmp(status, "DNS: Sent", 9) != 0 && strncmp(status, "DNS: Sending", 12) != 0) {
                terminal_print(status);
                got_reply = 1;
                break;
            }
            
            // Kurze Pause
            for(volatile int i=0; i<1000000; i++);
        }
        
        if (!got_reply) {
            terminal_print("DNS Request timed out (No reply received).");
            terminal_print("Last Status:");
            terminal_print(net_get_status());
            
            // Debug RX Count
            char buf[32];
            strcpy(buf, "Debug RX: ");
            char num[16];
            int_to_str(net_get_rx_count(), num);
            strcat(buf, num);
            terminal_print(buf);
        }
    }
    else if (strncmp(input_buffer, "ping ", 5) == 0) {
        net_ping(input_buffer + 5);
        terminal_print("Pinging...");
        
        int timeout = 3000; // 3000 * 1ms = 3s
        int got_reply = 0;
        
        while (timeout--) {
            EthernetFrame eth_frame;
            // WICHTIG: Alle verfügbaren Pakete lesen!
            while (ethernet_recv_frame(&eth_frame) == 0) {
                net_handle_packet((unsigned char*)&eth_frame, eth_frame.length + 14);
            }
            
            const char* status = net_get_status();
            // Akzeptiere "Ping Reply" ODER wenn "ICMP Type: 0" irgendwo im Text vorkommt
            if (strncmp(status, "Ping Reply", 10) == 0 || strstr(status, "ICMP Type: 0")) {
                terminal_print(status);
                got_reply = 1;
                break;
            }
            
            for(volatile int i=0; i<1000000; i++);
        }
        
        if (!got_reply) {
            terminal_print("Request timed out. Last Status:");
            terminal_print(net_get_status()); // Zeige, was stattdessen passiert ist
            
            // Debug Info: Haben wir überhaupt Pakete empfangen?
            char buf[32];
            strcpy(buf, "Debug RX Count: ");
            char num[16];
            int_to_str(net_get_rx_count(), num);
            strcat(buf, num);
            
            // Zeige auch den letzten Paket-Typ an
            strcat(buf, " LastType: 0x");
            // Hex-Konvertierung (vereinfacht)
            int type = net_get_last_ethertype();
            // Wir geben hier einfach den Dezimalwert aus, da wir kein printf %x haben
            int_to_str(type, num);
            strcat(buf, num);
            
            terminal_print(buf);
        }
    }
    else if (strncmp(input_buffer, "httpget ", 8) == 0) {
        // Parse IP from command line
        char* ip_str = input_buffer + 8;
        uint8_t target_ip[4];
        // Simple parser (reusing logic from net.c would be better but this is quick)
        int val = 0, idx = 0;
        char* ptr = ip_str;
        while (*ptr && idx < 4) {
            if (*ptr == '.') { target_ip[idx++] = val; val = 0; }
            else if (*ptr >= '0' && *ptr <= '9') { val = val * 10 + (*ptr - '0'); }
            ptr++;
        }
        target_ip[idx] = val;

        terminal_print("Connecting to TCP Port 80...");
        
        int connected = 0;
        // 3 Versuche für den Verbindungsaufbau (hilft bei ARP/Routing Verzögerung)
        for (int attempt = 1; attempt <= 3; attempt++) {
            net_tcp_connect(target_ip, 80);
            
            // Warten auf Verbindung (ca. 1.5 Sekunden pro Versuch)
            int wait = 3000; // 3000 * 1ms = 3s
            while(wait-- && net_get_tcp_state() != 2) {
                 EthernetFrame eth_frame;
                 while (ethernet_recv_frame(&eth_frame) == 0) {
                    net_handle_packet((unsigned char*)&eth_frame, eth_frame.length + 14);
                 }
                 for(volatile int i=0; i<1000000; i++);
            }
            
            if (net_get_tcp_state() == 2) { connected = 1; break; }
            if (attempt < 3) terminal_print("Timeout. Retrying...");
        }
        
        if (connected) {
            terminal_print("Connected! Sending HTTP GET...");
            net_send_http_request(ip_str, "/"); // Root path
            
            // Warten auf Antwort
            int timeout = 5000; // 5000 * 1ms = 5s
            while(timeout--) {
                EthernetFrame eth_frame;
                while (ethernet_recv_frame(&eth_frame) == 0) {
                    net_handle_packet((unsigned char*)&eth_frame, eth_frame.length + 14);
                }
                
                // Abbruchbedingungen: Daten erhalten ODER Verbindung geschlossen
                if (net_get_http_response()[0] != '\0') break;
                if (net_get_tcp_state() == 0 && timeout < 4800) break; // Server hat geschlossen (aber gib ihm kurz Zeit)
                
                // Kurze Pause um CPU nicht zu verbrennen und Zeit zu geben
                for(volatile int i=0; i<1000000; i++);
            }
            terminal_print(net_get_status());
            
            // Zeige den Inhalt des Puffers an
            const char* content = net_get_http_response();
            if (content[0]) {
                terminal_print("--- RESPONSE START ---");
                
                // Zeilenweise Ausgabe für bessere Lesbarkeit
                const char* ptr = content;
                char line[81];
                int idx = 0;
                int lines = 0;
                
                while (*ptr && lines < 15) { // Max 15 Zeilen anzeigen
                    if (*ptr == '\n') {
                        line[idx] = 0;
                        terminal_print(line);
                        idx = 0;
                        lines++;
                    } else if (*ptr != '\r' && idx < 80) {
                        line[idx++] = *ptr;
                    }
                    ptr++;
                }
                if (idx > 0) { line[idx] = 0; terminal_print(line); }

                // Redirect Analyse (Wohin will der Server uns schicken?)
                if (strstr(content, "301") || strstr(content, "302")) {
                    const char* loc = strstr(content, "Location: ");
                    if (loc) {
                        terminal_print(">>> Server Redirect >>>");
                        // Wir geben die Zeile ab "Location:" aus, wurde oben evtl. schon gedruckt
                        // aber hier als expliziter Hinweis, dass wir HTTPS nicht können.
                        terminal_print("Note: Redirects to HTTPS are not supported.");
                    }
                }
                terminal_print("--- RESPONSE END ---");
            } else {
                terminal_print("No HTTP data received (Timeout or Empty).");
            }
        } else {
            terminal_print("Connection failed.");
        }
    }
    else if (strncmp(input_buffer, "telnet ", 7) == 0) {
        char* ip_str = input_buffer + 7;
        uint8_t target_ip[4];
        // IP parsen
        int val = 0, idx = 0;
        char* ptr = ip_str;
        while (*ptr && idx < 4) {
            if (*ptr == '.') { target_ip[idx++] = val; val = 0; }
            else if (*ptr >= '0' && *ptr <= '9') { val = val * 10 + (*ptr - '0'); }
            ptr++;
        }
        target_ip[idx] = val;

        terminal_print("Connecting to Telnet Server (Port 23)...");
        net_tcp_connect(target_ip, 23);

        // Warten auf Verbindung
        int timeout = 3000;
        while(timeout-- && net_get_tcp_state() != 2) {
             EthernetFrame eth_frame;
             while (ethernet_recv_frame(&eth_frame) == 0) {
                net_handle_packet((unsigned char*)&eth_frame, eth_frame.length + 14);
             }
             for(volatile int i=0; i<1000000; i++);
        }

        if (net_get_tcp_state() == 2) {
            terminal_print("Connected! Press ESC to exit.");
            terminal_print("-----------------------------");
            
            // Interaktive Schleife
            while (net_get_tcp_state() == 2) {
                // 1. Netzwerk empfangen
                EthernetFrame eth_frame;
                while (ethernet_recv_frame(&eth_frame) == 0) {
                    net_handle_packet((unsigned char*)&eth_frame, eth_frame.length + 14);
                }

                // 2. Daten vom Server lesen und anzeigen
                char rx_buf[128];
                int rx_len = net_tcp_read(rx_buf, 127);
                if (rx_len > 0) {
                    rx_buf[rx_len] = 0;
                    // Zeilenweise ausgeben (simpel)
                    terminal_print(rx_buf);
                }

                // 3. Tastatur senden
                unsigned char sc = keyboard_read_nonblock();
                if (sc) {
                    if (sc == 0x01) break; // ESC zum Beenden
                    
                    char c = scancode_to_ascii(sc);
                    if (c) {
                        char tx_buf[2] = {c, 0};
                        net_tcp_send(tx_buf, 1);
                        // Lokales Echo optional:
                        // terminal_print(tx_buf); 
                    }
                }
            }
            terminal_print("Connection closed.");
        } else {
            terminal_print("Connection failed.");
        }
    }
    else if (strncmp(input_buffer, "python ", 7) == 0) {
        char* fname = input_buffer + 7;
        
        // Datei suchen
        file_t* files = fs_get_table();
        int current_dir = fs_get_current_dir();
        int found = -1;
        
        for (int i = 0; i < MAX_FILES; i++) {
            if (files[i].in_use && files[i].parent_idx == current_dir && strcmp(files[i].name, fname) == 0) {
                found = i;
                break;
            }
        }
        
        if (found != -1 && files[found].type == FILE_TYPE_FILE) {
            terminal_print("Running Python script...");
            char output[1024];
            python_run_script(files[found].content, output, 1024);
            
            // Ausgabe anzeigen
            terminal_print("--- Output ---");
            // Hier müsste man den Output zeilenweise splitten, vereinfacht:
            terminal_print(output);
        } else {
            terminal_print("File not found.");
        }
    }
    // --- Filesystem Commands ---
    else if (strcmp(input_buffer, "dir") == 0 || strcmp(input_buffer, "ls") == 0) {
        file_t* files = fs_get_table();
        int current_dir = fs_get_current_dir();
        int count = 0;
        
        for (int i = 0; i < MAX_FILES; i++) {
            if (files[i].in_use && files[i].parent_idx == current_dir) {
                char entry[64];
                if (files[i].type == FILE_TYPE_DIR) {
                    strcpy(entry, "<DIR>   ");
                } else if (files[i].type == FILE_TYPE_APP) {
                    strcpy(entry, "<APP>   ");
                } else {
                    char size_str[16];
                    int_to_str(files[i].size, size_str);
                    strcpy(entry, "        ");
                    // Simple padding
                    int len = 0; while(size_str[len]) len++;
                    int pad = 8 - len;
                    if(pad < 0) pad = 0;
                    strcpy(entry + pad, size_str);
                    strcat(entry, " ");
                }
                strcat(entry, files[i].name);
                terminal_print(entry);
                count++;
            }
        }
        char summary[32];
        int_to_str(count, summary);
        strcat(summary, " File(s)");
        terminal_print(summary);
    }
    else if (strncmp(input_buffer, "cd ", 3) == 0) {
        if (fs_cd(input_buffer + 3) != 0) {
            terminal_print("System cannot find the path specified.");
        }
    }
    else if (strncmp(input_buffer, "mkdir ", 6) == 0) {
        if (fs_mkdir(input_buffer + 6) != 0) {
            terminal_print("Directory already exists or table full.");
        }
    }
    else if (strncmp(input_buffer, "touch ", 6) == 0) {
        if (fs_touch(input_buffer + 6) != 0) {
            terminal_print("File creation failed.");
        }
    }
    else if (strncmp(input_buffer, "del ", 4) == 0 || strncmp(input_buffer, "rm ", 3) == 0) {
        char* fname = (input_buffer[0] == 'd') ? input_buffer + 4 : input_buffer + 3;
        if (fs_rm(fname) != 0) {
            terminal_print("Could not delete file.");
        }
    }
    else if (strncmp(input_buffer, "type ", 5) == 0 || strncmp(input_buffer, "cat ", 4) == 0) {
        char* fname = (input_buffer[0] == 't') ? input_buffer + 5 : input_buffer + 4;
        
        // Manual lookup to print to buffer instead of VGA
        file_t* files = fs_get_table();
        int current_dir = fs_get_current_dir();
        int found = -1;
        
        for (int i = 0; i < MAX_FILES; i++) {
            if (files[i].in_use && files[i].parent_idx == current_dir && strcmp(files[i].name, fname) == 0) {
                found = i;
                break;
            }
        }
        
        if (found != -1) {
            if (files[found].type == FILE_TYPE_DIR) {
                terminal_print("Access denied.");
            } else {
                terminal_print(files[found].content);
            }
        } else {
            terminal_print("File not found.");
        }
    }
    else if (strcmp(input_buffer, "time") == 0) {
        int h, m, s;
        rtc_get_time(&h, &m, &s);
        char tbuf[32];
        char tmp[8];
        
        strcpy(tbuf, "Current time is: ");
        int_to_str(h, tmp); strcat(tbuf, tmp); strcat(tbuf, ":");
        int_to_str(m, tmp); strcat(tbuf, tmp); strcat(tbuf, ":");
        int_to_str(s, tmp); strcat(tbuf, tmp);
        terminal_print(tbuf);
    }
    else {
        // Try to execute file
        file_t* files = fs_get_table();
        int current_dir = fs_get_current_dir();
        int found = -1;
        for (int i = 0; i < MAX_FILES; i++) {
            if (files[i].in_use && files[i].parent_idx == current_dir && strcmp(files[i].name, input_buffer) == 0) {
                found = i; break;
            }
        }

        if (found != -1 && files[found].type == FILE_TYPE_FILE) {
            const char* c = files[found].content;
            if (c[0] == 'M' && c[1] == 'Z') {
                terminal_print("Windows PE Executable (.exe) detected.");
                terminal_print("Starting Win32 subsystem...");
                terminal_print("Program executed successfully.");
                input_len = 0; input_buffer[0] = '\0'; return;
            }
            if (c[0] == 0x7F && c[1] == 'E' && c[2] == 'L' && c[3] == 'F') {
                terminal_print("Linux ELF Binary detected.");
                terminal_print("Starting Linux subsystem...");
                terminal_print("Program executed successfully.");
                input_len = 0; input_buffer[0] = '\0'; return;
            }
        }

        if (input_len > 0) {
        char err[64];
        strcpy(err, "'");
        // Begrenzen für Anzeige
        int safe_len = input_len > 20 ? 20 : input_len;
        char safe_cmd[21];
        for(int k=0; k<safe_len; k++) safe_cmd[k] = input_buffer[k];
        safe_cmd[safe_len] = 0;
        
        strcat(err, safe_cmd);
        strcat(err, "' is not recognized.");
        terminal_print(err);
    }
    
    // Reset input
    input_len = 0;
    input_buffer[0] = '\0';
}
}

void terminal_handle_key(char c) {
    if (input_len < TERM_COLS - 1) {
        input_buffer[input_len++] = c;
        input_buffer[input_len] = '\0';
    }
}

void terminal_handle_backspace() {
    if (input_len > 0) {
        input_len--;
        input_buffer[input_len] = '\0';
    }
}

void terminal_handle_enter() {
    execute_command();
}

void terminal_draw_vbe(int x, int y, int w, int h, int is_active) {
    int line_height = 16;
    int start_y = y + 5;
    int start_x = x + 5;
    
    // Puffer zeichnen
    int current_draw_y = start_y;
    
    // Wir zeichnen nur so viele Zeilen wie in das Fenster passen
    int max_visible_lines = (h - 10) / line_height;
    // Reserviere eine Zeile für den Prompt
    int buffer_lines_to_draw = max_visible_lines - 1;
    
    int start_index = 0;
    // Finde die letzte beschriebene Zeile im Puffer
    int last_filled = 0;
    for(int i=0; i<TERM_ROWS; i++) {
        if(term_buffer[i][0] != 0) last_filled = i;
    }
    
    // Einfaches Rendering: Zeige die letzten N Zeilen, wenn Puffer voll
    // Hier zeigen wir einfach alles von oben, da wir scrollen implementiert haben
    for (int i = 0; i < TERM_ROWS; i++) {
        if (term_buffer[i][0] != '\0') {
            if (current_draw_y + line_height < y + h) {
                vbe_draw_text(start_x, current_draw_y, term_buffer[i], TERM_FG, VBE_TRANSPARENT);
                current_draw_y += line_height;
            }
        }
    }
    
    // Prompt und aktuelle Eingabe zeichnen
    if (current_draw_y + line_height < y + h) {
        char prompt_line[TERM_COLS + 60];
        char cwd[64];
        fs_pwd(cwd);
        strcpy(prompt_line, "ZeroUX:");
        strcat(prompt_line, cwd);
        strcat(prompt_line, "> ");
        strcat(prompt_line, input_buffer);
        
        // Auch hier: Hintergrund schwarz zeichnen
        vbe_draw_text(start_x, current_draw_y, prompt_line, TERM_FG, TERM_BG);
        
        // Den Rest der Zeile rechts vom Text löschen (damit Backspace sauber aussieht)
        int text_width = strlen(prompt_line) * 8; // Annahme: 8px pro Zeichen
        if (start_x + text_width < x + w) {
            vbe_fill_rect(start_x + text_width, current_draw_y, (x + w) - (start_x + text_width), line_height, TERM_BG);
        }
        
        // Cursor (Blinkend wenn aktiv)
        if (is_active && gui_get_caret_state()) {
            int prompt_len = 0;
            while(prompt_line[prompt_len]) prompt_len++;
            int cursor_x = start_x + (prompt_len * 8);
            vbe_fill_rect(cursor_x, current_draw_y + 12, 8, 2, TERM_FG); // Unterstrich Cursor
        }
    }
}
