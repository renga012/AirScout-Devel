#ifndef MODES_DRIVING_H
#define MODES_DRIVING_H
#include <stdint.h>

#include "baseMode.h"
#include "globals.h"
#include "gui/elements/floating_parameters.h"

class DrivingMode : public BaseMode {
public:
    bool init();
    void run();
    void exit();

private:
    int32_t m_measurement_interval_ms;
    uint32_t m_measurements_per_chunk;
    int32_t m_chunk_index;
    uint32_t m_measurements_done;
    meas_data_t *m_data;
    GUI::Floating_Parameters m_floating_parameters;
};

#endif  // MODES_DRIVING_H
