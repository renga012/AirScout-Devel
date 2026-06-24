#include "sps30.h"

#include <hardware/i2c.h>
#include <pico/time.h>
#include <sensirion_common.h>
#include <sensirion_config.h>
#include <sensirion_i2c_hal.h>
#include <sps30_i2c.h>
#include <stdint.h>
#include <ulog.h>
#include <cstdio>

#include "globals.h"

Sps30::Sps30(uint8_t addr) : BaseSensor() {
    m_addr = addr;
}

int16_t Sps30::getValues(sps30_data_t *data) {
    printf("Reading SPS30");
    sps30_wake_up();
    i2c_set_baudrate(I2C_INST, 100000);
    sps30_start_measurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_UINT16);

    uint16_t data_ready_flag = 0;
    while(!data_ready_flag) {
        if(sps30_read_data_ready_flag(&data_ready_flag)) {
            // error
            break;
        }
        sleep_ms(100);
    }

    uint16_t error = sps30_read_measurement_values_uint16(&data->mc_1p0, &data->mc_2p5, &data->mc_4p0, &data->mc_10p0, &data->nc_0p5, &data->nc_1p0, &data->nc_2p5, &data->nc_4p0, &data->nc_10p0, &data->typical_particle_size);
    if(error != 0) {
        ulog_error("Failed to Read SPS30, error: %d", error);
    }
    
    
    printf("SPS30:\n");
    printf("mc 1p0%d\n", data->mc_1p0);
    printf("mc 2p5%d\n", data->mc_2p5);
    printf("mc 4p0%d\n", data->mc_4p0);
    printf("mc 10p%d\n", data->mc_10p0);
    printf("nc 0p5%d\n", data->nc_0p5);
    printf("nc 1p0%d\n", data->nc_1p0);
    printf("nc 2p5%d\n", data->nc_2p5);
    printf("nc 4p0%d\n", data->nc_4p0);
    printf("nc 10p%d\n", data->nc_10p0);
    printf("typical %d\n\n", data->typical_particle_size);

    
    
    sps30_stop_measurement();
    sps30_sleep();
    i2c_set_baudrate(I2C_INST, 400000);
    return error;
}

bool Sps30::init() {
    uint16_t error = 0;
    ulog_trace("Init start");

    i2c_set_baudrate(I2C_INST, 100000);
    sps30_wake_up();
    sps30_init(m_addr);
    sps30_stop_measurement();
    error = sps30_start_measurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_UINT16);
    sleep_ms(1000);
    error = sps30_stop_measurement();
    sps30_sleep();

    ulog_trace("Init end");
    if(error != 0) {
        ulog_error("Failed to initialize SPS30, error: %d", error);
        return false;
    }
    i2c_set_baudrate(I2C_INST, 400000);
    return true;
}

bool Sps30::deinit() {
    i2c_set_baudrate(I2C_INST, 100000);
    sps30_stop_measurement();
    sps30_sleep();
    i2c_set_baudrate(I2C_INST, 400000);
    return true;
}

/*
 * INSTRUCTIONS
 * ============
 *
 * Implement all functions where they are marked as IMPLEMENT.
 * Follow the function specification in the comments.
 */

/**
 * Select the current i2c bus by index.
 * All following i2c operations will be directed at that bus.
 *
 * THE IMPLEMENTATION IS OPTIONAL ON SINGLE-BUS SETUPS (all sensors on the same
 * bus)
 *
 * @param bus_idx   Bus index to select
 * @returns         0 on success, an error code otherwise
 */
int16_t sensirion_i2c_hal_select_bus(uint8_t bus_idx) {
    /* TODO:IMPLEMENT or leave empty if all sensors are located on one single
     * bus
     */
    return NOT_IMPLEMENTED_ERROR;
}

/**
 * Initialize all hard- and software components that are needed for the I2C
 * communication.
 */
void sensirion_i2c_hal_init(void) {
    /* TODO:IMPLEMENT */
}

/**
 * Release all resources initialized by sensirion_i2c_hal_init().
 */
void sensirion_i2c_hal_free(void) {
    /* TODO:IMPLEMENT or leave empty if no resources need to be freed */
}

/**
 * Execute one read transaction on the I2C bus, reading a given number of bytes.
 * If the device does not acknowledge the read command, an error shall be
 * returned.
 *
 * @param address 7-bit I2C address to read from
 * @param data    pointer to the buffer where the data is to be stored
 * @param count   number of bytes to read from I2C and store in the buffer
 * @returns 0 on success, error code otherwise
 */
int8_t sensirion_i2c_hal_read(uint8_t address, uint8_t *data, uint8_t count) {
    /* TODO:IMPLEMENT */
    int ret = i2c_read_blocking_until(I2C_INST, address, data, count, false, 1000);
    if(ret > 0) {
        return 0;
    } else {
        return ret;
    }
}

/**
 * Execute one write transaction on the I2C bus, sending a given number of
 * bytes. The bytes in the supplied buffer must be sent to the given address. If
 * the slave device does not acknowledge any of the bytes, an error shall be
 * returned.
 *
 * @param address 7-bit I2C address to write to
 * @param data    pointer to the buffer containing the data to write
 * @param count   number of bytes to read from the buffer and send over I2C
 * @returns 0 on success, error code otherwise
 */
int8_t sensirion_i2c_hal_write(uint8_t address, const uint8_t *data, uint8_t count) {
    /* TODO:IMPLEMENT */
    int ret = i2c_write_blocking_until(I2C_INST, address, data, count, false, 1000);
    if(ret > 0) {
        return 0;
    } else {
        return ret;
    }
}

/**
 * Sleep for a given number of microseconds. The function should delay the
 * execution for at least the given time, but may also sleep longer.
 *
 * Despite the unit, a <10 millisecond precision is sufficient.
 *
 * @param useconds the sleep time in microseconds
 */
void sensirion_i2c_hal_sleep_usec(uint32_t useconds) {
    /* TODO:IMPLEMENT */
    sleep_us(useconds);
}
