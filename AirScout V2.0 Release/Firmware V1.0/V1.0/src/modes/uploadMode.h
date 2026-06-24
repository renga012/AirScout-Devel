#ifndef MODES_UPLOADMODE_H
#define MODES_UPLOADMODE_H

#include <lwip/err.h>
#include <lwip/ip_addr.h>
#include <stddef.h>
#include <stdint.h>

#include "baseMode.h"

#define UPLOAD_HEADER_LEN 140

#define HTTP_POST_JSON_TEMPLATE "POST %s HTTP/1.1\r\nHost: %s\r\nAccept: */*\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s"
// #define HTTP_POST_JSON_TEMPLATE "POST %s HTTP/1.1 \r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s"
#define HTTP_GET_TEMPLATE "GET %s HTTP/1.1 \nHost: %s\nConnection: close\n\n"
#define UPLOAD_HOST "airscout.fri3dl.dev"
// #define UPLOAD_HOST_PATH "/index.php"
// #define UPLOAD_HOST "gitea.qcodespace.org"
// #define UPLOAD_HOST "example.com"
// #define UPLOAD_HOST "fw-download-alias1.raspberrypi.com"
#define UPLOAD_HOST_PATH "/api/measurement"
/*
#define TLS_CLIENT_SERVER UPLOAD_HOST
#define TLS_CLIENT_HTTP_REQUEST \
"GET / HTTP/1.1\r\n"        \
"Host: " UPLOAD_HOST  \
"\r\n"                      \
"Connection: close\r\n"     \
"\r\n"
*/

/*
#define TLS_CLIENT_SERVER        "fw-download-alias1.raspberrypi.com"
#define TLS_CLIENT_HTTP_REQUEST  "GET / HTTP/1.1\r\n" \
"Host: " TLS_CLIENT_SERVER "\r\n" \
"Connection: close\r\n" \
"\r\n"
#define TLS_CLIENT_TIMEOUT_SECS  15
*/

typedef struct {
    struct altcp_pcb *pcb;
    bool complete;
    int error;
    char *http_request;
    uint32_t http_request_len;
    int timeout;
} tls_client_t;

class UploadMode : public BaseMode {
public:
    bool init();
    void run();
    void exit();

private:
    void m_uploadNext();
    char *m_loadNextFile();
    int32_t m_toUpload;
    int32_t m_total;
    bool m_finished;
    bool m_error;
};

#endif  // MODES_UPLOADMODE_H
