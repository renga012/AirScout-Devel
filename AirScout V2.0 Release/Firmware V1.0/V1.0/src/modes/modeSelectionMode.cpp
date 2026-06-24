#include "modeSelectionMode.h"

#include <pico/time.h>
#include <pico/types.h>

#include "driver/init_hardware.h"
#include "gui/elements/selection.h"
#include "gui/gui_utils.h"
#include "main.h"
#include "modes/baseMode.h"
#include "ulog.h"
#include "utils.h"

static GUI::selection_item mode_selectons[] = {
    {.name = "Driving", .type = GUI::Selection::TASKS::ACTIVATE_MODE, .context = {.mode = MODES::Drive}},            // item    .cbk = &mode_selection_cbk,
    {.name = "Upload", .type = GUI::Selection::TASKS::ACTIVATE_MODE, .context = {.mode = MODES::Upload}},            // item    .cbk = &mode_selection_cbk,
    {.name = "Wifi Settings", .type = GUI::Selection::TASKS::ACTIVATE_MODE, .context = {.mode = MODES::WifiSetup}},  // item    .cbk = &mode_selection_cbk,
    {.name = "Sleep", .type = GUI::Selection::TASKS::ACTIVATE_MODE, .context = {.mode = MODES::Standby}},            // item    .cbk = &mode_selection_cbk,

    {.name = "Restart", .type = GUI::Selection::TASKS::ACTIVATE, .cbk = [](GUI::selection_item *item) { reboot(); }},

    {.name = "Settings", .type = GUI::Selection::TASKS::ACTIVATE_MODE, .context = {.mode = MODES::Settings}},  // item    .cbk = &mode_selection_cbk,
    {.name = "Setup", .type = GUI::Selection::TASKS::ACTIVATE_MODE, .context = {.mode = MODES::Setup}},            // item    .cbk = &mode_selection_cbk,
};

bool ModeSelectionMode::init() {
    m_selection = GUI::Selection(&disp, mode_selectons, sizeof(mode_selectons) / sizeof(GUI::selection_item));
    GUI::activateElement(&m_selection);
    return true;
}

void ModeSelectionMode::run() {
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

void ModeSelectionMode::exit() {
    GUI::resetActiveElements();
}