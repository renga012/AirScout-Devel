#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <ulog.h>

#include "modes/baseMode.h"
#include "modes/driving.h"
#include "modes/idleMode.h"
#include "modes/modeSelectionMode.h"
#include "modes/settingsMode.h"
#include "modes/setupMode.h"
#include "modes/standbyMode.h"
#include "modes/uploadMode.h"
#include "modes/wifiSetup.h"
#include "globals.h"

static uint64_t _______PROFILE_start_time_us;

#define LOG_PROFILE ULOG_LEVEL_6
const ulog_level_descriptor syslog_levels = {
    .max_level = ULOG_LEVEL_6,  // allow 0..7
    .names = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "PROFILE"},
};

#define START_PROFILE()  _______PROFILE_start_time_us = to_us_since_boot(get_absolute_time());
#define END_PROFILE(fun) ulog(LOG_PROFILE, "\"%s\" took %.2fms", fun, (to_us_since_boot(get_absolute_time()) - _______PROFILE_start_time_us) / 1000.0);

typedef struct {
    DrivingMode driving;
    WifiSetupMode wifiSetup;
    BaseMode base;
    IdleMode idle;
    UploadMode upload;
    SetupMode setup;
    StandbyMode sleep;
    ModeSelectionMode selection;
    SettingsMode settings;
    
    MODES activeModeType;
    MODES newModeType;
} modes_t;

void initLogger();
meas_data_t getFakeData();
meas_data_t getAllValues();
bool writeDataToFile(const meas_data_t *data, const uint16_t data_len, const char *path);

bool handleSwitchToMode(modes_t *modes);
void runActiveMode(modes_t *modes);
BTN getNextButton();

void generateUniqueId();
void startup();
void shutdown();
float calcAltitudeFromPressure(const float pressure);
void switchToMode(MODES mode);
void reboot();
#endif  // UTILS_H
