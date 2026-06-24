#ifndef GUI_ELEMENTS_STATUS_BAR_H
#define GUI_ELEMENTS_STATUS_BAR_H

#include <pico/types.h>
#include <stdint.h>

#include "driver/display.h"
#include "gui/elements/element.h"

#define STATUS_BAR_HEIGHT 52
namespace GUI {

class Status_Bar : public Element {
public:
    Status_Bar() = default;
    Status_Bar(Display *disp);

    void drawStatic();
    void drawContent();
    bool update(bool force);

    void setGNSSStatus(int8_t status);
    void setBattery(int8_t percent);
    void set_time_zone();

private:
    void m_drawTime();
    void m_drawGNSS();
    void m_drawBattery();

    datetime_t m_lastLocaltime;
    int8_t m_gnssStatus;
    int8_t m_batteryStatus;
    int32_t m_time_zone_offset;
};
}  // namespace GUI
#endif  // GUI_ELEMENTS_STATUS_BAR_H
