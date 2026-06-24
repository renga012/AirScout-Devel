#ifndef MODES_IDLEMODE_H
#define MODES_IDLEMODE_H

#include <stdint.h>
#include "globals.h"
#include "gui/elements/floating_parameters.h"
#include "modes/baseMode.h"

class IdleMode : public BaseMode {
public:
    IdleMode();
    bool init();
    void run();
    void exit();

private:
    meas_data_t m_data;
    GUI::Floating_Parameters m_floating_parameters;
    int32_t m_measurement_interval_ms = 0;
};

#endif  // MODES_IDLEMODE_H
