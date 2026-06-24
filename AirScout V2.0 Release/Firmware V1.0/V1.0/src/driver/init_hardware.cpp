
#include "init_hardware.h"

#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/regs/intctrl.h>
#include <hardware/spi.h>
#include <hardware/uart.h>
#include <pico/assert.h>
#include <pico/time.h>
#include <sd_card.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ulog.h>

#include "driver/baseSensor.h"
#include "globals.h"
#include "main.h"
#include "settings.h"

BME680 bme680(0x77);
M8Q_GNSS M8Q(I2C_INST, 0x42, uart0, 9600);
Display disp(spi1, DISPLAY_CS_PIN, DISPLAY_DC_PIN, DISPLAY_RST_PIN, DISPLAY_LED_PIN);
Sps30 sps30(0x69);

volatile uint32_t btn_input_pipe;
static volatile bool still_debouncing = false;

static void initButtonInterrupts() {
    gpio_set_irq_enabled(BTN1_GPIO, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(BTN2_GPIO, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(BTN3_GPIO, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(BTN4_GPIO, GPIO_IRQ_EDGE_RISE, true);
    still_debouncing = false;
}

static void deinitButtonInterrupts() {
    gpio_set_irq_enabled(BTN1_GPIO, 0, false);
    gpio_set_irq_enabled(BTN2_GPIO, 0, false);
    gpio_set_irq_enabled(BTN3_GPIO, 0, false);
    gpio_set_irq_enabled(BTN4_GPIO, 0, false);
    still_debouncing = true;
}

static int64_t alarm_callback(alarm_id_t id, __unused void *user_data) {
    initButtonInterrupts();
    return 0;
}

void button_irq_cbk(uint gpio, uint32_t event) {
    if(still_debouncing) {  // if multiple irqs are queued
        return;
    }
    deinitButtonInterrupts();
    add_alarm_in_ms(50, &alarm_callback, NULL, true);
    ulog_debug("Pressed %d, Event: %d", gpio, event);

    uint8_t mask = 0x00;
    // pipe is not full
    if((btn_input_pipe & 0xF0000000) == 0x0) {
        switch(gpio) {
            case BTN1_GPIO:
                mask = BTN1_BIT;
                break;
            case BTN2_GPIO:
                mask = BTN2_BIT;
                break;
            case BTN3_GPIO:
                mask = BTN3_BIT;
                break;
            case BTN4_GPIO:
                mask = BTN4_BIT;
                break;
        }

        // find next empty space
        for(uint i = 0; i < BTN_PIPE_LEN; i++) {
            if(btn_input_pipe >> (i * 4) == 0x0) {
                btn_input_pipe |= mask << (i * 4);
                break;
            }
        }
    }
}

void initButtons() {
    gpio_init(BTN1_GPIO);
    gpio_set_input_enabled(BTN1_GPIO, true);
    gpio_set_irq_enabled_with_callback(BTN1_GPIO, GPIO_IRQ_EDGE_RISE, true, &button_irq_cbk);
    gpio_init(BTN2_GPIO);
    gpio_set_input_enabled(BTN2_GPIO, true);
    gpio_set_irq_enabled_with_callback(BTN2_GPIO, GPIO_IRQ_EDGE_RISE, true, &button_irq_cbk);
    gpio_init(BTN3_GPIO);
    gpio_set_input_enabled(BTN3_GPIO, true);
    gpio_set_irq_enabled_with_callback(BTN3_GPIO, GPIO_IRQ_EDGE_RISE, true, &button_irq_cbk);
    gpio_init(BTN4_GPIO);
    gpio_set_input_enabled(BTN4_GPIO, true);
    gpio_set_irq_enabled_with_callback(BTN4_GPIO, GPIO_IRQ_EDGE_RISE, true, &button_irq_cbk);
}

void initI2C() {
    gpio_set_slew_rate(I2C_SDA_PIN, GPIO_SLEW_RATE_SLOW);
    gpio_set_slew_rate(I2C_SCL_PIN, GPIO_SLEW_RATE_SLOW);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    gpio_set_drive_strength(I2C_SDA_PIN, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(I2C_SCL_PIN, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_input_hysteresis_enabled(I2C_SDA_PIN, true);
    gpio_set_input_hysteresis_enabled(I2C_SCL_PIN, true);
    i2c_init(I2C_INST, I2C_BAUDRATE);
}

void initSPI() {
}

volatile uart_rx_databuf_t uart0_rx_data = {.data_read = 0, .buffer_full = false, .trash_data = true, .newData = false};
// RX interrupt handler
// has to read from the pipe, else the interrupt will not exit / is repeatetly called
void on_uart0_rx() {
    uart0_rx_data.newData = true;
    if(uart0_rx_data.trash_data || uart0_rx_data.buffer_full) {
        while(uart_is_readable(uart0)) {
            uart_getc(uart0);  // throw rx data away
        }
    }

    while(uart_is_readable(uart0)) {
        uart0_rx_data.buffer[uart0_rx_data.data_read] = uart_getc(uart0);
        uart0_rx_data.data_read++;

        if(uart0_rx_data.data_read >= uart0_rx_data.buff_len) {
            uart0_rx_data.buffer_full = true;
            break;
        }
    }
}

void initUART() {
    uart0_rx_data.buff_len = UART_RX_BUFF_SIZE;
    uart0_rx_data.buffer = (uint8_t *)calloc(UART_RX_BUFF_SIZE, sizeof(uint8_t));
    if(uart0_rx_data.buffer == NULL) {
        ulog_fatal("Failed to allocate uart0 rx buffer");
        hard_assert(1);
    }

    memset(uart0_rx_data.buffer, 0, UART_RX_BUFF_SIZE);
    uart_init(uart0, UART0_DEFAULT_BAUD);
    // gpio_set_slew_rate(UART0_TX_PIN, GPIO_SLEW_RATE_SLOW);
    gpio_set_function(UART0_TX_PIN, UART_FUNCSEL_NUM(uart0, UART0_TX_PIN));
    gpio_set_function(UART0_RX_PIN, UART_FUNCSEL_NUM(uart0, UART0_RX_PIN));
    // gpio_pull_up(UART0_TX_PIN);
    // gpio_pull_up(UART0_RX_PIN);

    // gpio_set_drive_strength(UART0_TX_PIN, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_input_hysteresis_enabled(UART0_TX_PIN, true);

    irq_set_exclusive_handler(UART0_IRQ, on_uart0_rx);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irqs_enabled(uart0, true, false);

    uart_set_fifo_enabled(uart0, false);
    uart_set_hw_flow(uart0, false, false);
    uart_set_format(uart0, 8, 1, UART_PARITY_NONE);
}

void initSensors() {
    bool status = false;
    SETSTATUS settingsstate = SETSTATUS::OK;

    settingsstate = user_settings.getBool("hardware/en_cam_m8q", &status);
    if(settingsstate == SETSTATUS::OK && status) {
        if(M8Q.init()) {
            M8Q.state = Sensor_State::OK;
        } else {
            M8Q.state = Sensor_State::FAIL;
            ulog_error("init cam m8q failed");
        }
    } else {
        M8Q.state = Sensor_State::DISABLED;
    }

    settingsstate = user_settings.getBool("hardware/en_bme680", &status);
    if(settingsstate == SETSTATUS::OK && status) {
        if(bme680.init()) {
            bme680.state = Sensor_State::OK;
        } else {
            bme680.state = Sensor_State::FAIL;
            ulog_error("init bme680 failed");
        }
    } else {
        bme680.state = Sensor_State::DISABLED;
    }

    settingsstate = user_settings.getBool("hardware/en_sps30", &status);
    if(settingsstate == SETSTATUS::OK && status) {
        if(sps30.init()) {
            sps30.state = Sensor_State::OK;
        } else {
            sps30.state = Sensor_State::FAIL;
            ulog_error("init sps30 failed");
        }
    } else {
        sps30.state = Sensor_State::DISABLED;
    }
}

void deinitSensors() {
    if(M8Q.state != Sensor_State::DISABLED) {
        M8Q.deinit();
    }
    if(bme680.state != Sensor_State::DISABLED) {
        bme680.deinit();
    }
    if(sps30.state != Sensor_State::DISABLED) {
        sps30.deinit();
    }
}