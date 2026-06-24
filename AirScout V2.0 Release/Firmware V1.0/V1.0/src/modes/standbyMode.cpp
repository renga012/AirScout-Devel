#include "standbyMode.h"

#include <pico/stdio.h>
#include <pico/sleep.h>

#include "driver/baseSensor.h"
#include "driver/init_hardware.h"
#include "globals.h"
#include "modes/baseMode.h"
#include "utils.h"

bool StandbyMode::init() {
    if(M8Q.state == Sensor_State::OK) {
        M8Q.dump_db();
    }

    return true;
}

void StandbyMode::run() {
    // only sleep when the navigation database is fully downloaded
    if(M8Q.dump_complete()) {
        sleep_ms(1000);

        shutdown();
        sleep_run_from_xosc();
        sleep_goto_dormant_until_pin(BTN1_GPIO, true, true);  // 😴

        sleep_power_up();
        stdio_init_all();
        switchToMode(MODES::Idle);
    }
}

void StandbyMode::exit() {
    startup();
}