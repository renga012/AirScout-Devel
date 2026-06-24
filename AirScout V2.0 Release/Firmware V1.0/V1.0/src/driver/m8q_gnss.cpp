#include "m8q_gnss.h"

#include <ff.h>
#include <hardware/i2c.h>
#include <hardware/rtc.h>
#include <hardware/uart.h>
#include <minmea.h>
#include <pico/time.h>
#include <pico/types.h>
#include <pico/util/datetime.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ulog.h>

#include "driver/baseSensor.h"
#include "driver/fs.h"
#include "globals.h"
#include "init_hardware.h"
#include "main.h"

M8Q_GNSS::M8Q_GNSS(i2c_inst_t *i2c, const uint8_t i2c_addr, uart_inst_t *uart, const uint32_t target_baudrate) : BaseSensor() {
    m_i2c = i2c;
    m_i2c_addr = i2c_addr;
    m_uart = uart;
    m_current_baudrate = UART0_DEFAULT_BAUD;
    m_target_baudrate = target_baudrate;
}

bool M8Q_GNSS::init() {
    ubx_msg_t cmd;
    uart0_rx_data.trash_data = false;
    // configure uart interface: only ubx
    cmd.msg_class = 0x06;
    cmd.msg_id = 0x00;
    cmd.payload_len = 20;

    {
        uint8_t tmp_payload[]{0x01,  // portID = UART1
                              0x00,  // reserved
                              0x00,  // txReady
                              0x00,
                              0xC0,  // mode = 0x 08 C0: 8N1
                              0x08,
                              0x00,
                              0x00,
                              (uint8_t)(m_target_baudrate & 0x000000FF),
                              (uint8_t)((m_target_baudrate & 0x0000FF00) >> 8),
                              (uint8_t)((m_target_baudrate & 0x00FF0000) >> 16),
                              (uint8_t)((m_target_baudrate & 0xFF000000) >> 24),  // convert baudrate
                              0x01,                                               // inProtoMask = UBX
                              0x00,
                              0x01,                                               // outProtoMask = UBX
                              0x00,
                              0x00,                                               // flags
                              0x00,
                              0x00,                                               // reserved
                              0x00};
        memcpy(cmd.payload, tmp_payload, cmd.payload_len);
    }
    uart_set_irqs_enabled(m_uart, false, false);

    // the command is send a second time incase the module was asleep
    m_sendCmd(&cmd);
    m_sendCmd(&cmd);

    uart_tx_wait_blocking(m_uart);
    ulog_info("Actual UART Baudrate: %d\n", uart_set_baudrate(m_uart, m_target_baudrate));
    uart_set_irqs_enabled(m_uart, true, false);
    sleep_ms(100);
    // configure i2c interface: only nmea read
    cmd.msg_class = 0x06;
    cmd.msg_id = 0x00;
    cmd.payload_len = 20;
    {
        uint8_t tmp_payload[] = {
            0x00,  // portID = I2C
            0x00,  // reserved
            0x00,
            0x00,  // txReady = None
            (uint8_t)(m_i2c_addr << 1),
            0x00,
            0x00,
            0x00,  // mode = i2c address, shift because R/W is first bit
            0x00,
            0x00,
            0x00,
            0x00,  // reserved
            0x00,
            0x00,  // inProtoMask = None
            0x02,
            0x00,  // outProtoMask = NMEA
            0x00,
            0x00,  // flags
            0x00,
            0x00   // reserved
        };
        memcpy(cmd.payload, tmp_payload, cmd.payload_len);
    }

    m_sendCmd(&cmd);

    // enable assist autonomous
    cmd.msg_class = 0x06;
    cmd.msg_id = 0x23;
    cmd.payload_len = 40;
    {
        uint8_t tmp_payload[] = {0x02, 0x00, 0x4C, 0x66, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x01,  // assist autonomous
                                 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        memcpy(cmd.payload, tmp_payload, cmd.payload_len);
    }
    m_sendCmd(&cmd);

    // disable GSV NMEA mgs
    cmd.msg_class = 0x06;
    cmd.msg_id = 0x01;
    cmd.payload_len = 8;
    {
        uint8_t tmp_payload[]{0xF0,  // nmea msg class
                              0x02,  // nmea msg id
                              0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        memcpy(cmd.payload, tmp_payload, cmd.payload_len);
    }
    // m_sendCmd(&cmd);

    // disable GSA NMEA mgs
    cmd.msg_class = 0x06;
    cmd.msg_id = 0x01;
    cmd.payload_len = 8;
    {
        uint8_t tmp_payload[]{0xF0,  // nmea msg class
                              0x03,  // nmea msg id
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        memcpy(cmd.payload, tmp_payload, cmd.payload_len);
    }
    // m_sendCmd(&cmd);
    updateSettings();
    // m_restoreDB();
    return true;
}

bool M8Q_GNSS::deinit() {
    // dump_db();

    // sleep
    ubx_msg_t cmd;
    cmd.msg_class = 0x02;
    cmd.msg_id = 0x41;
    cmd.payload_len = 8;
    {
        uint8_t tmp_payload[]{0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};  // sleep 0 days (only pin wakeup) in backup mode
        memcpy(cmd.payload, tmp_payload, cmd.payload_len);
    }
    m_sendCmd(&cmd);

    return true;
}

uint8_t M8Q_GNSS::m_parse_nmea_sentence(char *line, gnss_data_t *data) {
    uint8_t status;
    switch(minmea_sentence_id(line, false)) {
        case MINMEA_SENTENCE_RMC: {
            struct minmea_sentence_rmc frame;
            if(minmea_parse_rmc(&frame, line)) {
                data->latitude = minmea_tocoord(&frame.latitude);
                data->longitude = minmea_tocoord(&frame.longitude);
                struct tm tmp_tm;
                minmea_getdatetime(&tmp_tm, &frame.date, &frame.time);
                tm_to_datetime(&tmp_tm, &data->utc);

                data->valid = frame.valid;
                status = M8Q_OK;
            } else {
                printf("\n");
                ulog_warn("$xxRMC sentence is not parsed\n");
                status = M8Q_ERROR_NODATA;
            }
        } break;

        case MINMEA_SENTENCE_GGA: {
            struct minmea_sentence_gga frame;
            if(minmea_parse_gga(&frame, line)) {
                // printf("$xxGGA: fix quality: %d\n", frame.fix_quality);
                data->altitude = minmea_tofloat(&frame.altitude);
                status = M8Q_OK;
            } else {
                printf("\n");
                ulog_warn("$xxGGA sentence is not parsed\n");
                status = M8Q_ERROR_NODATA;
            }
        } break;

        case MINMEA_INVALID: {
            printf("\n");
            ulog_warn("%s sentence is not valid\n", line);
            status = M8Q_ERROR_NODATA;
        } break;

        default: {
            status = M8Q_ERROR_NODATA;
        } break;
    }
    return status;
}

void M8Q_GNSS::getValues(gnss_data_t *data) {
    raw_nmea_lines_t nmea_buffer;
    memset(nmea_buffer.buffer, 0, nmea_buffer.buff_len);

    char *token = NULL;
    const char delimiter[] = "\r\n";

    if(m_read_nmea(&nmea_buffer) != M8Q_OK) {
        return;
    }

    printf("%s\n", nmea_buffer.buffer);

    token = strtok(nmea_buffer.buffer, delimiter);
    while(token != NULL) {
        m_parse_nmea_sentence(token, data);
        token = strtok(NULL, delimiter);
    }
    if(data->valid) {
        if(!lastDataGood) {
            rtc_set_datetime(&data->utc);
            sleep_us(64);  // wait until the new time is set
            status_bar.setGNSSStatus(1);
        }
    }
    lastDataGood = data->valid;
}

void M8Q_GNSS::update_i2c() {
    const uint8_t start_reg = 253;
    uint8_t avail_data_buf[2];

    // poll the interface so it stays active or something
    i2c_write_blocking_until(m_i2c, m_i2c_addr, &start_reg, 1, true, 1000);
    i2c_read_blocking_until(m_i2c, m_i2c_addr, avail_data_buf, 2, false, 1000);
}

void M8Q_GNSS::updateSettings() {
    // set navigation rate
    int32_t measure_interval_ms;
    user_settings.getint("hardware/meas_interval_sec", &measure_interval_ms);
    measure_interval_ms *= 1000;

    ubx_msg_t cmd;
    cmd.msg_class = 0x06;
    cmd.msg_id = 0x08;
    cmd.payload_len = 6;

    if(measure_interval_ms > UINT16_MAX) {
        measure_interval_ms = UINT16_MAX;  // max out the measRate
    }

    {
        uint8_t tmp_payload[]{
            (uint8_t)(measure_interval_ms & 0x000000FF),
            (uint8_t)((measure_interval_ms & 0x0000FF00) >> 8),  // measRate
            0x01,
            0x00,                                                // navrate
            0x00,
            0x00,                                                // time ref
        };
        memcpy(cmd.payload, tmp_payload, cmd.payload_len);
    }
    ulog_debug("%d", measure_interval_ms);
    m_sendCmd(&cmd);
}

int8_t M8Q_GNSS::m_read_nmea(raw_nmea_lines_t *nmea_lines) {
    const uint8_t start_reg = 253;
    uint8_t avail_data_buf[3];
    uint32_t read_bytes = 0;
    uint16_t available_data = 0;

    // read from the last 3 Bytes, 253 + 254 contain how many bytes can be read from 255
    int err = i2c_write_blocking_until(m_i2c, m_i2c_addr, &start_reg, 1, true, 1000);
    if(err == PICO_ERROR_GENERIC || err == PICO_ERROR_TIMEOUT) {
        ulog_error("Failed to read M8Q: %d", err);
        return M8Q_ERROR_NODATA;
    }

    err = i2c_read_blocking_until(m_i2c, m_i2c_addr, avail_data_buf, 3, false, 1000);
    if(err == PICO_ERROR_GENERIC || err == PICO_ERROR_TIMEOUT) {
        ulog_error("Failed to read M8Q: %d", err);
        return M8Q_ERROR_NODATA;
    }

    available_data = (avail_data_buf[0] << 8) | (avail_data_buf[1] & 0xFF);

    if(available_data > 0 && avail_data_buf[2] != 0xFF) {  // 0xFF means no Data

        nmea_lines->buffer[0] = avail_data_buf[2];         //  while gathering the available Data, the first Byte of the Data was already read.
        read_bytes = 1;

        if(available_data >= nmea_lines->buff_len) {
            available_data = nmea_lines->buff_len - 1;
        }

        read_bytes += i2c_read_blocking_until(m_i2c, m_i2c_addr, (uint8_t *)nmea_lines->buffer + 1, available_data, false, 1000);
        if(read_bytes == PICO_ERROR_GENERIC || read_bytes == PICO_ERROR_TIMEOUT) {
            ulog_error("Failed to read M8Q: %d", err);
            return M8Q_ERROR_NODATA;
        }
        // ulog_debug("Read %d Bytes from GPS via I2C", read_bytes);

        nmea_lines->buffer[read_bytes - 1] = 0;  // add null terminator, the last Byte is 0xFF meaning no Data
        nmea_lines->read_bytes = read_bytes;
    } else {
        ulog_warn("No new Data");
        return M8Q_ERROR_NODATA;
    }

    if(nmea_lines->read_bytes <= 0) {
        ulog_error("Failed to read M8Q: %d", err);
        return M8Q_ERROR_NODATA;
    }

    return M8Q_OK;
}

uint16_t M8Q_GNSS::m_calc_crc(const ubx_msg_t *cmd) {
    size_t buffer_len = cmd->payload_len + 4;  // + msg class + msg id + payload len
    uint8_t *buffer = (uint8_t *)malloc(buffer_len);

    buffer[0] = cmd->msg_class;
    buffer[1] = cmd->msg_id;
    buffer[2] = cmd->payload_len & 0x00FF;
    buffer[3] = (cmd->payload_len & 0xFF00) >> 8;
    memcpy(buffer + 4, cmd->payload, cmd->payload_len);

    uint8_t CK_A = 0, CK_B = 0;
    for(size_t i = 0; i < buffer_len; i++) {
        CK_A = CK_A + buffer[i];
        CK_B = CK_B + CK_A;
    }
    free(buffer);
    return (CK_A << 8) | CK_B;
}

void M8Q_GNSS::m_sendCmd(ubx_msg_t *cmd) {
    size_t buffer_len = cmd->payload_len + 8;  // + ubx header + msg class + msg id + payload len + crc
    uint8_t *buffer = (uint8_t *)malloc(buffer_len);
    cmd->crc = m_calc_crc(cmd);

    memcpy(buffer, ubx_header, 2);
    buffer[2] = cmd->msg_class;
    buffer[3] = cmd->msg_id;
    buffer[4] = cmd->payload_len & 0x00FF;
    buffer[5] = (cmd->payload_len & 0xFF00) >> 8;
    memcpy(buffer + 6, cmd->payload, cmd->payload_len);

    buffer[buffer_len - 2] = (cmd->crc & 0xFF00) >> 8;
    buffer[buffer_len - 1] = cmd->crc & 0x00FF;

    ulog_debug("UBX CMD (len %d): ", buffer_len);
    for(size_t i = 0; i < buffer_len; i++) {
        printf("%02x", buffer[i]);
    }
    printf("\n");
    uart_write_blocking(m_uart, buffer, buffer_len);
    free(buffer);
}

void M8Q_GNSS::processUBX() {
    // uint16_t sync_char_pos[uart0_rx_data.data_read];
    if(!uart0_rx_data.data_read || !uart0_rx_data.newData) {
        return;
    }

    uint16_t *sync_char_pos = (uint16_t *)calloc(uart0_rx_data.data_read, sizeof(uint16_t));

    memset(sync_char_pos, 0, uart0_rx_data.data_read * sizeof(uint16_t));
    uint32_t sync_char_pos_cnt = 0;

    ulog_debug("New UART Data, %d Bytes:", uart0_rx_data.data_read);
    for(int32_t i = 0; i < uart0_rx_data.data_read; i++) {
        printf("%02X", uart0_rx_data.buffer[i]);
    }
    printf("\n");
    for(int32_t i = 0; i < uart0_rx_data.data_read - 1; i++) {
        if(!memcmp((void *)(uart0_rx_data.buffer + i), (void *)ubx_header, 2)) {
            sync_char_pos[sync_char_pos_cnt] = i;
            sync_char_pos_cnt++;
            // ulog_debug("Found Sync chars at pos %d", i);
        }
    }
    bool dbfound = false;
    // check if the received data is a db dump
    if(uart0_rx_data.buffer[sync_char_pos[0] + 2] == ubx_db_dump_msg[0] && uart0_rx_data.buffer[sync_char_pos[0] + 3] == ubx_db_dump_msg[1]) {
        dbfound = true;
        ulog_debug("DB Dump Found");
        FIL fp;
        FRESULT fr;
        UINT bytes_written = 0;

        if(m_newDump) {
            fr = f_open(&fp, ubx_db_dump_file, FA_CREATE_ALWAYS | FA_WRITE);
        } else {
            fr = f_open(&fp, ubx_db_dump_file, FA_OPEN_APPEND | FA_WRITE);
        }

        if(fr != FR_OK) {
            ulog_error("Could not open DB File (%d), %s", fr, FRESULT_str(fr));
        }

        fr = f_write(&fp, uart0_rx_data.buffer + sync_char_pos[0], uart0_rx_data.data_read - sync_char_pos[0], &bytes_written);

        if(fr != FR_OK) {
            ulog_error("Failed to write MSGs to DBdump");
        }
        if(uart0_rx_data.data_read - sync_char_pos[0] != bytes_written) {
            ulog_warn("Not all data was written, only %d Bytes from %d Bytes in total", bytes_written, uart0_rx_data.data_read - sync_char_pos[0]);
        }
        ulog_info("Successfully written %d Bytes to DBdump", bytes_written);
        fr = f_close(&fp);
        if(fr != FR_OK) {
            ulog_error("Failed to close dbdump file");
        }
        m_newDump = false;
        free(sync_char_pos);
        return;
    }

    // if the database is received, instead of parsing all msgs, only the last one is parsed. Reason is saving memory but we also have to check if the last msg is complete
    uint32_t fake_sync_char_pos_cnt = sync_char_pos_cnt;
    if(dbfound) {
        fake_sync_char_pos_cnt = 1;
        m_time_since_dbdump = get_absolute_time();
    }

    // ubx_msg_t received_msgs[sync_char_pos_cnt];
    // memset(received_msgs, 0, sizeof(received_msgs));
    ubx_msg_t *received_msgs = (ubx_msg_t *)calloc(sync_char_pos_cnt, sizeof(ubx_msg_t));
    uint32_t received_msgs_cnt = 0;

    int32_t pos = 0;
    bool lastmsg_incomplete = false;

    for(uint32_t i = sync_char_pos_cnt - fake_sync_char_pos_cnt; i < sync_char_pos_cnt; i++) {
        pos = sync_char_pos[i];
        // is the read buffer big enought to read out at least the minimum vaules of the message (2xheader, class, id, 2xpayload)
        if((pos + 5) > uart0_rx_data.data_read) {
            ulog_warn("UBX message can't read minimum");
            lastmsg_incomplete = true;
            break;
        }
        // WILLIM WAR HIER
        received_msgs[received_msgs_cnt].msg_class = uart0_rx_data.buffer[pos + 2];
        received_msgs[received_msgs_cnt].msg_id = uart0_rx_data.buffer[pos + 3];

        received_msgs[received_msgs_cnt].payload_len = uart0_rx_data.buffer[pos + 4];
        received_msgs[received_msgs_cnt].payload_len |= uart0_rx_data.buffer[pos + 5] << 8;

        if(received_msgs[received_msgs_cnt].payload_len + pos + 5 + 2 > uart0_rx_data.data_read) {  // cant read the whole message from buffer
            ulog_warn("UBX message incomplete");
            lastmsg_incomplete = true;
            break;
        }

        memcpy(received_msgs[received_msgs_cnt].payload, uart0_rx_data.buffer + pos + 6, received_msgs[received_msgs_cnt].payload_len);
        received_msgs[received_msgs_cnt].crc = uart0_rx_data.buffer[pos + 6 + received_msgs[received_msgs_cnt].payload_len] << 8;
        received_msgs[received_msgs_cnt].crc |= uart0_rx_data.buffer[pos + 6 + received_msgs[received_msgs_cnt].payload_len + 1];
        received_msgs_cnt++;
    }

    for(uint32_t i = 0; i < received_msgs_cnt; i++) {
        printf("MSG %02d: 0x%02x, 0x%02x with %d Bytes: ", i, received_msgs[i].msg_class, received_msgs[i].msg_id, received_msgs[i].payload_len);
        for(int j = 0; j < received_msgs[i].payload_len; j++) {
            printf("%02x", received_msgs[i].payload[j]);
        }
        printf(", CRC: %04x, check: %04x, = ", received_msgs[i].crc, m_calc_crc(&received_msgs[i]));
        if(m_checkCrc(&received_msgs[i])) {
            printf(":)\n");
        } else {
            printf(":(\n");
        }
    }
    ulog_debug("Received %d Bytes in total", uart0_rx_data.data_read);

    pos = sync_char_pos[sync_char_pos_cnt - 1];
    if(lastmsg_incomplete) {
        uint32_t tmp_buf_len = uart0_rx_data.data_read - pos;
        ulog_info("Saving %d Bytes for next parsing round, (%d-%d)", tmp_buf_len, uart0_rx_data.data_read, pos);
        uint8_t *tmp = (uint8_t *)calloc(1, tmp_buf_len);

        memcpy(tmp, uart0_rx_data.buffer + pos, tmp_buf_len);
        memset(uart0_rx_data.buffer, 0, uart0_rx_data.buff_len);
        memcpy(uart0_rx_data.buffer, tmp, tmp_buf_len);
        uart0_rx_data.data_read = tmp_buf_len;
        free(tmp);
    } else {
        memset(uart0_rx_data.buffer, 0, uart0_rx_data.buff_len);
        // uart0_rx_data.trash_data = true;
        uart0_rx_data.data_read = 0;
    }

    uart0_rx_data.buffer_full = false;
    uart0_rx_data.newData = false;
    free(received_msgs);
    free(sync_char_pos);
}

bool M8Q_GNSS::m_checkCrc(ubx_msg_t *msg) {
    return m_calc_crc(msg) == msg->crc;
}

void M8Q_GNSS::dump_db() {
    ubx_msg_t cmd;
    cmd.msg_class = ubx_db_dump_msg[0];
    cmd.msg_id = ubx_db_dump_msg[1];
    cmd.payload_len = 0;
    // uart0_rx_data.trash_data = false;
    ulog_info("Dumping DB...\n");
    m_newDump = true;
    m_time_since_dbdump = get_absolute_time();
    m_sendCmd(&cmd);

    // ubx_msg_t cmd;
    // cmd.msg_class = 0x06;
    // cmd.msg_id = 0x00;
    // cmd.payload_len = 1;
    // cmd.payload = (uint8_t*)malloc(1);
    // cmd.payload[0] = 0x01;
    // printf("Dumping DB...\n");
    // m_expectData(true);
    // m_sendCmd(&cmd);
    // free(cmd.payload);
}
bool M8Q_GNSS::dump_complete() {
    if(state != Sensor_State::OK) {
        return true;
    }

    // check for timeout
    if(absolute_time_diff_us(m_time_since_dbdump, get_absolute_time()) > (UBX_DBDUMP_TIMOUT_S * 1e6)) {
        ulog_warn("DB Dump Timeout");
        // uart0_rx_data.trash_data = true;
        return true;
    }

    return uart0_rx_data.trash_data;
}

void M8Q_GNSS::m_restoreDB() {
    FIL fp;
    FRESULT fr;
    fr = f_open(&fp, ubx_db_dump_file, FA_READ);
    if(fr != FR_OK) {
        ulog_warn("Error while opening %s\n%s", ubx_db_dump_file, FRESULT_str(fr));
        return;
    }

    uint64_t size = f_size(&fp);
    UINT bytes_read = 0;
    uint8_t *buffer = (uint8_t *)malloc(size);
    if(buffer == NULL) {
        ulog_fatal("Failed to allocate Buffer with %llu Bytes for DBdump", size);
    }

    fr = f_read(&fp, buffer, size, &bytes_read);
    if(fr != FR_OK) {
        ulog_error("Error while reading %s\n%s", ubx_db_dump_file, FRESULT_str(fr));
        free(buffer);
        return;
    }

    fr = f_close(&fp);
    if(fr != FR_OK) {
        ulog_error("Error while closing %s\n%s", ubx_db_dump_file, FRESULT_str(fr));
    }

    uart_write_blocking(m_uart, buffer, bytes_read);
    ulog_info("Restoring DB:");
    for(int i = 0; i < bytes_read; i++) {
        printf("%02X", buffer[i]);
    }
    printf("\n");
    free(buffer);
}