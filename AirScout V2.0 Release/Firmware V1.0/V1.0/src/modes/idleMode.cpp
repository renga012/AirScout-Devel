#include "idleMode.h"

#include <pico/time.h>
#include <pico/types.h>
#include <stdint.h>
#include <string.h>
#include <ulog.h>

#include "driver/baseSensor.h"
#include "driver/fs.h"
#include "driver/init_hardware.h"
#include "globals.h"
#include "gui/elements/floating_parameters.h"
#include "gui/elements/menu_bar.h"
#include "gui/gui_utils.h"
#include "main.h"
#include "modes/baseMode.h"
#include "utils.h"

IdleMode::IdleMode() {
    memset(&m_data, 0, sizeof(meas_data_t));
}

bool IdleMode::init() {
    user_settings.getint("hardware/meas_interval_sec", &m_measurement_interval_ms);
    m_measurement_interval_ms *= 1000;

    menu_bar.setLayout(GUI::Menu_Bar::Icons::NONE, GUI::Menu_Bar::Icons::NONE, GUI::Menu_Bar::Icons::NONE, GUI::Menu_Bar::Icons::MENU);
    m_floating_parameters = GUI::Floating_Parameters(&disp);
    GUI::activateElement(&m_floating_parameters);

    m_floating_parameters.updateData(&m_data);
    int32_t tmp;
    user_settings.getint("last_chunk_index", &tmp);

    m_floating_parameters.updateStorageData(getFreeSpace(), (uint32_t)tmp * MEASUREMENTS_PER_CHUNK);
    if(M8Q.state == Sensor_State::OK) {
        M8Q.updateSettings();
    }

    return true;
}
void IdleMode::run() {
    static absolute_time_t lastBTNCheck = 0;
    static absolute_time_t lastdataupdate = 0;

    if(absolute_time_diff_us(lastBTNCheck, get_absolute_time()) > BUTTON_CHECK_INTERVAL_MS * 1000) {
        lastBTNCheck = get_absolute_time();
        BTN btn = getNextButton();

        while(btn != BTN::NONE) {
            switch(btn) {
                case BTN::BTN4:
                    switchToMode(MODES::ModeSelection);
                    break;
                default:
                    break;
            }
            btn = getNextButton();
        }
    }

    if(absolute_time_diff_us(lastdataupdate, get_absolute_time()) > m_measurement_interval_ms * 1000) {
        lastdataupdate = get_absolute_time();

#ifdef USE_FAKE_VALS
        m_data = getFakeData();
#else
        m_data = getAllValues();
#endif

        m_floating_parameters.updateData(&m_data);
        ulog_info("IDLE: Update DATA");
    }
}

void IdleMode::exit() {
    GUI::resetActiveElements();
}
