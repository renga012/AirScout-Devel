#ifndef DRIVER_SPS30_H
#define DRIVER_SPS30_H

#include <stdint.h>

#include "driver/baseSensor.h"
#include "stdint.h"

typedef struct {
    uint16_t mc_1p0;
    uint16_t mc_2p5;
    uint16_t mc_4p0;
    uint16_t mc_10p0;
    uint16_t nc_0p5;
    uint16_t nc_1p0;
    uint16_t nc_2p5;
    uint16_t nc_4p0;
    uint16_t nc_10p0;
    uint16_t typical_particle_size;
} sps30_data_t;

class Sps30 : public BaseSensor {
public:
    Sps30(uint8_t addr);
    ~Sps30();
    bool init();
    bool deinit();
    int16_t getValues(sps30_data_t *data);

private:
    uint8_t m_addr;
};

#endif /* DRIVER_SPS30_H */
