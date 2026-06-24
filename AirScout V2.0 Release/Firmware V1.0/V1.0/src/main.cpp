#include "main.h"

#include <hardware/gpio.h>
#include <hardware/rtc.h>
#include <hardware/spi.h>
#include <pico/stdio.h>
#include <pico/types.h>
#include <ulog.h>

#include <vector>

#include "driver/battery.h"
#include "driver/init_hardware.h"
#include "globals.h"
#include "modes/baseMode.h"
#include "utils.h"

Settings user_settings(user_settings_path);
Font_Liberation_Mono8x13 font_Liberation_Mono8x13;
Font_Liberation_Mono10x16 font_Liberation_Mono10x16;
Font_Liberation_Mono13x21 font_Liberation_Mono13x21;
Font_Liberation_Mono17x28 font_Liberation_Mono17x28;

GUI::Menu_Bar menu_bar(&disp);
GUI::Status_Bar status_bar(&disp);
modes_t modes;

std::vector<GUI::Element *> active_elements;

int main() {
    startup();
    
    {
        bool isSetUp;
        user_settings.getBool("is_set_up", &isSetUp);
        if(isSetUp) {
            switchToMode(MODES::Idle);
        } else {
            switchToMode(MODES::Setup);
        }
        datetime_t t;
        t.day = 10;
        t.month = 4;
        t.year = 2026;
        t.hour = 15;
        t.min = 55;
        t.sec = 23;
        t.dotw = 5;
        rtc_set_datetime(&t);
        status_bar.update(true);
    }
    // switchToMode(MODES::WifiSetup);

    disp.setBrightness(20);
    while(true) {
        static absolute_time_t lastUBXCheck = 0;
        static absolute_time_t lastGUIupdate = 0;
        static absolute_time_t lastBatteryCheck = 0;
        runActiveMode(&modes);

        if(M8Q.state == Sensor_State::OK) {
            if(absolute_time_diff_us(lastUBXCheck, get_absolute_time()) > GNSS_UPDATE_INTERVAL_MS * 1000) {
                lastUBXCheck = get_absolute_time();
                M8Q.update_i2c();
                M8Q.processUBX();
            }
        }

        if(absolute_time_diff_us(lastBatteryCheck, get_absolute_time()) > 1000 * 1000) {
            lastBatteryCheck = get_absolute_time();
            status_bar.setBattery(getBatteryPercent());
        }

        if(absolute_time_diff_us(lastGUIupdate, get_absolute_time()) > GUI_UPDATE_INTERVAL_MS * 1000) {
            lastGUIupdate = get_absolute_time();
            for(GUI::Element *element : active_elements) {
                element->update(element->redraw);
                element->redraw = false;
            }
        }

        if(modes.activeModeType != modes.newModeType) {
            handleSwitchToMode(&modes);
        }
    }
}
