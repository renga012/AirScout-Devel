#include "bme680.h"

#include <bme68x.h>
#include <bme68x_defs.h>
#include <hardware/i2c.h>
#include <pico/error.h>
#include <pico/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ulog.h>

#include "globals.h"

static int8_t bme_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
static int8_t bme_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
static void bme_delay_us(uint32_t period, void *intf_ptr);

BME680::BME680(uint8_t dev_addr) : BaseSensor() {
    m_dev_addr = dev_addr;
    m_bme.read = bme_i2c_read;
    m_bme.write = bme_i2c_write;
    m_bme.intf = BME68X_I2C_INTF;
    m_bme.delay_us = bme_delay_us;
    m_bme.intf_ptr = &m_dev_addr;
    m_bme.amb_temp = 25;
}

bool BME680::init() {
    int8_t rslt;
    m_conf.filter = BME68X_FILTER_OFF;
    m_conf.odr = BME68X_ODR_NONE; /* This parameter defines the sleep duration after each profile */
    m_conf.os_hum = BME68X_OS_16X;
    m_conf.os_pres = BME68X_OS_16X;
    m_conf.os_temp = BME68X_OS_16X;

    rslt = bme68x_init(&m_bme);
    m_check_rslt("bme68x_init", rslt);

    rslt = bme68x_set_conf(&m_conf, &m_bme);
    m_check_rslt("bme68x_set_conf", rslt);

    m_heatr_conf.enable = BME68X_ENABLE;
    m_heatr_conf.heatr_temp = 300;
    m_heatr_conf.heatr_dur = 100;

    rslt = bme68x_set_heatr_conf(BME68X_FORCED_MODE, &m_heatr_conf, &m_bme);
    m_check_rslt("bme68x_set_heatr_conf", rslt);

    return true;
}

bool BME680::deinit() {
    bme68x_set_op_mode(BME68X_SLEEP_MODE, &m_bme);
    return true;
}

void BME680::getValues(bmedata_t *data, uint8_t *n_fields) {
    uint8_t rslt;
    rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, &m_bme);
    m_check_rslt("bme68x_set_op_mode", rslt);

    /* Calculate delay period in microseconds */
    uint32_t del_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &m_conf, &m_bme) + (m_heatr_conf.heatr_dur * 1000);
    m_bme.delay_us(del_period, m_bme.intf_ptr);
    // ulog_info("Bme waiting for %.2f ms...", del_period/1000.0);
    struct bme68x_data vals;
    rslt = bme68x_get_data(BME68X_FORCED_MODE, &vals, n_fields, &m_bme);
    bme68x_set_op_mode(BME68X_SLEEP_MODE, &m_bme);
    data->gas_resistance = vals.gas_resistance;
    data->humidity = vals.humidity;
    data->temperature = vals.temperature;
    data->pressure = vals.pressure;

    m_check_rslt("bme68x_get_data", rslt);
    m_calc_AQ(data);
}

void BME680::m_check_rslt(const char api_name[], int8_t rslt) {
    switch(rslt) {
        case BME68X_OK:

            /* Do nothing */
            break;
        case BME68X_E_NULL_PTR:
            ulog_error("API name [%s]  Error [%d] : Null pointer", api_name, rslt);
            break;
        case BME68X_E_COM_FAIL:
            ulog_error("API name [%s]  Error [%d] : Communication failure", api_name, rslt);
            break;
        case BME68X_E_INVALID_LENGTH:
            ulog_error("API name [%s]  Error [%d] : Incorrect length parameter", api_name, rslt);
            break;
        case BME68X_E_DEV_NOT_FOUND:
            ulog_error("API name [%s]  Error [%d] : Device not found", api_name, rslt);
            break;
        case BME68X_E_SELF_TEST:
            ulog_error("API name [%s]  Error [%d] : Self test error", api_name, rslt);
            break;
        case BME68X_W_NO_NEW_DATA:
            ulog_warn("API name [%s]  Warning [%d] : No new data found", api_name, rslt);
            break;
        default:
            ulog_error("API name [%s]  Error [%d] : Unknown error code", api_name, rslt);
            break;
    }
}

void BME680::m_calc_AQ(bmedata_t *data) {
    static const float hum_weighting = 0.25;  // so hum effect is 25% of the total air quality score
    static const float gas_weighting = 0.75;  // so gas effect is 75% of the total air quality score
    static const float hum_reference = 40;
    static const float gas_lolim = 5000;      // Bad air quality limit
    static const float gas_uplim = 50000;     // Good air quality limit

    float hum_score, gas_score;

    // Calculate humidity contribution to IAQ index
    float current_humidity = data->humidity;

    if(current_humidity >= 38 && current_humidity <= 42)
        hum_score = hum_weighting * 100;  // Humidity +/-5% around optimum
    else {                                // sub-optimal
        if(current_humidity < 38)
            hum_score = hum_weighting / hum_reference * current_humidity * 100;
        else {
            hum_score = ((-hum_weighting / (100 - hum_reference) * current_humidity) + 0.416666) * 100;
        }
    }

    // Calculate gas contribution to IAQ index
    float gas = data->gas_resistance;

    if(gas > gas_uplim)
        gas = gas_uplim;

    if(gas < gas_lolim)
        gas = gas_lolim;

    gas_score = (gas_weighting / (gas_uplim - gas_lolim) * gas - (gas_lolim * (gas_weighting / (gas_uplim - gas_lolim)))) * 100;
    float air_quality_score = hum_score + gas_score;

    data->score = (100 - air_quality_score) * 5;
}

static int8_t bme_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    uint8_t device_addr = *(uint8_t *)intf_ptr;

    int err = i2c_write_blocking_until(I2C_INST, device_addr, &reg_addr, 1, true, 1000);
    if(err == PICO_ERROR_GENERIC || err == PICO_ERROR_TIMEOUT) {
        return BME68X_E_COM_FAIL;
    }

    uint8_t read = i2c_read_blocking(I2C_INST, device_addr, reg_data, len, false);
    if(read) {
        return BME68X_OK;
    }
    return BME68X_E_COM_FAIL;
}

static int8_t bme_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    uint8_t device_addr = *(uint8_t *)intf_ptr;

    uint8_t *buf = (uint8_t *)malloc(len + 1);
    buf[0] = reg_addr;
    memcpy(buf + 1, reg_data, len);

    int err = i2c_write_blocking_until(I2C_INST, device_addr, buf, len + 1, false, 1000);
    if(err == PICO_ERROR_GENERIC || err == PICO_ERROR_TIMEOUT) {
        free(buf);
        return BME68X_E_COM_FAIL;
    }

    free(buf);
    return BME68X_OK;
}

static void bme_delay_us(uint32_t period, void *intf_ptr) {
    sleep_us(period);
}
