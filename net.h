#ifndef NET_H
#define NET_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

// Initialisiert den Netzwerk-Stack
void net_init();

// Sendet ein DHCP Discover Paket (für Retries)
void net_send_dhcp_discover();

// Sendet eine DNS-Anfrage für eine Domain (z.B. "google.com")
void net_dns_lookup(const char* domain);

// Gibt den aktuellen Netzwerk-Status-Text zurück (für GUI Overlay)
const char* net_get_status();

// Gibt die Anzahl der empfangenen Pakete zurück (RX Count)
int net_get_rx_count();

// Gibt den Typ des letzten empfangenen Pakets zurück (z.B. 0x0800 für IP)
uint16_t net_get_last_ethertype();

// --- TCP CLIENT ---
// Verbindet sich mit einer IP auf einem Port (z.B. 80)
void net_tcp_connect(uint8_t* ip, uint16_t port);
// Sendet einen HTTP GET Request (nur wenn verbunden)
void net_send_http_request(const char* host, const char* path);
// Gibt den TCP Status zurück (0=Closed, 1=Connecting, 2=Connected)
int net_get_tcp_state();
// Sendet rohe Daten über die offene TCP-Verbindung
void net_tcp_send(const char* data, int len);
// Liest Daten aus dem Empfangspuffer (und leert ihn)
int net_tcp_read(char* buf, int max_len);
// Gibt den empfangenen HTTP-Inhalt zurück
const char* net_get_http_response();
// ------------------

// NEU: Simuliert eine erfolgreiche Antwort (für Demo-Zwecke)
void net_simulate_http_response(const char* url);

// Sendet einen Ping (ICMP Echo Request) an eine IP
void net_ping(const char* ip_str);

// Verarbeitet ein empfangenes Ethernet-Paket
// Muss vom Ethernet-Treiber aufgerufen werden!
void net_handle_packet(uint8_t* data, int len);

// Hilfsfunktionen für Byte-Order (Big Endian <-> Little Endian)
uint16_t htons(uint16_t v);
uint16_t ntohs(uint16_t v);
uint32_t htonl(uint32_t v);
uint32_t ntohl(uint32_t v);

#endif
