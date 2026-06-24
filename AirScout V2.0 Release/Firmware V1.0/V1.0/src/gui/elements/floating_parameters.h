#ifndef GUI_ELEMENTS_FLOATING_PARAMETERS_H
#define GUI_ELEMENTS_FLOATING_PARAMETERS_H

#include <stdint.h>

#include "driver/display.h"
#include "gui/elements/element.h"
struct _meas_data_t;
typedef _meas_data_t meas_data_t;  // forward declare

namespace GUI {

class Floating_Parameters : public Element {
public:
    Floating_Parameters() = default;
    Floating_Parameters(Display *disp);

    void drawStatic();
    void drawContent();
    bool update(bool force);
    void updateData(meas_data_t *data);
    void updateStorageData(uint64_t free_space, uint32_t saved_stamps);

private:
    meas_data_t *m_data;
    uint32_t m_mib_free = 0;
    uint32_t m_saved_stamps = 0;

    void m_draw_static_con0(Display::COLOR color);
    Display::COLOR m_lastColor0;
    void m_draw_static_con1(Display::COLOR color);
    Display::COLOR m_lastColor1;
    void m_draw_static_con2(Display::COLOR color);
    Display::COLOR m_lastColor2;
    void m_draw_static_con3(Display::COLOR color);
    Display::COLOR m_lastColor3;

    void m_update_con0();
    void m_update_con1();
    void m_update_con2();
    void m_update_con3();
};
}  // namespace GUI

#endif  // GUI_ELEMENTS_FLOATING_PARAMETERS_H
