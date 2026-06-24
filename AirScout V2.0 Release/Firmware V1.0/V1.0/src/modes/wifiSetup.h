#ifndef MODES_WIFISETUP_H
#define MODES_WIFISETUP_H

#include <lwip/err.h>
#include <lwip/ip_addr.h>
#include <stdint.h>

extern "C" {
#include <dhcpserver.h>
#include <dnsserver.h>
}

#include "baseMode.h"

#define POLL_TIME_S                5
#define HTTP_GET                   "GET"
#define HTTP_POST                  "POST"
#define HTTP_HTML_RESPONSE_HEADERS "HTTP/1.1 %d %s\nContent-Length: %d\nContent-Type: text/html; charset=utf-8\nConnection: close\n\n"
#define HTTP_JSON_RESPONSE_HEADERS "HTTP/1.1 %d %s\nContent-Length: %d\nContent-Type: application/json; charset=utf-8\nConnection: close\n\n"
#define HTTP_JS_RESPONSE_HEADERS   "HTTP/1.1 %d %s\nContent-Length: %d\nContent-Type: text/javascript; charset=utf-8\nConnection: close\n\n"
#define HTTP_CSS_RESPONSE_HEADERS  "HTTP/1.1 %d %s\nContent-Length: %d\nContent-Type: text/css; charset=utf-8\nConnection: close\n\n"
#define HTTP_SVG_RESPONSE_HEADERS  "HTTP/1.1 %d %s\nContent-Length: %d\nContent-Type: image/svg+xml; charset=utf-8\nConnection: close\n\n"

#define HTTP_STATUS_RESPONSE_HEADERS "HTTP/1.1 %d %s\nConnection: close\n\n"

#define HTTP_HEADER_LEN 128
#define HTTP_404_BODY   "<!DOCTYPE html><html><head><title>Not found</title></head><body><h1>Page not found</h1></body></html>"
#define HTTP_500_BODY   "<!DOCTYPE html><html><head><title>Internal Error</title></head><body><h1>Internal Error</h1></body></html>"

#define HTTP_STATIC_LOCATION           "/static"
#define HTTP_WIFI_DATA_LOCATION        "/wifidata"
#define HTTP_WIFI_STORED_NETS_LOCATION "/stored_nets"
#define HTTP_SETTINGS_LOCATION         "/settings"

#define HTTP_STATIC_FILE_PATH "/http"

#define HTTP_RESPONSE_REDIRECT "HTTP/1.1 302 Redirect\nLocation: http://%s/static/index.html\n\n"

#define HTTP_RESPONSE_SETTINGS_BUFFER_LEN 200
#define HTTP_RESPONSE_SETTINGS            "{\"type\" : \"%s\", \"status\": \"%s\"}"

#define WIFI_SCAN_INTERVAL_S                 5
#define WIFI_SETUP_SETTINGS_CHECK_INTERVAL_S 1

typedef struct {
    struct tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
} tcp_server_t;

typedef struct {
    struct tcp_pcb *pcb;
    uint16_t sent_len;
    char *headers;
    char *result;
    uint8_t allocated_header_len;
    uint8_t header_len;
    uint16_t result_len;
    ip_addr_t *gw;
} tcp_connect_state_t;

class WifiSetupMode : public BaseMode {
private:
    tcp_server_t *m_tcp_server;
    dns_server_t m_dns_server;
    dhcp_server_t m_dhcp_server;

public:
    bool init() override;
    void run() override;
    void exit() override;
};

#endif  // MODES_WIFISETUP_H
