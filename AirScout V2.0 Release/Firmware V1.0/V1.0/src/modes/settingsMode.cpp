#include "settingsMode.h"

#include <ulog.h>

#include "driver/baseSensor.h"
#include "driver/init_hardware.h"
#include "gui/elements/selection.h"
#include "gui/gui_utils.h"
#include "main.h"
#include "utils.h"

static bool reboot_required = false;

static GUI::selection_item
    settings_selections[] =
        {
            // Settings
            {.name = "Measure Interval",
             .type = GUI::Selection::TASKS::NUMBER,                                                 // Type
             .cbk =
                 [](GUI::selection_item *item) {
                     user_settings.setint("hardware/meas_interval_sec", item->context.number.num);  // cbk
                 },
             .context = {.number = {.min = 10, .max = 1000, .step_size = 10, .unit = "s"}}},        // Measure Intervall
            {.name = "Brightness",
             .type = GUI::Selection::TASKS::NUMBER,                                                 // Type
             .cbk =
                 [](GUI::selection_item *item) {
                     user_settings.setint("brightness", item->context.number.num);                  // cbk
                     disp.setBrightness(item->context.number.num);
                 },
             .context = {.number = {.min = 10, .max = 100, .step_size = 10, .unit = ""}}},
            {.name = "BME680",
             .type = GUI::Selection::TASKS::TOGGLE,
             .cbk =
                 [](GUI::selection_item *item) {
                     ulog_info("BME Status: %d", item->context.toggle);
                     user_settings.setBool("hardware/en_bme680", item->context.toggle);  // cbk
                     reboot_required = true;
                 },
             .context = {.toggle = false}},                                              // BME680

            {.name = "SPS30",
             .type = GUI::Selection::TASKS::TOGGLE,
             .cbk =
                 [](GUI::selection_item *item) {
                     ulog_info("SPS Status: %d", item->context.toggle);
                     user_settings.setBool("hardware/en_sps30", item->context.toggle);  // cbk
                     reboot_required = true;
                 },
             .context = {.toggle = false}},                                             // SPS30

            {.name = "CAM M8Q",
             .type = GUI::Selection::TASKS::TOGGLE,
             .cbk =
                 [](GUI::selection_item *item) {
                     ulog_info("GNSS Status: %d", item->context.toggle);
                     user_settings.setBool("hardware/en_cam_m8q", item->context.toggle);  // cbk
                     reboot_required = true;
                 },
             .context = {.toggle = false}},                                               // CAM M8Q

            {.name = "Timezone shift",
             .type = GUI::Selection::TASKS::NUMBER,
             .cbk =
                 [](GUI::selection_item *item) {
                     user_settings.setint("time_zone", item->context.number.num);  // cbk
                     status_bar.set_time_zone();
                 },
             .context = {.number = {.min = -11, .max = 12, .num = 0, .step_size = 1, .unit = "h"}}},
};

bool SettingsMode::init() {
    user_settings.getint("hardware/meas_interval_sec", &settings_selections[0].context.number.num);
    user_settings.getBool("hardware/en_bme680", &settings_selections[1].context.toggle);
    user_settings.getBool("hardware/en_sps30", &settings_selections[2].context.toggle);
    user_settings.getBool("hardware/en_cam_m8q", &settings_selections[3].context.toggle);
    user_settings.getint("time_zone", &settings_selections[4].context.number.num);

    m_selection = GUI::Selection(&disp, settings_selections, sizeof(settings_selections) / sizeof(GUI::selection_item));
    GUI::activateElement(&m_selection);
    return true;
}

void SettingsMode::run() {
    static absolute_time_t lastBTNCheck = 0;

    if(absolute_time_diff_us(lastBTNCheck, get_absolute_time()) > BUTTON_CHECK_INTERVAL_MS * 1000) {
        lastBTNCheck = get_absolute_time();

        BTN btn = getNextButton();
        while(btn != BTN::NONE) {
            m_selection.processBTN(btn);
            btn = getNextButton();
        }
        if(m_selection.should_exit()) {
            switchToMode(MODES::Idle);
        }
    }
}

void SettingsMode::exit() {
    GUI::resetActiveElements();
    if(reboot_required) {
        reboot();
        reboot_required = false;
    }
}