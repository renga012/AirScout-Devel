#include "utils.h"

#include <cJSON.h>
#include <ff.h>
#include <hardware/adc.h>
#include <hardware/rtc.h>
#include <hardware/spi.h>
#include <math.h>
#include <pico/cyw43_arch.h>
#include <pico/stdio.h>
#include <pico/unique_id.h>
#include <stdint.h>
#include <string.h>
#include <ulog.h>
#include <cstdio>

extern "C" {
#include <sha256.h>
}

#include "driver/baseSensor.h"
#include "driver/fs.h"
#include "driver/init_hardware.h"
#include "driver/m8q_gnss.h"
#include "driver/sps30.h"
#include "globals.h"
#include "gui/gui_utils.h"
#include "main.h"
#include "modes/baseMode.h"
#include "modes/modeSelectionMode.h"
#include "settings.h"
#include "wifi.h"

void initLogger() {
    ulog_level_set_new_levels(&syslog_levels);
}

meas_data_t getFakeData() {
    meas_data_t data;
    data.bme680_data_good = 1;
    data.camm8q_data_good = 1;
    data.sps30_data_good = 1;
    data.time_good = 1;
    rtc_get_datetime(&data.time);
    data.gas = 27430;
    data.aq_score = 22;
    data.altitude = 214.2;
    data.humidity = 57.3423;
    data.temperature = 23.23213;
    data.latitude = 48.207215;
    data.longitude = 15.617774;
    data.pressure = 99103;
    data.mc_10p0 = 0;
    data.mc_1p0 = 0;
    data.mc_2p5 = 0;
    data.mc_4p0 = 0;

    data.nc_0p5 = 0;
    data.nc_10p0 = 0;
    data.nc_1p0 = 0;
    data.nc_2p5 = 0;
    data.nc_4p0 = 0;
    data.typical_particle_size = 0;
    return data;
}

meas_data_t getAllValues() {
    meas_data_t data;
    memset(&data, 0, sizeof(meas_data_t));

    if(!rtc_get_datetime(&data.time)) {
        data.time_good = false;
    }

    gnss_data_t gnss_data;
    if(M8Q.state == Sensor_State::OK) {
        M8Q.getValues(&gnss_data);
        if(gnss_data.valid) {
            data.altitude = gnss_data.altitude;
            data.latitude = gnss_data.latitude;
            data.longitude = gnss_data.longitude;
            data.camm8q_data_good = true;
            data.time_good = true;
        }
    }

    bmedata_t bme_data;
    uint8_t n_fields;
    if(bme680.state == Sensor_State::OK) {
        bme680.getValues(&bme_data, &n_fields);
        if(n_fields) {
            data.temperature = bme_data.temperature;
            data.humidity = bme_data.humidity;
            data.gas = bme_data.gas_resistance;
            data.pressure = bme_data.pressure;
            data.aq_score = bme_data.score;
            data.bme680_data_good = true;
        }
    }

    if(data.bme680_data_good) {
        float pressure_altitude = calcAltitudeFromPressure(data.pressure);
        if(data.camm8q_data_good) {
            data.altitude *= GNSS_ALT_WEIGTH;
            data.altitude += pressure_altitude * PRESSURE_ALT_WEIGTH;
        } else {
            data.altitude = pressure_altitude;
        }
    }

    sps30_data_t sps30_data;
    if(sps30.state == Sensor_State::OK) {
        if(!sps30.getValues(&sps30_data)) {
            data.mc_1p0 = sps30_data.mc_1p0;
            data.mc_2p5 = sps30_data.mc_2p5;
            data.mc_4p0 = sps30_data.mc_4p0;
            data.mc_10p0 = sps30_data.mc_10p0;
            data.nc_0p5 = sps30_data.nc_0p5;
            data.nc_1p0 = sps30_data.nc_1p0;
            data.nc_2p5 = sps30_data.nc_2p5;
            data.nc_4p0 = sps30_data.nc_4p0;
            data.nc_10p0 = sps30_data.nc_10p0;
            data.typical_particle_size = sps30_data.typical_particle_size;
            data.sps30_data_good = true;
        } else {
            // error
            data.sps30_data_good = false;
        }
    }

    return data;
}

bool writeDataToFile(const meas_data_t *data, const uint16_t data_len, const char *path) {
    cJSON *json = cJSON_CreateObject();
    cJSON *stamps = NULL;
    char tmpStr[32];

    stamps = cJSON_AddArrayToObject(json, "measurements");
    for(size_t index = 0; index < data_len; index++) {
        cJSON *stamp = cJSON_CreateObject();

        // TODO: Dont add bad data
        if(data->bme680_data_good) {
            if(cJSON_AddNumberToObject(stamp, "temperature", data[index].temperature) == NULL) {
                ulog_error("Cant add temperature to json, index %d", index);
            }

            if(cJSON_AddNumberToObject(stamp, "humidity", data[index].humidity) == NULL) {
                ulog_error("Cant add humidity to json, index %d", index);
            }

            if(cJSON_AddNumberToObject(stamp, "airPressure", data[index].pressure) == NULL) {
                ulog_error("Cant add pressure to json, index %d", index);
            }

            if(cJSON_AddNumberToObject(stamp, "gasResistance", data[index].gas) == NULL) {
                ulog_error("Cant add gas to json, index %d", index);
            }
        }
        if(data->camm8q_data_good) {
            if(cJSON_AddNumberToObject(stamp, "latitude", data[index].latitude) == NULL) {
                ulog_error("Cant add latitude to json, index %d", index);
            }

            if(cJSON_AddNumberToObject(stamp, "longitude", data[index].longitude) == NULL) {
                ulog_error("Cant add longitude to json, index %d", index);
            }
        }
        if(data->camm8q_data_good || data->bme680_data_good) {
            if(cJSON_AddNumberToObject(stamp, "altitude", data[index].altitude) == NULL) {
                ulog_error("Cant add altitude to json, index %d", index);
            }
        }

        if(data->sps30_data_good) {
            if(cJSON_AddNumberToObject(stamp, "part_1_mass", data[index].mc_1p0) == NULL) {
                ulog_error("Cant add mc_1p0 to json, index %d", index);
            }

            if(cJSON_AddNumberToObject(stamp, "part_2_5_mass", data[index].mc_2p5) == NULL) {
                ulog_error("Cant add mc_2p5 to json, index %d", index);
            }

            if(cJSON_AddNumberToObject(stamp, "part_4_mass", data[index].mc_4p0) == NULL) {
                ulog_error("Cant add mc_4p0 to json, index %d", index);
            }

            if(cJSON_AddNumberToObject(stamp, "part_10_mass", data[index].mc_10p0) == NULL) {
                ulog_error("Cant add mc_10p0 to json, index %d", index);
            }

            if(cJSON_AddNumberToObject(stamp, "nc_0p5", data[index].nc_0p5) == NULL) {
                ulog_error("Cant add nc_0p5 to json, index %d", index);
            }

            if(cJSON_AddNumberToObject(stamp, "part_1_particle_count", data[index].nc_1p0) == NULL) {
                ulog_error("Cant add nc_1p0 to json, index %d", index);
            }

            if(cJSON_AddNumberToObject(stamp, "part_2_5_particle_count", data[index].nc_2p5) == NULL) {
                ulog_error("Cant add nc_2p5 to json, index %d", index);
            }

            if(cJSON_AddNumberToObject(stamp, "part_4_particle_count", data[index].nc_4p0) == NULL) {
                ulog_error("Cant add nc_4p0 to json, index %d", index);
            }

            if(cJSON_AddNumberToObject(stamp, "part_10_particle_count", data[index].nc_10p0) == NULL) {
                ulog_error("Cant add nc_10p0 to json, index %d", index);
            }

            if(cJSON_AddNumberToObject(stamp, "typical_particle_size", data[index].typical_particle_size) == NULL) {
                ulog_error("Cant add typical_particle_size to json, index %d", index);
            }
        }

        memset(tmpStr, 0, 32);
        sprintf(tmpStr, "%04d-%02d-%02dT%02d:%02d:%02d", data[index].time.year, data[index].time.month, data[index].time.day, data[index].time.hour, data[index].time.min, data[index].time.sec);

        if(cJSON_AddStringToObject(stamp, "measuredAt", tmpStr) == NULL) {
            ulog_error("Cant add time to json, index %d, time: %s", index, tmpStr);
        }

        // sprintf(tmpStr, "%04d-%02d-%02dT%02d:%02d:%02d", data[index].time.year, data[index].time.month, data[index].time.day, data[index].time.hour, data[index].time.min, data[index].time.sec);
        cJSON_AddItemToArray(stamps, stamp);
    }
    char *tmp = NULL;
    if(user_settings.getString("token", &tmp) != SETSTATUS::OK) {
        tmp = NULL;
        ulog_error("Could not find token");
    }

    if(cJSON_AddStringToObject(json, "token", tmp) == NULL) {
        ulog_error("Cant add token to json, index %d, token: %s", index, tmp);
    }
    free(tmp);

    char *jsonString = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    // printf("%s\n", jsonString);

    FIL fp;
    FRESULT fr;

    fr = f_open(&fp, path, FA_WRITE | FA_CREATE_ALWAYS);
    if(fr != FR_OK) {
        ulog_error("Error while opening %s->%s", path, FRESULT_str(fr));
        free(jsonString);
        return false;
    }

    UINT bytes_written = 0;
    fr = f_write(&fp, jsonString, strlen(jsonString), &bytes_written);
    if(fr != FR_OK) {
        ulog_error("Error while writing %s->%s", path, FRESULT_str(fr));
    }

    ulog_info("Writing %d Bytes to %s. %d Bytes were actually written.", strlen(jsonString), path, bytes_written);
    free(jsonString);

    fr = f_close(&fp);
    if(fr != FR_OK) {
        ulog_error("Error while closing %s->%s", path, FRESULT_str(fr));
        return false;
    }

    return true;
}

bool handleSwitchToMode(modes_t *modes) {
    bool status = false;

    switch(modes->activeModeType) {
        case MODES::Drive:
            modes->driving.exit();
            break;
        case MODES::Idle:
            modes->idle.exit();
            break;
        case MODES::Upload:
            modes->upload.exit();
            break;
        case MODES::Standby:
            modes->sleep.exit();
            break;
        case MODES::Mode:
            modes->base.exit();
            break;
        case MODES::WifiSetup:
            modes->wifiSetup.exit();
            break;
        case MODES::Setup:
            modes->setup.exit();
        case MODES::ModeSelection:
            modes->selection.exit();
            break;
        case MODES::Settings:
            modes->settings.exit();
            break;
    }

    switch(modes->newModeType) {
        case MODES::Drive:
            ulog_info("Switching Mode to Driving");
            status = modes->driving.init();
            break;
        case MODES::Idle:
            ulog_info("Switching Mode to Idle");
            status = modes->idle.init();
            break;
        case MODES::Upload:
            ulog_info("Switching Mode to Uploading");
            status = modes->upload.init();
            break;
        case MODES::Standby:
            ulog_info("Switching Mode to Sleep");
            status = modes->sleep.init();
            break;
        case MODES::Mode:
            ulog_info("Switching Mode to Mode");
            status = modes->base.init();
            break;
        case MODES::WifiSetup:
            ulog_info("Switching Mode to Wifi Setup");
            status = modes->wifiSetup.init();
            break;
        case MODES::Setup:
            ulog_info("Switching Mode to Wifi Setup");
            status = modes->setup.init();
            break;
        case MODES::ModeSelection:
            ulog_info("Switching Mode to Selection");
            status = modes->selection.init();
            break;
        case MODES::Settings:
            ulog_info("Switching Mode to Settings");
            status = modes->settings.init();
            break;
    }
    modes->activeModeType = modes->newModeType;
    return status;
}

void runActiveMode(modes_t *modes) {
    switch(modes->activeModeType) {
        case MODES::Drive:
            modes->driving.run();
            break;
        case MODES::Idle:
            modes->idle.run();
            break;
        case MODES::Upload:
            modes->upload.run();
            break;
        case MODES::Standby:
            modes->sleep.run();
            break;
        case MODES::Mode:
            modes->base.run();
            break;
        case MODES::WifiSetup:
            modes->wifiSetup.run();
            break;
        case MODES::Setup:
            modes->setup.run();
            break;
        case MODES::ModeSelection:
            modes->selection.run();
            break;
        case MODES::Settings:
            modes->settings.run();
            break;
    }
}

BTN getNextButton() {
    BTN btn = BTN::NONE;
    switch(btn_input_pipe & 0xF) {
        case BTN1_BIT:
            btn = BTN::BTN1;
            break;
        case BTN2_BIT:
            btn = BTN::BTN2;
            break;
        case BTN3_BIT:
            btn = BTN::BTN3;
            break;
        case BTN4_BIT:
            btn = BTN::BTN4;
            break;
    }
    btn_input_pipe >>= 4;
    return btn;
}

void generateUniqueId() {
    static char uid[UID_LEN];

    uint8_t hash[SHA256_BLOCK_SIZE];
    SHA256_CTX ctx;
    sha256_init(&ctx);

    char id_out[9];
    pico_get_unique_board_id_string(id_out, sizeof(id_out));
    ulog_info("Unique ID: %s", id_out);
    sha256_update(&ctx, id_out, strlen(id_out));

    // the mac address is read when wifi is used
    wifi.enableAP("", "oooooooo", WAUTH::WPA2_AES_PSK);
    wifi.disableAP();

    uint8_t mac[6];
    cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac);
    ulog_info("MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    sha256_update(&ctx, (const char *)mac, 6);
    sha256_final(&ctx, hash);

    // convert to hex representation
    snprintf(uid, UID_LEN, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7], hash[8], hash[9], hash[10], hash[11], hash[12], hash[13], hash[14], hash[15], hash[16], hash[17], hash[18], hash[19], hash[20], hash[21], hash[22], hash[23], hash[24], hash[25], hash[26], hash[27], hash[28], hash[29], hash[30], hash[31]);

    ulog_info("UID: %s", uid);
    user_settings.setString("token", uid);

    snprintf(uid, 32, "AIRSCOUT_%02X%02X%02X%02X%02X%02X%02X%02X", hash[6], hash[7], hash[8], hash[9], hash[10], hash[11], hash[12], hash[13]);
    user_settings.setString("hostname", uid);
    ulog_info("Hostname: %s", uid);
    user_settings.saveToDisk();
}

void startup() {
    sleep_ms(1000);
    stdio_init_all();
    adc_init();
    rtc_init();
    initLogger();
    initI2C();
    initUART();
    initButtons();
    initfs(&fs);

    // Initialise the Wi-Fi chip
    if(cyw43_arch_init_with_country(CYW43_COUNTRY_AUSTRIA)) {
        ulog_error("Wi-Fi init failed");
        return;
    }

    wifi.disableAP();
    wifi.disableStation();

    spi_set_baudrate(spi1, SPI1_CLOCK);
    // ulog_info("SPI Baudrate %d", spi_set_baudrate(spi1, 25 * 1000 * 1000));

    disp.init();
    disp.draw_text(10, 10, 1, "Loading Settings..", &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);
    user_settings.loadSettings();
    user_settings.printjson();
    disp.draw_text(10, 10, 1, "Initialising Sensor\nHardware..", &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);
    // while (true) {
    // status_bar.update(true);
    // }
    initSensors();

    modes.driving = DrivingMode();
    modes.upload = UploadMode();
    modes.wifiSetup = WifiSetupMode();
    modes.idle = IdleMode();
    modes.setup = SetupMode();
    modes.sleep = StandbyMode();
    modes.selection = ModeSelectionMode();
    modes.activeModeType = MODES::Mode;
    modes.newModeType = MODES::Idle;

    status_bar.set_time_zone();
    active_elements.clear();
    GUI::activateElement(&status_bar);
    GUI::activateElement(&menu_bar);
}

void shutdown() {
    disp.cleanup();
    disp.setBrightness(0);
    disp.sleep(true);
    deinitSensors();
    cyw43_arch_deinit();
    f_unmount("");
}

// https://www.brisbanehotairballooning.com.au/pressure-and-altitude-conversion/
float calcAltitudeFromPressure(const float pressure) {
    double altitude = 0.0f;
    float pressure_hpa = pressure / 100.0f;  // convert to hpa

    double top = pow(10, log10(pressure_hpa / PRESSURE_AT_SEA_LEVEL_HPA) / 5.2558797) - 1.0f;
    static const double bottom = -6.8755856e-6;
    altitude = top / bottom;

    // convert from feet to meters
    altitude /= 3.28084f;

    return altitude;
}

void switchToMode(MODES mode) {
    modes.newModeType = mode;
}

void reboot() {
    shutdown();
    sleep_ms(1000);
    startup();
    switchToMode(MODES::Idle);
}