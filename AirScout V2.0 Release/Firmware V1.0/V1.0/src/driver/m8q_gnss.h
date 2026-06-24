#ifndef DRIVER_M8Q_GNSS_H
#define DRIVER_M8Q_GNSS_H

#include <hardware/i2c.h>
#include <hardware/uart.h>
#include <pico/types.h>
#include <stdint.h>

#include "baseSensor.h"

#define M8Q_ERROR_ALLOCATION -1
#define M8Q_ERROR_NODATA     -2
#define M8Q_OK               0

const uint8_t ubx_header[] = {0xB5, 0x62};
const uint8_t ubx_db_dump_msg[] = {0x13, 0x80};

typedef struct {
    float longitude;
    float latitude;
    float altitude;
    datetime_t utc;
    bool valid;
} gnss_data_t;

typedef struct {
    uint8_t msg_class;
    uint8_t msg_id;
    uint16_t payload_len;
    uint8_t payload[256];
    uint16_t crc;
} ubx_msg_t;

#define RAW_NMEA_BUFFER 1000
typedef struct _raw_nmea_lines_t {
    uint32_t buff_len = RAW_NMEA_BUFFER;
    char buffer[RAW_NMEA_BUFFER];
    uint32_t read_bytes;
} raw_nmea_lines_t;

class M8Q_GNSS : public BaseSensor {
public:
    M8Q_GNSS(i2c_inst_t *i2c, const uint8_t i2c_addr, uart_inst_t *uart, const uint32_t target_baudrate);
    ~M8Q_GNSS();
    bool init();
    bool deinit();
    void getValues(gnss_data_t *data);

    void updateSettings();
    void processUBX();
    void dump_db();
    void update_i2c();
    bool dump_complete();

private:
    i2c_inst_t *m_i2c;
    uint8_t m_i2c_addr;
    uart_inst_t *m_uart;
    uint32_t m_target_baudrate, m_current_baudrate;

    bool lastDataGood = false;
    bool m_newDump = true;
    absolute_time_t m_time_since_dbdump = 0;

    int8_t m_read_nmea(raw_nmea_lines_t *nmea_lines);
    uint16_t m_calc_crc(const ubx_msg_t *cmd);
    void m_sendCmd(ubx_msg_t *cmd);
    bool m_checkCrc(ubx_msg_t *msg);
    uint8_t m_parse_nmea_sentence(char *line, gnss_data_t *data);
    void m_restoreDB();
};

#endif  // DRIVER_M8Q_GNSS_H
