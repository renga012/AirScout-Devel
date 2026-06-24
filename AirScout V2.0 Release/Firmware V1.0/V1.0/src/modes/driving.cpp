#include "driving.h"

#include <ff.h>
#include <pico/types.h>

#include "driver/fs.h"
#include "driver/init_hardware.h"
#include "gui/elements/floating_parameters.h"
#include "gui/elements/menu_bar.h"
#include "gui/gui_utils.h"
#include "main.h"
#include "modes/baseMode.h"
#include "utils.h"

bool DrivingMode::init() {
    user_settings.getint("hardware/meas_interval_sec", &m_measurement_interval_ms);
    user_settings.getint("last_chunk_index", &m_chunk_index);
    m_measurement_interval_ms *= 1000;  // convert to ms
    m_measurements_per_chunk = MEASUREMENTS_PER_CHUNK;
    m_measurements_done = 0;
    f_mkdir("/chunks");
    m_data = (meas_data_t *)calloc(sizeof(meas_data_t), m_measurements_per_chunk);

    menu_bar.setLayout(GUI::Menu_Bar::Icons::BACK, GUI::Menu_Bar::Icons::NONE, GUI::Menu_Bar::Icons::NONE, GUI::Menu_Bar::Icons::NONE);
    m_floating_parameters = GUI::Floating_Parameters(&disp);
    GUI::activateElement(&m_floating_parameters);
    if(M8Q.state == Sensor_State::OK) {
        M8Q.updateSettings();
    }
    return true;
}

void DrivingMode::run() {
    static absolute_time_t lastBTNCheck = 0;
    static absolute_time_t lastMeasurement = 0;

    if(absolute_time_diff_us(lastBTNCheck, get_absolute_time()) > BUTTON_CHECK_INTERVAL_MS * 1000) {
        lastBTNCheck = get_absolute_time();
        BTN btn;
        do {
            btn = getNextButton();
            switch(btn) {
                case BTN::BTN1:
                    switchToMode(MODES::Idle);
                    break;
                case BTN::BTN2:
                // pause
                default:
                    break;
            }
        } while(btn != BTN::NONE);
    }

    if(absolute_time_diff_us(lastMeasurement, get_absolute_time()) > m_measurement_interval_ms * 1000) {
        // writeDataToFile(const meas_data_t *data, const uint16_t data_len, const char *path)
        lastMeasurement = get_absolute_time();

#ifdef USE_FAKE_VALS
        m_data[m_measurements_done] = getFakeData();
#else
        m_data[m_measurements_done] = getAllValues();
#endif

        /*
        if(m_data[m_measurements_done].camm8q_data_good) {
            m_data[m_measurements_done].latitude += ((rand() % 100) - 50) / 1000.0;
            m_data[m_measurements_done].longitude += ((rand() % 100) - 50) / 1000.0;
            // ulog_debug("%f, %f", a, b);
            // ulog_debug("%f, %f", m_data[m_measurements_done].latitude, m_data[m_measurements_done].longitude);
        }
        */
        // update gui
        m_floating_parameters.updateData(&m_data[m_measurements_done]);

        m_measurements_done++;
        if(m_measurements_done >= m_measurements_per_chunk) {
            m_measurements_done = 0;
            char tmp[100];
            sprintf(tmp, "/chunks/chunk_%d.json", m_chunk_index);
            writeDataToFile(m_data, m_measurements_per_chunk, tmp);
            m_chunk_index++;
            user_settings.setint("last_chunk_index", m_chunk_index);
        }

        m_floating_parameters.updateStorageData(getFreeSpace(), (m_chunk_index - 1) * m_measurements_per_chunk + m_measurements_done);
    }
}

void DrivingMode::exit() {
    GUI::resetActiveElements();
    // M8Q.dump_db();
    free(m_data);
}