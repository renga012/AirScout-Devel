#include "menu_bar.h"

#include <string.h>
#include <ulog.h>

namespace GUI {
Menu_Bar::Menu_Bar(Display *disp) : Element(disp) {
}

void Menu_Bar::drawStatic() {
    m_disp->fill_rectangle(0, MENU_BAR_BEGIN, 319, 52, Display::COLOR::BLACK);
    m_disp->draw_line(0, MENU_BAR_BEGIN, 319, MENU_BAR_BEGIN, Display::COLOR::GOLD);
    // m_disp->draw_sprite_from_file("/sprites/arrow-down.bin", 1, 428);
    // m_disp->draw_sprite_from_file("/sprites/arrow-up.bin", 50, 428);
    // m_disp->draw_sprite_from_file("/sprites/arrow-left.bin", 50 * 2, 428);
    // m_disp->draw_sprite_from_file("/sprites/arrow-right.bin", 50 * 3, 428);
    // m_disp->draw_sprite_from_file("/sprites/cross.bin", 50 * 4, 428);
    // m_disp->draw_sprite_from_file("/sprites/checkmark.bin", 50 * 4 + 32, 428);
    // m_disp->draw_sprite_from_file("/sprites/delete.bin", 50 * 4 + 66, 428);
}
void Menu_Bar::drawContent() {
    m_drawLayout();
}

bool Menu_Bar::update(bool force) {
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

void Menu_Bar::m_drawLayout() {
    m_disp->draw_sprite_from_file(m_getIconName(m_i1), 32, MENU_BAR_BEGIN + 3);
    m_disp->draw_sprite_from_file(m_getIconName(m_i2), 32 * 3, MENU_BAR_BEGIN + 3);
    m_disp->draw_sprite_from_file(m_getIconName(m_i3), 32 * 5, MENU_BAR_BEGIN + 3);
    m_disp->draw_sprite_from_file(m_getIconName(m_i4), 32 * 7, MENU_BAR_BEGIN + 3);
}

void Menu_Bar::setLayout(Icons i1, Icons i2, Icons i3, Icons i4) {
    m_i1 = i1;
    m_i2 = i2;
    m_i3 = i3;
    m_i4 = i4;
    m_changed = true;
}

char *Menu_Bar::m_getIconName(Icons icon) {
    static char name[60];
    memset(name, 0, sizeof(name));
    switch(icon) {
        case Icons::LEFT:
            memcpy(name, "/sprites/arrow-left.bin", MIN(strlen("/sprites/arrow-left.bin"), sizeof(name)));
            break;
        case Icons::RIGHT:
            memcpy(name, "/sprites/arrow-right.bin", MIN(strlen("/sprites/arrow-right.bin"), sizeof(name)));
            break;
        case Icons::UP:
            memcpy(name, "/sprites/arrow-up.bin", MIN(strlen("/sprites/arrow-up.bin"), sizeof(name)));
            break;
        case Icons::DOWN:
            memcpy(name, "/sprites/arrow-down.bin", MIN(strlen("/sprites/arrow-down.bin"), sizeof(name)));
            break;
        case Icons::CHECKMARK:
            memcpy(name, "/sprites/checkmark.bin", MIN(strlen("/sprites/checkmark.bin"), sizeof(name)));
            break;
        case Icons::CROSS:
            memcpy(name, "/sprites/cross.bin", MIN(strlen("/sprites/cross.bin"), sizeof(name)));
            break;
        case Icons::DELETE:
            memcpy(name, "/sprites/delete.bin", MIN(strlen("/sprites/delete.bin"), sizeof(name)));
            break;
        case Icons::BACK:
            memcpy(name, "/sprites/circular-arrow-left.bin", MIN(strlen("/sprites/circular-arrow-left.bin"), sizeof(name)));
            break;
        case Icons::PAUSE:
            memcpy(name, "/sprites/pause.bin", MIN(strlen("/sprites/pause.bin"), sizeof(name)));
            break;
        case Icons::RESUME:
            memcpy(name, "/sprites/resume.bin", MIN(strlen("/sprites/resume.bin"), sizeof(name)));
            break;
        case Icons::MENU:
            memcpy(name, "/sprites/menu.bin", MIN(strlen("/sprites/menu.bin"), sizeof(name)));
            break;
        case Icons::NONE:
        default:
            memcpy(name, "/sprites/blank.bin", MIN(strlen("/sprites/blank.bin"), sizeof(name)));
            break;
    }
    return name;
}

}  // namespace GUI