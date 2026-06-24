#include "selection.h"

#include <lwip/memp.h>
#include <pico/platform/compiler.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver/display.h"
#include "globals.h"
#include "gui/elements/menu_bar.h"
#include "gui/elements/status_bar.h"
#include "gui/gui_utils.h"
#include "main.h"
#include "ulog.h"
namespace GUI {

// tmp storage incase a values is discarded
static GUI::selection_item m_tmp_item = {};

Selection::Selection(Display *disp, selection_item *items, const int16_t item_cnt) {
    m_disp = disp;
    m_items = items;
    m_item_ctn = item_cnt;
    m_current_task = TASKS::SCROLL;
}
Selection::Selection() {
}
Selection::~Selection() {
}  // names

void Selection::drawStatic() {
    clear_except_status_menu(m_background_color);
    menu_bar.setLayout(Menu_Bar::Icons::CHECKMARK, Menu_Bar::Icons::CROSS, Menu_Bar::Icons::UP, Menu_Bar::Icons::DOWN);
    m_draw_items(m_offset);
    m_changed = true;
}

void Selection::drawContent() {
    switch(m_current_task) {
        case TASKS::SCROLL:
            if(m_shared_content_changed) {  // scroll
                m_shared_content_changed = false;
                m_draw_items(m_offset);
            }
            m_draw_selection_icon(&m_items[m_currently_selected], m_currently_selected);
            break;
        case TASKS::NUMBER:
            m_draw_number_menu(!m_shared_content_changed);
            m_shared_content_changed = false;
            break;
        case TASKS::TOGGLE:
            m_draw_toggle_menu(!m_shared_content_changed);
            m_shared_content_changed = false;
            break;
    }
}

bool Selection::update(bool force) {
    if(force || m_force_redraw) {
        drawStatic();
        drawContent();
        m_force_redraw = false;
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

// returns selected field
void Selection::processBTN(BTN btn) {
    switch(btn) {
        case BTN::NONE:
            break;
        case BTN::BTN1:
            m_handle_BTN1();
            break;
        case BTN::BTN2:
            m_handle_BTN2();
            break;
        case BTN::BTN3:
            m_handle_BTN3();
            break;
        case BTN::BTN4:
            m_handle_BTN4();
            break;
    }
}

bool Selection::m_draw_item(selection_item *m_items, uint16_t cnt) {
    const static uint8_t item_height = 21 + 5 + 5;  // font + top + bottom margin
    // if(STATUS_BAR_HEIGHT + 20 + (cnt * item_height) > MENU_BAR_BEGIN - 20) {
    // return false;
    // }
    static char tmp[21];
    snprintf(tmp, sizeof(tmp), "%-*s", sizeof(tmp), m_items->name);
    m_disp->draw_text((uint16_t)Selection::POS::TEXT_BEGIN_X, (uint16_t)Selection::POS::TEXT_BEGIN_Y + (cnt * (uint16_t)Selection::POS::ITEM_HEIGHT), 1, tmp, &font_Liberation_Mono13x21, m_text_color, m_background_color);
    return true;
}

void Selection::m_draw_selection_icon(selection_item *m_item, uint16_t cnt) {
    m_disp->fill_rectangle((uint16_t)Selection::POS::TEXT_BEGIN_X - 15, STATUS_BAR_HEIGHT + 1, 15, MENU_BAR_BEGIN - STATUS_BAR_HEIGHT - 1, m_background_color);

    m_disp->fill_triangle(                                                                                                                             // selection symbol
        (uint16_t)Selection::POS::TEXT_BEGIN_X - 15, (uint16_t)Selection::POS::TEXT_BEGIN_Y + cnt * (uint16_t)Selection::POS::ITEM_HEIGHT - 2,         // P1
        (uint16_t)Selection::POS::TEXT_BEGIN_X - 5, (uint16_t)Selection::POS::TEXT_BEGIN_Y + (cnt * (uint16_t)Selection::POS::ITEM_HEIGHT) + 10 - 2,   // P2
        (uint16_t)Selection::POS::TEXT_BEGIN_X - 15, (uint16_t)Selection::POS::TEXT_BEGIN_Y + (cnt * (uint16_t)Selection::POS::ITEM_HEIGHT) + 20 - 2,  // P3
        Display::COLOR::BLACK);                                                                                                                        // Color

    // m_disp->draw_sprite_from_file("/sprites/selection-right.bin", (uint16_t)Selection::POS::TEXT_BEGIN_X - 5 - 32, (uint16_t)Selection::POS::TEXT_BEGIN_Y + cnt * (uint16_t)Selection::POS::ITEM_HEIGHT);
    // m_disp->draw_rectangle((uint16_t)Selection::POS::TEXT_BEGIN_X - 5, (uint16_t)Selection::POS::TEXT_BEGIN_Y + 5, m_disp->measure_text(m_item->name, &font_Liberation_Mono13x21, 1), (uint16_t)Selection::POS::ITEM_HEIGHT, Display::COLOR::BLACK);
}

void Selection::m_draw_items(uint16_t offset) {
    uint16_t end = MIN(m_item_ctn, offset + m_items_per_page);

    for(int16_t i = offset; i < end; i++) {
        m_draw_item(&m_items[i], i - offset);
    }
}

selection_item *Selection::m_get_selected_item() {
    return &m_items[m_currently_selected + m_offset];
}

void Selection::m_handle_BTN1() {
    selection_item *item = m_get_selected_item();
    switch(m_current_task) {
        case TASKS::SCROLL:
            switch(item->type) {
                case TASKS::TOGGLE:
                    m_current_task = TASKS::TOGGLE;
                    m_tmp_item = *item;
                    // m_orig_toggle = item->context.toggle;
                    ulog_debug("Toggle for %s", item->name);
                    break;
                case TASKS::NUMBER:
                    m_current_task = TASKS::NUMBER;
                    m_tmp_item = *item;
                    ulog_debug("Number for %s", item->name);
                    break;
                case TASKS::ACTIVATE_MODE:
                    m_current_task = TASKS::ACTIVATE_MODE;
                    m_tmp_item = *item;
                    switchToMode(item->context.mode);
                    ulog_debug("Activate Mode for %s", item->name);
                    break;
                case TASKS::ACTIVATE:
                    m_current_task = TASKS::ACTIVATE;
                    m_tmp_item = *item;
                    m_tmp_item.cbk(&m_tmp_item);
                    ulog_debug("Activate for %s", item->name);
                    break;
            }
            break;

        case TASKS::NUMBER:
            item->context.number.num = m_tmp_item.context.number.num;
            m_tmp_item.cbk(&m_tmp_item);
            m_current_task = TASKS::SCROLL;
            m_shared_content_changed = true;
            m_force_redraw = true;
            break;
        case TASKS::ACTIVATE:
            break;
        case TASKS::TOGGLE:  // apply new value
            item->context.toggle = m_tmp_item.context.toggle;
            m_tmp_item.cbk(&m_tmp_item);
            m_current_task = TASKS::SCROLL;
            m_shared_content_changed = true;
            m_force_redraw = true;
            break;
    }
    m_changed = true;
}
void Selection::m_handle_BTN2() {
    switch(m_current_task) {
        case TASKS::SCROLL:
            m_should_exit = true;
            break;
        case TASKS::NUMBER:
            m_current_task = TASKS::SCROLL;
            m_shared_content_changed = true;
            m_force_redraw = true;
            break;
        case TASKS::ACTIVATE:
            m_current_task = TASKS::SCROLL;
            m_shared_content_changed = true;
            m_force_redraw = true;
            break;
        case TASKS::TOGGLE:
            m_current_task = TASKS::SCROLL;
            m_shared_content_changed = true;
            m_force_redraw = true;
            break;
    }
    m_changed = true;
}
void Selection::m_handle_BTN3() {
    switch(m_current_task) {
        case TASKS::SCROLL:
            if(m_currently_selected > 0) {
                ulog_info("UP");
                m_currently_selected--;
            } else {
                ulog_info("Scroll up");
                m_offset--;
                m_offset = MAX(m_offset, 0);
                m_shared_content_changed = true;
            }
            m_changed = true;
            break;
        case TASKS::NUMBER:  // count up
            if(m_tmp_item.context.number.num <= (m_tmp_item.context.number.max - m_tmp_item.context.number.step_size)) {
                m_tmp_item.context.number.num += m_tmp_item.context.number.step_size;
            }
            m_shared_content_changed = true;
            break;
        case TASKS::ACTIVATE:
            break;
        case TASKS::TOGGLE:
            m_tmp_item.context.toggle = !m_tmp_item.context.toggle;
            m_shared_content_changed = true;
            break;
    }
    m_changed = true;
}

void Selection::m_handle_BTN4() {
    switch(m_current_task) {
        case TASKS::SCROLL:
            if(m_currently_selected >= (m_item_ctn - 1)) {
                // nothing
            } else if(m_currently_selected < m_items_per_page - 1) {
                ulog_info("DOWN");
                m_currently_selected++;
            } else {
                ulog_info("Scroll down");
                m_offset++;
                m_offset = MIN(m_offset, m_item_ctn - m_items_per_page);
                m_shared_content_changed = true;
            }
            break;
        case TASKS::NUMBER:  // count down
            if(m_tmp_item.context.number.num >= (m_tmp_item.context.number.min + m_tmp_item.context.number.step_size)) {
                m_tmp_item.context.number.num -= m_tmp_item.context.number.step_size;
            }
            m_shared_content_changed = true;
            break;
        case TASKS::ACTIVATE:
            break;
        case TASKS::TOGGLE:
            m_tmp_item.context.toggle = !m_tmp_item.context.toggle;
            m_shared_content_changed = true;
            break;
    }
    m_changed = true;
}

void Selection::m_draw_toggle_menu(bool force) {
    static const uint16_t BEGIN_X = 40;
    static const uint16_t BEGIN_Y = 100;
    static const uint16_t WITDH = DISPLAY_WIDTH - (BEGIN_X * 2);
    static const uint16_t HEIGHT = 120;
    static const uint16_t MAIN_RADIUS = 20;

    static const uint16_t TEXT_PADDING_X = 15;
    static const uint16_t TEXT_PADDING_Y = 10;
    static const uint16_t TEXT_SPACING = 1;

    static const uint16_t IN_BEGIN_X = BEGIN_X + 20;
    static const uint16_t IN_BEGIN_Y = BEGIN_Y + 60;
    static const uint16_t IN_WIDTH = 200;
    static const uint16_t IN_HEIGHT = 40;
    static const uint16_t IN_RADIUS = 15;

    static const uint16_t IN_TEXT_PADDING_X = 5;
    static const uint16_t IN_TEXT_PADDING_Y = 10;

    if(force) {
        m_disp->fill_round_rectangle(BEGIN_X, BEGIN_Y, WITDH, HEIGHT, MAIN_RADIUS, Display::COLOR::BLACK);
        m_disp->draw_text(BEGIN_X + TEXT_PADDING_X, BEGIN_Y + TEXT_PADDING_Y, TEXT_SPACING, m_tmp_item.name, &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);
        m_disp->draw_text(BEGIN_X + TEXT_PADDING_X, BEGIN_Y + TEXT_PADDING_Y + 25, TEXT_SPACING, "Toggle", &font_Liberation_Mono10x16, Display::COLOR::GOLD, Display::COLOR::BLACK);
    }
    if(m_tmp_item.context.toggle == true) {
        m_disp->fill_round_rectangle(IN_BEGIN_X, IN_BEGIN_Y, IN_WIDTH, IN_HEIGHT, IN_RADIUS, Display::COLOR::BLACK);
        m_disp->fill_round_rectangle(IN_BEGIN_X, IN_BEGIN_Y, IN_WIDTH, IN_HEIGHT, IN_RADIUS, Display::COLOR::GREEN);
        m_disp->draw_text(IN_BEGIN_X + IN_TEXT_PADDING_X, IN_BEGIN_Y + IN_TEXT_PADDING_Y, TEXT_SPACING, "ON", &font_Liberation_Mono13x21, Display::COLOR::BLACK, Display::COLOR::GREEN);
    } else {
        m_disp->fill_round_rectangle(IN_BEGIN_X, IN_BEGIN_Y, IN_WIDTH, IN_HEIGHT, IN_RADIUS, Display::COLOR::BLACK);
        m_disp->fill_round_rectangle(IN_BEGIN_X, IN_BEGIN_Y, IN_WIDTH, IN_HEIGHT, IN_RADIUS, Display::COLOR::RED);
        m_disp->draw_text(IN_BEGIN_X + IN_TEXT_PADDING_X, IN_BEGIN_Y + IN_TEXT_PADDING_Y, TEXT_SPACING, "OFF", &font_Liberation_Mono13x21, Display::COLOR::BLACK, Display::COLOR::RED);
    }
}
void Selection::m_draw_number_menu(bool force) {
    static const uint16_t BEGIN_X = 40;
    static const uint16_t BEGIN_Y = 100;
    static const uint16_t WITDH = DISPLAY_WIDTH - (BEGIN_X * 2);
    static const uint16_t HEIGHT = 120;
    static const uint16_t MAIN_RADIUS = 20;

    static const uint16_t TEXT_PADDING_X = 15;
    static const uint16_t TEXT_PADDING_Y = 10;
    static const uint16_t TEXT_SPACING = 1;

    static const uint16_t IN_BEGIN_X = BEGIN_X + 20;
    static const uint16_t IN_BEGIN_Y = BEGIN_Y + 60;
    static const uint16_t IN_WIDTH = 200;
    static const uint16_t IN_HEIGHT = 40;
    static const uint16_t IN_RADIUS = 15;

    static const uint16_t IN_TEXT_PADDING_X = 5;
    static const uint16_t IN_TEXT_PADDING_Y = 10;

    if(force) {
        m_disp->fill_round_rectangle(BEGIN_X, BEGIN_Y, WITDH, HEIGHT, MAIN_RADIUS, Display::COLOR::BLACK);
        m_disp->draw_text(BEGIN_X + TEXT_PADDING_X, BEGIN_Y + TEXT_PADDING_Y, TEXT_SPACING, m_tmp_item.name, &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);
        m_disp->draw_text(BEGIN_X + TEXT_PADDING_X, BEGIN_Y + TEXT_PADDING_Y + 25, TEXT_SPACING, "Number", &font_Liberation_Mono10x16, Display::COLOR::GOLD, Display::COLOR::BLACK);
        char tmp[20];
        snprintf(tmp, sizeof(tmp), "%d<=x<=%d", m_tmp_item.context.number.min, m_tmp_item.context.number.max);
        m_disp->draw_text(IN_BEGIN_X + IN_TEXT_PADDING_X, IN_BEGIN_Y + IN_TEXT_PADDING_Y + 20, TEXT_SPACING, tmp, &font_Liberation_Mono10x16, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);
    }
    char tmp[10];
    snprintf(tmp, sizeof(tmp), "%d%s", m_tmp_item.context.number.num, m_tmp_item.context.number.unit);
    snprintf(tmp, sizeof(tmp), "%-*s", sizeof(tmp) - strlen(tmp), tmp);
    m_disp->draw_text(IN_BEGIN_X + IN_TEXT_PADDING_X, IN_BEGIN_Y + IN_TEXT_PADDING_Y, TEXT_SPACING, tmp, &font_Liberation_Mono13x21, Display::COLOR::WHITESMOKE, Display::COLOR::BLACK);
}

bool Selection::should_exit() {
    return m_should_exit;
}

}  // namespace GUI