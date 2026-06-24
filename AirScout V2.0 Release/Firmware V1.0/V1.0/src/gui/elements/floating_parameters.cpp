#include "floating_parameters.h"

#include <hardware/rtc.h>
#include <pico/types.h>
#include <string.h>
#include <ulog.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/display.h"
#include "globals.h"
#include "gui/gui_utils.h"
#include "main.h"

#define MAIN_ZERO_X   10
#define MAIN_ZERO_Y   80
#define BOX_WIDTH     145
#define BOX_HEIGHT    145
#define BOX_PADDING_X 10
#define BOX_PADDING_Y 20
#define RADIUS        25

namespace GUI {
Floating_Parameters::Floating_Parameters(Display *disp) : Element(disp) {
    m_data = NULL;
}

void Floating_Parameters::drawStatic() {
    clear_except_status_menu(Display::COLOR::BLACK);
    m_draw_static_con0(Display::COLOR::GREEN);
    m_draw_static_con1(Display::COLOR::GREEN);
    m_draw_static_con2(Display::COLOR::GREEN);
    m_draw_static_con3(Display::COLOR::GREEN);
}

void Floating_Parameters::drawContent() {
    if(m_data == NULL) {
        return;
    }
    m_update_con0();
    m_update_con1();
    m_update_con2();
    m_update_con3();
}

bool Floating_Parameters::update(bool force) {
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

void Floating_Parameters::updateData(meas_data_t *data) {
    m_data = data;
    m_changed = true;
}
void Floating_Parameters::updateStorageData(uint64_t free_space, uint32_t saved_stamps) {
    m_mib_free = free_space;
    m_saved_stamps = saved_stamps;
}

void Floating_Parameters::m_draw_static_con0(Display::COLOR color) {
    m_lastColor0 = color;
    m_disp->fill_round_rectangle(MAIN_ZERO_X, MAIN_ZERO_Y, BOX_WIDTH, BOX_HEIGHT, RADIUS, color);                                                           // top left
}
void Floating_Parameters::m_draw_static_con1(Display::COLOR color) {
    m_lastColor1 = color;
    m_disp->fill_round_rectangle(MAIN_ZERO_X + BOX_WIDTH + BOX_PADDING_X, MAIN_ZERO_Y, BOX_WIDTH, BOX_HEIGHT, RADIUS, color);                               // top right
}
void Floating_Parameters::m_draw_static_con2(Display::COLOR color) {
    m_lastColor2 = color;
    m_disp->fill_round_rectangle(MAIN_ZERO_X, MAIN_ZERO_Y + BOX_HEIGHT + BOX_PADDING_Y, BOX_WIDTH, BOX_HEIGHT, RADIUS, color);                              // bottom left
}
void Floating_Parameters::m_draw_static_con3(Display::COLOR color) {
    m_lastColor3 = color;
    m_disp->fill_round_rectangle(MAIN_ZERO_X + BOX_WIDTH + BOX_PADDING_X, MAIN_ZERO_Y + BOX_HEIGHT + BOX_PADDING_Y, BOX_WIDTH, BOX_HEIGHT, RADIUS, color);  // bottom right
}

void Floating_Parameters::m_update_con0() {
    char tmp[30];
    // top left

    // set colors
    static Display::COLOR lastColor = Display::COLOR::GREEN;
    Display::COLOR color;

    if(m_data->temperature > 16.0 && m_data->temperature < 26.0) {
        color = Display::COLOR::GREEN;
    } else if(m_data->temperature >= 26.0) {
        color = Display::COLOR::RED;
    } else {
        color = Display::COLOR::BLUE;
    }

    if(m_lastColor0 != color) {
        m_draw_static_con0(color);
    }

    memset(tmp, 0, 30);
    if(m_data->temperature >= 0.0) {
        snprintf(tmp, 30, " %.1f\260C", m_data->temperature);
    } else {
        snprintf(tmp, 30, "%.1f\260C", m_data->temperature);
    }

    m_disp->draw_text(MAIN_ZERO_X + 2, MAIN_ZERO_Y + 15, 1, tmp, &font_Liberation_Mono17x28, Display::COLOR::WHITESMOKE, color);
    snprintf(tmp, 30, "%.1f%%", m_data->humidity);
    m_disp->draw_text(MAIN_ZERO_X + 20, MAIN_ZERO_Y + 55, 1, tmp, &font_Liberation_Mono17x28, Display::COLOR::WHITESMOKE, color);
    snprintf(tmp, 30, "%.0fPa", m_data->pressure);
    m_disp->draw_text(MAIN_ZERO_X + 20, MAIN_ZERO_Y + 100, 1, tmp, &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, color);
}
void Floating_Parameters::m_update_con1() {
    char tmp[30];
    // top right
    // set colors
    static Display::COLOR lastColor = Display::COLOR::GREEN;
    Display::COLOR color;
    Display::COLOR fontColor;

    if(m_data->aq_score < 100 && m_data->mc_2p5 < 15) {
        color = Display::COLOR::GREEN;
        fontColor = Display::COLOR::WHITESMOKE;
    } else if(m_data->aq_score < 150 && m_data->mc_2p5 < 30) {
        color = Display::COLOR::YELLOW;
        fontColor = Display::COLOR::BLACK;
    } else {
        color = Display::COLOR::RED;
        fontColor = Display::COLOR::WHITESMOKE;
    }

    if(m_lastColor1 != color) {
        lastColor = color;
        m_draw_static_con1(color);
    }

    snprintf(tmp, 30, "AQ:%d", (int)m_data->aq_score);
    snprintf(tmp, 30, "%-*s", 7, tmp);  // make it always 10 chars long, this overrides remaining letters, if the len of the string changes
    m_disp->draw_text(MAIN_ZERO_X + BOX_WIDTH + BOX_PADDING_X + 10, MAIN_ZERO_Y + 20, 1, tmp, &font_Liberation_Mono13x21, fontColor, color);
    // particle stuff
    snprintf(tmp, 30, "2.5:%d", (int)m_data->mc_2p5);
    snprintf(tmp, 30, "%-*s", 7, tmp);  // make it always 10 chars long, this overrides remaining letters, if the len of the string changes
    m_disp->draw_text(MAIN_ZERO_X + BOX_WIDTH + BOX_PADDING_X + 10, MAIN_ZERO_Y + 50, 1, tmp, &font_Liberation_Mono13x21, fontColor, color);
    
    snprintf(tmp, 30, "4p:%d", (int)m_data->mc_4p0);
    snprintf(tmp, 30, "%-*s", 7, tmp);  // make it always 10 chars long, this overrides remaining letters, if the len of the string changes
    m_disp->draw_text(MAIN_ZERO_X + BOX_WIDTH + BOX_PADDING_X + 10, MAIN_ZERO_Y + 80, 1, tmp, &font_Liberation_Mono13x21, fontColor, color);

    snprintf(tmp, 30, "10p:%d", (int)m_data->mc_10p0);
    snprintf(tmp, 30, "%-*s", 7, tmp);  // make it always 10 chars long, this overrides remaining letters, if the len of the string changes
    m_disp->draw_text(MAIN_ZERO_X + BOX_WIDTH + BOX_PADDING_X + 10, MAIN_ZERO_Y + 110, 1, tmp, &font_Liberation_Mono13x21, fontColor, color);
}
void Floating_Parameters::m_update_con2() {
    char tmp[30];
    // bottom left
    static Display::COLOR lastColor = Display::COLOR::GREEN;
    Display::COLOR color = Display::COLOR::GREEN;

    if(m_lastColor2 != color) {
        lastColor = color;
        m_draw_static_con2(color);
    }

    snprintf(tmp, 30, "%dm", (int16_t)m_data->altitude);
    snprintf(tmp, 30, "%-*s", 5, tmp);
    m_disp->draw_text(MAIN_ZERO_X + 25, MAIN_ZERO_Y + BOX_HEIGHT + BOX_PADDING_Y + 20, 1, tmp, &font_Liberation_Mono17x28, Display::COLOR::WHITESMOKE, color);

    snprintf(tmp, 30, "LO%.4f\260", m_data->latitude);
    snprintf(tmp, 30, "%-*s", 9, tmp);
    m_disp->draw_text(MAIN_ZERO_X + 5, MAIN_ZERO_Y + BOX_HEIGHT + BOX_PADDING_Y + 60, 1, tmp, &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, color);

    snprintf(tmp, 30, "LA%.4f\260", m_data->longitude);
    snprintf(tmp, 30, "%-*s", 9, tmp);
    m_disp->draw_text(MAIN_ZERO_X + 5, MAIN_ZERO_Y + BOX_HEIGHT + BOX_PADDING_Y + 90, 1, tmp, &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, color);
}
void Floating_Parameters::m_update_con3() {
    // bottom right
    char tmp[30];
    static Display::COLOR lastColor = Display::COLOR::GREEN;
    Display::COLOR color = Display::COLOR::GREEN;

    if(m_lastColor3 != color) {
        lastColor = color;
        m_draw_static_con2(color);
    }

    snprintf(tmp, 30, "%dMiB", m_mib_free);
    snprintf(tmp, 30, "%-*s", 8, tmp);
    m_disp->draw_text(MAIN_ZERO_X + BOX_WIDTH + BOX_PADDING_X + 15, MAIN_ZERO_Y + BOX_HEIGHT + BOX_PADDING_Y + 30, 1, tmp, &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, color);

    snprintf(tmp, 30, "%dvals", m_saved_stamps);
    snprintf(tmp, 30, "%-*s", 8, tmp);
    m_disp->draw_text(MAIN_ZERO_X + BOX_WIDTH + BOX_PADDING_X + 15, MAIN_ZERO_Y + BOX_HEIGHT + BOX_PADDING_Y + 60, 1, tmp, &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, color);
    
}

}  // namespace GUI