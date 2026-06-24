#include "wifiSetup.h"

#include <cJSON.h>
#include <cyw43_ll.h>
#include <ff.h>
#include <lwip/tcp.h>
#include <pico/time.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ulog.h>

#include <cstdio>
// extern "C" {
// }

#include "driver/display.h"
#include "driver/fs.h"
#include "driver/init_hardware.h"
#include "globals.h"
#include "gui/elements/menu_bar.h"
#include "gui/elements/status_bar.h"
#include "gui/gui_utils.h"
#include "main.h"
#include "modes/baseMode.h"
#include "settings.h"
#include "utils.h"
#include "wifi.h"

enum class WIFI_SETUP_TYPE { NONE = 0, ADD, CONNECT, DELETE };

typedef struct {
    WIFI_SETUP_TYPE type;
    char *ssid;
    uint16_t ssid_len;
    char *passwd;
    uint16_t passwd_len;
    WSTATUS wstatus;
} wifi_setup_data_t;

static volatile wifi_setup_data_t wifi_setup_data = {.type = WIFI_SETUP_TYPE::NONE};

#define PARAM_TYPE            "type"
#define PARAM_TYPE_ADD        "add"
#define PARAM_TYPE_CONNECT    "connect"
#define PARAM_TYPE_DELETE     "delete"
#define PARAM_TYPE_DISCONNECT "disconnect"
#define PARAM_SSID            "ssid"
#define PARAM_PASSWD          "passwd"

// #define PARAM_ ""

#define _______ "\0\0\0\0"
static const char uri_encode_tbl[sizeof(int32_t) * 0x100] = {
    /*  0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f                        */
    "%00\0"
    "%01\0"
    "%02\0"
    "%03\0"
    "%04\0"
    "%05\0"
    "%06\0"
    "%07\0"
    "%08\0"
    "%09\0"
    "%0A\0"
    "%0B\0"
    "%0C\0"
    "%0D\0"
    "%0E\0"
    "%0F\0" /* 0:   0 ~  15 */
    "%10\0"
    "%11\0"
    "%12\0"
    "%13\0"
    "%14\0"
    "%15\0"
    "%16\0"
    "%17\0"
    "%18\0"
    "%19\0"
    "%1A\0"
    "%1B\0"
    "%1C\0"
    "%1D\0"
    "%1E\0"
    "%1F\0" /* 1:  16 ~  31 */
    "%20\0"
    "%21\0"
    "%22\0"
    "%23\0"
    "%24\0"
    "%25\0"
    "%26\0"
    "%27\0"
    "%28\0"
    "%29\0"
    "%2A\0"
    "%2B\0"
    "%2C\0" _______ _______ "%2F\0" /* 2:  32 ~  47 */
    _______ _______ _______ _______ _______ _______ _______ _______ _______ _______
    "%3A\0"
    "%3B\0"
    "%3C\0"
    "%3D\0"
    "%3E\0"
    "%3F\0"                                                                                                                         /* 3:  48 ~  63 */
    "%40\0" _______ _______ _______ _______ _______ _______ _______ _______ _______ _______ _______ _______ _______ _______ _______ /* 4:  64 ~  79 */
        _______ _______ _______ _______ _______ _______ _______ _______ _______ _______ _______
    "%5B\0"
    "%5C\0"
    "%5D\0"
    "%5E\0" _______                                                                                                                 /* 5:  80 ~  95 */
    "%60\0" _______ _______ _______ _______ _______ _______ _______ _______ _______ _______ _______ _______ _______ _______ _______ /* 6:  96 ~ 111 */
        _______ _______ _______ _______ _______ _______ _______ _______ _______ _______ _______
    "%7B\0"
    "%7C\0"
    "%7D\0" _______
    "%7F\0" /* 7: 112 ~ 127 */
    "%80\0"
    "%81\0"
    "%82\0"
    "%83\0"
    "%84\0"
    "%85\0"
    "%86\0"
    "%87\0"
    "%88\0"
    "%89\0"
    "%8A\0"
    "%8B\0"
    "%8C\0"
    "%8D\0"
    "%8E\0"
    "%8F\0" /* 8: 128 ~ 143 */
    "%90\0"
    "%91\0"
    "%92\0"
    "%93\0"
    "%94\0"
    "%95\0"
    "%96\0"
    "%97\0"
    "%98\0"
    "%99\0"
    "%9A\0"
    "%9B\0"
    "%9C\0"
    "%9D\0"
    "%9E\0"
    "%9F\0" /* 9: 144 ~ 159 */
    "%A0\0"
    "%A1\0"
    "%A2\0"
    "%A3\0"
    "%A4\0"
    "%A5\0"
    "%A6\0"
    "%A7\0"
    "%A8\0"
    "%A9\0"
    "%AA\0"
    "%AB\0"
    "%AC\0"
    "%AD\0"
    "%AE\0"
    "%AF\0" /* A: 160 ~ 175 */
    "%B0\0"
    "%B1\0"
    "%B2\0"
    "%B3\0"
    "%B4\0"
    "%B5\0"
    "%B6\0"
    "%B7\0"
    "%B8\0"
    "%B9\0"
    "%BA\0"
    "%BB\0"
    "%BC\0"
    "%BD\0"
    "%BE\0"
    "%BF\0" /* B: 176 ~ 191 */
    "%C0\0"
    "%C1\0"
    "%C2\0"
    "%C3\0"
    "%C4\0"
    "%C5\0"
    "%C6\0"
    "%C7\0"
    "%C8\0"
    "%C9\0"
    "%CA\0"
    "%CB\0"
    "%CC\0"
    "%CD\0"
    "%CE\0"
    "%CF\0" /* C: 192 ~ 207 */
    "%D0\0"
    "%D1\0"
    "%D2\0"
    "%D3\0"
    "%D4\0"
    "%D5\0"
    "%D6\0"
    "%D7\0"
    "%D8\0"
    "%D9\0"
    "%DA\0"
    "%DB\0"
    "%DC\0"
    "%DD\0"
    "%DE\0"
    "%DF\0" /* D: 208 ~ 223 */
    "%E0\0"
    "%E1\0"
    "%E2\0"
    "%E3\0"
    "%E4\0"
    "%E5\0"
    "%E6\0"
    "%E7\0"
    "%E8\0"
    "%E9\0"
    "%EA\0"
    "%EB\0"
    "%EC\0"
    "%ED\0"
    "%EE\0"
    "%EF\0" /* E: 224 ~ 239 */
    "%F0\0"
    "%F1\0"
    "%F2\0"
    "%F3\0"
    "%F4\0"
    "%F5\0"
    "%F6\0"
    "%F7\0"
    "%F8\0"
    "%F9\0"
    "%FA\0"
    "%FB\0"
    "%FC\0"
    "%FD\0"
    "%FE\0"
    "%FF" /* F: 240 ~ 255 */
};
#undef _______

#define __ 0xFF
static const unsigned char hexval[0x100] = {
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 00-0F */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 10-1F */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 20-2F */
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  __, __, __, __, __, __, /* 30-3F */
    __, 10, 11, 12, 13, 14, 15, __, __, __, __, __, __, __, __, __, /* 40-4F */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 50-5F */
    __, 10, 11, 12, 13, 14, 15, __, __, __, __, __, __, __, __, __, /* 60-6F */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 70-7F */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 80-8F */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 90-9F */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* A0-AF */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* B0-BF */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* C0-CF */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* D0-DF */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* E0-EF */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* F0-FF */
};
#undef __

static size_t uri_decode(const char *src, const size_t len, char *dst);

static void cbk_tcp_server_err(void *arg, err_t err);
static err_t cbk_tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
static err_t cbk_tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err);
static err_t cbk_tcp_server_sent(void *arg, struct tcp_pcb *pcb, u16_t len);

static void tcp_server_close(tcp_server_t *state);
static int check_requested_url(const char *request, const char *params, char *result, size_t max_result_len);
static err_t tcp_close_client_connection(tcp_connect_state_t *con_state, struct tcp_pcb *client_pcb, err_t close_err);
static bool tcp_server_open(void *arg);

static tcp_connect_state_t *create_tcp_connect_state();
static void destroy_tcp_connect_state(tcp_connect_state_t *conn);
static bool resize_tcp_connect_state_result(tcp_connect_state_t *con, int result_len);

// must be freed
#define STATIC_FILE_OK         0
#define STATIC_FILE_NOT_FOUND  -1
#define STATIC_FILE_DISK_ERROR -2

static int32_t getStaticFile(const char *path, char **content);
static uint16_t setResponseHeader(tcp_connect_state_t *con, const char *header_template, uint16_t status);
static bool doScan;
WSTATUS wifi_status = WSTATUS::NONE;

bool WifiSetupMode::init() {
    char *hostname, *token;
    user_settings.getString("hostname", &hostname);
    user_settings.getString("token", &token);
    char passwd[13];
    snprintf(passwd, sizeof(passwd), "%.*s", sizeof(passwd) - 1, token);
    printf("AP SSID: %s\n", hostname);
    printf("AP Passwd: %s\n", passwd);
    free(token);

    GUI::clear_except_status_menu(Display::COLOR::BLACK);
    menu_bar.setLayout(GUI::Menu_Bar::Icons::BACK, GUI::Menu_Bar::Icons::NONE, GUI::Menu_Bar::Icons::NONE, GUI::Menu_Bar::Icons::NONE);

    wifi.enableAP(hostname, passwd, WAUTH::WPA2_AES_PSK);

    m_tcp_server = (tcp_server_t *)calloc(1, sizeof(tcp_server_t));
    if(!m_tcp_server) {
        ulog_fatal("Failed to allocate TCP Server");
        free(hostname);
        return false;
    }

    ip4_addr_t mask;
    m_tcp_server->gw.addr = PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS);
    mask.addr = PP_HTONL(CYW43_DEFAULT_IP_MASK);
    //
    // Start the dhcp server
    dhcp_server_init(&m_dhcp_server, &m_tcp_server->gw, &mask);
    ulog_info("Starting DHCP Server on %s", ipaddr_ntoa(&m_tcp_server->gw));

    // Start the dns server
    dns_server_init(&m_dns_server, &m_tcp_server->gw);
    ulog_info("Starting DNS Server on %s", ipaddr_ntoa(&m_tcp_server->gw));

    if(!tcp_server_open(m_tcp_server)) {
        ulog_error("failed to open server");
        disp.draw_text(10, STATUS_BAR_HEIGHT + 10, 1, "Failed to start wifi", &font_Liberation_Mono8x13, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);
        free(hostname);
        return false;
    }
    m_tcp_server->complete = false;

    char tmp[30];
    disp.draw_text(10, STATUS_BAR_HEIGHT + 10, 1, "Connect to 192.168.4.1 in:", &font_Liberation_Mono8x13, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);

    snprintf(tmp, sizeof(tmp), "SSID: %s", hostname);
    free(hostname);
    disp.draw_text(10, STATUS_BAR_HEIGHT + 25, 1, tmp, &font_Liberation_Mono8x13, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);
    snprintf(tmp, sizeof(tmp), "Passwd: %s", passwd);
    disp.draw_text(10, STATUS_BAR_HEIGHT + 40, 1, tmp, &font_Liberation_Mono8x13, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);
    return true;
}

static cyw43_ev_scan_result_t *scan_results = NULL;
static uint16_t scan_result_cnt = 0;

void WifiSetupMode::exit() {
    tcp_server_close(m_tcp_server);
    dns_server_deinit(&m_dns_server);
    dhcp_server_deinit(&m_dhcp_server);
    free(scan_results);
}

void WifiSetupMode::run() {
    static absolute_time_t lastScan = 0;  // ensure that the fist scan happens immediately
    static absolute_time_t lastButtonCheck = 0;

    // TODO: calling HTTP_WIFI_DATA_LOCATION will return the data of the last scan and set a flag.
    // When this flag is set, a new scan is started and replaces the old values.
    // Instead of the scan running in parallel
    if(absolute_time_diff_us(lastScan, get_absolute_time()) > (WIFI_SCAN_INTERVAL_S * 1000 * 1000)) {
        lastScan = get_absolute_time();
        scan_result_cnt = 0;
        free(scan_results);  // default is NULL, so we can free without issue
        wifi.scan(&scan_results, &scan_result_cnt);
        for(int i = 0; i < scan_result_cnt; i++) {
            ulog_debug("ssid: %-32s, ssid len: %d, rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u", scan_results[i].ssid, scan_results[i].ssid_len, scan_results[i].rssi, scan_results[i].channel, scan_results[i].bssid[0], scan_results[i].bssid[1], scan_results[i].bssid[2], scan_results[i].bssid[3], scan_results[i].bssid[4], scan_results[i].bssid[5], scan_results[i].auth_mode);
        }
    }

    if(wifi_status != WSTATUS::NONE) {
        if(wifi_status != WSTATUS::BADAUTH && wifi_status != WSTATUS::FAIL) {
            disp.draw_text(5, 110, 1, "Connection successful", &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);
            switchToMode(MODES::Idle);
            // complete
        } else {
            // restart?
        }
    }

    if(absolute_time_diff_us(lastButtonCheck, get_absolute_time()) > BUTTON_CHECK_INTERVAL_MS * 1000) {
        BTN btn;
        do {
            btn = getNextButton();
            switch(btn) {
                case BTN::BTN1:
                    switchToMode(MODES::Idle);
                    break;
                default:
                    break;
            }
        } while(btn != BTN::NONE);
    }
}
/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

static err_t tcp_close_client_connection(tcp_connect_state_t *con_state, struct tcp_pcb *client_pcb, err_t close_err) {
    if(client_pcb) {
        assert(con_state && con_state->pcb == client_pcb);
        tcp_arg(client_pcb, NULL);
        tcp_poll(client_pcb, NULL, 0);
        tcp_sent(client_pcb, NULL);
        tcp_recv(client_pcb, NULL);
        tcp_err(client_pcb, NULL);
        err_t err = tcp_close(client_pcb);
        ulog_info("Closing connection");
        if(err != ERR_OK) {
            ulog_error("close failed %d, calling abort", err);
            tcp_abort(client_pcb);
            close_err = ERR_ABRT;
        }
        if(con_state) {
            destroy_tcp_connect_state(con_state);
            // free(con_state->headers);
            // free(con_state->result);
            // free(con_state);
        }
    }
    return close_err;
}

// static int check_requested_url(const char *request, const char *params, char *result, size_t max_result_len) {
static int generate_response(tcp_connect_state_t *con_state, const char *request, char *params) {
    int len = 0;  // con_state, request, param

    if(strncmp(request, HTTP_STATIC_LOCATION, strlen(HTTP_STATIC_LOCATION)) == 0) {
        // char *s = strstr(request, "HTTP/");
        char *file = strndup(request + strlen(HTTP_STATIC_LOCATION), strlen(request) - strlen(HTTP_STATIC_LOCATION));

        // append file to file path, static files are stored inside HTTP_STATIC_FILE_PATH
        char *path = (char *)calloc(1, strlen(HTTP_STATIC_FILE_PATH) + strlen(file) + 1);
        strcpy(path, HTTP_STATIC_FILE_PATH);
        strcat(path, file);
        free(file);
        int ret = STATIC_FILE_OK;

        ulog_debug("Opening %s...", path);
        char *content;
        ret = getStaticFile(path, &content);

        // reset header
        memset(con_state->headers, 0, con_state->allocated_header_len);
        if(ret > 0) {
            resize_tcp_connect_state_result(con_state, strlen(content));
            memcpy(con_state->result, content, con_state->result_len);
            // ulog_info("Strlen: %d, file size: %d", strlen(content), file_size);

            if(strstr(path, ".html")) {
                setResponseHeader(con_state, HTTP_HTML_RESPONSE_HEADERS, 200);
            } else if(strstr(path, ".css")) {
                setResponseHeader(con_state, HTTP_CSS_RESPONSE_HEADERS, 200);
            } else if(strstr(path, ".js")) {
                setResponseHeader(con_state, HTTP_JS_RESPONSE_HEADERS, 200);
            } else if(strstr(path, ".svg")) {
                // free(path);
                // return false;
                setResponseHeader(con_state, HTTP_SVG_RESPONSE_HEADERS, 200);
            } else {
                ulog_warn("HTTP: Unknown filetype in request: %s", path);
            }
            free(content);
        } else if(ret == STATIC_FILE_DISK_ERROR) {
            resize_tcp_connect_state_result(con_state, strlen(HTTP_500_BODY));
            memcpy(con_state->result, HTTP_500_BODY, strlen(HTTP_500_BODY));
            setResponseHeader(con_state, HTTP_HTML_RESPONSE_HEADERS, 500);

        } else {
            resize_tcp_connect_state_result(con_state, strlen(HTTP_404_BODY));
            memcpy(con_state->result, HTTP_404_BODY, strlen(HTTP_404_BODY));
            setResponseHeader(con_state, HTTP_HTML_RESPONSE_HEADERS, 404);
        }
        free(path);

    } else if(strncmp(request, HTTP_WIFI_DATA_LOCATION, strlen(HTTP_WIFI_DATA_LOCATION)) == 0) {
        cJSON *json = cJSON_CreateObject();
        cJSON *update = NULL;

        update = cJSON_AddArrayToObject(json, "update");
        for(uint16_t index = 0; index < scan_result_cnt; index++) {
            cJSON *net = cJSON_CreateObject();

            if(cJSON_AddNumberToObject(net, "rssi", scan_results[index].rssi) == NULL) {
                ulog_error("Cant add rssi to json, index %d", index);
            }

            if(cJSON_AddNumberToObject(net, "security", scan_results[index].auth_mode) == NULL) {
                ulog_error("Cant add security to json, index %d", index);
            }

            char tmp[33];
            memset(tmp, 0, 33);
            memcpy(tmp, (char *)scan_results[index].ssid, scan_results[index].ssid_len);

            if(cJSON_AddStringToObject(net, "ssid", tmp) == NULL) {
                ulog_error("Cant add rssi to ssid, index %d", index);
            }
            cJSON_AddItemToArray(update, net);
        }

        char *s = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
        if(!resize_tcp_connect_state_result(con_state, strlen(s))) {
            free(s);
            return false;
        }
        memcpy(con_state->result, s, con_state->result_len);
        free(s);
        setResponseHeader(con_state, HTTP_JSON_RESPONSE_HEADERS, 200);

    } else if(strncmp(request, HTTP_WIFI_STORED_NETS_LOCATION, strlen(HTTP_WIFI_STORED_NETS_LOCATION)) == 0) {
        wifi_connection_t *connections;
        uint16_t connection_cnt;

        user_settings.getWifiConnections(&connections, &connection_cnt);
        cJSON *json = cJSON_CreateObject();
        cJSON *nets = cJSON_AddArrayToObject(json, "known");

        for(int i = 0; i < connection_cnt; i++) {
            cJSON *net = cJSON_CreateObject();
            cJSON_AddStringToObject(net, "ssid", connections[i].ssid);
            cJSON_AddItemToArray(nets, net);
        }
        char *s = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);

        resize_tcp_connect_state_result(con_state, strlen(s));

        memcpy(con_state->result, s, con_state->result_len);
        free(s);

        setResponseHeader(con_state, HTTP_JSON_RESPONSE_HEADERS, 200);
        for(int i = 0; i < connection_cnt; i++) {
            wifi_connection_t_delete(&connections[i]);
        }
    } else if(strncmp(request, HTTP_SETTINGS_LOCATION, strlen(HTTP_SETTINGS_LOCATION)) == 0) {
        ulog_info("Params: %s", params);

        char *token = strtok(params, "&");
        WIFI_SETUP_TYPE type = WIFI_SETUP_TYPE::NONE;
        char *typeStr = NULL;

        while(token != NULL) {
            char *valueStr = strchr(token, '=');
            if(valueStr != NULL) {
                valueStr++;  // skip '=' sign if it exist
            }

            if(strncmp(token, PARAM_TYPE, strlen(PARAM_TYPE)) == 0) {
                ulog_debug("Type found");
                if(strncmp(valueStr, PARAM_TYPE_ADD, strlen(PARAM_TYPE_ADD)) == 0) {
                    type = WIFI_SETUP_TYPE::ADD;
                } else if(strncmp(valueStr, PARAM_TYPE_CONNECT, strlen(PARAM_TYPE_CONNECT)) == 0) {
                    type = WIFI_SETUP_TYPE::CONNECT;

                } else if(strncmp(valueStr, PARAM_TYPE_DELETE, strlen(PARAM_TYPE_DELETE)) == 0) {
                    type = WIFI_SETUP_TYPE::DELETE;
                } else {
                    ulog_warn("Invalid Type %s", valueStr);
                }
                typeStr = token;

            } else if(strncmp(token, PARAM_SSID, strlen(PARAM_SSID)) == 0) {
                ulog_debug("ssid found");
                wifi_setup_data.ssid = strdup(valueStr);
            } else if(strncmp(token, PARAM_PASSWD, strlen(PARAM_PASSWD)) == 0) {
                wifi_setup_data.passwd = strdup(valueStr);
                ulog_debug("passwd found");
            } else {
                ulog_warn("Invalid Token %s", token);
            }

            token = strtok(NULL, "&");
        }

        switch(type) {
            case WIFI_SETUP_TYPE::NONE:
                break;
            case WIFI_SETUP_TYPE::ADD:
                ulog_debug("WIFISETUP: adding WiFi...");
                wifi.disableAP();
                wifi_status = wifi.addConnection(wifi_setup_data.ssid, wifi_setup_data.passwd);
                break;
            case WIFI_SETUP_TYPE::CONNECT:
                ulog_debug("WIFISETUP: connecting to WiFi...");
                wifi.disableAP();
                wifi_status = wifi.enableConnection(wifi_setup_data.ssid);
                break;
            case WIFI_SETUP_TYPE::DELETE:
                ulog_debug("WIFISETUP: deleting WiFi...");
                wifi.deleteConnection(wifi_setup_data.ssid);
                break;
        }

        if(type == WIFI_SETUP_TYPE::DELETE) {
            char tmp[HTTP_RESPONSE_SETTINGS_BUFFER_LEN];
            snprintf(tmp, HTTP_RESPONSE_SETTINGS_BUFFER_LEN, HTTP_RESPONSE_SETTINGS, typeStr, "success");

            resize_tcp_connect_state_result(con_state, strlen(tmp));
            memcpy(con_state->result, tmp, con_state->result_len);

            setResponseHeader(con_state, HTTP_JSON_RESPONSE_HEADERS, 200);
        }

    } else {
        con_state->header_len = snprintf(con_state->headers, con_state->allocated_header_len, HTTP_RESPONSE_REDIRECT, ipaddr_ntoa(con_state->gw));
    }
    return true;
}

static err_t cbk_tcp_server_poll(void *arg, struct tcp_pcb *pcb) {
    // tcp_connect_state_t *con_state = (tcp_connect_state_t *)arg;
    ulog_info("tcp_server_poll_fn");
    return tcp_close_client_connection((tcp_connect_state_t *)arg, pcb, ERR_OK);  // Just disconnect clent?
}

static bool tcp_server_open(void *arg) {
    tcp_server_t *state = (tcp_server_t *)arg;
    ulog_info("starting server on port %d", TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if(!pcb) {
        ulog_error("failed to create pcb");
        return false;
    }

    err_t err = tcp_bind(pcb, IP_ANY_TYPE, TCP_PORT);
    if(err) {
        ulog_error("failed to bind to port %d", TCP_PORT);
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if(!state->server_pcb) {
        ulog_error("failed to listen");
        if(pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, cbk_tcp_server_accept);
    return true;
}

static void cbk_tcp_server_err(void *arg, err_t err) {
    // tcp_connect_state_t *con_state = (tcp_connect_state_t *)arg;
    if(err != ERR_ABRT) {
        ulog_error("tcp_client_err_fn %", err);
        tcp_close_client_connection((tcp_connect_state_t *)arg, ((tcp_connect_state_t *)arg)->pcb, err);
    }
}

static err_t cbk_tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    tcp_server_t *state = (tcp_server_t *)arg;
    if(err != ERR_OK || client_pcb == NULL) {
        ulog_warn("failure in accept");
        return ERR_VAL;
    }
    ulog_info("client connected");

    // Create the state for the connection
    tcp_connect_state_t *con_state = create_tcp_connect_state();
    if(!con_state) {
        ulog_warn("failed to allocate connect state");
        return ERR_MEM;
    }
    con_state->pcb = client_pcb;  // for checking
    con_state->gw = &state->gw;

    // setup connection to client
    tcp_arg(client_pcb, con_state);
    tcp_sent(client_pcb, cbk_tcp_server_sent);
    tcp_recv(client_pcb, cbk_tcp_server_recv);
    tcp_poll(client_pcb, cbk_tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, cbk_tcp_server_err);
    return ERR_OK;
}

static err_t cbk_tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    tcp_connect_state_t *con_state = (tcp_connect_state_t *)arg;
    uint16_t actual_header_len = 0;
    if(!p) {
        ulog_info("connection closed");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    assert(con_state && con_state->pcb == pcb);
    if(p->tot_len > 0) {
        ulog_debug("tcp_server_recv %d err %d", p->tot_len, err);
#if 0
        for (struct pbuf *q = p; q != NULL; q = q->next) {
            DEBUG_printf("in: %.*s\n", q->len, q->payload);
        }
#endif

        // create buffer
        uint32_t request_buffer_len = p->tot_len + 1;
        char *request_buffer = (char *)malloc(request_buffer_len);

        // Copy the request into the buffer
        // pbuf_copy_partial(p, con_state->headers, con_state->allocated_header_len, 0);
        pbuf_copy_partial(p, request_buffer, request_buffer_len, 0);

        // Handle GET request
        if(strncmp(HTTP_GET, request_buffer, sizeof(HTTP_GET) - 1) == 0) {
            uint16_t request_len = strstr(request_buffer, "HTTP/") - request_buffer - sizeof(HTTP_GET);
            // char *request = request_buffer + sizeof(HTTP_GET);  // + space

            char *url = (char *)calloc(1, request_len + 1);
            uri_decode(request_buffer + sizeof(HTTP_GET), request_len, url);
            if(url[strlen(url) - 1] == ' ') {
                url[strlen(url) - 1] = 0;
            }
            char *params = strchr(url, '?');

            if(params) {
                params++;  // skip the "?"
            }
            char *l = strchr(url, '?');

            // Remove parameters from the url. The url only points to the requested location
            if(l) {
                l[0] = 0;
            }

            // Generate content
            ulog_debug("Request: %s with Params %s", url, params);
            generate_response(con_state, url, params);
            free(url);
            free(request_buffer);

            // Send the headers to the client
            con_state->sent_len = 0;
            ulog_debug("Sending Header: \"%s\", %d Bytes", con_state->headers, con_state->header_len);
            err_t err = tcp_write(pcb, con_state->headers, con_state->header_len, 0);
            if(err != ERR_OK) {
                ulog_error("failed to write header data %d", err);
                return tcp_close_client_connection(con_state, pcb, err);
            }

            // Send the body to the client
            // ulog_info("Sending Body: \"%s\", %d Bytes", con_state->result, con_state->result_len);
            if(con_state->result_len) {
                err = tcp_write(pcb, con_state->result, con_state->result_len, 0);
                if(err != ERR_OK) {
                    ulog_error("failed to write result data %d", err);
                    return tcp_close_client_connection(con_state, pcb, err);
                }
            }
        } else if(strncmp(HTTP_POST, con_state->headers, sizeof(HTTP_POST) - 1) == 0) {  // POST
            ulog_info("POST request: %s", con_state->headers);
            ulog_info("POST body: %s", p->payload);
        }
        tcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);
    return ERR_OK;
}

static void tcp_server_close(tcp_server_t *state) {
    if(state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
}

static err_t cbk_tcp_server_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
    tcp_connect_state_t *con_state = (tcp_connect_state_t *)arg;
    ulog_debug("tcp_server_sent %u", len);
    con_state->sent_len += len;
    if(con_state->sent_len >= con_state->allocated_header_len + con_state->result_len) {
        ulog_debug("all done");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    return ERR_OK;
}

// free result
static int32_t getStaticFile(const char *path, char **content) {
    if(strlen(path) < 2) {
        return false;
    }
    FIL fp;
    FRESULT fr;
    FSIZE_t size;
    UINT bytes_read;

    fr = f_open(&fp, path, FA_READ);
    if(fr != FR_OK) {
        ulog_error("Error while opening %s->%s", path, FRESULT_str(fr));
        if(fr == FR_NO_FILE || fr == FR_NO_PATH) {
            return STATIC_FILE_NOT_FOUND;
        }
        return STATIC_FILE_DISK_ERROR;
    }

    size = f_size(&fp);
    *content = (char *)calloc(1, size + 2);
    if(*content == NULL) {
        f_close(&fp);
        ulog_fatal("Failed to allocated %d Bytes while reading %s", size, path);
        return STATIC_FILE_DISK_ERROR;
    }

    fr = f_read(&fp, *content, size, &bytes_read);
    if(fr != FR_OK) {
        ulog_error("Error while reading %s->%s", path, FRESULT_str(fr));
        f_close(&fp);
        return STATIC_FILE_DISK_ERROR;
    }

    f_close(&fp);
    return bytes_read;
}

static void destroy_tcp_connect_state(tcp_connect_state_t *conn) {
    free(conn->headers);
    free(conn->result);
    free(conn);
}

static tcp_connect_state_t *create_tcp_connect_state() {
    tcp_connect_state_t *con = (tcp_connect_state_t *)calloc(1, sizeof(tcp_connect_state_t));
    con->allocated_header_len = HTTP_HEADER_LEN;
    con->headers = (char *)calloc(1, HTTP_HEADER_LEN);
    con->result = NULL;
    return con;
}

static bool resize_tcp_connect_state_result(tcp_connect_state_t *con, int result_len) {
    con->result_len = result_len;
    con->result = (char *)realloc(con->result, con->result_len);
    if(con->result == NULL) {
        ulog_fatal("Can't allocate %d Bytes for http result", con->result_len);
        return false;
    }
    return true;
}

static uint16_t setResponseHeader(tcp_connect_state_t *con, const char *header_template, uint16_t status) {
    memset(con->headers, 0, con->allocated_header_len);
#define STATUS_TEXT_LEN 30
    char status_text[STATUS_TEXT_LEN];
    memset(status_text, 0, STATUS_TEXT_LEN);

    switch(status) {
        default:
        case 200:
            memcpy(status_text, "OK", strlen("OK"));
            break;
        case 404:
            memcpy(status_text, "Not Found", strlen("Not Found"));
            break;
        case 500:
            memcpy(status_text, "Internal Server Error", strlen("Internal Server Error"));
            break;
    }
    con->header_len = snprintf(con->headers, con->allocated_header_len, header_template, status, status_text, con->result_len);
    ulog_info("Setting Response header for %s", status_text);
    return con->header_len;
}

static size_t uri_decode(const char *src, const size_t len, char *dst) {
    size_t i = 0, j = 0;
    while(i < len) {
        int copy_char = 1;
        if(src[i] == '%' && i + 2 < len) {
            const unsigned char v1 = hexval[(unsigned char)src[i + 1]];
            const unsigned char v2 = hexval[(unsigned char)src[i + 2]];

            /* skip invalid hex sequences */
            if((v1 | v2) != 0xFF) {
                dst[j] = (v1 << 4) | v2;
                j++;
                i += 3;
                copy_char = 0;
            }
        }
        if(copy_char) {
            dst[j] = src[i];
            i++;
            j++;
        }
    }
    dst[j] = '\0';
    return j;
}