#include "element.h"

#include "driver/display.h"
namespace GUI {

Element::Element(Display *disp) {
    m_changed = true;
    m_disp = disp;
}
bool Element::update(bool force) {
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
void Element::drawStatic() {
}
void Element::drawContent() {
}
}  // namespace GUI