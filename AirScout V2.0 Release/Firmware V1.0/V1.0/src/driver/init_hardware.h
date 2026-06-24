#ifndef DRIVER_INIT_HARDWARE_H
#define DRIVER_INIT_HARDWARE_H
#include <stdint.h>

#include "driver/bme680.h"
#include "driver/display.h"
#include "driver/m8q_gnss.h"
#include "driver/sps30.h"

void initI2C();
void initSPI();
void initUART();
void initSensors();
void deinitSensors();
void initButtons();
#define UART_RX_BUFF_SIZE 10000

typedef struct {
    int32_t data_read; //  signed, because some functions would unflow an unsigned integer
    uint8_t *buffer;
    bool buffer_full;
    int32_t buff_len;
    bool trash_data;
    bool newData;
} uart_rx_databuf_t;

extern volatile uart_rx_databuf_t uart0_rx_data;

extern BME680 bme680;
extern M8Q_GNSS M8Q;
extern Sps30 sps30;
extern Display disp;
extern volatile uint32_t btn_input_pipe;

#endif  // DRIVER_INIT_HARDWARE_H
