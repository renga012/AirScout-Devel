#ifndef WIFI_H
#define WIFI_H
extern "C" {
#include <cyw43_ll.h>
#include <pico/cyw43_arch.h>
}

#include <stdint.h>

enum class WSTATUS { NONE = 0, OK, LINK_DOWN, JOIN, NOIP, UP, FAIL, NONET, BADAUTH, TIMEOUT };
enum class WAUTH {
    OPEN = 0,                       ///< No authorisation required (open)
    WPA_TKIP_PSK = 0x00200002,      ///< WPA authorisation
    WPA2_AES_PSK = 0x00400004,      ///< WPA2 authorisation (preferred)
    WPA2_MIXED_PSK = 0x00400006,    ///< WPA2/WPA mixed authorisation
    WPA3_SAE_AES_PSK = 0x01000004,  ///< WPA3 AES authorisation
    WPA3_WPA2_AES_PSK = 0x01400004
};

class WifiManager {
public:
    WifiManager();
    WSTATUS enableStation();
    WSTATUS disableStation();
    WSTATUS connectToNetwork(const char *ssid, const char *passwd, const WAUTH auth_type);
    WSTATUS connectToAvailable();
    WSTATUS disconnect();
    WSTATUS status_STA();
    char *statusToStr(WSTATUS status);
    WSTATUS getSTAIP(ip_addr_t *ipaddr);

    WSTATUS scan(cyw43_ev_scan_result_t **scan_results, uint16_t *result_cnt);
    WSTATUS addConnection(const char *ssid, const char *passwd);
    WSTATUS deleteConnection(const char *ssid);
    WSTATUS enableConnection(const char *ssid);

    WSTATUS enableAP(const char *ssid, const char *passwd, const WAUTH auth_type);
    WSTATUS disableAP();
    WSTATUS status_AP();
    WSTATUS getAPIP(ip_addr_t *ipaddr);
    WSTATUS getConnectedClients(uint8_t **macs_buffer, int *count);
    WSTATUS setHostname(const char *hostname);
    // tmp
    void printNet(cyw43_ev_scan_result_t *net);
    char *getCurrentlyConnected(){
        return m_currently_connected;
    }
    // void scan();

private:
    WSTATUS m_convert_status(int status);
    bool m_sta_active;
    bool m_ap_active;
    char *m_currently_connected = NULL;
};

int scan_result_cbk(void *env, const cyw43_ev_scan_result_t *result);
void copyScanResults(cyw43_ev_scan_result_t *dest, const cyw43_ev_scan_result_t *src);

extern WifiManager wifi;

#endif  // WIFI_H
