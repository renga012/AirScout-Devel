#ifndef DRIVER_BME680_H
#define DRIVER_BME680_H
#include <bme68x.h>
#include <bme68x_defs.h>
#include <hardware/i2c.h>
#include <stdint.h>

#include "baseSensor.h"

typedef struct {
    float temperature;
    float pressure;
    float humidity;
    float gas_resistance;
    float score;
} bmedata_t;

class BME680 : public BaseSensor {
public:
    BME680(uint8_t dev_addr);
    void getValues(bmedata_t *data, uint8_t *n_fields);
    bool init();
    bool deinit();

private:
    bme68x_dev m_bme;
    bme68x_conf m_conf;
    bme68x_heatr_conf m_heatr_conf;
    uint8_t m_dev_addr;
    void m_calc_AQ(bmedata_t *data);
    void m_check_rslt(const char api_name[], int8_t rslt);
};

#endif  // DRIVER_BME680_H
