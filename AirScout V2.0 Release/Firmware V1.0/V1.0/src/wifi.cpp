#include "wifi.h"
#include "ntp.h"

extern "C" {
#include <cyw43.h>
#include <cyw43_ll.h>
}

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reent.h>
#include <ulog.h>

#include "main.h"
#include "settings.h"

WifiManager wifi;

static uint16_t cbk_scan_result_count = 0;
static cyw43_ev_scan_result_t *cbk_scan_results = NULL;

WifiManager::WifiManager() {
    m_ap_active = false;
    m_sta_active = false;
    cbk_scan_results = NULL;
    cbk_scan_result_count = 0;
}

WSTATUS WifiManager::enableStation() {
    cyw43_arch_enable_sta_mode();
    m_sta_active = true;
    return WSTATUS::OK;
}

WSTATUS WifiManager::disableStation() {
    disconnect();
    cyw43_arch_disable_sta_mode();
    m_sta_active = false;
    return WSTATUS::OK;
}

WSTATUS WifiManager::connectToNetwork(const char *ssid, const char *passwd, const WAUTH auth_type) {
    enableStation();
    int err;
    if(strlen(passwd) == 0) {
        err = cyw43_arch_wifi_connect_timeout_ms(ssid, passwd, CYW43_AUTH_OPEN, 10000);
    } else {
        err = cyw43_arch_wifi_connect_timeout_ms(ssid, passwd, (uint32_t)auth_type, 10000);
    }

    if(err == 0) {
        setRTCviaNTP();
        m_currently_connected = strdup(ssid);
        return WSTATUS::OK;
    }
    free(m_currently_connected);
    

    WSTATUS s;

    switch(err) {
        case PICO_OK:  ///< No error; the operation succeeded
            s = WSTATUS::OK;
            break;
        case PICO_ERROR_GENERIC:
            s = WSTATUS::FAIL;
            break;  ///< An unspecified error occurred
        case PICO_ERROR_TIMEOUT:
            s = WSTATUS::TIMEOUT;
            break;  ///< The function failed due to timeout
        case PICO_ERROR_NO_DATA:
            s = WSTATUS::FAIL;
            break;  ///< Attempt for example to read from an empty buffer/FIFO
        case PICO_ERROR_NOT_PERMITTED:
            s = WSTATUS::FAIL;
            break;  ///< Permission violation e.g. write to read-only flash partition, or security violation
        case PICO_ERROR_INVALID_ARG:
            s = WSTATUS::FAIL;
            break;  ///< Argument is outside of range of supported values`
        case PICO_ERROR_IO:
            s = WSTATUS::FAIL;
            break;  ///< An I/O error occurred
        case PICO_ERROR_BADAUTH:
            s = WSTATUS::BADAUTH;
            break;  ///< The authorization failed due to bad credentials
        case PICO_ERROR_CONNECT_FAILED:
            s = WSTATUS::FAIL;
            break;  ///< The connection failed
        case PICO_ERROR_INSUFFICIENT_RESOURCES:
            s = WSTATUS::FAIL;
            break;  ///< Dynamic allocation of resources failed
        case PICO_ERROR_INVALID_ADDRESS:
            s = WSTATUS::FAIL;
            break;  ///< Address argument was out-of-bounds or was determined to be an address that the caller may not access
        case PICO_ERROR_BAD_ALIGNMENT:
            s = WSTATUS::FAIL;
            break;  ///< Address was mis-aligned (usually not on word boundary)
        case PICO_ERROR_INVALID_STATE:
            s = WSTATUS::FAIL;
            break;  ///< Something happened or failed to happen in the past, and consequently we (currently) can't service the request
        case PICO_ERROR_BUFFER_TOO_SMALL:
            s = WSTATUS::FAIL;
            break;  ///< A user-allocated buffer was too small to hold the result or working state of this function
        case PICO_ERROR_PRECONDITION_NOT_MET:
            s = WSTATUS::FAIL;
            break;  ///< The call failed because another function must be called first
        case PICO_ERROR_MODIFIED_DATA:
            s = WSTATUS::FAIL;
            break;  ///< Cached data was determined to be inconsistent with the actual version of the data
        case PICO_ERROR_INVALID_DATA:
            s = WSTATUS::FAIL;
            break;  ///< A data structure failed to validate
        case PICO_ERROR_NOT_FOUND:
            s = WSTATUS::FAIL;
            break;  ///< Attempted to access something that does not exist; or, a search failed
        case PICO_ERROR_UNSUPPORTED_MODIFICATION:
            s = WSTATUS::FAIL;
            break;  ///< Write is impossible based on previous writes; e.g. attempted to clear an OTP bit
        case PICO_ERROR_LOCK_REQUIRED:
            s = WSTATUS::FAIL;
            break;  ///< A required lock is not owned
        case PICO_ERROR_VERSION_MISMATCH:
            s = WSTATUS::FAIL;
            break;  ///< A version mismatch occurred (e.g. trying to run PIO version 1 code on RP2040)
        case PICO_ERROR_RESOURCE_IN_USE:
            s = WSTATUS::FAIL;
            break;  ///< The call could not proceed because the required resources were unavailable
    }

    return m_convert_status(err);
}

WSTATUS WifiManager::connectToAvailable() {
    cyw43_ev_scan_result_t *scan_results;
    uint16_t scan_result_cnt;
    wifi_connection_t *wifi_data;
    uint16_t wifi_data_len;
    int16_t found = -1;
    enableStation();
    scan(&scan_results, &scan_result_cnt);
    user_settings.getWifiConnections(&wifi_data, &wifi_data_len);

    for(uint16_t scn_i = 0; scn_i < scan_result_cnt; scn_i++) {
        for(uint16_t data_i = 0; data_i < wifi_data_len; data_i++) {
            // check ssid
            if(strncmp((const char *)scan_results[scn_i].ssid, wifi_data[data_i].ssid, MIN(scan_results[scn_i].ssid_len, strlen(wifi_data[data_i].ssid))) == 0) {
                found = data_i;
                break;
            }
        }
        if(found != -1) {
            break;
        }
    }
    if(found == -1){
        return WSTATUS::NONET;
    }

    WSTATUS status;
    ulog_info("Connecting to %s...", wifi_data[found].ssid);
    if(strlen(wifi_data[found].passwd) == 0) {  // Open network
        status = connectToNetwork(wifi_data[found].ssid, wifi_data[found].passwd, WAUTH::OPEN);
    } else {
        status = connectToNetwork(wifi_data[found].ssid, wifi_data[found].passwd, WAUTH::WPA2_AES_PSK);
    }

    for(uint16_t i = 0; i < wifi_data_len; i++) {
        wifi_connection_t_delete(&wifi_data[i]);
    }
    free(scan_results);
    return status;
}

WSTATUS WifiManager::disconnect() {
    free(m_currently_connected);
    m_currently_connected = NULL;
    if(cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA)) {
        return WSTATUS::FAIL;
    } else {
        return WSTATUS::OK;
    }
}

WSTATUS WifiManager::status_STA() {
    // cyw43_state
    int ret = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    switch(ret) {
        case CYW43_LINK_DOWN:     ///< link is down
            return WSTATUS::LINK_DOWN;

        case CYW43_LINK_BADAUTH:  ///< Authenticatation failure
            return WSTATUS::BADAUTH;

        case CYW43_LINK_FAIL:     ///< Connection failed
            return WSTATUS::FAIL;

        case CYW43_LINK_JOIN:     ///< Connected to wifi
            return WSTATUS::JOIN;

        case CYW43_LINK_NOIP:     ///< Connected to wifi, but no IP address
            return WSTATUS::NOIP;

        case CYW43_LINK_NONET:    ///< No matching SSID found (could be out of range, or down)
            return WSTATUS::NONET;

        case CYW43_LINK_UP:       ///< Connected to wifi with an IP address
            return WSTATUS::UP;

        default:
            return WSTATUS::FAIL;
    }
}

WSTATUS WifiManager::status_AP() {
    // cyw43_state
    int ret = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_AP);
    switch(ret) {
        case CYW43_LINK_DOWN:     ///< link is down
            return WSTATUS::LINK_DOWN;

        case CYW43_LINK_BADAUTH:  ///< Authenticatation failure
            return WSTATUS::BADAUTH;

        case CYW43_LINK_FAIL:     ///< Connection failed
            return WSTATUS::FAIL;

        case CYW43_LINK_JOIN:     ///< Connected to wifi
            return WSTATUS::JOIN;

        case CYW43_LINK_NOIP:     ///< Connected to wifi, but no IP address
            return WSTATUS::NOIP;

        case CYW43_LINK_NONET:    ///< No matching SSID found (could be out of range, or down)
            return WSTATUS::NONET;

        case CYW43_LINK_UP:       ///< Connected to wifi with an IP address
            return WSTATUS::UP;

        default:
            return WSTATUS::FAIL;
    }
}

// pass preallocated pointer (min 20 chars)
char *WifiManager::statusToStr(WSTATUS status) {
    static char str[20];
    switch(status) {
        case WSTATUS::OK:
            strcpy(str, "OK");
            break;

        case WSTATUS::LINK_DOWN:  ///< link is down
            strcpy(str, "LINK_DOWN");
            break;

        case WSTATUS::BADAUTH:  ///< Authenticatation failure
            strcpy(str, "BADAUTH");
            break;

        case WSTATUS::FAIL:  ///< Connection failed
            strcpy(str, "FAIL");
            break;

        case WSTATUS::JOIN:  ///< Connected to wifi
            strcpy(str, "JOIN");
            break;

        case WSTATUS::NOIP:  ///< Connected to wifi, but no IP address
            strcpy(str, "NOIP");
            break;

        case WSTATUS::NONET:  ///< No matching SSID found (could be out of range, or down)
            strcpy(str, "NONET");
            break;

        case WSTATUS::UP:  ///< Connected to wifi with an IP address
            strcpy(str, "UP");
            break;
        case WSTATUS::NONE:
            strcpy(str, "NONE");
            break;
        case WSTATUS::TIMEOUT:
            strcpy(str, "TIMEOUT");
            break;
    }
    return str;
}

int compareRSSI(const void *a, const void *b) {
    cyw43_ev_scan_result_t *_a = ((cyw43_ev_scan_result_t *)a);
    cyw43_ev_scan_result_t *_b = ((cyw43_ev_scan_result_t *)b);
    return (_a->rssi < _b->rssi) - (_a->rssi > _b->rssi);
}

// must free results.
WSTATUS WifiManager::scan(cyw43_ev_scan_result_t **scan_results, uint16_t *result_cnt) {
    if(cbk_scan_results != NULL) {
        free(cbk_scan_results);
        cbk_scan_results = NULL;
    }
    cbk_scan_result_count = 0;

    cyw43_wifi_scan_options_t scan_options = {0};
    int err = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, scan_result_cbk);

    if(err) {
        ulog_error("Scan Error\n");
        return WSTATUS::FAIL;
    }

    ulog_info("Wifi scan Starting...");
    while(cyw43_wifi_scan_active(&cyw43_state)) {
        tight_loop_contents();
    }

    *scan_results = NULL;
    *result_cnt = 0;
    if(cbk_scan_result_count <= 0) {
        return WSTATUS::OK;
    }

    qsort(cbk_scan_results, cbk_scan_result_count, sizeof(cyw43_ev_scan_result_t), compareRSSI);

    // for(int i = 0; i < cbk_scan_result_count; i++) {   printNet(&cbk_scan_results[i]);}

    *scan_results = (cyw43_ev_scan_result_t *)calloc(sizeof(cyw43_ev_scan_result_t), 1);
    if(*scan_results == NULL) {
        ulog_fatal("Allocation Error\n");
        return WSTATUS::FAIL;
    }

    bool found = false;
    for(int i = 0; i < cbk_scan_result_count; i++) {
        // ulog_debug("Checking: ssid: %-32s (%d long) rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u", cbk_scan_results[i].ssid, cbk_scan_results[i].ssid_len, cbk_scan_results[i].rssi, cbk_scan_results[i].channel, cbk_scan_results[i].bssid[0], cbk_scan_results[i].bssid[1], cbk_scan_results[i].bssid[2], cbk_scan_results[i].bssid[3], cbk_scan_results[i].bssid[4], cbk_scan_results[i].bssid[5], cbk_scan_results[i].auth_mode);
        // first result

        // no empty ssids
        if(cbk_scan_results[i].ssid_len == 0) {
            continue;
        }

        if(*result_cnt <= 0) {
            memcpy(*scan_results, cbk_scan_results, sizeof(cyw43_ev_scan_result_t));
            (*result_cnt)++;
            continue;
        }

        // check if the next net is already saved
        for(int j = 0; j < (*result_cnt); j++) {
            // ulog_debug("Comparing %-32s with %-32s", (char *)cbk_scan_results[i].ssid, (char *)((*scan_results)[j].ssid));
            found = !memcmp((char *)cbk_scan_results[i].ssid, (char *)((*scan_results)[j].ssid), 32);
            if(found)
                break;
        }
        if(found) {
            // ulog_debug("Net already saved");
            found = false;
            continue;
        }

        *scan_results = (cyw43_ev_scan_result_t *)realloc(*scan_results, sizeof(cyw43_ev_scan_result_t) * ((*result_cnt) + 1));
        if(*scan_results == NULL) {
            ulog_fatal("Allocation Error");
            return WSTATUS::FAIL;
        }

        // printf("Saving: "); printNet(&cbk_scan_results[i]);
        copyScanResults((*scan_results) + (*result_cnt), &cbk_scan_results[i]);
        (*result_cnt)++;
        // ulog_debug("\n Saved:"); for(int k = 0; k < (*result_cnt); k++) {printNet(&(*scan_results)[k]);}
    }

    free(cbk_scan_results);
    cbk_scan_results = NULL;
    cbk_scan_result_count = 0;

    ulog_info("Wifi scan complete");
    return WSTATUS::OK;
}

void WifiManager::printNet(cyw43_ev_scan_result_t *net) {
    printf("ssid: %-32s rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u\n", net->ssid, net->rssi, net->channel, net->bssid[0], net->bssid[1], net->bssid[2], net->bssid[3], net->bssid[4], net->bssid[5], net->auth_mode);
}

void copyScanResults(cyw43_ev_scan_result_t *dest, const cyw43_ev_scan_result_t *src) {
    memcpy(dest->_0, src->_0, 20);
    memcpy(dest->_1, src->_1, 4);
    memcpy(dest->_2, src->_2, 20);
    dest->_3 = src->_3;
    dest->auth_mode = src->auth_mode;
    memcpy(dest->bssid, src->bssid, 6);
    dest->channel = src->channel;
    dest->rssi = src->rssi;
    memcpy(dest->ssid, src->ssid, 32);
    dest->ssid_len = src->ssid_len;
}

// scan callback
// from pico-examples
int scan_result_cbk(void *env, const cyw43_ev_scan_result_t *result) {
    if(result) {
        if(cbk_scan_results == NULL) {
            cbk_scan_results = (cyw43_ev_scan_result_t *)malloc(sizeof(cyw43_ev_scan_result_t) * (cbk_scan_result_count + 1));
        } else {
            cbk_scan_results = (cyw43_ev_scan_result_t *)realloc(cbk_scan_results, sizeof(cyw43_ev_scan_result_t) * (cbk_scan_result_count + 1));
        }

        if(cbk_scan_results == NULL) {
            ulog_fatal("Allocation Error");
            return 0;
        }
        memcpy((cbk_scan_results) + cbk_scan_result_count, result, sizeof(cyw43_ev_scan_result_t));
        cbk_scan_result_count++;
        // printf("ssid: %-32s rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u\n", result->ssid, result->rssi, result->channel, result->bssid[0], result->bssid[1], result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5], result->auth_mode);
    }
    return 0;
}

WSTATUS WifiManager::addConnection(const char *ssid, const char *passwd) {
    WSTATUS s;
    if(strlen(passwd)) {
        s = connectToNetwork(ssid, passwd, WAUTH::WPA2_AES_PSK);
    } else {
        s = connectToNetwork(ssid, passwd, WAUTH::OPEN);
    }

    if(s == WSTATUS::BADAUTH) {
        return s;
    }
    user_settings.addWifiConnection(ssid, passwd);
    return WSTATUS::OK;
}

WSTATUS WifiManager::deleteConnection(const char *ssid) {
    user_settings.deleteWifiConnection(ssid);
    return WSTATUS::OK;
}

WSTATUS WifiManager::enableConnection(const char *ssid) {
    wifi_connection_t *wifi_data = NULL;
    uint16_t wifi_data_len = 0;
    int16_t found = -1;

    user_settings.getWifiConnections(&wifi_data, &wifi_data_len);
    for(int i = 0; i < wifi_data_len; i++) {
        if(strncmp(wifi_data[i].ssid, ssid, strlen(ssid)) == 0) {
            found = i;
            break;
        }
    }

    WSTATUS ret = connectToNetwork(wifi_data[found].ssid, wifi_data[found].passwd, WAUTH::WPA2_AES_PSK);

    for(int i = 0; i < wifi_data_len; i++) {
        wifi_connection_t_delete(&wifi_data[i]);
    }
    return ret;
}

WSTATUS WifiManager::enableAP(const char *ssid, const char *passwd, const WAUTH auth_type) {
    cyw43_arch_enable_ap_mode(ssid, passwd, (uint32_t)auth_type);

    m_ap_active = true;
    return WSTATUS::OK;
}

WSTATUS WifiManager::disableAP() {
    cyw43_arch_disable_ap_mode();

    m_ap_active = false;
    return WSTATUS::OK;
}
// https://forums.raspberrypi.com/viewtopic.php?t=367720
WSTATUS WifiManager::setHostname(const char *hostname) {
    cyw43_arch_lwip_begin();
    struct netif *n = &cyw43_state.netif[CYW43_ITF_STA];
    netif_set_hostname(n, hostname);
    netif_set_up(n);
    cyw43_arch_lwip_end();
    return WSTATUS::OK;
}

// https://forums.raspberrypi.com/viewtopic.php?t=374085
WSTATUS WifiManager::getAPIP(ip_addr_t *ipaddr) {
    if(status_AP() == WSTATUS::JOIN || status_AP() == WSTATUS::UP) {
        memcpy(ipaddr, &cyw43_state.netif[CYW43_ITF_AP].ip_addr, sizeof(ip_addr_t));
        return WSTATUS::OK;
    }
    return WSTATUS::FAIL;
}

WSTATUS WifiManager::getSTAIP(ip_addr_t *ipaddr) {
    if(status_STA() == WSTATUS::JOIN || status_STA() == WSTATUS::UP) {
        memcpy(ipaddr, &cyw43_state.netif[CYW43_ITF_AP].ip_addr, sizeof(ip_addr_t));
        return WSTATUS::OK;
    }
    return WSTATUS::FAIL;
}

// NOT WORKY
// must free macs_buffer
WSTATUS WifiManager::getConnectedClients(uint8_t **macs_buffer, int *count) {
    macs_buffer = NULL;

    cyw43_wifi_ap_get_max_stas(&cyw43_state, count);

    *macs_buffer = (uint8_t *)calloc(1, *count);
    if(macs_buffer == NULL) {
        ulog_fatal("Cant allocate memory for macs");
        return WSTATUS::FAIL;
    }
    cyw43_wifi_ap_get_stas(&cyw43_state, count, *macs_buffer);
    return WSTATUS::OK;
}

WSTATUS WifiManager::m_convert_status(int status) {
    switch(status) {
        case CYW43_LINK_DOWN:     ///< link is down
            return WSTATUS::LINK_DOWN;

        case CYW43_LINK_BADAUTH:  ///< Authenticatation failure
            return WSTATUS::BADAUTH;

        case CYW43_LINK_FAIL:     ///< Connection failed
            return WSTATUS::FAIL;

        case CYW43_LINK_JOIN:     ///< Connected to wifi
            return WSTATUS::JOIN;

        case CYW43_LINK_NOIP:     ///< Connected to wifi, but no IP address
            return WSTATUS::NOIP;

        case CYW43_LINK_NONET:    ///< No matching SSID found (could be out of range, or down)
            return WSTATUS::NONET;

        case CYW43_LINK_UP:       ///< Connected to wifi with an IP address
            return WSTATUS::UP;

        default:
            return WSTATUS::FAIL;
    }
}