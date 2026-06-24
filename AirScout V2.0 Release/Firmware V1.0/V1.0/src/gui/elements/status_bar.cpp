#include "status_bar.h"

#include <hardware/rtc.h>
#include <pico/types.h>
#include <pico/util/datetime.h>
#include <string.h>
#include <ulog.h>

#include <cstdio>

#include "driver/display.h"
#include "main.h"

static void datetime_apply_timezone(datetime_t *dt, int offset_hours) {
    struct tm t = {.tm_sec = dt->sec, .tm_min = dt->min, .tm_hour = dt->hour, .tm_mday = dt->day, .tm_mon = dt->month - 1, .tm_year = dt->year - 1900};

    // Convert to timestamp (seconds since epoch)
    time_t timestamp = mktime(&t);

    // Apply timezone offset (hours → seconds)
    timestamp += offset_hours * 3600;

    // Convert back
    struct tm *new_t = gmtime(&timestamp);

    dt->year = new_t->tm_year + 1900;
    dt->month = new_t->tm_mon + 1;
    dt->day = new_t->tm_mday;
    dt->hour = new_t->tm_hour;
    dt->min = new_t->tm_min;
    dt->sec = new_t->tm_sec;
}

namespace GUI {
Status_Bar::Status_Bar(Display *disp) : Element(disp) {
    rtc_get_datetime(&m_lastLocaltime);

    m_gnssStatus = 0;
    m_batteryStatus = 0;
}

void Status_Bar::drawStatic() {
    m_disp->fill_rectangle(0, 0, 319, STATUS_BAR_HEIGHT, Display::COLOR::BLACK);
    m_disp->draw_line(0, STATUS_BAR_HEIGHT, 319, STATUS_BAR_HEIGHT, Display::COLOR::GOLD);
}
void Status_Bar::drawContent() {
    m_drawTime();
    m_drawBattery();
    m_drawGNSS();
}

bool Status_Bar::update(bool force) {
    datetime_t tmp_time;
    rtc_get_datetime(&tmp_time);
    if(tmp_time.min != m_lastLocaltime.min || tmp_time.hour != m_lastLocaltime.hour || tmp_time.day != m_lastLocaltime.day) {
        // check if time is valid
        if(tmp_time.year > 0 && tmp_time.month < 32 && tmp_time.month > 0 && tmp_time.hour < 25 && tmp_time.hour > -1) {
            m_drawTime();
        }
    }

    if(force) {
        drawStatic();
        drawContent();
        m_changed = false;
        return true;
    }

    if(m_changed && !force) {
        drawContent();
        m_changed = false;
        return true;
    }
    return false;
}

void Status_Bar::setGNSSStatus(int8_t status) {
    m_gnssStatus = status;
    m_changed = true;
}
void Status_Bar::setBattery(int8_t percent) {
    m_batteryStatus = percent;
    m_changed = true;
}
void Status_Bar::set_time_zone() {
    user_settings.getint("time_zone", &m_time_zone_offset);
    m_changed = true;
}

void Status_Bar::m_drawTime() {
    rtc_get_datetime(&m_lastLocaltime);
    char tmp[30];
    memset(tmp, 0, 30);

    datetime_apply_timezone(&m_lastLocaltime, m_time_zone_offset);
    snprintf(tmp, 30, "%02d:%02d", m_lastLocaltime.hour + m_time_zone_offset, m_lastLocaltime.min);
    m_disp->draw_text(5, 6, 1, tmp, &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);

    memset(tmp, 0, 30);
    snprintf(tmp, 30, "%04d-%02d-%02d", m_lastLocaltime.year, m_lastLocaltime.month, m_lastLocaltime.day);
    m_disp->draw_text(5, 28, 1, tmp, &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);
}

void Status_Bar::m_drawGNSS() {
    if(m_gnssStatus == 0) {
        m_disp->draw_sprite_from_file("/sprites/gps-no.bin", 180, 5);
    } else {
        m_disp->draw_sprite_from_file("/sprites/gps.bin", 180, 5);
    }
}
void Status_Bar::m_drawBattery() {
    char tmp[30];
    memset(tmp, 0, 30);

    if(m_batteryStatus < 0) {
        // charging
        m_batteryStatus *= -1;
        snprintf(tmp, 30, "%02d%%", m_batteryStatus);
        m_disp->draw_text(275, 6, 1, tmp, &font_Liberation_Mono13x21, Display::COLOR::WHEAT, Display::COLOR::BLACK);
        m_disp->draw_sprite_from_file("sprites/battery-050-charging.bin", 278, 26);

    } else {
        snprintf(tmp, 30, "%02d%%", m_batteryStatus);
        m_disp->draw_text(275, 6, 1, tmp, &font_Liberation_Mono13x21, Display::COLOR::WHEAT, Display::COLOR::BLACK);
        if(m_batteryStatus > 80) {
            m_disp->draw_sprite_from_file("sprites/battery-100.bin", 278, 26);
        } else if(m_batteryStatus > 30) {
            m_disp->draw_sprite_from_file("sprites/battery-050.bin", 278, 26);
        } else {
            m_disp->draw_sprite_from_file("sprites/battery-010.bin", 278, 26);
        }
    }
}
}  // namespace GUI