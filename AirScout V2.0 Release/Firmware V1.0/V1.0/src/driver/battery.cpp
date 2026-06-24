#include "battery.h"

#include <hardware/adc.h>
#include <hardware/gpio.h>
#include <pico/time.h>
#include <stdint.h>

#include "globals.h"
#include "ulog.h"

// tmp
#define ADC_BAT   28
#define BAT_FULL  4095
#define BAT_EMPTY 0

int8_t getBatteryPercent() {
    int8_t val;
    adc_gpio_init(ADC_BAT);
    gpio_init(ADC_REF_CRTL);
    gpio_set_dir(ADC_REF_CRTL, GPIO_OUT);
    gpio_put(ADC_REF_CRTL, 1);

    gpio_init(BAT_CHARGING_INT);
    gpio_set_dir(BAT_CHARGING_INT, GPIO_IN);

    sleep_ms(1);
    adc_select_input(ADC_BAT - 26);  // 26 is the first adc pin
    uint16_t result = adc_read();
    ulog_info("ADC res: %d", result);

    // gpio_put(ADC_REF_CRTL, 0);

    // not optimal
    val = result * (100.0 / 4096.0);

    if(!gpio_get(BAT_CHARGING_INT)) {
        val *= -1;
    }

    return val;
}