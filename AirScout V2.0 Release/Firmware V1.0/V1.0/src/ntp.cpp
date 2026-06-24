#include "ntp.h"

extern "C" {
#include <lwip/dns.h>
#include <lwip/pbuf.h>
#include <lwip/udp.h>
}
#include <hardware/rtc.h>
#include <pico/cyw43_arch.h>
#include <pico/platform/common.h>
#include <pico/stdlib.h>
#include <pico/types.h>
#include <pico/util/datetime.h>
#include <string.h>
#include <time.h>
#include <ulog.h>

typedef struct NTP_T_ {
    ip_addr_t ntp_server_address;
    struct udp_pcb *ntp_pcb;
    async_at_time_worker_t request_worker;
    async_at_time_worker_t resend_worker;
    bool end;
} NTP_T;

#define NTP_SERVER         "pool.ntp.org"
#define NTP_MSG_LEN        48
#define NTP_PORT           123
#define NTP_DELTA          2208988800  // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_TEST_TIME_MS   (30 * 1000)
#define NTP_RESEND_TIME_MS (10 * 1000)

// Called with results of operation
static void ntp_result(NTP_T *state, int status, time_t *result) {
    if(status == 0 && result) {
        struct tm *utc = gmtime(result);
        ulog_info("got ntp response: %02d/%02d/%04d %02d:%02d:%02d", utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900, utc->tm_hour, utc->tm_min, utc->tm_sec);
        datetime_t date;
        time_to_datetime(*result, &date);
        rtc_set_datetime(&date);
    }
    state->end = true;
    async_context_remove_at_time_worker(cyw43_arch_async_context(), &state->resend_worker);

    // hard_assert(async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &state->request_worker, NTP_TEST_TIME_MS));  // repeat the request in future

    // printf("Next request in %ds", NTP_TEST_TIME_MS / 1000);
}

// Make an NTP request
static void ntp_request(NTP_T *state) {
    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t *req = (uint8_t *)p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

// Call back with a DNS result
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    NTP_T *state = (NTP_T *)arg;
    if(ipaddr) {
        state->ntp_server_address = *ipaddr;
        ulog_debug("ntp address %s", ipaddr_ntoa(ipaddr));
        ntp_request(state);
    } else {
        ulog_error("ntp dns request failed");
        ntp_result(state, -1, NULL);
    }
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    NTP_T *state = (NTP_T *)arg;
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    // Check the result
    if(ip_addr_cmp(addr, &state->ntp_server_address) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN && mode == 0x4 && stratum != 0) {
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
        uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
        time_t epoch = seconds_since_1970;
        ntp_result(state, 0, &epoch);
    } else {
        ulog_error("invalid ntp response");
        ntp_result(state, -1, NULL);
    }
    pbuf_free(p);
}

// Called to make a NTP request
static void request_worker_fn(__unused async_context_t *context, async_at_time_worker_t *worker) {
    NTP_T *state = (NTP_T *)worker->user_data;
    hard_assert(async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &state->resend_worker, NTP_RESEND_TIME_MS));  // in case UDP request is lost
    int err = dns_gethostbyname(NTP_SERVER, &state->ntp_server_address, ntp_dns_found, state);
    if(err == ERR_OK) {
        ntp_request(state);                                                                                                      // Cached DNS result, make NTP request
    } else if(err != ERR_INPROGRESS) {                                                                                           // ERR_INPROGRESS means expect a callback
        ulog_error("dns request failed");
        ntp_result(state, -1, NULL);
    }
}

// Called to resend an NTP request if it appears to get lost
static void resend_worker_fn(__unused async_context_t *context, async_at_time_worker_t *worker) {
    NTP_T *state = (NTP_T *)worker->user_data;
    ulog_error("ntp request failed");
    ntp_result(state, -1, NULL);
}

// Perform initialisation
static NTP_T *ntp_init(void) {
    NTP_T *state = (NTP_T *)calloc(1, sizeof(NTP_T));
    if(!state) {
        ulog_fatal("failed to allocate state");
        return NULL;
    }
    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if(!state->ntp_pcb) {
        ulog_error("failed to create pcb");
        free(state);
        return NULL;
    }
    udp_recv(state->ntp_pcb, ntp_recv, state);
    state->request_worker.do_work = request_worker_fn;
    state->request_worker.user_data = state;
    state->resend_worker.do_work = resend_worker_fn;
    state->resend_worker.user_data = state;
    state->end = false;
    return state;
}

bool setRTCviaNTP() {
    NTP_T *state = ntp_init();
    if(!state)
        return false;
    if(!async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &state->request_worker, 0)) {  // make the first request
        free(state);
        return false;
    }
    while(!state->end) {
        tight_loop_contents();
    }
    return true;
}
