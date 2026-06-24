#ifndef GLOBALS_H
#define GLOBALS_H

#include <pico/types.h>

// #define USE_FAKE_VALS

#define I2C_SDA_PIN  26
#define I2C_SCL_PIN  27
#define I2C_BAUDRATE 400000
#define I2C_INST      i2c1

#define SPI1_SCK_PIN  10
#define SPI1_MOSI_PIN 15
#define SPI1_MISO_PIN 12
#define SPI1_CLOCK    25000000

#define UART0_TX_PIN       0
#define UART0_RX_PIN       1
#define UART0_DEFAULT_BAUD 9600
#define UART0_TARGET_BAUD  460800

#define DISPLAY_CS_PIN  9
#define DISPLAY_DC_PIN  7
#define DISPLAY_LED_PIN 5
#define DISPLAY_RST_PIN 8
#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  480

#define ADC_REF_CRTL     4
#define BAT_CHARGING_INT 2

#define GNSS_RST     21
#define GNSS_EXT_INT 22
#define ADC_BAT      28

#define BTN1_GPIO 19
#define BTN2_GPIO 20
#define BTN3_GPIO 17
#define BTN4_GPIO 18

#define NO_BTN       0x00
#define BTN1_BIT     0x1
#define BTN2_BIT     0x2
#define BTN3_BIT     0x4
#define BTN4_BIT     0x8
#define BTN_PIPE_LEN (sizeof(uint32_t) * 8) / 4

enum class BTN { NONE = 0, BTN1, BTN2, BTN3, BTN4 };

const char ubx_db_dump_file[] = "dbdump.bin";
const char user_settings_path[] = "user_settings.json";
#define DEFAULT_SETTINS_STR "{\"hardware\":{\"en_bme680\":true,\"en_sps30\":true,\"en_cam_m8q\":true,\"meas_interval_sec\":20},\"brightness\":50,\"display_auto_off_sec\":20, \"wifi_connections\":[],\"hostname\":\"\",\"token\":\"\",\"is_set_up\":false,\"time_zone\":0,\"last_chunk_index\":0}"

#define TCP_PORT                    80
#define CYW43_DEFAULT_IP_AP_ADDRESS LWIP_MAKEU32(192, 168, 4, 1)
#define CYW43_DEFAULT_IP_MASK       LWIP_MAKEU32(255, 255, 255, 0)

#define BUTTON_CHECK_INTERVAL_MS   10
#define GUI_UPDATE_INTERVAL_MS     100
#define GNSS_UPDATE_INTERVAL_MS    1000
#define BATTERY_UPDATE_INTERVAL_MS 10000

#define UID_LEN                   65  // sha256 in hex numbers + null terminator
#define ALWAYS_ACTIVE_ELEMENTS    2
#define MEASUREMENTS_PER_CHUNK    20
#define PRESSURE_AT_SEA_LEVEL_HPA 1013.25

#define PRESSURE_ALT_WEIGTH 0.1f
#define GNSS_ALT_WEIGTH     0.9f

#define UBX_DBDUMP_TIMOUT_S 10

#define ADD_AIRSCOUT_URL_TEMPLATE "https://airscout.fri3dl.dev/api/airscout/add?token=%s"

typedef struct _meas_data_t {
    bool time_good;
    datetime_t time;

    // bme
    bool bme680_data_good;
    float temperature;
    float humidity;
    float pressure;
    uint32_t gas;
    float aq_score;

    // cam m8q
    bool camm8q_data_good;
    double latitude;
    double longitude;

    // bme & camm8q
    float altitude;

    // sps30
    bool sps30_data_good;
    uint16_t mc_1p0;
    uint16_t mc_2p5;
    uint16_t mc_4p0;
    uint16_t mc_10p0;
    uint16_t nc_0p5;
    uint16_t nc_1p0;
    uint16_t nc_2p5;
    uint16_t nc_4p0;
    uint16_t nc_10p0;
    uint16_t typical_particle_size;
} meas_data_t;

#endif /* GLOBALS_H */
