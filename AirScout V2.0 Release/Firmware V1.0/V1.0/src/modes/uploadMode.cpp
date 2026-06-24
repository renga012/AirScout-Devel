#include "uploadMode.h"

#include <ff.h>
#include <hardware/regs/dreq.h>
#include <lwip/altcp_tls.h>
#include <lwip/dns.h>
#include <lwip/pbuf.h>
#include <pico/cyw43_arch.h>
#include <pico/time.h>
#include <stdio.h>
#include <string.h>
#include <ulog.h>

extern "C" {
// mbedtls...
#ifdef NDEBUG
void cyw43_thread_lock_check(void) {
}
#endif
}

#include "driver/display.h"
#include "driver/fs.h"
#include "driver/init_hardware.h"
#include "gui/elements/menu_bar.h"
#include "gui/elements/status_bar.h"
#include "gui/gui_utils.h"
#include "main.h"
#include "utils.h"
#include "wifi.h"

static void tls_client_connect_to_server_ip(const ip_addr_t *ipaddr, tls_client_t *state);
static bool tls_client_open(const char *hostname, void *arg);
static tls_client_t *tls_client_init(void);

// wifi, tcp client, progress bar, cancel button, finish popup with exit, no wifi found error
static bool lastComplete = true;
static struct altcp_tls_config *tls_config = NULL;
static ip_addr_t server_ip;
// static char *nextPayload = NULL;
static tls_client_t *tls_client;

bool run_tls_client_test(const uint8_t *cert, size_t cert_len, const char *server, char *request, int timeout);

bool UploadMode::init() {
    GUI::clear_except_status_menu(Display::COLOR::BLACK);

    menu_bar.setLayout(GUI::Menu_Bar::Icons::BACK, GUI::Menu_Bar::Icons::NONE, GUI::Menu_Bar::Icons::NONE, GUI::Menu_Bar::Icons::NONE);
    user_settings.getint("last_chunk_index", &m_toUpload);
    m_total = m_toUpload;

    disp.draw_text(10, STATUS_BAR_HEIGHT + 10, 1, "Connecting to wifi..", &font_Liberation_Mono10x16, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);

    WSTATUS ws = wifi.connectToAvailable();
    m_error = false;
    if(ws != WSTATUS::OK) {
        ulog_warn("Wifi connection failed, retry...");
        ws = wifi.connectToAvailable();
        if(ws != WSTATUS::OK) {
            m_error = true;
            ulog_warn("Wifi connection failed");
        }
    } else {
        ulog_info("Wifi Connected");
    }

    ulog_trace("m_error: %d", m_error);
    if(m_error) {
        disp.draw_text(10, STATUS_BAR_HEIGHT + 10, 1, "Wifi connection Failed", &font_Liberation_Mono10x16, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);
        return false;
    }

    // m_progress_bar = GUI::Progress_Bar(&disp, 0, m_total);
    // m_progress_bar.setTitle("Upload Progress");
    // GUI::activateElement(&m_progress_bar);
    // disp.fill_round_rectangle(20, 100, 280, 160, 20, Display::COLOR::GRAY);

    char tmp[12];
    snprintf(tmp, sizeof(tmp), "%05d/%05d", 0, m_total);
    disp.draw_text(30, 110, 1, tmp, &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);

    tls_config = altcp_tls_create_config_client(NULL, 0);
    tls_client = tls_client_init();

    // tls_client->http_request_len = strlen(TLS_CLIENT_HTTP_REQUEST) + 1;
    // tls_client->http_request = (char *)calloc(1, strlen(TLS_CLIENT_HTTP_REQUEST) + 1);
    // memcpy(tls_client->http_request, TLS_CLIENT_HTTP_REQUEST, tls_client->http_request_len);

    // bool pass = run_tls_client_test(NULL, 0, TLS_CLIENT_SERVER, TLS_CLIENT_HTTP_REQUEST, TLS_CLIENT_TIMEOUT_SECS);
    tls_client->timeout = 15;
    if(!tls_client_open(UPLOAD_HOST, tls_client)) {
        ulog_error("Couldn't open tls client");
        return false;
    }

    /*
    if(!resolve_hostname(&server_ip, UPLOAD_HOST)) {
        ulog_warn("Cant resolve %s", UPLOAD_HOST);
        error = true;
    }

    m_pcb = NULL;
    if(!connect_to_host(&server_ip, &m_pcb)) {
        // printf("Failed to connect to https://%s:%d\n", char_ipaddr, LWIP_IANA_PORT_HTTPS);
        // TODO: Disconnect from network
        ulog_warn("Connection to %s failed", UPLOAD_HOST);
        error = true;
    } else {
        ulog_info("Connected to %s", UPLOAD_HOST);
    }
    */

    return true;
}

void UploadMode::run() {
    static absolute_time_t lastBTNCheck = 0;
    static absolute_time_t lastUploadCheck = 0;

    if(absolute_time_diff_us(lastBTNCheck, get_absolute_time()) > BUTTON_CHECK_INTERVAL_MS * 1000) {
        lastBTNCheck = get_absolute_time();
        BTN btn;
        do {
            btn = getNextButton();
            switch(btn) {
                case BTN::BTN1:
                    switchToMode(MODES::Idle);
                    return;
                    break;
                case BTN::BTN2:
                // pause
                default:
                    break;
            }
        } while(btn != BTN::NONE);
    }

    if(absolute_time_diff_us(lastUploadCheck, get_absolute_time()) > 100 * 1000 && !m_error) {
        lastUploadCheck = get_absolute_time();
        if(m_toUpload <= 0) {
            m_finished = true;
            ulog_info("Upload finished");
        }

        if(lastComplete && !m_finished) {
            lastComplete = false;
            m_uploadNext();
        }

        if(tls_client->error) {
            // lastComplete = true;
            disp.draw_text(10, 200, 1, "Upload error", &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);
        }
    }
}

void UploadMode::exit() {
    GUI::resetActiveElements();
    free(tls_client);
    altcp_tls_free_config(tls_config);
}

void UploadMode::m_uploadNext() {
    if(server_ip.addr == 0) {
        lastComplete = true;
        ulog_warn("Invalid IP-Address: %s", ipaddr_ntoa(&server_ip));
        return;
    }
    
    char *data = m_loadNextFile();
    if(data == NULL && m_toUpload > 0) {  // sometimes the last file is not actually saved
        m_toUpload--;
        lastComplete = true;
        return;
    }

    // free(data);
    // lastComplete = true;
    // return;

    tls_client->http_request_len = UPLOAD_HEADER_LEN + strlen(data) + 1;
    tls_client->http_request = (char *)realloc(tls_client->http_request, tls_client->http_request_len);

    // snprintf(tls_client->http_request, tls_client->http_request_len, HTTP_GET_TEMPLATE, UPLOAD_HOST_PATH, UPLOAD_HOST);
    // snprintf(tls_client->http_request, tls_client->http_request_len, HTTP_POST_JSON_TEMPLATE, UPLOAD_HOST_PATH, UPLOAD_HOST, 0, "");
    snprintf(tls_client->http_request, tls_client->http_request_len, HTTP_POST_JSON_TEMPLATE, UPLOAD_HOST_PATH, UPLOAD_HOST, strlen(data), data);

    free(data);
    ulog_info("POST:\n%s", tls_client->http_request);

    cyw43_arch_lwip_begin();
    tls_client_connect_to_server_ip(&server_ip, tls_client);
    cyw43_arch_lwip_end();

    ulog_info("Request sent");

    // lastComplete = true;
}

char *UploadMode::m_loadNextFile() {
    char *nextPayload = NULL;
    FIL fp;
    FRESULT fr;
    char path[100];
    sprintf(path, "/chunks/chunk_%d.json", m_toUpload);

    fr = f_open(&fp, path, FA_READ);
    if(fr != FR_OK) {
        ulog_warn("Error while opening %s\n%s", path, FRESULT_str(fr));
        return NULL;
    }

    uint64_t size = f_size(&fp);
    UINT bytes_read = 0;
    nextPayload = (char *)calloc(size, 1);
    if(nextPayload == NULL) {
        ulog_fatal("Failed to allocate Buffer with %llu Bytes for POST Data", size);
        return NULL;
    }

    fr = f_read(&fp, nextPayload, size, &bytes_read);
    if(fr != FR_OK) {
        ulog_error("Error while reading %s\n%s", path, FRESULT_str(fr));
        free(nextPayload);
        return NULL;
    }

    fr = f_close(&fp);
    if(fr != FR_OK) {
        ulog_error("Error while closing %s\n%s", path, FRESULT_str(fr));
    }
    m_toUpload--;
    // m_progress_bar.setCurrent(m_total - m_toUpload);

    char tmp[12];
    snprintf(tmp, sizeof(tmp), "%05d/%05d", m_total - m_toUpload, m_total);
    disp.draw_text(30, 110, 1, tmp, &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);

    return nextPayload;
}

static err_t tls_client_close(void *arg) {
    tls_client_t *state = (tls_client_t *)arg;
    err_t err = ERR_OK;

    state->complete = true;
    if(state->pcb != NULL) {
        altcp_arg(state->pcb, NULL);
        altcp_poll(state->pcb, NULL, 0);
        altcp_recv(state->pcb, NULL);
        altcp_err(state->pcb, NULL);
        err = altcp_close(state->pcb);
        if(err != ERR_OK) {
            printf("close failed %d, calling abort\n", err);
            altcp_abort(state->pcb);
            err = ERR_ABRT;
        }
        state->pcb = NULL;
    }
    return err;
}

static err_t tls_client_connected(void *arg, struct altcp_pcb *pcb, err_t err) {
    tls_client_t *state = (tls_client_t *)arg;
    if(err != ERR_OK) {
        printf("connect failed %d\n", err);
        return tls_client_close(state);
    }

    printf("connected to server, sending request\n");
    err = altcp_write(state->pcb, state->http_request, strlen(state->http_request), TCP_WRITE_FLAG_COPY);
    if(err != ERR_OK) {
        printf("error writing data, err=%d", err);
        return tls_client_close(state);
    }

    return ERR_OK;
}

static err_t tls_client_poll(void *arg, struct altcp_pcb *pcb) {
    tls_client_t *state = (tls_client_t *)arg;
    printf("timed out\n");
    state->error = PICO_ERROR_TIMEOUT;
    return tls_client_close(arg);
}

static void tls_client_err(void *arg, err_t err) {
    tls_client_t *state = (tls_client_t *)arg;
    ulog_error("tls_client_err %d", err);
    tls_client_close(state);
    state->error = PICO_ERROR_GENERIC;
}

static err_t tls_client_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err) {
    tls_client_t *state = (tls_client_t *)arg;
    if(!p) {
        printf("connection closed\n");
        return tls_client_close(state);
    }

    if(p->tot_len > 0) {
        /* For simplicity this examples creates a buffer on stack the size of the data pending here,
           and copies all the data to it in one go.
           Do be aware that the amount of data can potentially be a bit large (TLS record size can be 16 KB),
           so you may want to use a smaller fixed size buffer and copy the data to it using a loop, if memory is a concern */
        char buf[p->tot_len + 1];

        pbuf_copy_partial(p, buf, p->tot_len, 0);
        buf[p->tot_len] = 0;

        printf("***\nnew data received from server:\n***\n\n%s\n", buf);

        altcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);
    lastComplete = true;
    return ERR_OK;
}

static void tls_client_connect_to_server_ip(const ip_addr_t *ipaddr, tls_client_t *state) {
    err_t err;
    u16_t port = 443;

    printf("connecting to server IP %s port %d\n", ipaddr_ntoa(ipaddr), port);
    err = altcp_connect(state->pcb, ipaddr, port, tls_client_connected);
    if(err != ERR_OK) {
        fprintf(stderr, "error initiating connect, err=%d\n", err);
        tls_client_close(state);
    }
}

static void tls_client_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    server_ip = *ipaddr;
    if(ipaddr) {
        printf("DNS resolving complete\n");
        // tls_client_connect_to_server_ip(ipaddr, (tls_client_t *)arg);
    } else {
        printf("error resolving hostname %s\n", hostname);
        tls_client_close(arg);
    }
}

static bool tls_client_open(const char *hostname, void *arg) {
    err_t err;
    tls_client_t *state = (tls_client_t *)arg;

    state->pcb = altcp_tls_new(tls_config, IPADDR_TYPE_ANY);
    if(!state->pcb) {
        printf("failed to create pcb\n");
        return false;
    }

    altcp_arg(state->pcb, state);
    altcp_poll(state->pcb, tls_client_poll, state->timeout * 2);
    altcp_recv(state->pcb, tls_client_recv);
    altcp_err(state->pcb, tls_client_err);

    /* Set SNI */
    int ret = mbedtls_ssl_set_hostname((mbedtls_ssl_context *)altcp_tls_context(state->pcb), hostname);
    ulog_debug("resolving %s", hostname);

    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();

    err = dns_gethostbyname(hostname, &server_ip, tls_client_dns_found, state);
    if(err == ERR_OK) {
        /* host is in DNS cache */
        // tls_client_connect_to_server_ip(&server_ip, state);
    } else if(err != ERR_INPROGRESS) {
        printf("error initiating DNS resolving, err=%d\n", err);
        tls_client_close(state->pcb);
    }

    cyw43_arch_lwip_end();

    return err == ERR_OK || err == ERR_INPROGRESS;
}

// Perform initialisation
static tls_client_t *tls_client_init(void) {
    tls_client_t *state = (tls_client_t *)calloc(1, sizeof(tls_client_t));
    if(!state) {
        printf("failed to allocate state\n");
        return NULL;
    }

    return state;
}

bool run_tls_client_test(const uint8_t *cert, size_t cert_len, const char *server, char *request, int timeout) {
    /* No CA certificate checking */
    tls_config = altcp_tls_create_config_client(cert, cert_len);
    assert(tls_config);

    // mbedtls_ssl_conf_authmode(&tls_config->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);

    tls_client_t *state = tls_client_init();
    if(!state) {
        return false;
    }
    state->http_request = request;
    state->timeout = timeout;
    if(!tls_client_open(server, state)) {
        return false;
    }
    while(!state->complete) {
        // the following #ifdef is only here so this same example can be used in multiple modes;
        // you do not need it in your code
#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for Wi-Fi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        // you can poll as often as you like, however if you have nothing else to do you can
        // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        sleep_ms(1000);
#endif
    }
    int err = state->error;
    free(state);
    altcp_tls_free_config(tls_config);
    return err == 0;
}
