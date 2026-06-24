#ifndef GUI_ELEMENTS_MENU_BAR_H
#define GUI_ELEMENTS_MENU_BAR_H

#include "gui/elements/element.h"

#define MENU_BAR_BEGIN 427

namespace GUI {

class Menu_Bar : public Element {
public:
    enum class Icons { NONE, LEFT, RIGHT, UP, DOWN, CHECKMARK, CROSS, DELETE, BACK, PAUSE, RESUME, MENU };
    Menu_Bar() = default;
    Menu_Bar(Display *disp);

    void drawStatic();
    void drawContent();
    bool update(bool force);
    void setLayout(Icons i1, Icons i2, Icons i3, Icons i4);

private:
    Icons m_i1 = Icons::LEFT, m_i2 = Icons::RIGHT, m_i3 = Icons::UP, m_i4 = Icons::DOWN;
    char *m_getIconName(Icons icon);
    void m_drawLayout();
};
}  // namespace GUI
#endif  // GUI_ELEMENTS_MENU_BAR_H
